# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-01)

**Core value:** Claude can control any Qt application with zero learning curve
**Current focus:** v1.1 Distribution & Compatibility — Phase 13 in progress

## Current Position

Phase: 13 of 13 (PyPI Publication)
Plan: 1 of 2 complete
Status: In progress
Last activity: 2026-02-03 — Completed 13-01-PLAN.md (Probe Download CLI)

Progress: [##################...] 75% (46/~62 plans)

## Performance Metrics

**v1.0 Velocity:**
- Total plans completed: 33
- Total execution time: 4.88 hours
- Commits: 157
- Timeline: 7 days (2026-01-25 to 2026-02-01)

**v1.1 Velocity:**
- Plans completed: 12
- Commits: 22

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
| Qt min versions: 5.15.1 / 6.5 | 11.1-02 | CMake FATAL_ERROR enforces minimum; QT_DISABLE_DEPRECATED_BEFORE=0x050F00 |
| CI matrix: Qt 6.2 -> 6.5 | 11.1-02 | 6.2 is below new minimum; replaced with 6.5.3 LTS |
| No Qt deps in vcpkg port | 12-01 | Port relies on user's Qt; project CMakeLists.txt handles find_package |
| Per-artifact SHA512 | 12-02 | Binary port has per-Qt-version hash for single-file downloads |
| CLI subcommand architecture | 13-01 | qtmcp serve / qtmcp download-probe - cleaner separation |
| Stdlib-only download module | 13-01 | No new deps (urllib, hashlib, pathlib) for lightweight package |

### Pending Todos

None.

### Roadmap Evolution

- Phase 11.1 inserted after Phase 11: Qt 5.15 / Qt 6 source compatibility (URGENT)

### Blockers/Concerns

- macOS deferred to separate milestone (v1.2?)
- Attach to running process deferred
- vcpkg ports have SHA512 placeholder values (must update for first release)

## Session Continuity

Last session: 2026-02-03
Stopped at: Completed 13-01-PLAN.md (Probe Download CLI)
Resume file: None

---
*State updated: 2026-02-03 (Phase 13 plan 1 complete — download-probe CLI added)*
