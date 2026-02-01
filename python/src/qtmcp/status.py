"""Status resource for exposing probe connection state."""

from __future__ import annotations

import json
import logging

from fastmcp import FastMCP

logger = logging.getLogger(__name__)


def register_status_resource(mcp: FastMCP) -> None:
    """Register the qtmcp://status resource on the server."""

    @mcp.resource("qtmcp://status")
    def probe_status() -> str:
        """Current probe connection status."""
        from qtmcp.server import get_mode, get_probe

        try:
            probe = get_probe()
            return json.dumps({
                "connected": probe.is_connected,
                "ws_url": probe.ws_url,
                "mode": get_mode(),
            })
        except RuntimeError:
            return json.dumps({
                "connected": False,
                "ws_url": None,
                "mode": get_mode(),
            })
