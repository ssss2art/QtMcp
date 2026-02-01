# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2025-01-29)

**Core value:** Claude can control any Qt application with zero learning curve
**Current focus:** Phase 6 - Extended Introspection (QML + Model/View)

## Current Position

Phase: 6 of 7 (Extended Introspection)
Plan: 2 of 4 in current phase
Status: In progress
Last activity: 2026-02-01 - Completed 06-02-PLAN.md (ModelNavigator)

Progress: [##########################..] ~90% (27/30 plans)

## Performance Metrics

**Velocity:**
- Total plans completed: 27
- Average duration: 9.8 min
- Total execution time: 4.43 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01-foundation | 6 | 66 min | 11.0 min |
| 02-core-introspection | 6 | 104 min | 17.3 min |
| 03-native-mode | 4 | 24 min | 6.0 min |
| 04-computer-use-mode | 5 | 22 min | 4.4 min |
| 05-chrome-mode | 4 | 32 min | 8.0 min |
| 06-extended-introspection | 2 | 13 min | 6.5 min |

**Recent Trend:**
- Last 6 plans: 05-01 (4 min), 05-02 (8 min), 05-03 (10 min), 05-04 (10 min), 06-01 (7 min), 06-02 (6 min)
- Trend: Stable execution times

*Updated after each plan completion*

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

| Decision | Rationale | Plan |
|----------|-----------|------|
| Qt6 preferred, Qt5 fallback | Qt6 is current, but Qt5 5.15+ still widely used | 01-01 |
| nlohmann_json/spdlog optional | Phase 1 uses Qt built-ins (QJsonDocument, QDebug) | 01-01 |
| WINDOWS_EXPORT_ALL_SYMBOLS | Simplifies DLL development, can switch later | 01-01 |
| QTest instead of GTest | Eliminates external dependency, always available with Qt | 01-03 |
| QTMCP_EXPORT macro for QObjects | Required for proper MOC symbol export in Windows DLL | 01-03 |
| InitOnce API instead of std::call_once | MSVC std::call_once uses TLS internally, breaks in injected DLLs | 01-02 |
| Minimal DllMain pattern | Only DisableThreadLibraryCalls + flag; defer all Qt work | 01-02 |
| Single-client WebSocket semantics | Per CONTEXT.md - reject additional connections with CloseCodePolicyViolated | 01-04 |
| Server persists after disconnect | Keeps listening for reconnection | 01-04 |
| Q_COREAPP_STARTUP_FUNCTION for auto-init | Triggers probe initialization when Qt starts, no manual call needed | 01-05 |
| RAII HandleGuard for Windows handles | Automatic cleanup prevents resource leaks on error paths | 01-05 |
| Preserve existing LD_PRELOAD | Prepend to existing value rather than replacing | 01-05 |
| Atomic re-entry guard for hooks | std::atomic<bool> instead of thread_local to avoid TLS issues in DLLs | 02-01 |
| Destructor hook cleanup | Uninstall hooks in ObjectRegistry destructor to prevent shutdown crash | 02-01 |
| IDs computed at hook time | AddQObject fires at construction start, before derived ctors - ID reflects minimal state | 02-02 |
| QPointer for safe ID lookup | m_idToObject uses QPointer to detect deleted objects without dangling pointer | 02-02 |
| ID collision with ~N suffix | Duplicate IDs get ~1, ~2, etc. suffix for uniqueness | 02-02 |
| Explicit Qt type JSON conversion | QPoint/QSize/QRect/QColor explicitly converted, not relying on QJsonValue::fromVariant | 02-03 |
| Structured fallback for unknown types | Unknown types return {_type, value} for debugging | 02-03 |
| Dynamic property verify by read-back | Qt setProperty returns false for new dynamic properties | 02-04 |
| Method lookup by name + arg count | Simple approach without signature parsing | 02-04 |
| SignalRelay for dynamic connections | QMetaMethod-based signals need QObject with slot for connection | 02-05 |
| DirectConnection for cleanup caching | Caches objectId before QueuedConnection delivers objectRemoved | 02-05 |
| QTest for input simulation | Cross-platform reliable using Qt's built-in test functions | 02-06 |
| Base64 PNG for screenshots | JSON-friendly, universally supported format | 02-06 |
| devicePixelRatio in geometry | High-DPI awareness for Retina/HiDPI displays | 02-06 |
| JSON-RPC notification format for push | Push notifications use {"jsonrpc":"2.0","method":"qtmcp.X","params":{}} | 02-07 |
| sendMessage() on WebSocketServer | Added for outbound notification delivery to connected client | 02-07 |
| QStringView::mid() for Qt6 | midRef() removed in Qt6, use QStringView instead | 03-01 |
| Q_GLOBAL_STATIC for SymbolicNameMap | Consistent singleton pattern with ObjectRegistry, SignalMonitor | 03-01 |
| Auto-load name map from env/file | QTMCP_NAME_MAP env var or qtmcp-names.json in CWD | 03-01 |
| JsonRpcException for structured errors | Separate class in jsonrpc_handler.h with code + message + data | 03-02 |
| CreateErrorResponse with data overload | QJsonDocument-based for proper nested JSON support | 03-02 |
| Numeric IDs cleared on disconnect | Via WebSocketServer::clientDisconnected signal | 03-02 |
| Chrome/xdotool key aliases both mapped | Enter/Return both to Key_Return, ArrowUp/Up both to Key_Up | 04-01 |
| Manual QMouseEvent for new mouse ops | QTest kept for existing click/doubleClick, new ops use sendEvent | 04-01 |
| Scroll 120 units per tick | Standard QWheelEvent angleDelta convention | 04-01 |
| captureWindowLogical scales by DPR | 1:1 coordinate matching regardless of HiDPI | 04-01 |
| Window-relative coords by default | screenAbsolute=true for screen coords, matches Chrome CU convention | 04-02 |
| Active window fallback to visible widget | Searches QApplication::topLevelWidgets() if activeWindow() is null | 04-02 |
| delay_ms param on click methods | Allows UI settle time before action | 04-02 |
| Allow empty screenshot on minimal platform | captureWindowLogical uses grabWindow() which returns empty on minimal; test verifies structure always | 04-03 |
| File-scope static for virtual cursor tracking | All cu.* lambdas stateless (no this capture); avoids changing lambda semantics | 04-05 |
| RoleMapper as purely static class | Stateless lookup table needs no instance management | 05-01 |
| Q_GLOBAL_STATIC for ConsoleMessageCapture | Consistent with ObjectRegistry, SignalMonitor singleton patterns | 05-01 |
| "generic" fallback for unknown roles | Chrome/ARIA convention for unmapped roles | 05-01 |
| InputSimulator for chr.click mouse fallback | Consistent with CU mode pattern, not QTest directly | 05-02 |
| chr.find clears refs before search | Fresh refs assigned to matches only, avoids stale refs | 05-02 |
| ConsoleMessageCapture installed before API registration | Catches early messages during startup | 05-02 |
| Verify click by response fields not QSignalSpy | Accessibility pressAction on minimal platform doesn't emit clicked signal | 05-03 |
| ConsoleMessageCapture installed per-test in init() | Clean message buffer per test for deterministic assertions | 05-03 |
| chr.find appends to ref map | Enables multi-find workflows without destroying previous refs | 05-04 |
| readPage remains authoritative (clears find refs) | readPage rebuilds full tree, clearing all refs including find refs | 05-04 |
| Same name fallback in find as walkNode | Consistency between chr.readPage and chr.find output | 05-04 |
| Qt Quick as optional dependency | QTMCP_HAS_QML compile guard; inline stubs when not available | 06-01 |
| QML id highest priority in ID generation | Human-authored QML ids are ideal stable identifiers for paths | 06-01 |
| Short type names for anonymous QML items | QQuick prefix is internal detail, not meaningful to users | 06-01 |
| Static utility class for ModelNavigator | Same pattern as RoleMapper - stateless, no QObject inheritance | 06-02 |
| Smart pagination: all if <=100, else 100 | Small models complete; large models paginated to avoid response bloat | 06-02 |
| Three-step resolveModel | Direct cast, QAbstractItemView, property("model") QML fallback | 06-02 |

### Pending Todos

None - all known bugs resolved.

### Blockers/Concerns

**From Research:**
- Windows DLL pitfalls (CRT mismatch, TLS, DllMain) must be addressed in Phase 1 [ADDRESSED in 01-02]
- Object Registry needs mutex protection from day one (qtHookData not thread-safe) [IMPLEMENTED in 02-01]
- Chrome Mode (Phase 5) research complete - infrastructure classes built in 05-01
- QML introspection (Phase 6) may require private Qt APIs - feasibility TBD

**From Plan 01-01:**
- Build artifacts go to `build/bin/Debug/` on Windows (MSVC multi-config generator behavior)
- Qt 6.5+ changed QWebSocket::error to errorOccurred - handled with QT_VERSION_CHECK

**From Plan 01-02:**
- ensureInitialized() available for triggering deferred init from any code path
- Linux constructor checks QTMCP_ENABLED=0 to disable probe

**From Plan 01-03:**
- ctest cannot find Qt DLLs automatically - tests must be run with Qt in PATH
- Q_GLOBAL_STATIC requires public constructor in Qt6

**From Plan 01-04:**
- Full integration testing requires Qt DLLs in PATH
- Server prints startup message to stderr for debugging injection

**From Plan 01-05:**
- Windows file locking: DLL cannot be rebuilt while process using it is running
- Launcher now works end-to-end: qtmcp-launch target.exe -> ws://localhost:9222

**From Plan 01-06:**
- Phase 1 complete - all INJ requirements verified working
- Test app available for Phase 2+ development

**From Plan 02-01:**
- Hook callbacks can re-enter during singleton creation - guard with atomic flag
- ObjectRegistry destructor must uninstall hooks to prevent crash on exit
- Tests need QTMCP_ENABLED=0 env var to disable auto-initialization

**From Plan 02-02:**
- Object IDs computed at hook time (before objectName is set) - tests must account for this
- Qt Test output not visible in console - use `-o file,txt` format for debugging
- findById() safely handles deleted objects via QPointer

**From Plan 02-03:**
- Qt `signals` macro conflicts with variable names - avoid naming variables "signals"
- variantToJson handles common Qt types; unknown types fall back to structured output

**From Plan 02-04:**
- getProperty/setProperty support both declared and dynamic properties
- invokeMethod only calls slots and Q_INVOKABLE methods (not property getters)
- Dynamic property setProperty verifies by reading back (Qt returns false for new props)

**From Plan 02-05:**
- SignalRelay helper class needed for dynamic signal connections
- m_destroyedObjectIds cache bridges DirectConnection/QueuedConnection timing
- Lifecycle notifications are opt-in (disabled by default)

**From Plan 02-06:**
- InputSimulator uses QTest functions - position defaults to widget center
- Screenshots are base64 PNG - decode to verify PNG magic bytes
- HitTest::widgetIdAt integrates with ObjectRegistry for hierarchical IDs

**From Plan 02-07:**
- All 21 JSON-RPC methods registered in RegisterBuiltinMethods()
- Push notifications wired via Probe::initialize() -> SignalMonitor connections
- WebSocketServer::sendMessage() added for outbound notifications

**From Plan 03-01:**
- ErrorCodes provides constexpr error codes in -32001 to -32052 range
- ResponseEnvelope::wrap() returns QJsonObject {result, meta{timestamp}}
- SymbolicNameMap auto-loads from QTMCP_NAME_MAP env or qtmcp-names.json
- ObjectResolver resolves numeric (#N), symbolic, and hierarchical path IDs
- QString::midRef() removed in Qt6 - use QStringView::mid() instead

**From Plan 03-02:**
- NativeModeApi registers 29 qt.* methods (old qtmcp.* methods kept for backward compat)
- JsonRpcException caught before std::exception in HandleMessage
- Numeric IDs cleared on client disconnect
- Name map auto-loaded during Probe::initialize()

## Phase 6 Progress

| Plan | Name | Status |
|------|------|--------|
| 06-01 | QML Inspector Infrastructure | COMPLETE |
| 06-02 | Model/View Navigation (ModelNavigator) | COMPLETE |
| 06-03 | QML + Model API Wiring | PENDING |
| 06-04 | Extended Introspection Testing | PENDING |

**From Plan 06-01:**
- QmlItemInfo struct + inspectQmlItem()/stripQmlPrefix()/isQmlItem() in qml_inspector.h/.cpp
- Qt Quick is optional dependency: QTMCP_HAS_QML compile guard with inline stub fallbacks
- QML id takes highest priority in generateIdSegment() for QQuickItems
- Anonymous QML items use short type name (Rectangle not QQuickRectangle)
- serializeObjectInfo() and tree serialization include isQmlItem, qmlId, qmlFile, qmlTypeName
- QML error codes (-32080 to -32082) and Model error codes (-32090 to -32093) added
- All 11 existing test suites pass with zero regressions

**From Plan 06-02:**
- ModelNavigator: static utility class with 6 methods (listModels, getModelInfo, getModelData, resolveModel, resolveRoleName, getRoleNames)
- Smart pagination: all rows if <=100, first 100 otherwise; explicit offset/limit supported
- resolveModel: direct QAbstractItemModel* cast, QAbstractItemView::model(), property("model") QML fallback
- resolveRoleName: checks model->roleNames() first, then 12 standard Qt roles
- Internal Qt models filtered (className starts with "Q" and contains "Internal")
- All 11 existing test suites pass with zero regressions

## Phase 5 Progress

| Plan | Name | Status |
|------|------|--------|
| 05-01 | Chrome Mode Infrastructure Classes | COMPLETE |
| 05-02 | ChromeModeApi (8 chr.* methods) | COMPLETE |
| 05-03 | Chrome Mode API Testing | COMPLETE |
| 05-04 | chr.find Gap Closure (ref wipe + name fallback) | COMPLETE |

**From Plan 05-02:**
- ChromeModeApi registers 8 chr.* methods: readPage, click, formInput, getPageText, find, navigate, tabsContext, readConsoleMessages
- All 3 API modes (qt.*, cu.*, chr.*) active simultaneously on same JsonRpcHandler
- Ephemeral ref lifecycle: cleared on readPage, find, and client disconnect
- Multi-strategy form input: ComboBox > toggle > value > editable text
- QAccessible::Button == QAccessible::PushButton in Qt6 (same enum value 43)
- ConsoleMessageCapture installed before API registrations in Probe::initialize()
- All 10 existing test suites pass with zero regressions

**From Plan 05-04 (Gap Closure):**
- chr.find no longer clears ref map; refCounter seeded from s_refToAccessible.size()
- buildFindMatchNode uses 3-step name fallback matching walkNode (accessible name -> objectName -> className)
- 29 Chrome Mode tests (26 original + 3 regression) all pass
- Multi-find workflows now work: refs from first find survive second find call

**From Plan 05-03:**
- 26 integration tests verify all 8 chr.* methods end-to-end
- readPage, click, formInput, getPageText, find, tabsContext, readConsoleMessages all tested
- Error cases: invalid ref, stale ref, unsupported form input
- All 11 test suites (11 executables) pass with zero regressions
- Phase 5 complete - all Chrome Mode success criteria verified

**From Plan 05-01:**
- RoleMapper maps ~55 QAccessible::Role to Chrome/ARIA names, isInteractive() covers 18 roles
- ConsoleMessageCapture: Q_GLOBAL_STATIC singleton, ring buffer (1000), chains to previous handler
- AccessibilityTreeWalker: recursive walk with ref_N assignment, supports "interactive"/"all" filter
- Chrome Mode error codes -32070 to -32076 added to error_codes.h
- New files in src/probe/accessibility/ - CMakeLists.txt update needed in Plan 05-02

## Phase 4 Progress

| Plan | Name | Status |
|------|------|--------|
| 04-01 | Interaction Layer Primitives | COMPLETE |
| 04-02 | ComputerUseModeApi | COMPLETE |
| 04-03 | Testing | COMPLETE |
| 04-04 | Probe Resilience Gap Closure | COMPLETE |
| 04-05 | Virtual Cursor Position Tracking | COMPLETE |

**From Plan 04-01:**
- KeyNameMapper provides case-insensitive Chrome/xdotool key name to Qt::Key lookup
- InputSimulator extended with mousePress/mouseRelease/mouseMove/scroll/mouseDrag
- Screenshot extended with captureScreen (full screen) and captureWindowLogical (HiDPI-aware)
- New files need CMakeLists.txt update in Plan 02 for build integration
- All new mouse methods use manual QMouseEvent construction (not QTest)

**From Plan 04-02:**
- 13 cu.* methods registered alongside 29 qt.* methods on same JsonRpcHandler
- ComputerUseModeApi follows exact NativeModeApi pattern (anonymous namespace helpers, envelope wrapping)
- key_name_mapper.cpp/h now included in CMakeLists.txt (was only created in 04-01)
- Error codes -32060 to -32063 reserved for CU-specific errors
- Both APIs active simultaneously - no mode gating yet

**From Plan 04-03:**
- 10 KeyNameMapper unit tests + 17 ComputerUseModeApi integration tests
- All 10 test suites (10 executables) pass with zero regressions
- Screenshot tests handle minimal platform (empty grabWindow result)
- Phase 4 complete - all CU requirements verified

## Phase 3 Progress

| Plan | Name | Status |
|------|------|--------|
| 03-01 | Infrastructure Classes | COMPLETE |
| 03-02 | NativeModeApi | COMPLETE |
| 03-03 | Testing | COMPLETE |
| 03-04 | Object ID Resolution Gap Closure | COMPLETE |

**From Plan 03-03:**
- 29 integration tests verify complete qt.* API surface
- Per-test init/cleanup ensures isolation (fresh handler + API + widgets)
- ObjectResolver.clearNumericIds() needed in cleanup to prevent cross-test state leakage
- All 8 test suites (8 executables, 100+ test functions total) pass

**From Plan 03-04 (Gap Closure):**
- getTopLevelObjects() must include QCoreApplication::instance() itself, not just its children
- Without this fix, ALL global findByObjectId lookups by hierarchical path fail
- testFindByIdGlobal regression test prevents this from regressing

**From Plan 04-04 (Gap Closure):**
- NativeModeApi and ComputerUseModeApi each in independent try/catch - probe survives partial API failure
- All 11 legacy qtmcp.* methods accept both "id" and "objectId" parameter names
- fprintf used for registration logging (safe in DLL context before Qt logging init)

**From Plan 04-05 (Gap Closure):**
- cu.cursorPosition returns virtual position from last CU coordinate action, not physical OS cursor
- File-scope static s_lastSimulatedPosition/s_hasSimulatedPosition for stateless lambda compatibility
- All 9 coordinate-based cu.* methods update tracked position via trackPosition() helper
- Response includes "virtual": true/false to indicate position source
- Fallback to QCursor::pos() when no CU actions have been performed

## Session Continuity

Last session: 2026-02-01
Stopped at: Completed 06-02-PLAN.md (ModelNavigator)
Resume file: None

---
*State updated: 2026-02-01 (completed 06-02 - ModelNavigator utility class for model introspection)*
