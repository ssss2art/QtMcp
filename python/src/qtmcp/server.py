"""FastMCP server factory with lifespan-managed WebSocket connection."""

from __future__ import annotations

import asyncio
import json
import logging
import os
from contextlib import asynccontextmanager
from typing import AsyncIterator

from fastmcp import FastMCP

from qtmcp.connection import ProbeConnection

logger = logging.getLogger(__name__)

# Module-level reference so resources/tools can access the connection
_probe: ProbeConnection | None = None
_mode: str = "native"


def get_probe() -> ProbeConnection:
    """Get the current probe connection. Raises RuntimeError if not connected."""
    if _probe is None:
        raise RuntimeError("Probe connection not initialized")
    return _probe


def get_mode() -> str:
    """Get the current server mode."""
    return _mode


def create_server(
    mode: str,
    ws_url: str,
    target: str | None = None,
    port: int = 9222,
    launcher_path: str | None = None,
) -> FastMCP:
    """Create a FastMCP server for the given mode.

    Args:
        mode: API mode - "native", "cu", or "chrome".
        ws_url: WebSocket URL of the QtMCP probe.
        target: Optional path to Qt application exe to auto-launch.
        port: Port for auto-launched probe.
        launcher_path: Optional path to qtmcp-launch executable.

    Returns:
        Configured FastMCP instance ready to run.
    """
    global _mode
    _mode = mode

    @asynccontextmanager
    async def lifespan(server: FastMCP) -> AsyncIterator[dict]:
        global _probe
        process = None

        try:
            actual_ws_url = ws_url

            if target is not None:
                launcher = (
                    launcher_path
                    or os.environ.get("QTMCP_LAUNCHER")
                    or "qtmcp-launch"
                )
                logger.debug(
                    "Launching target %s via %s on port %d", target, launcher, port
                )
                process = await asyncio.create_subprocess_exec(
                    launcher,
                    target,
                    "--port",
                    str(port),
                    stdout=asyncio.subprocess.PIPE,
                    stderr=asyncio.subprocess.PIPE,
                )
                # Wait for probe to start
                await asyncio.sleep(1.5)
                actual_ws_url = f"ws://localhost:{port}"

            conn = ProbeConnection(actual_ws_url)
            await conn.connect()
            _probe = conn

            yield {"probe": conn}

        finally:
            _probe = None
            if conn is not None:
                await conn.disconnect()
            if process is not None:
                process.terminate()
                try:
                    await asyncio.wait_for(process.wait(), timeout=5.0)
                except asyncio.TimeoutError:
                    process.kill()

    mcp = FastMCP(f"QtMCP {mode.title()}", lifespan=lifespan)

    # Register mode-specific tools
    if mode == "native":
        from qtmcp.tools.native import register_native_tools

        register_native_tools(mcp)
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
