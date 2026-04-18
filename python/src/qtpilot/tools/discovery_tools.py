"""Discovery tools -- always registered regardless of mode."""

from __future__ import annotations

from fastmcp import Context, FastMCP


def register_discovery_tools(mcp: FastMCP) -> None:
    """Register probe discovery and connection management tools."""

    @mcp.tool
    async def qtpilot_list_probes(ctx: Context) -> dict:
        """List all qtPilot probes discovered on the network via UDP broadcast.

        Returns a list of probes with their app name, PID, Qt version,
        WebSocket URL, and connection status. Use qtpilot_connect_probe
        to connect to one.

        Example: qtpilot_list_probes()
        """
        from qtpilot.server import get_discovery, get_probe

        discovery = get_discovery()
        if discovery is None:
            return {"error": "Discovery is not enabled"}

        # Prune stale probes before listing
        discovery.prune_stale()

        current_probe = get_probe()
        current_url = current_probe.ws_url if current_probe and current_probe.is_connected else None

        probes = []
        for probe in discovery.probes.values():
            probes.append({
                "app_name": probe.app_name,
                "pid": probe.pid,
                "qt_version": probe.qt_version,
                "ws_url": probe.ws_url,
                "hostname": probe.hostname,
                "mode": probe.mode,
                "uptime": round(probe.uptime, 1),
                "connected": probe.ws_url == current_url,
            })

        return {"probes": probes, "count": len(probes)}

    @mcp.tool
    async def qtpilot_connect_probe(ws_url: str, ctx: Context = None) -> dict:
        """Connect to a qtPilot probe by its WebSocket URL.

        Disconnects any existing probe connection first.
        Get available URLs from qtpilot_list_probes.

        Args:
            ws_url: WebSocket URL (e.g. "ws://192.168.1.100:9222")

        Example: qtpilot_connect_probe(ws_url="ws://localhost:9222")
        """
        from qtpilot.server import connect_to_probe

        try:
            conn = await connect_to_probe(ws_url)
            return {"connected": True, "ws_url": conn.ws_url}
        except Exception as e:
            return {"connected": False, "error": str(e)}

    @mcp.tool
    async def qtpilot_disconnect_probe(ctx: Context) -> dict:
        """Disconnect from the currently connected qtPilot probe.

        Example: qtpilot_disconnect_probe()
        """
        from qtpilot.server import disconnect_probe, get_probe

        probe = get_probe()
        if probe is None or not probe.is_connected:
            return {"disconnected": False, "reason": "No probe connected"}

        ws_url = probe.ws_url
        await disconnect_probe()
        return {"disconnected": True, "ws_url": ws_url}

    @mcp.tool
    async def qtpilot_status(ctx: Context = None) -> dict:
        """Get the full qtPilot session status in one call.

        Returns the active mode, the list of available modes, probe connection
        state, and discovery state (including all currently discovered probes).

        Example: qtpilot_status()
        """
        from qtpilot.server import get_discovery, get_probe, get_state

        state = get_state()
        probe = get_probe()
        discovery = get_discovery()

        connection: dict = {
            "connected": probe is not None and probe.is_connected,
        }
        if probe and probe.is_connected:
            connection["ws_url"] = probe.ws_url
            connection["probe_version"] = getattr(probe, "probe_version", None)
            connection["protocol_version"] = getattr(probe, "protocol_version", None)

        disc_info: dict = {
            "active": discovery is not None and discovery.is_running,
            "probes": [],
        }
        if discovery:
            discovery.prune_stale()
            current_url = probe.ws_url if probe and probe.is_connected else None
            for dp in discovery.probes.values():
                disc_info["probes"].append({
                    "ws_url": dp.ws_url,
                    "app_name": dp.app_name,
                    "pid": dp.pid,
                    "qt_version": dp.qt_version,
                    "hostname": dp.hostname,
                    "mode": dp.mode,
                    "uptime": round(dp.uptime, 1),
                    "connected": dp.ws_url == current_url,
                })

        return {
            "mode": state.mode,
            "available_modes": ["native", "cu", "chrome", "all"],
            "connection": connection,
            "discovery": disc_info,
        }

    @mcp.tool
    async def qtPilot_probe_status(ctx: Context) -> dict:
        """Get the current probe connection and discovery status.

        Returns connection state, discovery state, and count of
        discovered probes.

        Example: qtPilot_probe_status()
        """
        from qtpilot.server import get_discovery, get_mode, get_probe

        probe = get_probe()
        discovery = get_discovery()

        result = {
            "mode": get_mode(),
            "connected": probe is not None and probe.is_connected,
            "ws_url": probe.ws_url if probe and probe.is_connected else None,
            "discovery_active": discovery is not None and discovery.is_running,
            "discovered_probes": len(discovery.probes) if discovery else 0,
        }
        return result

    @mcp.tool
    async def qtpilot_get_mode(ctx: Context) -> dict:
        """Get the current API mode and list of available modes.

        Returns the active mode and all valid mode choices.

        Example: qtpilot_get_mode()
        """
        from qtpilot.server import get_state

        state = get_state()
        return {
            "mode": state.mode,
            "available": ["native", "cu", "chrome", "all"],
        }

    @mcp.tool
    async def qtpilot_set_mode(mode: str, ctx: Context) -> dict:
        """Switch the active API mode, changing which tools are available.

        Modes:
        - "native": Qt object introspection (qt_* tools)
        - "cu": Computer use / screenshot-based (cu_* tools)
        - "chrome": Accessibility tree (chr_* tools)
        - "all": All tools from all modes

        Args:
            mode: Target mode - "native", "cu", "chrome", or "all"

        Example: qtpilot_set_mode(mode="native")
        """
        from qtpilot.server import get_state

        state = get_state()
        result = state.set_mode(mode)
        if "error" not in result and result.get("changed", False):
            await ctx.send_tool_list_changed()
        return result
