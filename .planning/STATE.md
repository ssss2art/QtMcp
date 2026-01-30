# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2025-01-29)

**Core value:** Claude can control any Qt application with zero learning curve
**Current focus:** Phase 2 - Core Introspection (IN PROGRESS)

## Current Position

Phase: 2 of 7 (Core Introspection)
Plan: 6 of 6 in current phase
Status: Phase complete
Last activity: 2026-01-30 - Completed 02-06-PLAN.md (UI Interaction)

Progress: [##########] 100%

## Performance Metrics

**Velocity:**
- Total plans completed: 10
- Average duration: 13.4 min
- Total execution time: 2.23 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01-foundation | 6 | 66 min | 11.0 min |
| 02-core-introspection | 4 | 68 min | 17.0 min |

**Recent Trend:**
- Last 6 plans: 01-05 (15 min), 01-06 (8 min), 02-01 (32 min), 02-02 (10 min), 02-03 (11 min), 02-06 (15 min)
- Trend: Phase 2 plans averaging 17 min

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
| QTest for input simulation | Cross-platform reliable using Qt's built-in test functions | 02-06 |
| Base64 PNG for screenshots | JSON-friendly, universally supported format | 02-06 |
| devicePixelRatio in geometry | High-DPI awareness for Retina/HiDPI displays | 02-06 |

### Pending Todos

None yet.

### Blockers/Concerns

**From Research:**
- Windows DLL pitfalls (CRT mismatch, TLS, DllMain) must be addressed in Phase 1 [ADDRESSED in 01-02]
- Object Registry needs mutex protection from day one (qtHookData not thread-safe) [IMPLEMENTED in 02-01]
- Chrome Mode (Phase 5) needs research - QAccessible to Chrome tree mapping is novel
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

**From Plan 02-06:**
- InputSimulator uses QTest functions - position defaults to widget center
- Screenshots are base64 PNG - decode to verify PNG magic bytes
- HitTest::widgetIdAt integrates with ObjectRegistry for hierarchical IDs

## Phase 2 Progress

| Plan | Name | Status |
|------|------|--------|
| 02-01 | Object Registry | COMPLETE |
| 02-02 | Object ID System | COMPLETE |
| 02-03 | Meta Inspector | COMPLETE |
| 02-04 | Signal Monitor | SKIPPED (external partial work) |
| 02-05 | Property/Method Ops | SKIPPED (external partial work) |
| 02-06 | UI Interaction | COMPLETE |

**Note:** Plans 02-04 and 02-05 have partial external changes that need reconciliation before execution.

## Session Continuity

Last session: 2026-01-30
Stopped at: Completed 02-06-PLAN.md (UI Interaction)
Resume file: .planning/phases/03-jsonrpc/03-01-PLAN.md (next phase)

---
*State updated: 2026-01-30*
