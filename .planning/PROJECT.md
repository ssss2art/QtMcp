# QtMCP

## What This Is

QtMCP is a lightweight, MIT-licensed injection library that enables AI assistants (Claude), test automation frameworks, and debugging tools to inspect and control Qt applications at runtime. It consists of a C++ probe (15,912 LOC) injected into Qt apps via LD_PRELOAD/DLL injection, a WebSocket transport layer with JSON-RPC 2.0, three API modes (Native, Computer Use, Chrome), and a Python MCP server (1,380 LOC) for Claude integration.

## Core Value

Claude can control any Qt application with zero learning curve — the probe exposes familiar APIs (Computer Use, Chrome-style) that Claude already knows.

## Requirements

### Validated

- C++ probe loads via LD_PRELOAD (Linux) and DLL injection (Windows) — v1.0
- WebSocket server accepts connections on configurable port — v1.0
- Object discovery: find by objectName, className, get object tree — v1.0
- Object inspection: properties, methods, signals, geometry — v1.0
- UI interaction: click, sendKeys, screenshot — v1.0
- Signal monitoring: subscribe/unsubscribe, push events — v1.0
- Native API: hierarchical object IDs, full Qt introspection (33 qt.* methods) — v1.0
- Computer Use API: screenshot, coordinates, mouse/keyboard actions (13 cu.* methods) — v1.0
- Chrome API: accessibility tree, refs, form_input, semantic click (8 chr.* methods) — v1.0
- Windows launcher (qtmcp-launch.exe) — v1.0
- Python MCP server with 53 tool definitions — v1.0
- Python client library for automation scripts — v1.0
- QML item introspection and Model/View navigation — v1.0

### Active

#### v1.1 Distribution & Compatibility
- [ ] Multi-Qt version build system (5.15, 5.15.1-patched, 6.2, 6.8, 6.9)
- [ ] GitHub Actions CI/CD matrix build (5 Qt versions × 2 platforms)
- [ ] GitHub Releases with prebuilt probe binaries (10 artifacts)
- [ ] vcpkg port — source build (user builds against their Qt)
- [ ] vcpkg port — binary download (prebuilt from GitHub Releases)
- [ ] Python MCP server published to PyPI (`pip install qtmcp`)

### Out of Scope

- macOS support — deferred, different injection approach required
- Attach to running process — launch-only injection for MVP
- Authentication — rely on localhost binding for security
- Recording/playback — future feature
- Qt 4 support — never
- Mobile (Android/iOS) — future
- Built-in GUI — never (headless tool)
- Plugin system — never

## Context

Shipped v1.0 with 17,292 LOC (15,912 C++ + 1,380 Python).
Tech stack: C++17, Qt 5.15 LTS, Qt WebSockets, JSON-RPC 2.0, Python 3.8+, FastMCP 2.14.
85 source files, 12 test suites, 34 UAT tests all passing.
53 MCP tools across 3 modes for Claude integration.

## Constraints

- **Platform**: Windows and Linux only — macOS requires different injection approach
- **Qt Version**: Qt 5.15, 5.15.1-patched, 6.2, 6.8, 6.9 — probe must build against each
- **Injection**: Launch-only — no attach to running process
- **Security**: Localhost binding default — no authentication
- **Dependencies**: Qt Core, Qt Network, Qt WebSockets modules required

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| LD_PRELOAD (Linux) / DLL injection (Windows) | Standard approach, proven by GammaRay | Good |
| WebSocket transport | Allows multiple clients, browser-compatible | Good |
| JSON-RPC 2.0 | Standard protocol, good tooling support | Good |
| Three API modes | Claude already knows Computer Use and Chrome APIs | Good |
| Python MCP server (not C++) | Faster iteration, MCP SDK available in Python | Good |
| Hierarchical object IDs | Human-readable, reflects actual Qt object tree | Good |
| InitOnce API instead of std::call_once | Avoids TLS issues in injected DLLs on Windows | Good |
| Q_COREAPP_STARTUP_FUNCTION | Auto-init when Qt starts, no manual call needed | Good |
| Ephemeral refs for Chrome Mode | Fresh refs per readPage, avoids stale refs | Good |
| Qt Quick as optional dependency | QTMCP_HAS_QML compile guard, graceful degradation | Good |
| Minimal DllMain pattern | Only DisableThreadLibraryCalls + flag; defer all Qt work | Good |

---
*Last updated: 2026-02-01 after v1.1 milestone start*
