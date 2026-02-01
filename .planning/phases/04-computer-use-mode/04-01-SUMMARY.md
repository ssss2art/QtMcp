---
phase: 04-computer-use-mode
plan: 01
subsystem: interaction
tags: [qt-key-mapping, mouse-simulation, screenshot, qwheelevent, hidpi, chrome-compat]

# Dependency graph
requires:
  - phase: 02-core-introspection
    provides: "InputSimulator, Screenshot base classes"
provides:
  - "KeyNameMapper: Chrome/xdotool key name to Qt::Key translation"
  - "InputSimulator: mousePress, mouseRelease, mouseMove, scroll, mouseDrag"
  - "Screenshot: captureScreen (full screen), captureWindowLogical (HiDPI-aware)"
affects: [04-02-computer-use-mode-api, 04-03-computer-use-mode-testing]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Manual QMouseEvent/QWheelEvent construction for operations QTest cannot handle"
    - "Case-insensitive QHash lookup with toLower() keys for key name resolution"

key-files:
  created:
    - src/probe/interaction/key_name_mapper.h
    - src/probe/interaction/key_name_mapper.cpp
  modified:
    - src/probe/interaction/input_simulator.h
    - src/probe/interaction/input_simulator.cpp
    - src/probe/interaction/screenshot.h
    - src/probe/interaction/screenshot.cpp

key-decisions:
  - "Both Chrome and xdotool key name aliases map to same Qt::Key (e.g. Enter/Return both to Key_Return)"
  - "Manual QMouseEvent for new mouse ops, QTest kept for existing click/doubleClick"
  - "Scroll uses 120 units per tick standard (QWheelEvent angleDelta convention)"
  - "captureWindowLogical scales by devicePixelRatio for 1:1 coordinate matching"

patterns-established:
  - "KeyNameMapper: static QHash initialized once via static local for O(1) lookup"
  - "Extended InputSimulator: new methods use sendEvent pattern, existing use QTest"

# Metrics
duration: 3min
completed: 2026-01-31
---

# Phase 4 Plan 01: Interaction Layer Primitives Summary

**Chrome-to-Qt key name mapper, extended mouse primitives (press/release/move/scroll/drag), and full-screen/logical-pixel screenshot capture**

## Performance

- **Duration:** 3 min
- **Started:** 2026-02-01T02:10:37Z
- **Completed:** 2026-02-01T02:13:34Z
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments
- KeyNameMapper resolves Chrome/xdotool key names to Qt::Key with case-insensitive O(1) lookup
- InputSimulator extended with 5 new mouse methods using manual QMouseEvent/QWheelEvent (not QTest)
- Screenshot extended with full-screen capture and HiDPI-aware logical-pixel window capture

## Task Commits

Each task was committed atomically:

1. **Task 1: Create KeyNameMapper utility class** - `1e2dcbf` (feat)
2. **Task 2: Extend InputSimulator with mouse press/release/move, scroll, drag** - `062763f` (feat)
3. **Task 3: Extend Screenshot with full-screen capture and logical-pixel scaling** - `3945b42` (feat)

## Files Created/Modified
- `src/probe/interaction/key_name_mapper.h` - Chrome/xdotool key name to Qt::Key mapper interface
- `src/probe/interaction/key_name_mapper.cpp` - Static lookup table (60+ keys), modifier map, combo parser
- `src/probe/interaction/input_simulator.h` - Added mousePress, mouseRelease, mouseMove, scroll, mouseDrag declarations
- `src/probe/interaction/input_simulator.cpp` - Manual QMouseEvent/QWheelEvent implementations for all new methods
- `src/probe/interaction/screenshot.h` - Added captureScreen and captureWindowLogical declarations
- `src/probe/interaction/screenshot.cpp` - Full-screen grab and DPI-aware window capture implementations

## Decisions Made
- Both "Enter" and "Return" map to Qt::Key_Return (main keyboard Enter, not numpad)
- Single-char keys (a-z, 0-9, punctuation) resolved via direct ASCII-to-Qt::Key mapping
- New mouse methods use manual QMouseEvent construction; existing methods keep QTest for stability
- mouseDrag resolves child widgets at start/end independently via window->childAt()
- captureWindowLogical uses qRound for integer pixel dimensions after DPI scaling

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- All interaction-layer primitives ready for ComputerUseModeApi (Plan 02)
- KeyNameMapper provides parseKeyCombo() for cu.key method
- InputSimulator provides all mouse operations for cu.click, cu.drag, cu.scroll, cu.mouseDown, cu.mouseUp
- Screenshot provides captureScreen/captureWindowLogical for cu.screenshot
- Build verification deferred to Plan 02 (CMakeLists.txt update needed for new files)

---
*Phase: 04-computer-use-mode*
*Completed: 2026-01-31*
