# qtmcp

MCP server for controlling Qt applications via the QtMCP probe.

QtMCP enables Claude and other MCP-compatible AI assistants to interact with Qt desktop applications through a native probe that exposes the Qt object tree, properties, signals, and visual state.

## Installation

```bash
pip install qtmcp
```

## Quick Start

1. **Download the probe** for your Qt version:

```bash
# Download probe matching your app's Qt version
qtmcp download-probe --qt-version 6.8

# Other available versions: 5.15, 6.5, 6.8, 6.9
qtmcp download-probe --qt-version 5.15

# Override the default compiler (default: gcc13 on Linux, msvc17 on Windows)
qtmcp download-probe --qt-version 6.8 --compiler gcc14
```

2. **Launch your Qt application** with the probe:

```bash
# Auto-launch target app with probe injection
qtmcp serve --mode native --target /path/to/your-qt-app.exe
```

3. **Connect Claude** to the MCP server via your client configuration.

## Features

- **Three API modes**: Native (full Qt access), Computer Use (screenshots + clicks), Chrome (DevTools-compatible)
- **53 MCP tools** for Qt introspection and automation
- **Works with Qt 5.15 and Qt 6.x** applications
- **Zero modification** to target applications required

## Server Modes

```bash
# Native mode - full Qt object tree access
qtmcp serve --mode native --ws-url ws://localhost:9222

# Chrome mode - DevTools-compatible protocol
qtmcp serve --mode chrome --target /path/to/app.exe

# Computer Use mode - screenshot-based interaction
qtmcp serve --mode cu --ws-url ws://localhost:9222
```

## Claude Desktop Configuration

Add to your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "qtmcp": {
      "command": "qtmcp",
      "args": ["serve", "--mode", "native", "--target", "/path/to/your/qt-app"]
    }
  }
}
```

## Claude Code Configuration

```bash
claude mcp add --transport stdio qtmcp -- qtmcp serve --mode native --ws-url ws://localhost:9222
```

## Requirements

- Python 3.11 or later
- Qt application with QtMCP probe loaded
- Windows or Linux (macOS support planned)

## Links

- [Full Documentation](https://github.com/ssss2art/QtMcp#readme)
- [Releases & Probe Downloads](https://github.com/ssss2art/QtMcp/releases)
- [Issue Tracker](https://github.com/ssss2art/QtMcp/issues)

## License

MIT License - see [LICENSE](https://github.com/ssss2art/QtMcp/blob/main/LICENSE) for details.
