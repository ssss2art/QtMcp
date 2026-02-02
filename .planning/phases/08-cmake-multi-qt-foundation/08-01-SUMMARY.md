---
phase: 08-cmake-multi-qt-foundation
plan: 01
subsystem: build
tags: [cmake, qt-version, install-layout, artifact-naming]
dependency-graph:
  requires: []
  provides: [QTMCP_QT_VERSION_TAG, versioned-install-layout, QTMCP_QT_DIR-hint]
  affects: [09-ci-matrix, 12-vcpkg-port]
tech-stack:
  added: []
  patterns: [versioned-artifact-naming, debug-postfix-convention]
key-files:
  created: []
  modified:
    - CMakeLists.txt
    - src/probe/CMakeLists.txt
    - src/launcher/CMakeLists.txt
    - cmake/QtMCPConfig.cmake.in
decisions:
  - id: BUILD-01-naming
    description: "Artifact names encode Qt major.minor: qtmcp-probe-qt{M}.{m}[d].{ext}"
  - id: BUILD-01-install
    description: "Libraries install to lib/qtmcp/qt{M}.{m}/, CMake config to share/cmake/QtMCP/"
  - id: BUILD-01-export
    description: "Exported target renamed from QtMCP::probe to QtMCP::Probe (capital P)"
metrics:
  duration: ~8 minutes
  completed: 2026-02-02
---

# Phase 8 Plan 01: CMake Versioned Artifact Naming Summary

**One-liner:** Qt-version-encoded artifact names (qtmcp-probe-qt6.9[d].dll) with versioned install layout under lib/qtmcp/qt{M}.{m}/

## What Was Done

### Task 1: QTMCP_QT_DIR hint and version tag variables
- Added `QTMCP_QT_DIR` cache PATH option that prepends to `CMAKE_PREFIX_PATH`, giving it priority over any other Qt detection
- Added explicit fatal error when neither Qt5 nor Qt6 is found, with helpful guidance message
- Computed `QTMCP_QT_VERSION_TAG` (e.g. `qt6.9`) from detected Qt version using regex extraction
- Defined versioned install directories: `QTMCP_INSTALL_LIBDIR` = `lib/qtmcp/qt{M}.{m}/`, `QTMCP_INSTALL_BINDIR` = `bin/qtmcp/qt{M}.{m}/`
- Updated all install() targets: libraries to versioned lib dir, DLLs to versioned bin dir, headers to unversioned `include/qtmcp/`, CMake config to `share/cmake/QtMCP/`
- Updated `QtMCPConfig.cmake.in` to properly detect Qt5 or Qt6 (was hardcoded to Qt5)
- Added version tag to configuration summary output

### Task 2: Versioned OUTPUT_NAME with debug suffix
- Probe: `OUTPUT_NAME` changed from `qtmcp-probe` to `qtmcp-probe-${QTMCP_QT_VERSION_TAG}`
- Launcher: `OUTPUT_NAME` changed from `qtmcp-launch` to `qtmcp-launch-${QTMCP_QT_VERSION_TAG}`
- Both targets: `DEBUG_POSTFIX "d"` added for debug builds
- Probe `EXPORT_NAME` changed from `probe` to `Probe` (capital P), alias updated to `QtMCP::Probe`

## Verified Artifacts

**Release build (Qt 6.9.1):**
- `bin/Release/qtmcp-probe-qt6.9.dll`
- `bin/Release/qtmcp-launch-qt6.9.exe`
- `lib/Release/qtmcp-probe-qt6.9.lib`

**Debug build (Qt 6.9.1):**
- `bin/Debug/qtmcp-probe-qt6.9d.dll`
- `bin/Debug/qtmcp-launch-qt6.9d.exe`
- `lib/Debug/qtmcp-probe-qt6.9d.lib`

**Install layout (Release):**
- `install/lib/qtmcp/qt6.9/qtmcp-probe-qt6.9.lib`
- `install/bin/qtmcp/qt6.9/qtmcp-probe-qt6.9.dll`
- `install/include/qtmcp/` (all headers, unversioned)
- `install/share/cmake/QtMCP/` (CMake config files)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Fixed QtMCPConfig.cmake.in Qt version detection**
- **Found during:** Task 1
- **Issue:** Config template hardcoded `find_dependency(Qt5 ...)` and `find_dependency(nlohmann_json)` / `find_dependency(spdlog)` which are optional
- **Fix:** Updated to try Qt6 first then fall back to Qt5, removed mandatory optional dependencies
- **Files modified:** cmake/QtMCPConfig.cmake.in
- **Commit:** 0b409f3

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| Version tag format `qt{M}.{m}` (e.g. qt6.9) | Matches Qt's own versioning, readable, sortable |
| Install libs to `lib/qtmcp/qt{M}.{m}/` | Allows side-by-side Qt version installs |
| CMake config to `share/cmake/QtMCP/` | Avoids collision with versioned lib/ directory |
| Headers unversioned at `include/qtmcp/` | Same API regardless of Qt version |
| Export name `QtMCP::Probe` (capital P) | Follows CMake convention for exported targets |

## Next Phase Readiness

- Phase 9 (CI): Can configure matrix builds using `-DQTMCP_QT_DIR=<path>` for each Qt version
- Phase 12 (vcpkg): Install layout matches standard vcpkg port structure expectations
- All existing tests still compile and link against renamed targets (verified)
