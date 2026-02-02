---
phase: 10-patched-qt-ci
plan: 01
subsystem: ci
tags: [github-actions, composite-action, qt-build, cache, patches]
requires: []
provides:
  - Reusable composite action for building patched Qt from source
  - Patch directory scaffold with convention documentation
affects:
  - 10-02 (CI workflow will call the composite action)
  - 11 (release workflow will reuse the composite action)
tech-stack:
  added: [actions/cache@v4, jom]
  patterns: [composite-action, cache-key-design, platform-conditional-steps]
key-files:
  created:
    - .github/actions/build-qt/action.yml
    - .ci/patches/5.15.1/README.md
    - .ci/patches/5.15.1/.gitkeep
  modified: []
key-decisions:
  - Cache key includes patches hash + OS + Qt version + cfgv1 constant (no restore-keys)
  - git apply for cross-platform patch application (not POSIX patch)
  - Windows uses cmd shell for configure.bat, bash for everything else
  - Install prefix cached (not build tree) to keep cache under 1 GB
duration: 3m
completed: 2026-02-02
---

# Phase 10 Plan 01: Build-Qt Composite Action Summary

Reusable composite action that downloads, patches, configures, builds, installs, and caches Qt from source for both Linux and Windows.

## Performance

- Duration: ~3 minutes
- Tasks: 2/2
- Commits: 2

## Accomplishments

1. Created patch directory scaffold at `.ci/patches/5.15.1/` with naming convention docs, cache invalidation explanation, and `.gitkeep` for empty directory tracking.

2. Created the composite action at `.github/actions/build-qt/action.yml` with 14 steps covering the full build pipeline: path setup, cache restore, source download (tar.xz on Linux, zip on Windows), patch application via `git apply`, Linux dependency installation, jom download for Windows, platform-specific configure (bash on Linux, cmd on Windows), parallel build, install, private header verification, and output.

## Task Commits

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 | Create patch directory scaffold | b3918d4 | .ci/patches/5.15.1/README.md, .gitkeep |
| 2 | Create composite action | c378d77 | .github/actions/build-qt/action.yml |

## Files Created

- `.github/actions/build-qt/action.yml` - 259-line composite action
- `.ci/patches/5.15.1/README.md` - Patch convention documentation
- `.ci/patches/5.15.1/.gitkeep` - Directory tracker

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| No restore-keys in cache | Partial match would restore unpatched/wrong-patched Qt |
| cfgv1 constant in cache key | Bump to cfgv2 when configure flags change, avoids hashing entire action.yml |
| git apply (not POSIX patch) | Cross-platform: works identically on Linux and Windows via Git |
| cmd shell for Windows configure | configure.bat requires native cmd with MSVC environment vars |
| Separate install steps per OS | make install (Linux) vs jom install (Windows) |
| MSVC setup is caller responsibility | Composite actions cannot run uses steps conditionally; caller uses ilammy/msvc-dev-cmd |

## Deviations from Plan

None -- plan executed exactly as written.

## Issues Encountered

None.

## Next Phase Readiness

Plan 10-02 can now call the composite action to build patched Qt in a CI workflow. The action interface is:
- Inputs: `qt-version`, `patches-dir`, `install-prefix`
- Outputs: `qt-dir`, `cache-hit`
- Prerequisite: MSVC dev environment on Windows (ilammy/msvc-dev-cmd@v1 with toolset 14.29)
