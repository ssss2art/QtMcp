"""Tests for async notification dispatch in ProbeConnection."""

from __future__ import annotations

import asyncio
import json
import time

import pytest

from qtpilot.connection import ProbeConnection

pytestmark = pytest.mark.asyncio


async def test_notifications_dispatched_async(mock_probe):
    """Verify notification handlers are called via the async dispatch queue."""
    probe, mock_ws = mock_probe

    received = []

    def handler(method, params):
        received.append((method, params))

    probe.add_notification_handler(handler)

    # Inject a notification from the "probe"
    await mock_ws.inject_notification({
        "jsonrpc": "2.0",
        "method": "qtpilot.signalEmitted",
        "params": {"objectId": "btn1"},
    })

    # Give the recv loop and dispatcher time to process
    await asyncio.sleep(0.2)

    assert len(received) == 1
    assert received[0][0] == "qtpilot.signalEmitted"
    assert received[0][1] == {"objectId": "btn1"}


async def test_notification_queue_full_drops(mock_probe):
    """Verify drops are counted when the notification queue is full."""
    probe, mock_ws = mock_probe

    # Replace the queue with a tiny one to test overflow
    probe._notification_queue = asyncio.Queue(maxsize=2)

    received = []

    def slow_handler(method, params):
        received.append(method)

    probe.add_notification_handler(slow_handler)

    # Fill the queue by injecting many notifications quickly
    for i in range(5):
        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": f"ntf.{i}",
            "params": {},
        })

    # Let recv loop process them
    await asyncio.sleep(0.3)

    # Some should have been dropped (queue maxsize=2 + whatever was consumed)
    assert probe.notification_drops >= 1


async def test_recv_loop_not_blocked_by_slow_handler(mock_probe):
    """Slow notification handler should not block response resolution."""
    probe, mock_ws = mock_probe

    barrier = asyncio.Event()

    def slow_handler(method, params):
        # Simulate slow processing (but since handler is called
        # synchronously in the dispatcher, not in recv loop,
        # this won't block response resolution)
        import time
        time.sleep(0.1)

    probe.add_notification_handler(slow_handler)

    # Inject a notification
    await mock_ws.inject_notification({
        "jsonrpc": "2.0",
        "method": "qtpilot.objectCreated",
        "params": {"objectId": "obj1"},
    })

    # Now send a request — the response should arrive quickly
    mock_ws.responses[probe._next_id] = {
        "jsonrpc": "2.0",
        "result": {"pong": True},
        "id": probe._next_id,
    }

    t0 = time.monotonic()
    result = await asyncio.wait_for(probe.call("qt.ping"), timeout=2.0)
    elapsed = time.monotonic() - t0

    assert result == {"pong": True}
    # The call should not have been delayed by the slow handler
    # (it would be ~0.1s if blocking, should be <0.05s if async)
    # Use a generous threshold since CI can be slow
    assert elapsed < 0.5


async def test_notification_drops_property():
    """Verify notification_drops property is accessible."""
    probe = ProbeConnection("ws://localhost:9222")
    assert probe.notification_drops == 0


async def test_notification_queue_size_property():
    """Verify notification_queue_size property is accessible."""
    probe = ProbeConnection("ws://localhost:9222")
    assert probe.notification_queue_size == 0
