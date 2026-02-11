# QtMcp

Qt MCP server enabling AI assistants to introspect and control Qt applications.

## What is QtMcp?

QtMcp lets Claude and other AI assistants interact with any Qt desktop application. It works by injecting a lightweight probe into the target application that exposes its UI through the [Model Context Protocol (MCP)](https://modelcontextprotocol.io/).

**No source code modifications required** - works with any Qt application.

```
┌─────────────────────────────────────────────────────────────────────────┐
│  TARGET MACHINE                                                         │
│                                                                         │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  Qt Application                                                   │  │
│  │  ┌─────────────────────────────────────────────────────────────┐  │  │
│  │  │  QtMCP Probe (libqtmcp.so / qtmcp.dll)                      │  │  │
│  │  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │  │  │
│  │  │  │ Object       │  │ Introspector │  │ WebSocket Server │   │  │  │
│  │  │  │ Tracker      │  │              │  │                  │   │  │  │
│  │  │  └──────────────┘  └──────────────┘  └────────┬─────────┘   │  │  │
│  │  └───────────────────────────────────────────────│─────────────┘  │  │
│  └──────────────────────────────────────────────────│────────────────┘  │
│                                                     │ WebSocket         │
│  ┌──────────────────────────────────────────────────▼────────────────┐  │
│  │  Qt MCP Server (Python)                                           │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐   │  │
│  │  │ QtMCP Client    │  │ MCP Tools       │  │ stdio Transport │   │  │
│  │  │ (WebSocket)     │  │                 │  │                 │   │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘   │  │
│  └───────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                           ┌─────────────────┐
                           │  Claude / LLM   │
                           └─────────────────┘
```

## Quick Start

### Option 1: pip install (Recommended)

```bash
pip install qtmcp

# Download probe for your Qt version (use --compiler to override default)
qtmcp download-probe --qt-version 6.8

# Launch your app with the probe and start the MCP server
qtmcp serve --mode native --target /path/to/your-qt-app
```

See [python/README.md](python/README.md) for complete CLI documentation.

### Option 2: Build from Source

```bash
git clone https://github.com/ssss2art/QtMcp.git
cd QtMcp
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64
cmake --build build
```

See [docs/BUILDING.md](docs/BUILDING.md) for detailed build instructions.

## Features

- **Three API modes** for different use cases:
  - **Native** - Full Qt object tree introspection
  - **Computer Use** - Screenshot-based interaction (Anthropic API compatible)
  - **Chrome** - Accessibility tree with refs (Claude in Chrome compatible)
- **53 MCP tools** for Qt introspection and automation
- **Works with Qt 5.15 and Qt 6.x** applications
- **Zero modification** to target applications required
- **Cross-platform** - Windows and Linux (macOS planned)

## Connecting to Claude

### Claude Desktop

Add to `claude_desktop_config.json`:

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

### Claude Code

```bash
claude mcp add --transport stdio qtmcp -- qtmcp serve --mode native --ws-url ws://localhost:9222
```

## Documentation

| Document | Description |
|----------|-------------|
| [Getting Started](docs/GETTING-STARTED.md) | Installation and first steps |
| [Building from Source](docs/BUILDING.md) | Compile QtMcp yourself |
| [API Reference](qtmcp-specification.md) | Complete tool and protocol documentation |
| [API Modes](qtmcp-compatibility-modes.md) | Detailed mode comparisons |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | Common issues and solutions |
| [Python CLI](python/README.md) | qtmcp command documentation |

## Platform Support

| Platform | Qt 5.15 | Qt 6.5+ | Status |
|----------|---------|---------|--------|
| **Windows x64** | ✅ | ✅ | Supported |
| **Linux x64** | ✅ | ✅ | Supported |
| **macOS** | - | - | Planned (v1.1) |

## Requirements

- **Runtime:** Python 3.11+ (for MCP server)
- **Target apps:** Qt 5.15+ or Qt 6.5+
- **Build:** CMake 3.16+, C++17 compiler

## License

MIT License - see [LICENSE](LICENSE) for details.

## Links

- [GitHub Repository](https://github.com/ssss2art/QtMcp)
- [Releases & Probe Downloads](https://github.com/ssss2art/QtMcp/releases)
- [Issue Tracker](https://github.com/ssss2art/QtMcp/issues)
