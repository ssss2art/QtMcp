"""CLI entry point for the QtMCP MCP server."""

from __future__ import annotations

import argparse
import logging
import os
import sys


def main() -> None:
    """Parse arguments and run the MCP server."""
    logging.basicConfig(level=logging.DEBUG, stream=sys.stderr)

    parser = argparse.ArgumentParser(
        prog="qtmcp",
        description="MCP server for controlling Qt applications via QtMCP probe",
    )
    parser.add_argument(
        "--mode",
        required=True,
        choices=["native", "cu", "chrome"],
        help="API mode to expose (native, cu, or chrome)",
    )
    parser.add_argument(
        "--ws-url",
        default=os.environ.get("QTMCP_WS_URL", "ws://localhost:9222"),
        help="WebSocket URL of the QtMCP probe (default: ws://localhost:9222)",
    )
    parser.add_argument(
        "--target",
        default=None,
        help="Path to Qt application exe to auto-launch",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("QTMCP_PORT", "9222")),
        help="Port for auto-launched probe (default: 9222)",
    )
    parser.add_argument(
        "--launcher-path",
        default=os.environ.get("QTMCP_LAUNCHER"),
        help="Path to qtmcp-launch executable",
    )

    args = parser.parse_args()

    ws_url = args.ws_url
    if args.target:
        ws_url = f"ws://localhost:{args.port}"

    from qtmcp.server import create_server

    server = create_server(
        mode=args.mode,
        ws_url=ws_url,
        target=args.target,
        port=args.port,
        launcher_path=args.launcher_path,
    )
    server.run()
