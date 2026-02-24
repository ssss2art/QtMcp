# Troubleshooting QtMcp

This guide covers common issues and their solutions when using QtMcp.

## Probe Not Loading

### Symptoms
- Application starts but no `[QtMCP]` messages in stderr
- `qtmcp serve` can't connect to the probe
- No WebSocket server on expected port

### Solutions

#### Qt Version Mismatch

The probe must match your application's Qt major.minor version exactly.

**Check your app's Qt version:**
```bash
# Linux
ldd /path/to/app | grep -i qt

# Windows PowerShell
Get-ChildItem "C:\path\to\app" | Where-Object { $_.Name -match "Qt.*\.dll" }
```

**Download the correct probe:**
```bash
# For Qt 6.8
qtmcp download-probe --qt-version 6.8

# For Qt 5.15
qtmcp download-probe --qt-version 5.15-patched
```

#### Windows: DLL Not Found

The `qtmcp.dll` must be discoverable by Windows DLL search order.

**Solutions:**
1. Use `qtmcp_launch_app` which handles paths automatically
2. Put the probe DLL in the same directory as the target app
3. Add the probe directory to PATH

**Verify the DLL can be found:**
```powershell
# Check if dependencies are satisfied
where.exe qtmcp_probe.dll
```

#### Linux: LD_PRELOAD Issues

**Check the preload works:**
```bash
LD_PRELOAD=/path/to/libqtmcp.so ldd /path/to/app
# Should show libqtmcp.so in the list
```

**Common issues:**
- Wrong architecture (32-bit vs 64-bit)
- Missing Qt dependencies for the probe itself
- SELinux or AppArmor blocking preload

**Debug with:**
```bash
LD_DEBUG=libs LD_PRELOAD=/path/to/libqtmcp.so ./app 2>&1 | grep qtmcp
```

#### Probe Loads but Doesn't Initialize

The probe defers initialization until `QCoreApplication` exists. If the app crashes early:

```bash
# Enable verbose stderr output
QTMCP_PORT=9222 ./your-app 2>&1 | grep QtMCP
```

Look for:
- `[QtMCP] Probe singleton created` - DLL loaded successfully
- `[QtMCP] Object hooks installed` - Full initialization completed
- `[QtMCP] ERROR:` - Initialization failed

## Startup / Connection Timing

### Symptoms
- Tools return "No probe connected"
- "Timed out waiting for probe" errors after calling `qtmcp_launch_app`

### Understanding the Startup Sequence

The MCP server starts immediately without launching any application.
When Claude calls `qtmcp_launch_app`, the tool:
1. Launches the target via the launcher (with probe injection)
2. Waits for the probe to announce itself via UDP discovery
3. Retries WebSocket connection with exponential backoff (up to `--connect-timeout` seconds, default 30)

### Solutions

**Increase the timeout** if the app is slow to start (set at server level):
```bash
qtmcp serve --mode native --connect-timeout 60
```

**Specify Qt path** if Qt libs aren't alongside the target:
```bash
# Linux
qtmcp serve --mode native --qt-path /opt/Qt/6.8.0/gcc_64/lib

# Windows
qtmcp serve --mode native --qt-path C:\Qt\6.8.0\msvc2022_64\bin
```

**Check probe stderr output** for initialization messages:
- `[QtMCP] Probe singleton created` — DLL loaded
- `[QtMCP] Probe initialized on port 9222` — ready for connections
- `[QtMCP] ERROR:` — something went wrong

## Connection Issues

### Symptoms
- `qtmcp serve` times out connecting
- "Connection refused" errors
- Probe loads but no WebSocket connection

### Solutions

#### Port Already in Use

Check if something else is using the port:

```bash
# Linux
ss -tlnp | grep 9222
netstat -tlnp | grep 9222

# Windows
netstat -ano | findstr 9222
```

**Change the port:**
```bash
# Set via environment variable before launching app
QTMCP_PORT=9999 ./your-app

# Then ask Claude to connect to the new port:
# qtmcp_connect_probe(ws_url="ws://localhost:9999")
```

#### Firewall Blocking WebSocket

On Windows, ensure the probe can accept connections:

1. Check Windows Firewall settings
2. Add an exception for the port (9222 by default)
3. Or temporarily disable firewall for testing

On Linux, check iptables/nftables/firewalld rules.

#### Wrong WebSocket URL

Ensure the URL matches when connecting manually:
```
# Default URL — ask Claude to run:
qtmcp_connect_probe(ws_url="ws://localhost:9222")

# If using a different port
qtmcp_connect_probe(ws_url="ws://localhost:9999")

# If connecting from a different machine
qtmcp_connect_probe(ws_url="ws://192.168.1.100:9222")
```

#### Probe on Different Host

By default, the probe binds to `localhost` only. For remote connections, this is a security feature. Run the MCP server on the same machine as the Qt application.

## Claude Not Seeing Tools

### Symptoms
- Claude says "I don't have access to QtMcp tools"
- No tools appear in Claude's tool list
- MCP server starts but Claude can't use it

### Solutions

#### Check MCP Server Configuration

**Claude Desktop:** Verify `claude_desktop_config.json`:
```json
{
  "mcpServers": {
    "qtmcp": {
      "command": "qtmcp",
      "args": ["serve", "--mode", "native"]
    }
  }
}
```

**Common mistakes:**
- Typo in "mcpServers" (note the capital S)
- Wrong path to target application
- Missing `qtmcp` in PATH

**Claude Code:** Verify with:
```bash
claude mcp list
```

#### Verify Probe is Running

Before connecting Claude, confirm the probe works:

1. Start the app with probe: `qtmcp-launch /path/to/app --detach`
2. Check for `[QtMCP] Probe initialized` in output
3. In another terminal, test WebSocket: `wscat -c ws://localhost:9222`

#### Mode Mismatch

If using a specific mode, ensure tools match expectations:

- `--mode native` provides Qt-specific tools like `get_object_tree`
- `--mode chrome` provides web-like tools like `read_page`
- `--mode cu` provides Computer Use tools like `screenshot`
- `--mode all` provides all tools

## Qt Version Detection

### Determining App's Qt Version

**From the binary:**
```bash
# Linux
strings /path/to/app | grep "Qt version"
objdump -p /path/to/app | grep NEEDED | grep -i qt

# Windows
strings "C:\path\to\app.exe" | findstr "Qt version"
```

**From running application:**
```bash
# If app has About dialog, check there
# Or look at loaded libraries:

# Linux
cat /proc/$(pgrep app-name)/maps | grep -i qt

# Windows (use Process Explorer or similar)
```

### Available Probe Versions

| Your App Uses | Download | Default Compiler |
|---------------|----------|-----------------|
| Qt 6.5, 6.6, 6.7, 6.8 | `--qt-version 6.8` | gcc13 (Linux), msvc17 (Windows) |
| Qt 5.15.x | `--qt-version 5.15-patched` | gcc13 (Linux), msvc17 (Windows) |
| Qt 5.12 and earlier | Not supported | - |

## Platform-Specific Issues

### Windows

#### Visual C++ Runtime Missing

The probe requires the Visual C++ Redistributable. Install from:
https://aka.ms/vs/17/release/vc_redist.x64.exe

**Symptoms:**
- "VCRUNTIME140.dll not found"
- "MSVCP140.dll not found"

#### Qt DLLs Not Found

If using a standalone probe, Qt DLLs must be available.

**Solutions:**
1. Run `windeployqt` on the probe DLL
2. Ensure Qt bin directory is in PATH
3. Copy Qt DLLs next to the probe

#### UAC/Elevation Issues

If the target app requires elevation, run the launcher as administrator:
```powershell
Start-Process -Verb RunAs qtmcp-launch.exe your-app.exe
```

### Linux

#### LD_PRELOAD Caveats

Some applications clear `LD_PRELOAD` or use `setuid`. This can prevent probe loading.

**Workarounds:**
- Launch the app differently (avoid sudo, avoid setuid wrappers)
- Modify the app's launch script to preserve LD_PRELOAD

#### Library Path Issues

If the probe can't find Qt libraries:
```bash
# Option 1: Use --qt-path on the serve command (applies to all launch tool calls)
qtmcp serve --mode native --qt-path /path/to/Qt/6.8.0/gcc_64/lib

# Option 2: Set LD_LIBRARY_PATH manually
export LD_LIBRARY_PATH=/path/to/Qt/6.8.0/gcc_64/lib:$LD_LIBRARY_PATH
LD_PRELOAD=/path/to/libqtmcp.so ./your-app
```

#### Wayland vs X11

QtMcp should work with both, but some features (especially screenshots) may behave differently. Test with:
```bash
QT_QPA_PLATFORM=xcb ./your-app  # Force X11
QT_QPA_PLATFORM=wayland ./your-app  # Force Wayland
```

## Runtime Errors

### "QCoreApplication not created yet"

The probe tried to initialize before the app created its `QCoreApplication`. This usually resolves automatically, but if it persists:

1. Ensure the app creates `QCoreApplication` early
2. Check for static initialization issues in the app

### "Failed to start WebSocket server"

The port is likely in use. Check with:
```bash
# Linux
ss -tlnp | grep 9222

# Windows
netstat -ano | findstr 9222
```

Kill the conflicting process or use a different port:
```bash
QTMCP_PORT=9999 ./your-app
```

### Crash on Startup

Enable debug output:
```bash
# Linux
QTMCP_PORT=9222 gdb -ex run -ex bt ./your-app

# Windows (with Visual Studio)
devenv /debugexe your-app.exe
```

Common causes:
- Qt version mismatch between probe and app
- Missing dependencies
- Incompatible compiler flags (debug vs release)

## Getting Help

If you're still stuck:

1. **Check existing issues:** https://github.com/ssss2art/QtMcp/issues
2. **Open a new issue** with:
   - Operating system and version
   - Qt version (app and probe)
   - Full error messages/stack traces
   - Steps to reproduce

## Debug Information to Collect

When reporting issues, include:

```bash
# System info
uname -a  # Linux
systeminfo  # Windows

# Qt version
qmake --version
# or
/path/to/Qt/bin/qmake --version

# Probe loading attempt
QTMCP_PORT=9222 LD_PRELOAD=/path/to/probe ./app 2>&1 | head -50

# ldd output (Linux)
ldd /path/to/libqtmcp.so
ldd /path/to/your-app

# Dependencies (Windows)
dumpbin /dependents qtmcp_probe.dll
```
