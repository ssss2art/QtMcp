# QtMCP Python Server

MCP server that bridges Claude to Qt applications via the QtMCP probe.
Supports three modes: **Native** (full Qt introspection), **Computer Use**
(screenshot + coordinates), and **Chrome** (accessibility tree + refs).

## Installation

From the `python/` directory:

```bash
pip install .
```

Or with [uv](https://docs.astral.sh/uv/):

```bash
uv pip install .
```

## Quick Start

Connect to an already-running QtMCP probe:

```bash
qtmcp --mode native --ws-url ws://localhost:9222
```

The server communicates with Claude over stdio (JSON-RPC) and forwards
tool calls to the Qt probe over WebSocket.

## Claude Desktop Configuration

Add one of the following blocks to your Claude Desktop `claude_desktop_config.json`.

### Native Mode (connect to running probe)

```json
{
  "mcpServers": {
    "qtmcp-native": {
      "command": "uv",
      "args": ["run", "--with", "fastmcp<3", "--with", "websockets",
               "python", "-m", "qtmcp", "--mode", "native",
               "--ws-url", "ws://localhost:9222"]
    }
  }
}
```

### Native Mode (auto-launch target app)

```json
{
  "mcpServers": {
    "qtmcp-native": {
      "command": "uv",
      "args": ["run", "--with", "fastmcp<3", "--with", "websockets",
               "python", "-m", "qtmcp", "--mode", "native",
               "--target", "/path/to/your/qt-app"]
    }
  }
}
```

### Computer Use Mode (connect to running probe)

```json
{
  "mcpServers": {
    "qtmcp-cu": {
      "command": "uv",
      "args": ["run", "--with", "fastmcp<3", "--with", "websockets",
               "python", "-m", "qtmcp", "--mode", "cu",
               "--ws-url", "ws://localhost:9222"]
    }
  }
}
```

### Computer Use Mode (auto-launch target app)

```json
{
  "mcpServers": {
    "qtmcp-cu": {
      "command": "uv",
      "args": ["run", "--with", "fastmcp<3", "--with", "websockets",
               "python", "-m", "qtmcp", "--mode", "cu",
               "--target", "/path/to/your/qt-app"]
    }
  }
}
```

### Chrome Mode (connect to running probe)

```json
{
  "mcpServers": {
    "qtmcp-chrome": {
      "command": "uv",
      "args": ["run", "--with", "fastmcp<3", "--with", "websockets",
               "python", "-m", "qtmcp", "--mode", "chrome",
               "--ws-url", "ws://localhost:9222"]
    }
  }
}
```

### Chrome Mode (auto-launch target app)

```json
{
  "mcpServers": {
    "qtmcp-chrome": {
      "command": "uv",
      "args": ["run", "--with", "fastmcp<3", "--with", "websockets",
               "python", "-m", "qtmcp", "--mode", "chrome",
               "--target", "/path/to/your/qt-app"]
    }
  }
}
```

### Windows (Claude Desktop)

On Windows, Claude Desktop requires the `cmd /c` wrapper:

```json
{
  "mcpServers": {
    "qtmcp-native": {
      "command": "cmd",
      "args": ["/c", "uv", "run", "--with", "fastmcp<3", "--with", "websockets",
               "python", "-m", "qtmcp", "--mode", "native",
               "--ws-url", "ws://localhost:9222"]
    }
  }
}
```

Replace `--mode native` with `--mode cu` or `--mode chrome` as needed.
The `--ws-url` can be replaced with `--target C:\\path\\to\\app.exe` for
auto-launch.

## Claude Code Configuration

Add the server with the `claude mcp add` command:

### Native Mode

```bash
claude mcp add --transport stdio qtmcp-native -- uv run --with "fastmcp<3" --with websockets python -m qtmcp --mode native --ws-url ws://localhost:9222
```

### Computer Use Mode

```bash
claude mcp add --transport stdio qtmcp-cu -- uv run --with "fastmcp<3" --with websockets python -m qtmcp --mode cu --ws-url ws://localhost:9222
```

### Chrome Mode

```bash
claude mcp add --transport stdio qtmcp-chrome -- uv run --with "fastmcp<3" --with websockets python -m qtmcp --mode chrome --ws-url ws://localhost:9222
```

For auto-launch, replace `--ws-url ws://localhost:9222` with
`--target /path/to/your/qt-app`.

## CLI Reference

| Argument | Environment Variable | Default | Description |
|---|---|---|---|
| `--mode` | -- | (required) | API mode: `native`, `cu`, or `chrome` |
| `--ws-url` | `QTMCP_WS_URL` | `ws://localhost:9222` | WebSocket URL of the running probe |
| `--target` | -- | -- | Path to Qt app exe to auto-launch |
| `--port` | `QTMCP_PORT` | `9222` | Port for auto-launched probe |
| `--launcher-path` | `QTMCP_LAUNCHER` | `qtmcp-launch` | Path to the qtmcp-launch executable |

When `--target` is provided, the server launches the Qt application with
the probe injected, waits for it to start, then connects. The `--ws-url`
is ignored in this case and constructed from `--port`.

## Modes

### Native (~32 tools, `qt_*`)

Full Qt object introspection. Find objects by name or class, read and
write properties, invoke methods, subscribe to signals, query models,
inspect QML bindings, and take screenshots. Best for structured
automation where you know the widget tree.

### Computer Use (13 tools, `cu_*`)

Screenshot-based automation. Take a screenshot, click at pixel
coordinates, type text, press keys, scroll, and drag. Works like a
virtual mouse and keyboard. Best when you need to interact with the UI
visually.

### Chrome (8 tools, `chr_*`)

Accessibility-tree automation. Read the page structure with numbered
refs, click refs, fill form inputs, search for elements by text, and
read console messages. Modeled after Claude's Chrome extension API. Best
for form-filling and navigation workflows.

## Architecture

```
Claude <-> stdio <-> QtMCP MCP Server <-> WebSocket <-> Qt Probe (injected in app)
```

The QtMCP MCP Server is a Python process that:

1. Receives MCP tool calls from Claude over stdio (JSON-RPC 2.0)
2. Translates them into Qt probe commands
3. Sends them over WebSocket to the probe DLL injected in the target app
4. Returns the probe's response back to Claude

The probe is a C++ shared library that hooks into the Qt event loop and
exposes the application's object tree, properties, methods, signals, and
UI over a WebSocket JSON-RPC API.
