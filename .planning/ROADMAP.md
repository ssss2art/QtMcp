# Roadmap: QtMCP

## Milestones

- **v1.0 MVP** — Phases 1-7 (shipped 2026-02-01) — [archive](milestones/v1.0-ROADMAP.md)
- **v1.1 Distribution & Compatibility** — Phases 8-13 (in progress)

## Phases

<details>
<summary>v1.0 MVP (Phases 1-7) — SHIPPED 2026-02-01</summary>

- [x] Phase 1: Foundation (6/6 plans) — completed 2026-01-30
- [x] Phase 2: Core Introspection (7/7 plans) — completed 2026-01-30
- [x] Phase 3: Native Mode (4/4 plans) — completed 2026-01-31
- [x] Phase 4: Computer Use Mode (5/5 plans) — completed 2026-01-31
- [x] Phase 5: Chrome Mode (4/4 plans) — completed 2026-02-01
- [x] Phase 6: Extended Introspection (4/4 plans) — completed 2026-02-01
- [x] Phase 7: Python Integration (3/3 plans) — completed 2026-02-01

</details>

### v1.1 Distribution & Compatibility (In Progress)

**Milestone Goal:** Anyone can install QtMCP from their preferred package manager and CI produces tested binaries for every supported Qt version.

- [x] **Phase 8: CMake Multi-Qt Foundation** - Build system produces correctly named, relocatable, multi-Qt artifacts — completed 2026-02-02
- [ ] **Phase 9: CI Matrix Build** - GitHub Actions builds probe for 4 Qt versions on 2 platforms
- [ ] **Phase 10: Patched Qt 5.15.1 CI** - CI builds and caches custom-patched Qt 5.15.1, then builds probe against it
- [ ] **Phase 11: Release Automation** - Tag push produces a GitHub Release with all 10 binaries and checksums
- [ ] **Phase 12: vcpkg Port** - Users can install QtMCP probe via vcpkg overlay port
- [ ] **Phase 13: PyPI Publication** - pip install qtmcp provides working MCP server with probe downloader

## Phase Details

### Phase 8: CMake Multi-Qt Foundation
**Goal**: Build system correctly produces versioned, Qt-aware, relocatable artifacts for both Qt5 and Qt6
**Depends on**: Phase 7 (v1.0 complete)
**Requirements**: BUILD-01, BUILD-02, BUILD-03
**Research**: Skip (standard CMake patterns)
**Success Criteria** (what must be TRUE):
  1. Building against Qt 5.15 produces a file named `qtmcp-probe-qt5.15.dll` (Windows) or `.so` (Linux)
  2. Building against Qt 6.8 produces a file named `qtmcp-probe-qt6.8.dll` / `.so`
  3. A downstream project can `find_package(QtMCP)` after install regardless of whether it uses Qt5 or Qt6
  4. `cmake --install` produces artifacts in standard layout with versioned paths that are relocatable
**Plans:** 2 plans
Plans:
- [x] 08-01-PLAN.md — Versioned artifact naming and install layout
- [x] 08-02-PLAN.md — find_package config and qtmcp_inject_probe helper

### Phase 9: CI Matrix Build
**Goal**: Every push validates the probe builds cleanly on 4 Qt versions across Windows and Linux
**Depends on**: Phase 8
**Requirements**: CICD-01, CICD-02, CICD-03, CICD-04
**Research**: Skip (standard GitHub Actions patterns)
**Success Criteria** (what must be TRUE):
  1. Push to main triggers matrix build across Qt 5.15, 6.2, 6.8, 6.9 on both Windows (MSVC) and Linux (GCC) — 8 cells total
  2. All 8 matrix cells produce probe binaries as uploadable artifacts
  3. A failing cell blocks the overall check (matrix is not allow-failure)
  4. Qt is installed via `jurplel/install-qt-action@v4` with version-appropriate runners (Ubuntu 22.04 for Qt 5.15, 24.04 for Qt 6.x)
**Plans:** 1 plan
Plans:
- [ ] 09-01-PLAN.md — Matrix CI workflow with 8 cells (4 Qt versions x 2 platforms)

### Phase 10: Patched Qt 5.15.1 CI
**Goal**: CI can build and cache a custom-patched Qt 5.15.1 and produce probe binaries against it
**Depends on**: Phase 9
**Requirements**: CICD-05, CICD-06
**Research**: NEEDED (build recipe for patched Qt 5.15.1 from source, caching strategy)
**Success Criteria** (what must be TRUE):
  1. CI workflow builds Qt 5.15.1 from source with user-supplied patches on both Windows and Linux
  2. Built Qt is cached aggressively (cache hit skips 30-60 min build)
  3. Probe builds successfully against the patched Qt 5.15.1 on both platforms
  4. Patched Qt build is isolated (does not interfere with standard matrix cells)
**Plans**: TBD

### Phase 11: Release Automation
**Goal**: Pushing a version tag automatically produces a complete GitHub Release with all binaries
**Depends on**: Phase 9, Phase 10
**Requirements**: CICD-07, CICD-08
**Research**: Skip (standard release workflow patterns)
**Success Criteria** (what must be TRUE):
  1. Pushing a `v*` tag triggers a release workflow that collects all 10 probe binaries (8 standard + 2 patched)
  2. Release page lists all 10 binaries with correct Qt-version-encoded filenames
  3. A SHA256SUMS file is included in the release listing checksums for every artifact
  4. Release is created via `softprops/action-gh-release@v2` using `upload-artifact@v4` merge pattern
**Plans**: TBD

### Phase 12: vcpkg Port
**Goal**: Users can install QtMCP via vcpkg overlay port using either their own Qt5 or Qt6
**Depends on**: Phase 11
**Requirements**: VCPKG-01, VCPKG-02, VCPKG-03
**Research**: Skip (standard vcpkg overlay port patterns)
**Success Criteria** (what must be TRUE):
  1. Source overlay port builds probe from source against the user's existing Qt installation (no vcpkg qtbase dependency declared)
  2. Binary overlay port downloads the correct prebuilt probe from GitHub Releases
  3. Both port types work with Qt5 and Qt6 installations
  4. `vcpkg install qtmcp --overlay-ports=./ports` succeeds on a clean environment with Qt already installed
**Plans**: TBD

### Phase 13: PyPI Publication
**Goal**: `pip install qtmcp` gives users a working MCP server with a CLI to fetch the correct probe
**Depends on**: Phase 11
**Requirements**: PYPI-01, PYPI-02, PYPI-03, PYPI-04
**Research**: Skip (standard Python packaging patterns)
**Success Criteria** (what must be TRUE):
  1. `pip install qtmcp` installs cleanly from PyPI and provides `qtmcp` CLI entry point
  2. `qtmcp download-probe --qt-version 6.8` fetches the correct probe binary from GitHub Releases
  3. The installed package is a pure-Python wheel (no compiled extensions, no bundled probe)
  4. Publishing uses Trusted Publishers (OIDC) workflow — no API tokens stored in secrets
**Plans**: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Foundation | v1.0 | 6/6 | Complete | 2026-01-30 |
| 2. Core Introspection | v1.0 | 7/7 | Complete | 2026-01-30 |
| 3. Native Mode | v1.0 | 4/4 | Complete | 2026-01-31 |
| 4. Computer Use Mode | v1.0 | 5/5 | Complete | 2026-01-31 |
| 5. Chrome Mode | v1.0 | 4/4 | Complete | 2026-02-01 |
| 6. Extended Introspection | v1.0 | 4/4 | Complete | 2026-02-01 |
| 7. Python Integration | v1.0 | 3/3 | Complete | 2026-02-01 |
| 8. CMake Multi-Qt Foundation | v1.1 | 2/2 | Complete | 2026-02-02 |
| 9. CI Matrix Build | v1.1 | 0/1 | Planned | - |
| 10. Patched Qt 5.15.1 CI | v1.1 | 0/TBD | Not started | - |
| 11. Release Automation | v1.1 | 0/TBD | Not started | - |
| 12. vcpkg Port | v1.1 | 0/TBD | Not started | - |
| 13. PyPI Publication | v1.1 | 0/TBD | Not started | - |

---
*Roadmap created: 2025-01-29*
*v1.0 shipped: 2026-02-01*
*v1.1 roadmap added: 2026-02-01*
