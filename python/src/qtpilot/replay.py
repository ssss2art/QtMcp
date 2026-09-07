"""Deterministic replay of a recorded qtPilot session.

A message log written by :class:`~qtpilot.message_logger.MessageLogger` is a transcript, not a
test: it records what was driven and what came back, interleaved, with timings and request ids
that differ on every run. This module turns one into something re-runnable.

The split that makes it work is that a session divides cleanly into two kinds of call. A few
methods *change* the application -- clicks, keystrokes, property writes, method invocations --
and everything else only *observes* it. Re-driving the first kind against a fresh application and
comparing the second kind is an assertion about behaviour, and it needs no bespoke test written
by hand for each flow.

Deliberately narrow: this reads and diffs, and knows nothing about connections. Driving a probe
belongs to the caller, so a scenario can be checked against a live application, against another
recording, or against itself in a unit test.
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable

# Calls that change the application. These are what a replay re-drives.
MUTATING_METHODS: frozenset[str] = frozenset({
    "qt.ui.click",
    "qt.ui.clickItem",
    "qt.ui.sendKeys",
    "qt.properties.set",
    "qt.methods.invoke",
})

# Calls whose results describe the application, and so are worth asserting on.
#
# An allow-list rather than "everything that is not mutating": qt.ping and qt.version describe the
# harness, the qt.names.* and qt.signals.* families describe the session's own bookkeeping, and
# qt.ui.screenshot returns image bytes that belong in a visual golden. None of them say anything
# about the application under test, and asserting on them would fail runs for reasons a reader
# cannot act on.
OBSERVING_METHODS: frozenset[str] = frozenset({
    "qt.objects.tree",
    "qt.objects.inspect",
    "qt.objects.search",
    "qt.properties.get",
    "qt.models.list",
    "qt.models.data",
    "qt.models.search",
    "qt.ui.geometry",
    "qt.ui.hitTest",
})

# Timing, present at every depth and different in every run. "timestamp" is the probe's own
# epoch-millisecond stamp inside a result's meta block, and is every bit as volatile as the
# entry's ts -- it only shows up once a scenario is run against a real log rather than a fixture.
VOLATILE_KEYS: frozenset[str] = frozenset({"ts", "dur_ms", "timestamp"})

# The JSON-RPC request id. Stripped only from the top level of an entry: nested "id" keys are
# object identifiers -- the single most meaningful thing a result carries -- and removing those
# would leave a diff unable to tell one widget from another.
REQUEST_ID_KEY = "id"

# What MessageLogger._truncate leaves behind when a value was too large to log.
_TRUNCATED = re.compile(r"\.\.\.<truncated \d+c>$|^<image:\d+b>$")

# The fallback identity the probe gives an object with no registered name: a class name and a
# creation counter. The counter depends on the order objects happened to be constructed, so it is
# not stable between runs and asserting on it would fail every replay.
#
# Masking it keeps the shape of a result comparable while giving up on telling two unnamed
# objects apart. A recording that needs that distinction should register names for them first
# (qt.names.register / qt.names.load), which is the stable identity replay is designed around.
_GENERATED_HANDLE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)~\d+$")


def _is_truncated(value: Any) -> bool:
    """Whether a logged value is a placeholder rather than the real thing."""
    return isinstance(value, str) and _TRUNCATED.search(value) is not None


def normalise(value: Any, *, top_level: bool = True) -> Any:
    """Strip the fields that differ between two runs of the same session.

    :param value: A log entry, or any value nested inside one.
    :param top_level: False for values already inside an entry.
    :return: A copy without timing, and without the request id at the outermost level.

    .. note:: Timing is stripped at every depth, because a diff that reported a nested
       ``dur_ms`` would be reporting the clock. The request id is stripped only at the top:
       deeper down, ``id`` names an object rather than a call.
    """
    if isinstance(value, dict):
        drop = set(VOLATILE_KEYS)
        if top_level:
            drop.add(REQUEST_ID_KEY)
        return {k: normalise(v, top_level=False) for k, v in value.items() if k not in drop}
    if isinstance(value, list):
        return [normalise(item, top_level=False) for item in value]
    if isinstance(value, str):
        return _GENERATED_HANDLE.sub(r"\1~*", value)
    return value


def _equivalent(expected: Any, actual: Any) -> bool:
    """Compare two normalised values, treating logged placeholders as wildcards.

    .. note:: A truncated value is not what the application returned, so holding a replay to it
       would fail every run over a difference the logger introduced.
    """
    if _is_truncated(expected) or _is_truncated(actual):
        return True
    if isinstance(expected, dict) and isinstance(actual, dict):
        if expected.keys() != actual.keys():
            return False
        return all(_equivalent(expected[k], actual[k]) for k in expected)
    if isinstance(expected, list) and isinstance(actual, list):
        return len(expected) == len(actual) and all(
            _equivalent(e, a) for e, a in zip(expected, actual)
        )
    return expected == actual


@dataclass(frozen=True)
class Action:
    """A call that changes the application, and so is re-driven on replay."""

    method: str
    params: dict


@dataclass(frozen=True)
class Observation:
    """A call that describes the application, and so is asserted on."""

    method: str
    params: dict
    result: Any
    error: str | None = None


@dataclass
class Step:
    """One driven action and everything observed before the next one.

    .. note:: Step 0 has no action. It holds the baseline -- whatever was inspected before the
       session drove anything -- so a scenario can assert on the state it started from.
    """

    index: int
    action: Action | None = None
    observations: list[Observation] = field(default_factory=list)
    notifications: list[tuple[str, dict]] = field(default_factory=list)


@dataclass
class Scenario:
    """A parsed session: an ordered sequence of steps."""

    steps: list[Step]
    source: str = "<memory>"

    @property
    def is_replayable(self) -> bool:
        """Whether there is anything to drive.

        .. note:: A level-1 log records tool names but no wire traffic, so it parses into a
           baseline and nothing else. Such a scenario would pass unconditionally, which is worse
           than failing -- hence an explicit answer rather than an empty run.
        """
        return any(step.action is not None for step in self.steps)


@dataclass(frozen=True)
class Divergence:
    """One way a replay differed from what was recorded."""

    step: int
    kind: str  # "observation", "notification", "error", "missing_step"
    method: str
    expected: Any
    actual: Any

    def __str__(self) -> str:
        return f"step {self.step}: {self.kind} in {self.method}: expected {self.expected!r}, got {self.actual!r}"


def parse_entries(entries: Iterable[dict]) -> Scenario:
    """Split a sequence of log entries into steps.

    :param entries: Log entries in the order they were written.
    :return: The parsed scenario. Always has at least the baseline step.
    """
    steps: list[Step] = [Step(index=0)]
    pending: dict[Any, dict] = {}

    for raw in entries:
        direction = raw.get("dir")

        if direction == "req":
            pending[raw.get("id")] = raw
            continue

        if direction == "ntf":
            steps[-1].notifications.append((raw.get("method", ""), normalise(raw.get("params", {}))))
            continue

        if direction not in ("res", "err"):
            continue

        method = raw.get("method", "")
        request = pending.pop(raw.get("id"), {})
        params = normalise(request.get("params", {}))

        if method in MUTATING_METHODS:
            # The action's own result is not an observation: re-driving it produces a fresh one,
            # and asserting on it would assert that the driver worked, not that the app behaved.
            steps.append(Step(index=len(steps), action=Action(method=method, params=params)))
            continue

        if method in OBSERVING_METHODS:
            steps[-1].observations.append(
                Observation(
                    method=method,
                    params=params,
                    result=normalise(raw.get("result")) if direction == "res" else None,
                    error=raw.get("error") if direction == "err" else None,
                )
            )

    return Scenario(steps=steps)


def load_scenario(path: str | Path) -> Scenario:
    """Read a JSON Lines message log.

    :param path: The log file.
    :return: The parsed scenario, tagged with where it came from.
    :raises ValueError: If a line is not valid JSON, naming the line so it can be found.
    """
    entries: list[dict] = []
    text = Path(path).read_text(encoding="utf-8")

    for number, line in enumerate(text.splitlines(), start=1):
        if not line.strip():
            continue
        try:
            entries.append(json.loads(line))
        except json.JSONDecodeError as exc:
            raise ValueError(f"{path}: line {number} is not valid JSON: {exc}") from exc

    scenario = parse_entries(entries)
    scenario.source = str(path)
    return scenario


def _diff_observations(expected: Step, actual: Step) -> list[Divergence]:
    """Compare one step's observations against another's."""
    divergences: list[Divergence] = []

    for index, want in enumerate(expected.observations):
        got = actual.observations[index] if index < len(actual.observations) else None

        if got is None:
            divergences.append(
                Divergence(expected.index, "observation", want.method, want.result, None)
            )
            continue

        if got.error is not None and want.error is None:
            divergences.append(
                Divergence(expected.index, "error", want.method, want.result, got.error)
            )
            continue

        if not _equivalent(want.result, got.result):
            divergences.append(
                Divergence(expected.index, "observation", want.method, want.result, got.result)
            )

    return divergences


def _diff_notifications(expected: Step, actual: Step) -> list[Divergence]:
    """Compare notifications as a multiset.

    .. note:: Delivery order between independent objects is not something the application
       promises, so comparing sequences would make a replay flaky rather than strict.
    """
    def key(notification: tuple[str, dict]) -> str:
        return json.dumps(notification, sort_keys=True)

    remaining = [key(n) for n in actual.notifications]
    divergences: list[Divergence] = []

    for notification in expected.notifications:
        wanted = key(notification)
        if wanted in remaining:
            remaining.remove(wanted)
        else:
            divergences.append(
                Divergence(expected.index, "notification", notification[0], notification[1], None)
            )

    return divergences


def diff_steps(expected: list[Step], actual: list[Step]) -> list[Divergence]:
    """Compare a recorded run against a replayed one.

    :param expected: Steps as recorded.
    :param actual: Steps as replayed.
    :return: Every difference found, in step order. Empty means the runs agree.
    """
    divergences: list[Divergence] = []

    for index, want in enumerate(expected):
        if index >= len(actual):
            action = want.action.method if want.action else "<baseline>"
            divergences.append(Divergence(want.index, "missing_step", action, action, None))
            continue

        got = actual[index]
        divergences.extend(_diff_observations(want, got))
        divergences.extend(_diff_notifications(want, got))

    return divergences
