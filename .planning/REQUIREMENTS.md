# Requirements: QtMCP v1.1

**Defined:** 2026-02-01
**Core Value:** Claude can control any Qt application with zero learning curve

## v1.1 Requirements

### Build System

- [x] **BUILD-01**: Probe output encodes Qt version in filename (e.g. `qtmcp-probe-qt5.15.dll`, `qtmcp-probe-qt6.8.so`)
- [x] **BUILD-02**: QtMCPConfig.cmake.in works for both Qt5 and Qt6 consumers (templated with QT_VERSION_MAJOR)
- [x] **BUILD-03**: CMake install targets produce versioned, relocatable artifacts with proper install components

### CI/CD

- [ ] **CICD-01**: GitHub Actions matrix builds probe for Qt 5.15 on Windows (MSVC) and Linux (GCC)
- [ ] **CICD-02**: GitHub Actions matrix builds probe for Qt 6.2 on Windows (MSVC) and Linux (GCC)
- [ ] **CICD-03**: GitHub Actions matrix builds probe for Qt 6.8 on Windows (MSVC) and Linux (GCC)
- [ ] **CICD-04**: GitHub Actions matrix builds probe for Qt 6.9 on Windows (MSVC) and Linux (GCC)
- [ ] **CICD-05**: CI builds custom-patched Qt 5.15.1 from source with aggressive caching
- [ ] **CICD-06**: CI builds probe against patched Qt 5.15.1 on Windows (MSVC) and Linux (GCC)
- [ ] **CICD-07**: Tag-triggered release workflow collects all 10 probe binaries into GitHub Release
- [ ] **CICD-08**: Release includes SHA256 checksums for all artifacts

### Packaging — vcpkg

- [ ] **VCPKG-01**: Source overlay port builds probe against user's Qt installation (no qtbase vcpkg dependency)
- [ ] **VCPKG-02**: Binary overlay port downloads prebuilt probe from GitHub Releases
- [ ] **VCPKG-03**: Port works with both Qt5 and Qt6 installations

### Packaging — PyPI

- [ ] **PYPI-01**: Python MCP server publishable as pure-Python wheel to PyPI
- [ ] **PYPI-02**: `pip install qtmcp` installs working MCP server with CLI entry point
- [ ] **PYPI-03**: `qtmcp download-probe` CLI command fetches correct probe binary for user's Qt version from GitHub Releases
- [ ] **PYPI-04**: PyPI publishing uses Trusted Publishers (OIDC), not API tokens

## Future Requirements

### macOS Support (v1.2)

- **MAC-01**: DYLD_INSERT_LIBRARIES injection on macOS
- **MAC-02**: macOS launcher
- **MAC-03**: CI builds for macOS (Clang/AppleClang)
- **MAC-04**: macOS probe binaries in GitHub Releases

### Enhanced Distribution

- **DIST-01**: Code signing for Windows DLLs
- **DIST-02**: Submission to vcpkg curated registry
- **DIST-03**: Debug/Release build variants in releases
- **DIST-04**: Attach to running process support

## Out of Scope

| Feature | Reason |
|---------|--------|
| macOS support | Separate milestone (v1.2), different injection approach |
| Code signing | Requires certificate purchase, defer to post-distribution |
| vcpkg curated registry | Requires review process, overlay port first |
| Bundling probe in Python wheel | Architecture mismatch — probe is Qt-version-specific, wheel is platform-generic |
| Debug/Release variants | Release-only for initial distribution, reduces matrix complexity |
| Qt 6.5 support | Not requested, LTS focus is 6.2 and 6.8 |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUILD-01 | Phase 8 | Complete |
| BUILD-02 | Phase 8 | Complete |
| BUILD-03 | Phase 8 | Complete |
| CICD-01 | Phase 9 | Pending |
| CICD-02 | Phase 9 | Pending |
| CICD-03 | Phase 9 | Pending |
| CICD-04 | Phase 9 | Pending |
| CICD-05 | Phase 10 | Pending |
| CICD-06 | Phase 10 | Pending |
| CICD-07 | Phase 11 | Pending |
| CICD-08 | Phase 11 | Pending |
| VCPKG-01 | Phase 12 | Pending |
| VCPKG-02 | Phase 12 | Pending |
| VCPKG-03 | Phase 12 | Pending |
| PYPI-01 | Phase 13 | Pending |
| PYPI-02 | Phase 13 | Pending |
| PYPI-03 | Phase 13 | Pending |
| PYPI-04 | Phase 13 | Pending |

**Coverage:**
- v1.1 requirements: 18 total
- Mapped to phases: 18
- Unmapped: 0

---
*Requirements defined: 2026-02-01*
*Last updated: 2026-02-01 — traceability mapped to phases 8-13*
