---
phase: 11-release-automation
plan: 01
title: "Reusable CI Triggers"
subsystem: ci
tags: [github-actions, workflow_call, reusable-workflows]
dependency-graph:
  requires: [09-01, 10-02]
  provides: ["ci.yml callable", "ci-patched-qt.yml callable"]
  affects: [11-02]
tech-stack:
  added: []
  patterns: ["reusable workflows via workflow_call"]
key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/ci-patched-qt.yml
decisions: []
metrics:
  duration: "~2 minutes"
  completed: "2026-02-02"
---

# Phase 11 Plan 01: Reusable CI Triggers Summary

**One-liner:** Added workflow_call triggers to both CI workflows enabling reuse by release workflow

## What Was Done

Added `workflow_call:` trigger to the `on:` block of both CI workflow files:

1. **ci.yml** - Now has 4 triggers: push, pull_request, workflow_dispatch, workflow_call
2. **ci-patched-qt.yml** - Now has 3 triggers: push, workflow_dispatch, workflow_call

This enables the release workflow (plan 02) to invoke both CI workflows as reusable workflows, avoiding duplication of build logic while producing all 10 artifacts.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Add workflow_call trigger to ci.yml | 4b4c545 | .github/workflows/ci.yml |
| 2 | Add workflow_call trigger to ci-patched-qt.yml | 337cc41 | .github/workflows/ci-patched-qt.yml |

## Verification Results

- Both files pass YAML validation (python yaml.safe_load)
- `grep -c workflow_call` returns 1 for each file
- git diff shows exactly 2 insertions, 0 deletions -- no other lines changed

## Deviations from Plan

None - plan executed exactly as written.

## Next Phase Readiness

Plan 11-02 (release workflow) can now call both CI workflows via `uses: ./.github/workflows/ci.yml` and `uses: ./.github/workflows/ci-patched-qt.yml` with the workflow_call trigger.
