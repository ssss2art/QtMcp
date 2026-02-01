---
phase: 04-computer-use-mode
plan: 05
subsystem: computer-use-api
tags: [cursor-tracking, virtual-position, computer-use, gap-closure]
dependency-graph:
  requires: [04-01, 04-02]
  provides: [virtual-cursor-tracking, cu-cursorPosition-consistency]
  affects: []
tech-stack:
  added: []
  patterns: [file-scope-static-state, virtual-cursor-fallback]
key-files:
  created: []
  modified:
    - src/probe/api/computer_use_mode_api.cpp
decisions:
  - id: file-scope-static
    choice: "File-scope static for position tracking instead of class member"
    reason: "All cu.* lambdas are stateless (no this capture); consistent with existing pattern"
metrics:
  duration: 4 min
  completed: 2026-01-31
---

# Phase 04 Plan 05: Virtual Cursor Position Tracking Summary

**One-liner:** cu.cursorPosition now returns last simulated position from CU actions instead of physical OS cursor, with fallback to QCursor::pos() when no CU actions have occurred.

## What Was Done

### Task 1: Add virtual cursor position tracking to ComputerUseModeApi

Added virtual cursor position tracking so cu.cursorPosition reflects the simulated position from CU coordinate-based actions rather than always reading the physical OS cursor via QCursor::pos().

**Changes to `src/probe/api/computer_use_mode_api.cpp`:**

1. Added file-scope static variables in the anonymous namespace:
   - `s_lastSimulatedPosition` (QPoint) - stores the last position in screen-absolute coordinates
   - `s_hasSimulatedPosition` (bool) - tracks whether any CU action has set a position

2. Added `trackPosition()` helper that converts window-relative or screen-absolute coordinates to global coordinates and stores them.

3. Inserted `trackPosition()` calls in all 9 coordinate-based methods:
   - cu.click, cu.rightClick, cu.middleClick, cu.doubleClick
   - cu.mouseMove, cu.mouseDown, cu.mouseUp
   - cu.drag (tracks end position)
   - cu.scroll

4. Updated cu.cursorPosition to prefer virtual position when available, falling back to QCursor::pos() when no CU actions have been performed. Added `"virtual": true/false` field to response.

**Commit:** `00c3b0b`

## Deviations from Plan

None - plan executed exactly as written.

## Verification

- Build: `cmake --build build` succeeded with zero errors
- Tests: All 10/10 test suites passed (ctest -C Debug)
- cu.cursorPosition response is backward compatible (all existing fields preserved, "virtual" is additive)

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| File-scope static instead of class member | All cu.* lambdas are stateless (no `this` capture); using file-scope static avoids changing lambda capture semantics and is consistent with existing anonymous namespace helpers |
| Global (screen-absolute) coordinate storage | Consistent with QCursor::pos() return value; mapToGlobal at track time, mapFromGlobal at query time |
| "virtual" boolean in response | Informational field letting callers know whether position came from CU simulation or physical cursor |

## Next Phase Readiness

This closes the cu.cursorPosition UAT gap. The virtual cursor is now consistent with all CU coordinate-based actions. No blockers for future phases.
