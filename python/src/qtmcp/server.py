"""FastMCP server factory with lifespan-managed WebSocket connection."""

from __future__ import annotations

import asyncio
import logging
import os
import platform
from contextlib import asynccontextmanager
from dataclasses import dataclass, field
from typing import AsyncIterator

from fastmcp import FastMCP

from qtmcp.connection import ProbeConnection, ProbeError
from qtmcp.discovery import DiscoveryListener
from qtmcp.event_recorder import EventRecorder

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Server configuration — set once at startup, read by tools
# ---------------------------------------------------------------------------


@dataclass
class ServerConfig:
    """Settings passed from CLI args, available to MCP tools."""

    mode: str = "native"
    qt_path: str | None = None
    launcher_path: str | None = None
    qt_version: str | None = None
    default_port: int = 9222
    connect_timeout: float = 30.0
    discovery_port: int = 9221

    # Processes launched by the launch tool, tracked for cleanup
    managed_processes: list = field(default_factory=list)


_config = ServerConfig()

# Module-level state so resources/tools can access the connection and discovery
_probe: ProbeConnection | None = None
_discovery: DiscoveryListener | None = None
_recorder: EventRecorder = EventRecorder()
_mode: str = "native"


def get_config() -> ServerConfig:
    """Get the current server configuration."""
    return _config


def get_probe() -> ProbeConnection | None:
    """Get the current probe connection, or None if not connected."""
    return _probe


def require_probe() -> ProbeConnection:
    """Get the current probe connection. Raises ProbeError if not connected.

    Tools should call this to get a connected probe; the error message
    guides Claude to use qtmcp_launch_app, qtmcp_list_probes, or
    qtmcp_connect_probe.
    """
    if _probe is None or not _probe.is_connected:
        raise ProbeError(
            "No probe connected. Use qtmcp_launch_app to launch a Qt application "
            "with the probe, or qtmcp_list_probes to see already-running probes, "
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


def build_launch_env(qt_path: str | None) -> dict[str, str] | None:
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


async def wait_for_probe_ready(
    ws_url: str,
    timeout: float,
    discovery: DiscoveryListener | None = None,
    process: asyncio.subprocess.Process | None = None,
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
    port: int = 9222,
    launcher_path: str | None = None,
    discovery_port: int = 9221,
    discovery_enabled: bool = True,
    qt_version: str | None = None,
    qt_path: str | None = None,
    connect_timeout: float = 30.0,
) -> FastMCP:
    """Create a FastMCP server for the given mode.

    The server starts with only the discovery listener active.
    No application is launched and no probe connection is made at startup.
    Use the qtmcp_launch_app or qtmcp_inject_probe tools to start an
    application and connect to its probe.

    Args:
        mode: API mode - "native", "cu", or "chrome".
        port: Default WebSocket port for probes (default: 9222).
        launcher_path: Optional path to qtmcp-launch executable.
        discovery_port: UDP port for probe discovery (default: 9221).
        discovery_enabled: Whether to start the discovery listener.
        qt_version: Optional Qt version for probe auto-detection (e.g. "5.15").
        qt_path: Optional path to Qt lib/bin dir, prepended to library search path.
        connect_timeout: Max seconds to wait for probe connection (default: 30).

    Returns:
        Configured FastMCP instance ready to run.
    """
    global _mode, _config
    _mode = mode
    _config = ServerConfig(
        mode=mode,
        qt_path=qt_path,
        launcher_path=launcher_path,
        qt_version=qt_version,
        default_port=port,
        connect_timeout=connect_timeout,
        discovery_port=discovery_port,
    )

    @asynccontextmanager
    async def lifespan(server: FastMCP) -> AsyncIterator[dict]:
        global _discovery

        try:
            # Start discovery listener — the only startup action.
            # Tools handle launching apps and connecting to probes.
            if discovery_enabled:
                _discovery = DiscoveryListener(port=discovery_port)
                await _discovery.start()
                logger.info("QtMCP server ready (mode=%s). Use qtmcp_launch_app "
                            "or qtmcp_connect_probe to connect to a Qt application.", mode)
            else:
                logger.info("QtMCP server ready (mode=%s, discovery disabled).", mode)

            yield {"discovery": _discovery}

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

            # Terminate any processes launched by tools
            for process in _config.managed_processes:
                try:
                    process.terminate()
                    await asyncio.wait_for(process.wait(), timeout=5.0)
                except (asyncio.TimeoutError, ProcessLookupError):
                    try:
                        process.kill()
                    except ProcessLookupError:
                        pass
            _config.managed_processes.clear()

    mcp = FastMCP(f"QtMCP {mode.title()}", lifespan=lifespan)

    # Register discovery tools (always available regardless of mode)
    from qtmcp.tools.discovery_tools import register_discovery_tools

    register_discovery_tools(mcp)

    # Register lifecycle tools (launch / inject — always available)
    from qtmcp.tools.lifecycle_tools import register_lifecycle_tools

    register_lifecycle_tools(mcp)

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
