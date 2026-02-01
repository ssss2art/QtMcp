# Project Milestones: QtMCP

## v1.0 MVP (Shipped: 2026-02-01)

**Delivered:** Complete Qt application control library with C++ probe injection, three API modes (Native, Computer Use, Chrome), and Python MCP server for Claude integration.

**Phases completed:** 1-7 (33 plans total)

**Key accomplishments:**

- DLL/LD_PRELOAD injection with WebSocket transport and JSON-RPC 2.0 protocol
- Full Qt object introspection: discovery, properties, methods, signals, UI interaction
- Three API modes: Native (33 qt.* methods), Computer Use (13 cu.* methods), Chrome (8 chr.* methods)
- QML item inspection and Model/View data navigation
- Python MCP server with 53 tool definitions for Claude integration
- 12 automated test suites, 34 UAT tests, all passing

**Stats:**

- 85 source files (71 C++ + 14 Python)
- 17,292 lines of code (15,912 C++ + 1,380 Python)
- 7 phases, 33 plans, 157 commits
- 7 days from initial commit to ship (2026-01-25 to 2026-02-01)

**Git range:** `9284bf5` (Initial commit) → `6a0f178` (test(07): complete UAT)

**What's next:** v1.1 — macOS support, attach to running process, or advanced introspection features

---
