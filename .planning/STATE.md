# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2025-01-29)

**Core value:** Claude can control any Qt application with zero learning curve
**Current focus:** Phase 1 - Foundation

## Current Position

Phase: 1 of 7 (Foundation)
Plan: 4 of 6 in current phase
Status: In progress
Last activity: 2026-01-30 - Completed 01-04-PLAN.md (WebSocket Server)

Progress: [####------] 40%

## Performance Metrics

**Velocity:**
- Total plans completed: 4
- Average duration: 10.8 min
- Total execution time: 0.72 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01-foundation | 4 | 43 min | 10.8 min |

**Recent Trend:**
- Last 5 plans: 01-01 (16 min), 01-03 (17 min), 01-02 (4 min), 01-04 (6 min)
- Trend: Improving (01-04 fast due to focused scope)

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

### Pending Todos

None yet.

### Blockers/Concerns

**From Research:**
- Windows DLL pitfalls (CRT mismatch, TLS, DllMain) must be addressed in Phase 1 [ADDRESSED in 01-02]
- Object Registry needs mutex protection from day one (qtHookData not thread-safe)
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

## Session Continuity

Last session: 2026-01-30
Stopped at: Completed 01-04-PLAN.md
Resume file: .planning/phases/01-foundation/01-05-PLAN.md

---
*State updated: 2026-01-30*
