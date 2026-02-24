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

The MCP server starts instantly without launching any application. Claude
then uses the `qtmcp_launch_app` or `qtmcp_inject_probe` tools to start
or attach to a Qt application on demand.

## Installation Options

### Option 1: pip install (Recommended)

The easiest way to get started is using the Python package:

```bash
pip install qtmcp
```

Then download the probe for your Qt version:

```bash
# Download probe matching your app's Qt version
qtmcp download-tools --qt-version 6.8

# Other available versions: 5.15, 6.5, 6.8, 6.9
qtmcp download-tools --qt-version 5.15
```

See [python/README.md](../python/README.md) for complete CLI documentation.

### Option 2: Download Pre-built Binaries

Download probe binaries directly from [GitHub Releases](https://github.com/ssss2art/QtMcp/releases).

Each release includes probes for each supported Qt version, for both platforms:
- `qtmcp-probe-qt5.15-linux-gcc13.so` / `qtmcp-probe-qt5.15-windows-msvc17.dll`
- `qtmcp-probe-qt6.5-linux-gcc13.so` / `qtmcp-probe-qt6.5-windows-msvc17.dll`
- `qtmcp-probe-qt6.8-linux-gcc13.so` / `qtmcp-probe-qt6.8-windows-msvc17.dll`
- `qtmcp-probe-qt6.9-linux-gcc13.so` / `qtmcp-probe-qt6.9-windows-msvc17.dll`
- `qtmcp-launcher-linux` / `qtmcp-launcher-windows.exe`

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
| Qt Version | Probe Name | Default Compiler | Notes |
|------------|------------|-----------------|-------|
| Qt 5.15.x | `qt5.15` | gcc13 / msvc17 | For Qt 5 applications |
| Qt 6.5.x | `qt6.5` | gcc13 / msvc17 | For Qt 6.5 applications |
| Qt 6.8.x | `qt6.8` | gcc13 / msvc17 | For Qt 6.8 applications |
| Qt 6.9.x | `qt6.9` | gcc13 / msvc17 | For Qt 6.9 applications |

The probe must match your application's Qt major.minor version.

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
      "args": ["serve", "--mode", "native"]
    }
  }
}
```

If Qt isn't deployed alongside the target app, add `--qt-path` so the
launch tool can find the Qt libraries:
```json
{
  "mcpServers": {
    "qtmcp": {
      "command": "qtmcp",
      "args": [
        "serve", "--mode", "native",
        "--qt-path", "C:\\Qt\\6.8.0\\msvc2022_64\\bin"
      ]
    }
  }
}
```

### Claude Code

```bash
claude mcp add --transport stdio qtmcp -- qtmcp serve --mode native
```

With a Qt path:
```bash
claude mcp add --transport stdio qtmcp -- qtmcp serve --mode native --qt-path /opt/Qt/6.8.0/gcc_64/lib
```

### How It Works

Once the MCP server is registered, Claude has access to these lifecycle tools:

1. **`qtmcp_launch_app`** — Launch a new Qt application with the probe injected.
   Claude calls this when you ask it to work with a Qt app:
   ```
   "Launch /path/to/myapp and inspect its UI"
   ```

2. **`qtmcp_inject_probe`** — Inject the probe into an already-running Qt application
   (Linux only, requires gdb). Useful when the app is already open:
   ```
   "Connect to the Qt app running as PID 12345"
   ```

3. **`qtmcp_connect_probe`** — Connect to a probe that's already running
   (e.g., the app was started with LD_PRELOAD or has the probe linked in):
   ```
   "Connect to the probe at ws://localhost:9222"
   ```

4. **`qtmcp_list_probes`** — Discover probes on the network via UDP broadcast.

### Verifying the Connection

1. Ask Claude to launch your Qt application (or connect to a running one)
2. Check that Claude reports a successful connection
3. Try a simple command: "Take a screenshot of the Qt application"

## Running the Probe Manually

If you prefer to start the application yourself (without using the
`qtmcp_launch_app` tool), you can load the probe manually and then have
Claude connect to it.

### Using `qtmcp-launch`

```bash
# Windows
qtmcp-launch.exe your-app.exe --detach

# Linux
qtmcp-launch your-app --detach
```

Then ask Claude to connect: *"Connect to the probe at ws://localhost:9222"*

### Using LD_PRELOAD (Linux)

```bash
LD_PRELOAD=/path/to/libqtmcp-probe.so ./your-app
```

### CMake Integration (Link into Your Project)

If you build QtMcp from source, you can link the probe directly into your CMake project. This is useful during development when you always want the probe available.

**1. Build and install QtMcp:**

```bash
cd QtMcp
cmake --preset release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64
cmake --build --preset release
cmake --install build/release --prefix /opt/qtmcp
```

On Windows:
```powershell
cmake --preset windows-release -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build --preset windows-release
cmake --install build/windows-release --prefix C:\qtmcp
```

**2. In your project's CMakeLists.txt:**

```cmake
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
find_package(QtMCP REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE Qt6::Core Qt6::Widgets)
qtmcp_inject_probe(myapp)
```

**3. Configure your project with both Qt and QtMcp in the prefix path:**

```bash
cmake -B build -DCMAKE_PREFIX_PATH="/path/to/Qt/6.8.0/gcc_64;/opt/qtmcp"
cmake --build build
```

On Windows:
```powershell
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64;C:\qtmcp"
cmake --build build
```

`qtmcp_inject_probe()` handles the platform details automatically:
- **Windows:** Copies the probe DLL next to your executable after each build
- **Linux:** Generates a helper script (`qtmcp-preload-myapp.sh`) that launches your app with `LD_PRELOAD` set

The `QtMCPConfig.cmake` package auto-detects your project's Qt version (Qt5 or Qt6) and resolves to the matching probe binary, so the same QtMcp install can work with either.

## Environment Variables

The probe reads these environment variables at startup:

| Variable | Default | Description |
|----------|---------|-------------|
| `QTMCP_PORT` | `9222` | WebSocket server port |
| `QTMCP_MODE` | `all` | API mode: `native`, `chrome`, `computer_use`, or `all` |

The MCP server reads these additional environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `QTMCP_QT_PATH` | *(none)* | Path to Qt lib/bin directory, added to library search path when launching targets |
| `QTMCP_CONNECT_TIMEOUT` | `30` | Max seconds to wait for probe connection |
| `QTMCP_LAUNCHER` | *(auto)* | Path to qtmcp-launch executable |

### Specifying the Qt Path

If the target application doesn't have Qt deployed alongside it (i.e., Qt DLLs/shared
libraries aren't in the same directory), you need to tell QtMCP where to find them:

```bash
# Via CLI argument (applies to all tool invocations)
qtmcp serve --mode native --qt-path /opt/Qt/6.8.0/gcc_64/lib

# Via environment variable
export QTMCP_QT_PATH=/opt/Qt/6.8.0/gcc_64/lib
qtmcp serve --mode native
```

This prepends the path to `LD_LIBRARY_PATH` (Linux) or `PATH` (Windows) when
the `qtmcp_launch_app` tool starts a target application.

## Choosing an API Mode

QtMcp supports three API modes, selectable via `--mode`:

### Native Mode (`--mode native`)
Full Qt object tree introspection. Use this for:
- Test automation
- Deep inspection of widget properties
- Signal/slot monitoring
- Programmatic UI control

```bash
qtmcp serve --mode native
```

### Computer Use Mode (`--mode cu`)
Screenshot-based interaction using pixel coordinates. Use this for:
- Visual tasks
- Custom widgets without accessibility info
- Games or canvas-based UIs

```bash
qtmcp serve --mode cu
```

### Chrome Mode (`--mode chrome`)
Accessibility tree with element references. Use this for:
- Form filling
- Semantic element selection
- When you want Claude to "see" the UI like a web page

```bash
qtmcp serve --mode chrome
```

## Next Steps

- [API Reference](../qtmcp-specification.md) - Complete tool and protocol documentation
- [API Modes Deep Dive](../qtmcp-compatibility-modes.md) - Detailed mode comparisons
- [Building from Source](BUILDING.md) - Compile QtMcp yourself
- [Troubleshooting](TROUBLESHOOTING.md) - Common issues and solutions
