---
phase: 10
plan: 02
subsystem: ci
tags: [github-actions, patched-qt, ci-workflow, qt-5.15.1]
depends_on: [10-01]
requires:
  - "10-01: Build-Qt composite action"
provides:
  - "CI workflow for patched Qt 5.15.1 on Linux + Windows"
  - "Artifact upload with patched naming convention"
affects:
  - "Future patched Qt versions (workflow template)"
  - "Phase 11+: Downstream consumers of patched Qt artifacts"
tech-stack:
  added: []
  patterns:
    - "Separate workflow for expensive from-source builds (push-only, no PRs)"
    - "MSVC v142 toolset pinning for Qt 5.15 compatibility"
key-files:
  created:
    - .github/workflows/ci-patched-qt.yml
  modified: []
key-decisions:
  - decision: "Push-to-main only trigger"
    reason: "Cold-cache Qt builds take 30-60 min; avoid on PRs to save CI minutes"
  - decision: "windows-2022 not windows-latest"
    reason: "Qt 5.15.1 requires MSVC v142 toolset which is guaranteed on windows-2022"
  - decision: "Artifact name qt5.15-patched suffix"
    reason: "Distinguish patched builds from standard ci.yml qt5.15 artifacts"
  - decision: "Install dir uses qt5.15 not qt5.15-patched"
    reason: "CMake install layout derives from Qt version, not patch status"
duration: "3 minutes"
completed: 2026-02-02
---

# Phase 10 Plan 02: Patched Qt CI Workflow Summary

CI workflow that builds patched Qt 5.15.1 from source via composite action, then builds/tests/installs the probe on Linux and Windows with 120-min timeout and cache-aware summary reporting.

## Performance

- Tasks: 1/1
- Duration: ~3 minutes
- Commits: 1

## Accomplishments

1. Created `ci-patched-qt.yml` with 2-cell matrix (Linux GCC on ubuntu-22.04, Windows MSVC on windows-2022)
2. Wired composite action from plan 10-01 with qt-version and patches-dir inputs
3. MSVC dev environment set up before composite action (required prerequisite)
4. Aligned vcpkg settings with standard CI (same commit ID, extras feature)
5. Added install layout verification for both platforms
6. Artifact upload with `qt5.15-patched` naming to distinguish from standard CI
7. Cache status reporting in GitHub Step Summary

## Task Commits

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 | Write the patched Qt CI workflow | 103ec74 | .github/workflows/ci-patched-qt.yml |

## Files Created

- `.github/workflows/ci-patched-qt.yml` -- 140-line workflow with 14 steps per matrix cell

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| Push-to-main only (no PR trigger) | Cold Qt builds take 30-60 min; save CI minutes |
| windows-2022 pinned (not windows-latest) | MSVC v142 toolset guaranteed for Qt 5.15.1 |
| Artifact suffix qt5.15-patched | Distinguish from standard ci.yml qt5.15 artifacts |
| Install dir uses qt5.15 (not patched) | CMake install layout derives from Qt version |

## Deviations from Plan

None -- plan executed exactly as written.

## Issues Encountered

None.

## Next Phase Readiness

Phase 10 is now complete (both plans done):
- Plan 10-01: Composite action for building/caching patched Qt
- Plan 10-02: CI workflow consuming that action

The patched Qt CI pipeline is fully defined. It will run on first push to main that touches any of the path-filtered files. The first run will be a cold-cache build (30-60 min); subsequent runs restore from cache unless patches change.
