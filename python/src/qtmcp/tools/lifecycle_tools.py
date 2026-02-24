"""Lifecycle tools — launch applications and inject probes."""

from __future__ import annotations

import asyncio
import logging
import os
import platform
import shutil

from fastmcp import Context, FastMCP

logger = logging.getLogger(__name__)


def register_lifecycle_tools(mcp: FastMCP) -> None:
    """Register application launch and probe injection tools."""

    @mcp.tool
    async def qtmcp_launch_app(
        target: str,
        qt_path: str | None = None,
        port: int | None = None,
        qt_version: str | None = None,
        connect_timeout: float | None = None,
        ctx: Context = None,
    ) -> dict:
        """Launch a Qt application with the QtMCP probe injected.

        Starts the target executable via the qtmcp-launch tool, which
        handles probe injection automatically (LD_PRELOAD on Linux,
        DLL injection on Windows). Then waits for the probe's WebSocket
        server to come online and connects to it.

        Args:
            target: Path to the Qt application executable.
            qt_path: Path to Qt lib/bin directory (overrides server default).
            port: WebSocket port for the probe (default: server config or 9222).
            qt_version: Qt version for probe selection (e.g. "5.15", "6.8").
            connect_timeout: Seconds to wait for connection (default: server config).

        Example: qtmcp_launch_app(target="/path/to/myapp")
        Example: qtmcp_launch_app(target="C:\\myapp.exe", qt_path="C:\\Qt\\6.8\\bin")
        """
        from qtmcp.server import (
            build_launch_env,
            get_config,
            get_discovery,
            wait_for_probe_ready,
        )

        config = get_config()
        actual_port = port or config.default_port
        actual_qt_path = qt_path or config.qt_path
        actual_qt_version = qt_version or config.qt_version
        actual_timeout = connect_timeout or config.connect_timeout

        # Resolve launcher binary
        from qtmcp.download import get_launcher_filename

        launcher = (
            config.launcher_path
            or os.environ.get("QTMCP_LAUNCHER")
            or get_launcher_filename()
        )

        # Build launch command
        launch_args = [launcher, target, "--port", str(actual_port), "--detach"]
        if actual_qt_version:
            launch_args.extend(["--qt-version", actual_qt_version])

        logger.info("Launching %s via %s on port %d", target, launcher, actual_port)

        # Build environment with Qt path
        launch_env = build_launch_env(actual_qt_path)

        try:
            process = await asyncio.create_subprocess_exec(
                *launch_args,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                env=launch_env,
            )
        except FileNotFoundError:
            return {
                "launched": False,
                "error": (
                    f"Launcher not found: {launcher!r}. "
                    "Install with: qtmcp download-tools --qt-version <VERSION>"
                ),
            }
        except OSError as e:
            return {
                "launched": False,
                "error": f"Could not start launcher {launcher!r}: {e}",
            }

        # Track the process for cleanup on server shutdown
        config.managed_processes.append(process)

        # Wait for probe and connect
        ws_url = f"ws://localhost:{actual_port}"
        try:
            await wait_for_probe_ready(
                ws_url,
                timeout=actual_timeout,
                discovery=get_discovery(),
                process=process,
            )
            return {
                "launched": True,
                "connected": True,
                "target": target,
                "ws_url": ws_url,
                "pid": process.pid,
            }
        except ConnectionError as e:
            return {
                "launched": True,
                "connected": False,
                "target": target,
                "ws_url": ws_url,
                "pid": process.pid,
                "error": str(e),
            }

    @mcp.tool
    async def qtmcp_inject_probe(
        pid: int,
        probe_path: str | None = None,
        qt_path: str | None = None,
        port: int | None = None,
        connect_timeout: float | None = None,
        ctx: Context = None,
    ) -> dict:
        """Inject the QtMCP probe into a running Qt application.

        Loads the probe shared library into the target process so it can be
        controlled via QtMCP. On Linux this uses gdb to call dlopen in the
        target process (requires gdb and ptrace permissions). On Windows
        this is not yet supported — use qtmcp_launch_app instead.

        After injection the probe starts its WebSocket server and this tool
        auto-connects to it.

        Args:
            pid: Process ID of the running Qt application.
            probe_path: Explicit path to probe library. Auto-detected if omitted.
            qt_path: Path to Qt lib/bin directory (for LD_LIBRARY_PATH).
            port: Expected WebSocket port for the probe (default: 9222).
            connect_timeout: Seconds to wait for connection (default: server config).

        Example: qtmcp_inject_probe(pid=12345)
        Example: qtmcp_inject_probe(pid=12345, probe_path="/opt/qtmcp/libqtmcp-probe-qt6.8.so")
        """
        from qtmcp.server import get_config, get_discovery, wait_for_probe_ready

        config = get_config()
        actual_port = port or config.default_port
        actual_timeout = connect_timeout or config.connect_timeout

        is_windows = platform.system() == "Windows"

        if is_windows:
            return {
                "injected": False,
                "error": (
                    "Runtime injection into running processes is not yet supported "
                    "on Windows. Use qtmcp_launch_app to start the application "
                    "with the probe from the beginning."
                ),
            }

        # --- Linux injection via gdb + dlopen ---

        # Check gdb is available
        gdb_path = shutil.which("gdb")
        if not gdb_path:
            return {
                "injected": False,
                "error": (
                    "gdb is required for runtime probe injection on Linux but "
                    "was not found in PATH. Install it with: "
                    "apt install gdb (Debian/Ubuntu) or yum install gdb (RHEL/Fedora)."
                ),
            }

        # Resolve probe library path
        actual_probe_path = probe_path
        if not actual_probe_path:
            actual_probe_path = _find_probe_library(config.qt_version)
        if not actual_probe_path:
            return {
                "injected": False,
                "error": (
                    "Could not find the QtMCP probe library. Provide probe_path "
                    "explicitly, or run: qtmcp download-tools --qt-version <VERSION>"
                ),
            }

        if not os.path.isfile(actual_probe_path):
            return {
                "injected": False,
                "error": f"Probe library not found at: {actual_probe_path}",
            }

        # Build environment for gdb (needs Qt libs on LD_LIBRARY_PATH)
        gdb_env = None
        effective_qt_path = qt_path or config.qt_path
        if effective_qt_path:
            gdb_env = os.environ.copy()
            existing = gdb_env.get("LD_LIBRARY_PATH", "")
            if existing:
                gdb_env["LD_LIBRARY_PATH"] = effective_qt_path + os.pathsep + existing
            else:
                gdb_env["LD_LIBRARY_PATH"] = effective_qt_path

        # Set QTMCP_PORT in the target via gdb, then dlopen the probe
        abs_probe = os.path.abspath(actual_probe_path)
        gdb_commands = [
            # Set the port environment variable before loading the probe
            f'call (int)setenv("QTMCP_PORT", "{actual_port}", 1)',
            # Load the probe library — constructor + Q_COREAPP_STARTUP_FUNCTION
            # will auto-initialize since QCoreApplication already exists
            f'call (void*)dlopen("{abs_probe}", 2)',
            "detach",
            "quit",
        ]

        logger.info("Injecting probe into PID %d via gdb", pid)
        gdb_args = [gdb_path, "-batch", "-p", str(pid)]
        for cmd in gdb_commands:
            gdb_args.extend(["-ex", cmd])

        try:
            result = await asyncio.create_subprocess_exec(
                *gdb_args,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                env=gdb_env,
            )
            stdout, stderr = await asyncio.wait_for(
                result.communicate(), timeout=30.0
            )
        except asyncio.TimeoutError:
            return {
                "injected": False,
                "error": "gdb timed out after 30 seconds",
            }
        except OSError as e:
            return {
                "injected": False,
                "error": f"Failed to run gdb: {e}",
            }

        if result.returncode != 0:
            stderr_text = stderr.decode("utf-8", errors="replace").strip()
            # Common issue: ptrace not allowed
            if "ptrace" in stderr_text.lower() or "not permitted" in stderr_text.lower():
                return {
                    "injected": False,
                    "error": (
                        f"gdb could not attach to PID {pid} (permission denied). "
                        "On Linux, ensure ptrace is allowed: "
                        "echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope"
                    ),
                }
            return {
                "injected": False,
                "error": f"gdb exited with code {result.returncode}: {stderr_text[:500]}",
            }

        logger.info("Probe injected into PID %d, waiting for WebSocket", pid)

        # Wait for the probe to start and connect
        ws_url = f"ws://localhost:{actual_port}"
        try:
            await wait_for_probe_ready(
                ws_url,
                timeout=actual_timeout,
                discovery=get_discovery(),
            )
            return {
                "injected": True,
                "connected": True,
                "pid": pid,
                "ws_url": ws_url,
                "probe_path": abs_probe,
            }
        except ConnectionError as e:
            return {
                "injected": True,
                "connected": False,
                "pid": pid,
                "ws_url": ws_url,
                "error": str(e),
            }


def _find_probe_library(qt_version: str | None) -> str | None:
    """Search common locations for the QtMCP probe library.

    Looks adjacent to the launcher binary, in the current directory,
    and in standard install locations.
    """
    is_windows = platform.system() == "Windows"

    if is_windows:
        glob_patterns = ["qtmcp-probe*.dll"]
    else:
        glob_patterns = ["libqtmcp-probe*.so", "qtmcp-probe*.so"]

    search_dirs = [
        os.getcwd(),
        os.path.dirname(shutil.which("qtmcp-launch") or ""),
    ]

    import glob as glob_mod

    for directory in search_dirs:
        if not directory or not os.path.isdir(directory):
            continue
        for pattern in glob_patterns:
            matches = glob_mod.glob(os.path.join(directory, pattern))
            if qt_version:
                tag = f"qt{qt_version}"
                matches = [m for m in matches if tag in os.path.basename(m)]
            if len(matches) == 1:
                return matches[0]
            if len(matches) > 1:
                # Multiple matches — pick the first alphabetically
                logger.warning(
                    "Multiple probe libraries found: %s. Using %s. "
                    "Specify probe_path explicitly for a specific one.",
                    matches,
                    sorted(matches)[0],
                )
                return sorted(matches)[0]

    return None
