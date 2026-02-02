# Phase 8: CMake Multi-Qt Foundation - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Build system correctly produces versioned, Qt-aware, relocatable artifacts for both Qt5 and Qt6. One Qt version per build invocation; CI matrix handles multiple versions. No CI or packaging concerns — those are Phases 9-13.

</domain>

<decisions>
## Implementation Decisions

### Artifact naming
- Major.Minor version encoded in filename: `qtmcp-probe-qt5.15.dll` / `libqtmcp-probe-qt6.8.so`
- Platform/compiler NOT in filename — directory structure handles that
- Linux keeps `lib` prefix (follows platform convention)
- Debug builds use `d` suffix: `qtmcp-probe-qt6.8d.dll` / `libqtmcp-probe-qt6.8d.so`

### Install layout
- GNUInstallDirs standard layout: `lib/`, `include/`, `share/cmake/QtMCP/`
- Qt5 and Qt6 artifacts in versioned subdirectories: `lib/qtmcp/qt5.15/`, `lib/qtmcp/qt6.8/`
- Headers ARE installed — downstream C++ projects can link and use the API
- Fully relocatable: CMake config files use relative paths, no hardcoded prefixes

### find_package integration
- Auto-detects Qt version from consumer's project (if consumer has `find_package(Qt6)`, QtMCP picks Qt6 artifacts)
- Single exported target: `QtMCP::Probe` — resolves to correct binary based on detected Qt version
- Full version support: `QtMCPConfigVersion.cmake` with `SameMajorVersion` compatibility
- Provides helper function: `qtmcp_inject_probe(target)` for easy integration

### Qt version detection
- One Qt per CMake build invocation — CI matrix runs multiple builds
- Supports both `CMAKE_PREFIX_PATH` and custom `-DQTMCP_QT_DIR` hint, with hint taking precedence
- Fatal error at configure time if no Qt found — clear message pointing to `CMAKE_PREFIX_PATH` or `QTMCP_QT_DIR`
- Minimum Qt version floor: Qt 5.15+

### Claude's Discretion
- Internal CMake variable naming conventions
- How `qtmcp_inject_probe()` discovers the probe at runtime (LD_PRELOAD, QT_PLUGIN_PATH, etc.)
- CMake config file generation approach (configure_file vs install(EXPORT))
- Header install structure within `include/`

</decisions>

<specifics>
## Specific Ideas

- Naming follows Qt's own convention for debug suffix (`d` appended before extension)
- `qtmcp_inject_probe(target)` should make integration trivial for downstream consumers
- Versioned subdirs under `lib/qtmcp/` keep multi-Qt installs cleanly separated

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 08-cmake-multi-qt-foundation*
*Context gathered: 2026-02-01*
