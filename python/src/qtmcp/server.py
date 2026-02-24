"""FastMCP server factory with lifespan-managed WebSocket connection."""

from __future__ import annotations

import asyncio
import logging
import os
import platform
from contextlib import asynccontextmanager
from typing import AsyncIterator

from fastmcp import FastMCP

from qtmcp.connection import ProbeConnection, ProbeError
from qtmcp.discovery import DiscoveryListener
from qtmcp.event_recorder import EventRecorder

logger = logging.getLogger(__name__)

# Module-level state so resources/tools can access the connection and discovery
_probe: ProbeConnection | None = None
_discovery: DiscoveryListener | None = None
_recorder: EventRecorder = EventRecorder()
_mode: str = "native"


def get_probe() -> ProbeConnection | None:
    """Get the current probe connection, or None if not connected."""
    return _probe


def require_probe() -> ProbeConnection:
    """Get the current probe connection. Raises ProbeError if not connected.

    Tools should call this to get a connected probe; the error message
    guides Claude to use qtmcp_list_probes / qtmcp_connect_probe.
    """
    if _probe is None or not _probe.is_connected:
        raise ProbeError(
            "No probe connected. Use qtmcp_list_probes to see available probes, "
            "then qtmcp_connect_probe to connect to one."
        )
    return _probe


def get_discovery() -> DiscoveryListener | None:
    """Get the discovery listener, or None if discovery is disabled."""
    return _discovery


def get_mode() -> str:
    """Get the current server mode."""
    return _mode


def get_recorder() -> EventRecorder:
    """Get the shared EventRecorder instance."""
    return _recorder


async def connect_to_probe(ws_url: str) -> ProbeConnection:
    """Connect to a probe at the given WebSocket URL.

    Disconnects any existing probe connection first.
    """
    global _probe

    if _probe is not None and _probe.is_connected:
        logger.info("Disconnecting from current probe at %s", _probe.ws_url)
        await _probe.disconnect()
        _probe = None

    conn = ProbeConnection(ws_url)
    await conn.connect()
    _probe = conn
    logger.info("Connected to probe at %s", ws_url)
    return conn


async def disconnect_probe() -> None:
    """Disconnect the current probe connection if any."""
    global _probe
    if _probe is not None:
        await _probe.disconnect()
        _probe = None


def _build_launch_env(qt_path: str | None) -> dict[str, str] | None:
    """Build an environment dict with Qt library paths prepended.

    Returns None (inherit current env) when no qt_path is given.
    """
    if not qt_path:
        return None

    env = os.environ.copy()
    is_windows = platform.system() == "Windows"

    if is_windows:
        # On Windows, Qt DLLs are found via PATH
        existing = env.get("PATH", "")
        env["PATH"] = qt_path + os.pathsep + existing
        logger.info("Prepended %s to PATH for target launch", qt_path)
    else:
        # On Linux, Qt .so files are found via LD_LIBRARY_PATH
        existing = env.get("LD_LIBRARY_PATH", "")
        if existing:
            env["LD_LIBRARY_PATH"] = qt_path + os.pathsep + existing
        else:
            env["LD_LIBRARY_PATH"] = qt_path
        logger.info("Prepended %s to LD_LIBRARY_PATH for target launch", qt_path)

    return env


async def _wait_for_probe_ready(
    ws_url: str,
    timeout: float,
    discovery: DiscoveryListener | None,
    process: asyncio.subprocess.Process | None,
) -> None:
    """Wait for the probe to become connectable using retry with backoff.

    If discovery is available, we first wait for a discovery announcement
    (which means the probe's WebSocket server is listening) before
    attempting the WebSocket connection. This avoids hammering a port
    that isn't open yet.

    Falls back to pure retry-with-backoff when discovery is unavailable.
    """
    deadline = asyncio.get_event_loop().time() + timeout

    # Phase 1: If we have discovery, wait for the probe to announce itself.
    # The C++ probe sends its first UDP announce immediately after the WS
    # server starts listening, so this is a reliable readiness signal.
    if discovery is not None:
        remaining = deadline - asyncio.get_event_loop().time()
        if remaining > 0:
            logger.info(
                "Waiting up to %.1fs for probe discovery announcement...", remaining
            )
            probe_info = await discovery.wait_for_probe(timeout=min(remaining, timeout))
            if probe_info:
                logger.info(
                    "Probe discovered: %s (pid=%d) at %s",
                    probe_info.app_name,
                    probe_info.pid,
                    probe_info.ws_url,
                )

    # Phase 2: Retry WebSocket connection with exponential backoff.
    # Even after discovery, the WS handshake can race briefly.
    delay = 0.5
    max_delay = 4.0
    attempt = 0

    while True:
        remaining = deadline - asyncio.get_event_loop().time()
        if remaining <= 0:
            raise ConnectionError(
                f"Timed out after {timeout:.0f}s waiting for probe at {ws_url}. "
                "Check that the target application started correctly and "
                "the probe loaded (look for '[QtMCP]' messages in stderr)."
            )

        # Check if the launched process crashed
        if process is not None and process.returncode is not None:
            stderr_data = b""
            if process.stderr:
                try:
                    stderr_data = await asyncio.wait_for(
                        process.stderr.read(4096), timeout=1.0
                    )
                except (asyncio.TimeoutError, Exception):
                    pass
            stderr_text = stderr_data.decode("utf-8", errors="replace").strip()
            msg = f"Target process exited with code {process.returncode}"
            if stderr_text:
                msg += f": {stderr_text[:500]}"
            raise ConnectionError(msg)

        attempt += 1
        try:
            await connect_to_probe(ws_url)
            logger.info("Probe connection established (attempt %d)", attempt)
            return
        except Exception as e:
            logger.debug(
                "Connection attempt %d to %s failed: %s", attempt, ws_url, e
            )
            await asyncio.sleep(min(delay, remaining))
            delay = min(delay * 2, max_delay)


def create_server(
    mode: str,
    ws_url: str | None = None,
    target: str | None = None,
    port: int = 9222,
    launcher_path: str | None = None,
    discovery_port: int = 9221,
    discovery_enabled: bool = True,
    qt_version: str | None = None,
    qt_path: str | None = None,
    connect_timeout: float = 30.0,
) -> FastMCP:
    """Create a FastMCP server for the given mode.

    Args:
        mode: API mode - "native", "cu", or "chrome".
        ws_url: Optional WebSocket URL to auto-connect on startup.
        target: Optional path to Qt application exe to auto-launch.
        port: Port for auto-launched probe.
        launcher_path: Optional path to qtmcp-launch executable.
        discovery_port: UDP port for probe discovery (default: 9221).
        discovery_enabled: Whether to start the discovery listener.
        qt_version: Optional Qt version for probe auto-detection (e.g. "5.15").
        qt_path: Optional path to Qt lib/bin dir, prepended to library search path.
        connect_timeout: Max seconds to wait for probe connection on startup.

    Returns:
        Configured FastMCP instance ready to run.
    """
    global _mode
    _mode = mode

    @asynccontextmanager
    async def lifespan(server: FastMCP) -> AsyncIterator[dict]:
        global _probe, _discovery
        process = None

        try:
            # Start discovery listener
            if discovery_enabled:
                _discovery = DiscoveryListener(port=discovery_port)
                await _discovery.start()

            # Auto-launch target if specified
            actual_ws_url = ws_url
            if target is not None:
                from qtmcp.download import get_launcher_filename

                launcher = (
                    launcher_path
                    or os.environ.get("QTMCP_LAUNCHER")
                    or get_launcher_filename()
                )
                logger.info(
                    "Launching target %s via %s on port %d", target, launcher, port
                )
                launch_args = [
                    launcher,
                    target,
                    "--port",
                    str(port),
                ]
                if qt_version:
                    launch_args.extend(["--qt-version", qt_version])

                # Build environment with Qt path if provided
                launch_env = _build_launch_env(qt_path)

                try:
                    process = await asyncio.create_subprocess_exec(
                        *launch_args,
                        stdout=asyncio.subprocess.PIPE,
                        stderr=asyncio.subprocess.PIPE,
                        env=launch_env,
                    )
                except FileNotFoundError:
                    raise FileNotFoundError(
                        f"Launcher not found: {launcher!r}. "
                        "Install with: qtmcp download-tools --qt-version <VERSION>"
                    )
                except OSError as e:
                    raise OSError(
                        f"Could not start launcher {launcher!r}: {e}. "
                        "Install with: qtmcp download-tools --qt-version <VERSION>"
                    ) from e
                actual_ws_url = f"ws://localhost:{port}"

            # Auto-connect with retry when a URL is available
            if actual_ws_url is not None:
                try:
                    await _wait_for_probe_ready(
                        actual_ws_url,
                        timeout=connect_timeout,
                        discovery=_discovery if target is not None else None,
                        process=process,
                    )
                except ConnectionError as e:
                    logger.warning(
                        "Could not auto-connect to %s: %s. "
                        "Use qtmcp_list_probes and qtmcp_connect_probe to connect later.",
                        actual_ws_url,
                        e,
                    )
                except Exception as e:
                    logger.warning(
                        "Could not auto-connect to %s: %s. "
                        "Use qtmcp_list_probes and qtmcp_connect_probe to connect later.",
                        actual_ws_url,
                        e,
                    )

            yield {"probe": _probe, "discovery": _discovery}

        finally:
            # Stop recording if active
            if _recorder.is_recording and _probe is not None and _probe.is_connected:
                try:
                    await _recorder.stop(_probe)
                except Exception:
                    logger.debug("Failed to stop recording during shutdown", exc_info=True)

            # Disconnect probe
            await disconnect_probe()

            # Stop discovery
            if _discovery is not None:
                await _discovery.stop()
                _discovery = None

            # Terminate launched process
            if process is not None:
                process.terminate()
                try:
                    await asyncio.wait_for(process.wait(), timeout=5.0)
                except asyncio.TimeoutError:
                    process.kill()

    mcp = FastMCP(f"QtMCP {mode.title()}", lifespan=lifespan)

    # Register discovery tools (always available regardless of mode)
    from qtmcp.tools.discovery_tools import register_discovery_tools

    register_discovery_tools(mcp)

    # Register mode-specific tools
    if mode == "native":
        from qtmcp.tools.native import register_native_tools

        register_native_tools(mcp)

        from qtmcp.tools.recording_tools import register_recording_tools

        register_recording_tools(mcp)
    elif mode == "cu":
        from qtmcp.tools.cu import register_cu_tools

        register_cu_tools(mcp)
    elif mode == "chrome":
        from qtmcp.tools.chrome import register_chrome_tools

        register_chrome_tools(mcp)

    # Register status resource
    from qtmcp.status import register_status_resource

    register_status_resource(mcp)

    return mcp
