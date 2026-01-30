# QtMCP

## What This Is

QtMCP is a lightweight, MIT-licensed injection library that enables AI assistants (Claude), test automation frameworks, and debugging tools to inspect and control Qt applications at runtime. It consists of a C++ probe injected into Qt apps, a WebSocket transport layer, and a Python MCP server for Claude integration.

## Core Value

Claude can control any Qt application with zero learning curve — the probe exposes familiar APIs (Computer Use, Chrome-style) that Claude already knows.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] C++ probe loads via LD_PRELOAD (Linux) and DLL injection (Windows)
- [ ] WebSocket server accepts connections on configurable port
- [ ] Object discovery: find by objectName, className, get object tree
- [ ] Object inspection: properties, methods, signals, geometry
- [ ] UI interaction: click, sendKeys, screenshot
- [ ] Signal monitoring: subscribe/unsubscribe, push events
- [ ] Native API: hierarchical object IDs, full Qt introspection
- [ ] Computer Use API: screenshot, coordinates, mouse/keyboard actions
- [ ] Chrome API: accessibility tree, refs, form_input, semantic click
- [ ] Windows launcher (qtmcp-launch.exe)
- [ ] Python MCP server with tool definitions
- [ ] Python client library for automation scripts
- [ ] Basic QML item introspection

### Out of Scope

- macOS support — deferred to v1.1
- Attach to running process — launch-only injection for MVP
- Authentication — rely on localhost binding for security
- Recording/playback — future feature
- Qt 4 support — never
- Mobile (Android/iOS) — future
- Built-in GUI — never (headless tool)
- Plugin system — never

## Context

**Technical environment:**
- C++ probe with Qt 5.15 LTS (primary), Qt 6.x (secondary)
- WebSocket transport using Qt WebSockets module
- JSON-RPC 2.0 message format
- Python 3.8+ for MCP server and client library

**Key technical hooks:**
- Qt's internal `qtHookData` array for object lifecycle tracking (AddQObject, RemoveQObject)
- `qt_register_signal_spy_callbacks` for signal monitoring
- Same undocumented-but-stable APIs used by GammaRay

**Object identification:**
- Hierarchical paths: `QMainWindow/centralWidget/QPushButton#submitButton`
- Rules: `/` separates segments, `#objectName` suffix if set, `[index]` for disambiguation

**Three API modes:**
1. **Native** — Full Qt introspection with hierarchical IDs
2. **Computer Use** — Screenshot + coordinates (matches `computer_20250124`)
3. **Chrome** — Accessibility tree + integer refs (matches Claude in Chrome)

## Constraints

- **Platform**: Windows and Linux only for MVP — macOS requires different injection approach
- **Qt Version**: Qt 5.15 LTS primary target — must build against user's Qt installation
- **Injection**: Launch-only — no attach to running process (simplifies implementation)
- **Security**: Localhost binding default — no authentication in MVP
- **Dependencies**: Qt Core, Qt Network, Qt WebSockets modules required

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| LD_PRELOAD (Linux) / DLL injection (Windows) | Standard approach, proven by GammaRay | — Pending |
| WebSocket transport | Allows multiple clients, browser-compatible | — Pending |
| JSON-RPC 2.0 | Standard protocol, good tooling support | — Pending |
| Three API modes | Claude already knows Computer Use and Chrome APIs | — Pending |
| Python MCP server (not C++) | Faster iteration, MCP SDK available in Python | — Pending |
| Hierarchical object IDs | Human-readable, reflects actual Qt object tree | — Pending |

---
*Last updated: 2025-01-29 after initialization*
