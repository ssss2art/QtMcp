---
phase: 04-computer-use-mode
plan: 02
subsystem: api
tags: [computer-use, json-rpc, screenshot, input-simulation, coordinate-resolution]

# Dependency graph
requires:
  - phase: 04-computer-use-mode/01
    provides: "InputSimulator extensions, Screenshot extensions, KeyNameMapper, HitTest"
  - phase: 03-native-mode
    provides: "NativeModeApi pattern, ResponseEnvelope, ErrorCodes, JsonRpcHandler"
provides:
  - "ComputerUseModeApi class with 13 cu.* JSON-RPC methods"
  - "CU-specific error codes (kNoActiveWindow, kCoordinateOutOfBounds, kNoFocusedWidget, kKeyParseError)"
  - "Coordinate resolution helpers (window-relative and screen-absolute)"
affects: [04-computer-use-mode/03, 05-chrome-mode]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Coordinate-based API surface (no objectId needed)"
    - "Active window fallback to first visible top-level widget"
    - "include_screenshot optional param on all action methods"

key-files:
  created:
    - src/probe/api/computer_use_mode_api.h
    - src/probe/api/computer_use_mode_api.cpp
  modified:
    - src/probe/api/error_codes.h
    - src/probe/core/probe.cpp
    - src/probe/CMakeLists.txt

key-decisions:
  - "Window-relative coordinates by default, screenAbsolute=true for screen coords"
  - "Active window fallback searches visible top-level widgets"
  - "delay_ms param on click methods for UI settle time"

patterns-established:
  - "cu.* namespace for computer use mode methods"
  - "ResolvedTarget struct for coordinate-to-widget resolution"
  - "maybeAddScreenshot helper for optional screenshot in response"

# Metrics
duration: 7min
completed: 2026-01-31
---

# Phase 4 Plan 2: ComputerUseModeApi Summary

**13 cu.* JSON-RPC methods for coordinate-based screen interaction with screenshot/click/type/scroll/cursor primitives**

## Performance

- **Duration:** 7 min
- **Started:** 2026-02-01T02:18:13Z
- **Completed:** 2026-02-01T02:25:13Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- ComputerUseModeApi class registering all 13 cu.* methods on JsonRpcHandler
- Coordinate resolution supporting window-relative and screen-absolute modes
- 4 new CU-specific error codes in -32060 to -32063 range
- Both qt.* and cu.* APIs active simultaneously via Probe::initialize()

## Task Commits

Each task was committed atomically:

1. **Task 1: Create ComputerUseModeApi with all 13 cu.* methods** - `08ea62f` (feat)
2. **Task 2: Wire ComputerUseModeApi into Probe and update CMake** - `98a0e89` (feat)

## Files Created/Modified
- `src/probe/api/computer_use_mode_api.h` - ComputerUseModeApi class declaration
- `src/probe/api/computer_use_mode_api.cpp` - All 13 cu.* method handlers (600+ lines)
- `src/probe/api/error_codes.h` - Added 4 CU error codes
- `src/probe/core/probe.cpp` - ComputerUseModeApi wiring in initialize()
- `src/probe/CMakeLists.txt` - Added computer_use_mode_api and key_name_mapper files

## Decisions Made
- Window-relative coordinates by default (screenAbsolute=true for screen coords) - matches Chrome CU convention
- Active window fallback to first visible top-level widget - handles apps without explicit active window
- delay_ms param on click methods allows UI settle time before action
- imageWithDimensions helper decodes PNG to get width/height for cu.screenshot response

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All 13 cu.* methods registered and callable via WebSocket
- Ready for Phase 4 Plan 3 (Testing) to verify method behavior
- Both native and computer use APIs coexist on same JsonRpcHandler

---
*Phase: 04-computer-use-mode*
*Completed: 2026-01-31*
