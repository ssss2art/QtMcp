# Getting Started with QtMcp

This guide walks you through setting up QtMcp to enable AI assistants to control Qt applications.

## Overview

QtMcp consists of two main components:

1. **The Probe** - A shared library (`qtmcp.dll`/`libqtmcp.so`) that loads into your Qt application and exposes its object tree via WebSocket
2. **The MCP Server** - A Python CLI (`qtmcp`) that connects Claude to the probe

```
┌────────────────────────────┐     ┌──────────────────┐     ┌─────────────┐
│  Qt Application            │     │  qtmcp serve     │     │  Claude     │
│  ┌──────────────────────┐  │ WS  │  (MCP Server)    │ MCP │             │
│  │  QtMcp Probe         │◄─┼─────┤                  │◄────┤             │
│  └──────────────────────┘  │     │                  │     │             │
└────────────────────────────┘     └──────────────────┘     └─────────────┘
```

## Installation Options

### Option 1: pip install (Recommended)

The easiest way to get started is using the Python package:

```bash
pip install qtmcp
```

Then download the probe for your Qt version:

```bash
# For Qt 6.8 applications
qtmcp download-probe --qt-version 6.8

# For Qt 5.15 applications (patched for older compatibility)
qtmcp download-probe --qt-version 5.15-patched
```

See [python/README.md](../python/README.md) for complete CLI documentation.

### Option 2: Download Pre-built Binaries

Download probe binaries directly from [GitHub Releases](https://github.com/ssss2art/QtMcp/releases).

Each release includes:
- `qtmcp-probe-qt6.8-windows-x64.zip` - Windows probe for Qt 6.8
- `qtmcp-probe-qt5.15-patched-windows-x64.zip` - Windows probe for Qt 5.15
- `qtmcp-probe-qt6.8-linux-x64.tar.gz` - Linux probe for Qt 6.8
- `qtmcp-probe-qt5.15-patched-linux-x64.tar.gz` - Linux probe for Qt 5.15

### Option 3: Build from Source

See [BUILDING.md](BUILDING.md) for instructions on compiling QtMcp yourself.

## Choosing Your Qt Version

The probe must match your target application's Qt major.minor version. To check what Qt version an application uses:

**Windows:**
```powershell
# Look for Qt DLLs in the application directory
dir "C:\path\to\app" | findstr Qt
# Qt6Core.dll = Qt 6.x, Qt5Core.dll = Qt 5.x
```

**Linux:**
```bash
# Check linked libraries
ldd /path/to/app | grep -i qt
# libQt6Core.so.6 = Qt 6.x, libQt5Core.so.5 = Qt 5.x
```

Available probe versions:
| Qt Version | Probe Version | Notes |
|------------|---------------|-------|
| Qt 6.5-6.8 | `qt6.8` | Recommended for modern Qt 6 apps |
| Qt 5.15.x | `qt5.15-patched` | Patched for source compatibility |

## Running Your Application with the Probe

### Method 1: Using `qtmcp serve --target` (Recommended)

The simplest approach - let `qtmcp` handle probe injection automatically:

```bash
# Windows
qtmcp serve --mode native --target "C:\path\to\your-app.exe"

# Linux
qtmcp serve --mode native --target /path/to/your-app
```

This automatically:
1. Locates the correct probe for your platform
2. Sets up environment variables
3. Launches the application with the probe loaded
4. Starts the MCP server

### Method 2: Using `qtmcp-launch` Directly

For more control, use the launcher directly:

**Windows:**
```cmd
qtmcp-launch.exe your-app.exe --app-args arg1 arg2
```

**Linux:**
```bash
# LD_PRELOAD-based injection
LD_PRELOAD=/path/to/libqtmcp.so ./your-app arg1 arg2
```

Then start the MCP server separately:
```bash
qtmcp serve --mode native --ws-url ws://localhost:9222
```

### Environment Variables

The probe reads these environment variables at startup:

| Variable | Default | Description |
|----------|---------|-------------|
| `QTMCP_PORT` | `9222` | WebSocket server port |
| `QTMCP_MODE` | `all` | API mode: `native`, `chrome`, `computer_use`, or `all` |

Example:
```bash
# Linux
QTMCP_PORT=9999 QTMCP_MODE=native LD_PRELOAD=/path/to/libqtmcp.so ./your-app

# Windows (via qtmcp-launch)
set QTMCP_PORT=9999
set QTMCP_MODE=native
qtmcp-launch.exe your-app.exe
```

## Connecting to Claude

### Claude Desktop

Add to your `claude_desktop_config.json`:

**Windows:** `%APPDATA%\Claude\claude_desktop_config.json`
**macOS:** `~/Library/Application Support/Claude/claude_desktop_config.json`

```json
{
  "mcpServers": {
    "qtmcp": {
      "command": "qtmcp",
      "args": ["serve", "--mode", "native", "--target", "C:\\path\\to\\your-app.exe"]
    }
  }
}
```

### Claude Code

```bash
claude mcp add --transport stdio qtmcp -- qtmcp serve --mode native --ws-url ws://localhost:9222
```

### Verifying the Connection

1. Start your Qt application with the probe loaded
2. Check that the probe started: look for `[QtMCP] Probe initialized` in stderr
3. Connect to Claude and ask it to list available tools
4. Try a simple command: "Take a screenshot of the Qt application"

## Choosing an API Mode

QtMcp supports three API modes, selectable via `--mode`:

### Native Mode (`--mode native`)
Full Qt object tree introspection. Use this for:
- Test automation
- Deep inspection of widget properties
- Signal/slot monitoring
- Programmatic UI control

```bash
qtmcp serve --mode native --target /path/to/app
```

### Computer Use Mode (`--mode cu`)
Screenshot-based interaction using pixel coordinates. Use this for:
- Visual tasks
- Custom widgets without accessibility info
- Games or canvas-based UIs

```bash
qtmcp serve --mode cu --target /path/to/app
```

### Chrome Mode (`--mode chrome`)
Accessibility tree with element references. Use this for:
- Form filling
- Semantic element selection
- When you want Claude to "see" the UI like a web page

```bash
qtmcp serve --mode chrome --target /path/to/app
```

### All Modes (`--mode all`)
Exposes tools from all three modes. Useful for experimentation.

```bash
qtmcp serve --mode all --target /path/to/app
```

## Next Steps

- [API Reference](../qtmcp-specification.md) - Complete tool and protocol documentation
- [API Modes Deep Dive](../qtmcp-compatibility-modes.md) - Detailed mode comparisons
- [Building from Source](BUILDING.md) - Compile QtMcp yourself
- [Troubleshooting](TROUBLESHOOTING.md) - Common issues and solutions
