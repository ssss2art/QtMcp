# Requirements: QtMCP

**Defined:** 2025-01-29
**Core Value:** Claude can control any Qt application with zero learning curve

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Injection & Transport

- [x] **INJ-01**: Probe loads via LD_PRELOAD on Linux ✓
- [x] **INJ-02**: Probe loads via DLL injection on Windows with launcher (qtmcp-launch.exe) ✓
- [x] **INJ-03**: WebSocket server starts on configurable port (default 9222) ✓
- [x] **INJ-04**: JSON-RPC 2.0 message handling over WebSocket ✓
- [x] **INJ-05**: Environment variable configuration (QTMCP_PORT, QTMCP_BIND, QTMCP_MODE, etc.) ✓

### Object Introspection

- [x] **OBJ-01**: Find objects by objectName ✓
- [x] **OBJ-02**: Find objects by className ✓
- [x] **OBJ-03**: Get full or partial object tree hierarchy ✓
- [x] **OBJ-04**: Get detailed object info (class, inheritance, geometry, visibility) ✓
- [x] **OBJ-05**: List all properties of an object with types and values ✓
- [x] **OBJ-06**: Get specific property value ✓
- [x] **OBJ-07**: Set property value ✓
- [x] **OBJ-08**: List all invokable methods (slots, Q_INVOKABLE) ✓
- [x] **OBJ-09**: Invoke methods on objects ✓
- [x] **OBJ-10**: List all signals of an object ✓
- [x] **OBJ-11**: Hierarchical object IDs (e.g., QMainWindow/centralWidget/QPushButton#submit) ✓

### Signal Monitoring

- [x] **SIG-01**: Subscribe to signals from specific objects ✓
- [x] **SIG-02**: Unsubscribe from signals ✓
- [x] **SIG-03**: Push signal emission events to connected clients ✓
- [x] **SIG-04**: Push object created events ✓
- [x] **SIG-05**: Push object destroyed events ✓

### UI Interaction

- [x] **UI-01**: Simulate mouse click on widget (left, right, middle button) ✓
- [x] **UI-02**: Simulate keyboard input (text, key combinations) ✓
- [x] **UI-03**: Take screenshot of widget or entire window ✓
- [x] **UI-04**: Get widget geometry (local and global coordinates) ✓
- [x] **UI-05**: Coordinate-to-element hit testing ✓

### Native Mode API

- [ ] **NAT-01**: All object discovery methods exposed (findByObjectName, findByClassName, getObjectTree)
- [ ] **NAT-02**: All inspection methods exposed (getObjectInfo, listProperties, listMethods, listSignals)
- [ ] **NAT-03**: All mutation methods exposed (setProperty, invokeMethod)
- [ ] **NAT-04**: All interaction methods exposed (click, sendKeys, screenshot, getGeometry)
- [ ] **NAT-05**: All signal monitoring methods exposed (subscribeSignals, unsubscribeSignals)

### Computer Use Mode API

- [ ] **CU-01**: Screenshot action returns base64 PNG image
- [ ] **CU-02**: Left click at pixel coordinates
- [ ] **CU-03**: Right click at pixel coordinates
- [ ] **CU-04**: Double click at pixel coordinates
- [ ] **CU-05**: Mouse move to coordinates
- [ ] **CU-06**: Click and drag from start to end coordinates
- [ ] **CU-07**: Type text at current focus
- [ ] **CU-08**: Send key combinations (e.g., ctrl+s)
- [ ] **CU-09**: Scroll at coordinates (up, down, left, right)
- [ ] **CU-10**: Get cursor position

### Chrome Mode API

- [ ] **CHR-01**: Read page returns accessibility tree with numbered refs
- [ ] **CHR-02**: Click element by ref number
- [ ] **CHR-03**: Form input by ref number (for QLineEdit, QTextEdit, QSpinBox, etc.)
- [ ] **CHR-04**: Get page text (all visible text content)
- [ ] **CHR-05**: Find elements by natural language query
- [ ] **CHR-06**: Navigate (tabs, menus, back/forward)
- [ ] **CHR-07**: Tabs context (list windows, active window)
- [ ] **CHR-08**: Read console messages (qDebug, qWarning, etc.)

### QML Support

- [ ] **QML-01**: QML items appear in object tree as QQuickItem subclasses
- [ ] **QML-02**: QML id used in hierarchical paths when available
- [ ] **QML-03**: Basic QML properties accessible via standard introspection
- [ ] **QML-04**: QML context properties accessible
- [ ] **QML-05**: QML binding information accessible

### Model/View Support

- [ ] **MV-01**: List QAbstractItemModel instances in application
- [ ] **MV-02**: Get model row/column count
- [ ] **MV-03**: Get data at model index (with role specification)
- [ ] **MV-04**: Navigate model hierarchy (parent, children)

### Python MCP Server

- [ ] **PY-01**: MCP server connects to probe via WebSocket
- [ ] **PY-02**: MCP tool definitions for Native mode
- [ ] **PY-03**: MCP tool definitions for Computer Use mode
- [ ] **PY-04**: MCP tool definitions for Chrome mode
- [ ] **PY-05**: Mode switching via configuration or command

### Python Client Library

- [ ] **CLI-01**: WebSocket client connects to probe
- [ ] **CLI-02**: Async API for all probe methods
- [ ] **CLI-03**: Convenience methods for common operations
- [ ] **CLI-04**: Example automation scripts

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Extended Platform Support

- **PLAT-01**: macOS support (different injection approach required)
- **PLAT-02**: Qt 6.x as co-primary target (currently secondary)

### Advanced Introspection

- **ADV-01**: State machine visualization
- **ADV-02**: Layout diagnostic overlays
- **ADV-03**: Action recording for learning

### Security

- **SEC-01**: Authentication for remote connections
- **SEC-02**: Property/method blocklist for sensitive data

### Deployment

- **DEP-01**: vcpkg port
- **DEP-02**: Pre-built binaries for common Qt versions

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| macOS support | Different injection approach, deferred to v1.1 |
| Attach to running process | Adds significant complexity, launch-only for MVP |
| Image-based element recognition | Brittle, slow — use Qt's native object model instead |
| Recording/playback system | High complexity, not core to AI automation |
| Qt 4 support | Legacy, not worth supporting |
| Mobile (Android/iOS) | Different platforms entirely |
| Built-in GUI | Tool is headless; visualization via external tools |
| Plugin system | Adds complexity without clear benefit |
| Custom scripting language | JSON-RPC is sufficient; let AI use native reasoning |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| INJ-01 | Phase 1 | Complete |
| INJ-02 | Phase 1 | Complete |
| INJ-03 | Phase 1 | Complete |
| INJ-04 | Phase 1 | Complete |
| INJ-05 | Phase 1 | Complete |
| OBJ-01 | Phase 2 | Complete |
| OBJ-02 | Phase 2 | Complete |
| OBJ-03 | Phase 2 | Complete |
| OBJ-04 | Phase 2 | Complete |
| OBJ-05 | Phase 2 | Complete |
| OBJ-06 | Phase 2 | Complete |
| OBJ-07 | Phase 2 | Complete |
| OBJ-08 | Phase 2 | Complete |
| OBJ-09 | Phase 2 | Complete |
| OBJ-10 | Phase 2 | Complete |
| OBJ-11 | Phase 2 | Complete |
| SIG-01 | Phase 2 | Complete |
| SIG-02 | Phase 2 | Complete |
| SIG-03 | Phase 2 | Complete |
| SIG-04 | Phase 2 | Complete |
| SIG-05 | Phase 2 | Complete |
| UI-01 | Phase 2 | Complete |
| UI-02 | Phase 2 | Complete |
| UI-03 | Phase 2 | Complete |
| UI-04 | Phase 2 | Complete |
| UI-05 | Phase 2 | Complete |
| NAT-01 | Phase 3 | Pending |
| NAT-02 | Phase 3 | Pending |
| NAT-03 | Phase 3 | Pending |
| NAT-04 | Phase 3 | Pending |
| NAT-05 | Phase 3 | Pending |
| CU-01 | Phase 4 | Pending |
| CU-02 | Phase 4 | Pending |
| CU-03 | Phase 4 | Pending |
| CU-04 | Phase 4 | Pending |
| CU-05 | Phase 4 | Pending |
| CU-06 | Phase 4 | Pending |
| CU-07 | Phase 4 | Pending |
| CU-08 | Phase 4 | Pending |
| CU-09 | Phase 4 | Pending |
| CU-10 | Phase 4 | Pending |
| CHR-01 | Phase 5 | Pending |
| CHR-02 | Phase 5 | Pending |
| CHR-03 | Phase 5 | Pending |
| CHR-04 | Phase 5 | Pending |
| CHR-05 | Phase 5 | Pending |
| CHR-06 | Phase 5 | Pending |
| CHR-07 | Phase 5 | Pending |
| CHR-08 | Phase 5 | Pending |
| QML-01 | Phase 6 | Pending |
| QML-02 | Phase 6 | Pending |
| QML-03 | Phase 6 | Pending |
| QML-04 | Phase 6 | Pending |
| QML-05 | Phase 6 | Pending |
| MV-01 | Phase 6 | Pending |
| MV-02 | Phase 6 | Pending |
| MV-03 | Phase 6 | Pending |
| MV-04 | Phase 6 | Pending |
| PY-01 | Phase 7 | Pending |
| PY-02 | Phase 7 | Pending |
| PY-03 | Phase 7 | Pending |
| PY-04 | Phase 7 | Pending |
| PY-05 | Phase 7 | Pending |
| CLI-01 | Phase 7 | Pending |
| CLI-02 | Phase 7 | Pending |
| CLI-03 | Phase 7 | Pending |
| CLI-04 | Phase 7 | Pending |

**Coverage:**
- v1 requirements: 59 total
- Mapped to phases: 59
- Unmapped: 0

---
*Requirements defined: 2025-01-29*
*Last updated: 2025-01-29 after roadmap creation*
