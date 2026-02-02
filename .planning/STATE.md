# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-01)

**Core value:** Claude can control any Qt application with zero learning curve
**Current focus:** v1.1 Distribution & Compatibility — Phase 11.1 in progress

## Current Position

Phase: 11.1 of 13 (Qt5/Qt6 Source Compatibility)
Plan: 1 of 2 complete
Status: In progress
Last activity: 2026-02-02 — Completed 11.1-01-PLAN.md (Compat Headers)

Progress: [###############......] 68% (42/~62 plans)

## Performance Metrics

**v1.0 Velocity:**
- Total plans completed: 33
- Total execution time: 4.88 hours
- Commits: 157
- Timeline: 7 days (2026-01-25 to 2026-02-01)

**v1.1 Velocity:**
- Plans completed: 8
- Commits: 14

## Accumulated Context

### Decisions

See PROJECT.md Key Decisions table for full log.

| Decision | Phase | Detail |
|----------|-------|--------|
| Artifact naming: qt{M}.{m} tag | 08-01 | qtmcp-probe-qt6.9[d].dll format |
| Install layout: versioned lib dirs | 08-01 | lib/qtmcp/qt{M}.{m}/ for side-by-side |
| Export name: QtMCP::Probe | 08-01 | Capital P, follows CMake convention |
| Manual IMPORTED target | 08-02 | Replaced CMake EXPORT with manual target in config for versioned path support |
| DLL in lib/ not bin/ | 08-02 | Single search location for DLL and import lib in config file |
| vcpkg Qt feature-gate | 09-01 | Qt deps moved to opt-in vcpkg feature to avoid 30+ min CI builds |
| Include-style matrix | 09-01 | Runner OS varies per Qt version; include entries not combinatorial |
| No restore-keys for Qt cache | 10-01 | Partial match would restore unpatched Qt; exact key only |
| git apply for patches | 10-01 | Cross-platform consistency; works on both Linux and Windows |
| Push-only trigger for patched CI | 10-02 | Cold Qt builds take 30-60 min; no PR trigger saves CI minutes |
| windows-2022 pinned for patched CI | 10-02 | MSVC v142 toolset guaranteed for Qt 5.15.1 compatibility |
| fail_on_unmatched_files in release | 11-02 | Prevents incomplete releases if any probe binary missing |

### Pending Todos

None.

### Roadmap Evolution

- Phase 11.1 inserted after Phase 11: Qt 5.15 / Qt 6 source compatibility (URGENT)

### Blockers/Concerns

- macOS deferred to separate milestone (v1.2?)
- Attach to running process deferred

## Session Continuity

Last session: 2026-02-02
Stopped at: Completed 11.1-01-PLAN.md (Compat Headers)
Resume file: None

---
*State updated: 2026-02-02 (Phase 11.1 plan 01 complete -- compat headers created)*
