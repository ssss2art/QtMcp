"""CLI entry point for the QtMCP MCP server."""

from __future__ import annotations

import argparse
import logging
import os
import sys


def cmd_serve(args: argparse.Namespace) -> int:
    """Run the MCP server."""
    # ws_url is None unless explicitly provided or a target is specified
    ws_url = None
    if args.target:
        ws_url = f"ws://localhost:{args.port}"
    elif args.ws_url:
        ws_url = args.ws_url

    from qtmcp.server import create_server

    server = create_server(
        mode=args.mode,
        ws_url=ws_url,
        target=args.target,
        port=args.port,
        launcher_path=args.launcher_path,
        discovery_port=args.discovery_port,
        discovery_enabled=not args.no_discovery,
    )
    server.run()
    return 0


def cmd_download_probe(args: argparse.Namespace) -> int:
    """Download probe binary from GitHub Releases."""
    from qtmcp.download import (
        AVAILABLE_VERSIONS,
        ChecksumError,
        DownloadError,
        UnsupportedPlatformError,
        VersionNotFoundError,
        download_probe,
    )

    try:
        path = download_probe(
            qt_version=args.qt_version,
            output_dir=args.output_dir,
            verify_checksum_flag=not args.no_verify,
            release_tag=args.release,
            compiler=args.compiler,
        )
        print(f"Downloaded probe to: {path}")
        return 0
    except VersionNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        print(f"Available versions: {', '.join(sorted(AVAILABLE_VERSIONS))}", file=sys.stderr)
        return 1
    except UnsupportedPlatformError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    except ChecksumError as e:
        print(f"Error: {e}", file=sys.stderr)
        print("Try --no-verify to skip checksum verification (not recommended).", file=sys.stderr)
        return 1
    except DownloadError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


def create_parser() -> argparse.ArgumentParser:
    """Create the argument parser with subcommands."""
    parser = argparse.ArgumentParser(
        prog="qtmcp",
        description="QtMCP - MCP server for controlling Qt applications",
    )

    subparsers = parser.add_subparsers(
        title="commands",
        dest="command",
        required=True,
        metavar="COMMAND",
    )

    # --- serve subcommand ---
    serve_parser = subparsers.add_parser(
        "serve",
        help="Run the MCP server",
        description="Start the MCP server to control Qt applications via the QtMCP probe.",
    )
    serve_parser.add_argument(
        "--mode",
        required=True,
        choices=["native", "cu", "chrome"],
        help="API mode to expose (native, cu, or chrome)",
    )
    serve_parser.add_argument(
        "--ws-url",
        default=os.environ.get("QTMCP_WS_URL"),
        help="WebSocket URL of the QtMCP probe (auto-connect on startup)",
    )
    serve_parser.add_argument(
        "--target",
        default=None,
        help="Path to Qt application exe to auto-launch",
    )
    serve_parser.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("QTMCP_PORT", "9222")),
        help="Port for auto-launched probe (default: 9222)",
    )
    serve_parser.add_argument(
        "--launcher-path",
        default=os.environ.get("QTMCP_LAUNCHER"),
        help="Path to qtmcp-launch executable",
    )
    serve_parser.add_argument(
        "--discovery-port",
        type=int,
        default=int(os.environ.get("QTMCP_DISCOVERY_PORT", "9221")),
        help="UDP port for probe discovery (default: 9221)",
    )
    serve_parser.add_argument(
        "--no-discovery",
        action="store_true",
        help="Disable UDP probe discovery",
    )
    serve_parser.set_defaults(func=cmd_serve)

    # --- download-probe subcommand ---
    download_parser = subparsers.add_parser(
        "download-probe",
        help="Download probe binary from GitHub Releases",
        description=(
            "Download the QtMCP probe DLL/SO for your Qt version from GitHub Releases.\n\n"
            "Available Qt versions: 5.15, 5.15-patched, 6.5, 6.8, 6.9\n\n"
            "Platform and compiler are auto-detected:\n"
            "  - Linux   -> linux-gcc13   (.so)\n"
            "  - Windows -> windows-msvc17 (.dll)\n\n"
            "Override the compiler with --compiler (e.g., gcc14, mingw13, clang18).\n\n"
            "Example:\n"
            "  qtmcp download-probe --qt-version 6.8\n"
            "  qtmcp download-probe --qt-version 5.15-patched --output-dir ./lib\n"
            "  qtmcp download-probe --qt-version 6.8 --compiler gcc14"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    download_parser.add_argument(
        "--qt-version",
        required=True,
        metavar="VERSION",
        help="Qt version to download probe for (e.g., 6.8, 5.15, 5.15-patched)",
    )
    download_parser.add_argument(
        "--output-dir",
        "-o",
        default=None,
        metavar="DIR",
        help="Directory to save the probe (default: current directory)",
    )
    download_parser.add_argument(
        "--no-verify",
        action="store_true",
        help="Skip SHA256 checksum verification (not recommended)",
    )
    download_parser.add_argument(
        "--release",
        default="latest",
        metavar="TAG",
        help="Release tag to download from (default: latest)",
    )
    download_parser.add_argument(
        "--compiler",
        default=None,
        metavar="COMPILER",
        help="Compiler suffix (e.g., gcc13, msvc17, mingw13). Auto-detected if omitted.",
    )
    download_parser.set_defaults(func=cmd_download_probe)

    return parser


def main() -> None:
    """Parse arguments and run the appropriate command."""
    logging.basicConfig(level=logging.DEBUG, stream=sys.stderr)

    parser = create_parser()
    args = parser.parse_args()

    # Run the command and exit with its return code
    sys.exit(args.func(args))
