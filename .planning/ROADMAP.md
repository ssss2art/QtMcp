# Roadmap: QtMCP

## Overview

QtMCP is built foundation-first: establish injection and transport (highest technical risk on Windows), then build introspection layers (Object Registry, Signals, UI), then expose three API modes (Native, Computer Use, Chrome), then add extended introspection (QML, Model/View), and finally wrap with Python MCP server and client library. This order follows technical dependencies and front-loads risk.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Foundation** - DLL/LD_PRELOAD injection and WebSocket transport layer ✓
- [x] **Phase 2: Core Introspection** - Object registry, signal monitoring, and UI interaction primitives ✓
- [ ] **Phase 3: Native Mode** - Full Qt introspection API exposed via JSON-RPC
- [ ] **Phase 4: Computer Use Mode** - Screenshot + coordinate-based automation API
- [ ] **Phase 5: Chrome Mode** - Accessibility tree with numbered refs API (RESEARCH FLAG)
- [ ] **Phase 6: Extended Introspection** - QML items and Model/View data access
- [ ] **Phase 7: Python Integration** - MCP server and client library

## Phase Details

### Phase 1: Foundation
**Goal**: Probe can be injected into any Qt application and accepts WebSocket connections
**Depends on**: Nothing (first phase)
**Requirements**: INJ-01, INJ-02, INJ-03, INJ-04, INJ-05
**Success Criteria** (what must be TRUE):
  1. User can launch any Qt application with probe injected via LD_PRELOAD on Linux
  2. User can launch any Qt application with probe injected via qtmcp-launch.exe on Windows
  3. User can connect to probe's WebSocket server and receive JSON-RPC responses
  4. User can configure port via CLI flags (--port)
  5. Probe handles Windows DLL pitfalls correctly (CRT matching, no TLS, minimal DllMain)
**Plans**: 6 plans in 5 waves

Plans:
- [x] 01-01-PLAN.md - CMake build system and project structure (Wave 1) ✓
- [x] 01-02-PLAN.md - Probe singleton with safe platform initialization (Wave 2) ✓
- [x] 01-03-PLAN.md - JSON-RPC 2.0 handler with unit tests (Wave 2) ✓
- [x] 01-04-PLAN.md - WebSocket server with single-client logic (Wave 3) ✓
- [x] 01-05-PLAN.md - Launcher CLI with platform injection (Wave 4) ✓
- [x] 01-06-PLAN.md - Test app and end-to-end verification (Wave 5) ✓

### Phase 2: Core Introspection
**Goal**: Probe can discover, inspect, and interact with any QObject in the target application
**Depends on**: Phase 1
**Requirements**: OBJ-01, OBJ-02, OBJ-03, OBJ-04, OBJ-05, OBJ-06, OBJ-07, OBJ-08, OBJ-09, OBJ-10, OBJ-11, SIG-01, SIG-02, SIG-03, SIG-04, SIG-05, UI-01, UI-02, UI-03, UI-04, UI-05
**Success Criteria** (what must be TRUE):
  1. User can find objects by objectName or className via JSON-RPC
  2. User can get full object tree with hierarchical IDs (e.g., QMainWindow/centralWidget/QPushButton#submit)
  3. User can read/write properties, list methods, and invoke slots on any object
  4. User can subscribe to signals and receive push notifications when they emit
  5. User can simulate clicks, send keystrokes, take screenshots, and perform hit testing
**Plans**: 7 plans in 4 waves

Plans:
- [x] 02-01-PLAN.md - Object Registry with qtHookData hooks (Wave 1) ✓
- [x] 02-02-PLAN.md - Hierarchical object IDs and tree serialization (Wave 2) ✓
- [x] 02-03-PLAN.md - Meta Inspector for property/method/signal listing (Wave 2) ✓
- [x] 02-04-PLAN.md - Property get/set and method invocation (Wave 3) ✓
- [x] 02-05-PLAN.md - Signal monitoring with push notifications (Wave 3) ✓
- [x] 02-06-PLAN.md - UI interaction (click, keyboard, screenshot, hit test) (Wave 3) ✓
- [x] 02-07-PLAN.md - JSON-RPC integration for all introspection methods (Wave 4) ✓

### Phase 3: Native Mode
**Goal**: Complete Native Mode API surface ready for AI agents to use full Qt introspection
**Depends on**: Phase 2
**Requirements**: NAT-01, NAT-02, NAT-03, NAT-04, NAT-05
**Success Criteria** (what must be TRUE):
  1. All discovery methods (findByObjectName, findByClassName, getObjectTree) available in Native Mode
  2. All inspection methods (getObjectInfo, listProperties, listMethods, listSignals) available in Native Mode
  3. All mutation methods (setProperty, invokeMethod) available in Native Mode
  4. All interaction methods (click, sendKeys, screenshot, getGeometry) available in Native Mode
  5. All signal monitoring methods (subscribeSignals, unsubscribeSignals) available in Native Mode
**Plans**: TBD

Plans:
- [ ] 03-01: TBD (planning not yet started)

### Phase 4: Computer Use Mode
**Goal**: AI agents can control Qt applications using screenshot and pixel coordinates
**Depends on**: Phase 2
**Requirements**: CU-01, CU-02, CU-03, CU-04, CU-05, CU-06, CU-07, CU-08, CU-09, CU-10
**Success Criteria** (what must be TRUE):
  1. User can request screenshot and receive base64 PNG image
  2. User can perform all mouse actions (click, right-click, double-click, drag, move) at pixel coordinates
  3. User can type text and send key combinations at current focus
  4. User can scroll in any direction at specified coordinates
  5. User can query current cursor position
**Plans**: TBD

Plans:
- [ ] 04-01: TBD (planning not yet started)

### Phase 5: Chrome Mode
**Goal**: AI agents can control Qt applications using accessibility tree and numbered refs (RESEARCH FLAG)
**Depends on**: Phase 2
**Requirements**: CHR-01, CHR-02, CHR-03, CHR-04, CHR-05, CHR-06, CHR-07, CHR-08
**Success Criteria** (what must be TRUE):
  1. User can request accessibility tree and receive numbered refs for interactive elements
  2. User can click elements and input to form fields using ref numbers
  3. User can get all visible text content from the application
  4. User can find elements using natural language queries
  5. User can navigate tabs/menus and read console messages (qDebug, etc.)
**Plans**: TBD

**Research Flag**: QAccessible tree mapping to Chrome accessibility format is novel. No existing examples. Requires research during planning to determine: role/state/property mappings, ref stability, which Qt accessibility features map to Chrome format.

Plans:
- [ ] 05-01: TBD (planning not yet started)

### Phase 6: Extended Introspection
**Goal**: Probe can inspect QML items and navigate QAbstractItemModel hierarchies
**Depends on**: Phase 2
**Requirements**: QML-01, QML-02, QML-03, QML-04, QML-05, MV-01, MV-02, MV-03, MV-04
**Success Criteria** (what must be TRUE):
  1. QML items appear in object tree as QQuickItem subclasses with QML id in hierarchical paths
  2. QML properties, context properties, and binding information accessible via standard introspection
  3. User can list all QAbstractItemModel instances in the application
  4. User can navigate model hierarchy and get data at any index with specified roles
**Plans**: TBD

**Research Flag**: QML introspection may require private Qt APIs for deep features (context/bindings). Need to validate which features are achievable with public APIs only.

Plans:
- [ ] 06-01: TBD (planning not yet started)

### Phase 7: Python Integration
**Goal**: Claude can control Qt applications through MCP server with all three API modes
**Depends on**: Phase 3, Phase 4, Phase 5
**Requirements**: PY-01, PY-02, PY-03, PY-04, PY-05, CLI-01, CLI-02, CLI-03, CLI-04
**Success Criteria** (what must be TRUE):
  1. Python MCP server connects to probe via WebSocket and exposes all tools to Claude
  2. MCP tool definitions available for Native, Computer Use, and Chrome modes
  3. User can switch modes via configuration or command
  4. Python client library provides async API for all probe methods
  5. Example automation scripts demonstrate common operations
**Plans**: TBD

Plans:
- [ ] 07-01: TBD (planning not yet started)

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7

Note: Phases 3, 4, 5 can potentially execute in parallel after Phase 2 completes (they all depend on Phase 2 but not each other).

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Foundation | 6/6 | Complete | 2026-01-30 |
| 2. Core Introspection | 7/7 | Complete | 2026-01-30 |
| 3. Native Mode | 0/TBD | Not started | - |
| 4. Computer Use Mode | 0/TBD | Not started | - |
| 5. Chrome Mode | 0/TBD | Not started | - |
| 6. Extended Introspection | 0/TBD | Not started | - |
| 7. Python Integration | 0/TBD | Not started | - |

---
*Roadmap created: 2025-01-29*
*Depth: standard (7 phases)*
*Coverage: 59/59 v1 requirements mapped*
