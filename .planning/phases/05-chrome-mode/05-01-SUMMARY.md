---
phase: 05-chrome-mode
plan: 01
subsystem: accessibility
tags: [accessibility, role-mapping, console-capture, tree-walker, chrome-mode]

dependency_graph:
  requires: [02-01, 03-01]
  provides: [RoleMapper, ConsoleMessageCapture, AccessibilityTreeWalker, ChromeModeErrorCodes]
  affects: [05-02, 05-03]

tech_stack:
  added: []
  patterns: [static-utility-class, Q_GLOBAL_STATIC-singleton, ring-buffer, recursive-tree-walk]

file_tracking:
  key_files:
    created:
      - src/probe/accessibility/role_mapper.h
      - src/probe/accessibility/role_mapper.cpp
      - src/probe/accessibility/console_message_capture.h
      - src/probe/accessibility/console_message_capture.cpp
      - src/probe/accessibility/accessibility_tree_walker.h
      - src/probe/accessibility/accessibility_tree_walker.cpp
    modified:
      - src/probe/api/error_codes.h

decisions:
  - id: static-utility-for-role-mapper
    decision: "RoleMapper is purely static (no QObject, no singleton)"
    rationale: "Stateless lookup table needs no instance management"
  - id: q-global-static-for-console-capture
    decision: "ConsoleMessageCapture uses Q_GLOBAL_STATIC singleton"
    rationale: "Consistent with ObjectRegistry and SignalMonitor patterns"
  - id: generic-fallback-for-unknown-roles
    decision: "Unknown QAccessible roles map to 'generic'"
    rationale: "Matches Chrome/ARIA convention for unmapped roles"

metrics:
  duration: 4 min
  completed: 2026-01-31
---

# Phase 05 Plan 01: Chrome Mode Infrastructure Classes Summary

**One-liner:** RoleMapper (55 Qt-to-Chrome role mappings), ConsoleMessageCapture (ring buffer message handler), and AccessibilityTreeWalker (recursive JSON tree with ref_N identifiers)

## What Was Done

### Task 1: RoleMapper, ConsoleMessageCapture, and Error Codes
- **RoleMapper** (`role_mapper.h/cpp`): Static utility class mapping ~55 `QAccessible::Role` values to Chrome/ARIA role name strings via file-scope `QHash`. `toChromeName()` returns the mapped string (fallback: "generic"). `isInteractive()` checks against a `QSet` of 18 interactive roles (Button, CheckBox, RadioButton, ComboBox, SpinBox, Slider, Dial, EditableText, Link, MenuItem, PageTab, ListItem, TreeItem, Cell, ButtonMenu, ButtonDropDown, ScrollBar, HotkeyField).
- **ConsoleMessageCapture** (`console_message_capture.h/cpp`): Q_GLOBAL_STATIC singleton that installs a Qt message handler via `qInstallMessageHandler()`. Records messages into a QMutex-guarded ring buffer (MAX_MESSAGES=1000). Chains to previous handler so messages are never swallowed. `messages()` supports regex pattern filtering and error-only filtering, returning newest first. `ConsoleMessage` struct captures type, message, file, line, function, timestamp.
- **Error codes**: Added 7 Chrome Mode error codes (-32070 to -32076) to `error_codes.h`: kRefNotFound, kRefStale, kFormInputUnsupported, kTreeTooLarge, kFindTooManyResults, kNavigateInvalid, kConsoleNotAvailable.

### Task 2: AccessibilityTreeWalker
- **AccessibilityTreeWalker** (`accessibility_tree_walker.h/cpp`): Static utility class with `walk(QWidget*, WalkOptions)` returning `WalkResult`. Calls `QAccessible::setActive(true)` to ensure accessibility initialization. Recursive `walkNode()` builds nested QJsonObject tree with:
  - `ref`: ref_1, ref_2, ... (all elements in "all" mode, interactive-only in "interactive" mode)
  - `role`: Chrome name via RoleMapper::toChromeName()
  - `name`: fallback chain QAccessible::Name -> objectName() -> className()
  - `states`: focused, disabled, checked, expanded/collapsed, selected, readonly, pressed, hasPopup, modal, editable, multiline, password (only truthy values)
  - `bounds`: {x, y, width, height} from iface->rect()
  - Qt extras: objectName, className, objectId (from ObjectRegistry)
  - `children`: array of child nodes (only if non-empty)
- Respects maxDepth (default 15), maxChars (default 50000) with truncation tracking
- WalkResult includes refMap (ref string to QAccessibleInterface*), totalNodes, truncated flag

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| RoleMapper as purely static class | Stateless lookup table needs no instance management |
| Q_GLOBAL_STATIC for ConsoleMessageCapture | Consistent with existing singleton patterns |
| "generic" fallback for unknown roles | Chrome/ARIA convention for unmapped roles |
| Name fallback: accessible name -> objectName -> className | Prevents empty-name nodes in tree output |
| Expanded/collapsed as expanded:true/false | Matches Chrome state representation |
| Structural parents included without refs in "interactive" mode | Provides context for interactive elements |

## Deviations from Plan

None - plan executed exactly as written.

## Verification

- [x] All 6 new files exist in src/probe/accessibility/
- [x] error_codes.h contains 7 new Chrome Mode error codes (-32070 to -32076)
- [x] RoleMapper covers ~55 roles from research mapping table
- [x] ConsoleMessageCapture chains to previous handler (never swallows)
- [x] AccessibilityTreeWalker produces nested JSON with ref assignments
- [x] Build verification deferred to Plan 05-02 (CMakeLists.txt update)

## Commits

| Commit | Description |
|--------|-------------|
| f9606e6 | feat(05-01): add RoleMapper, ConsoleMessageCapture, and Chrome Mode error codes |
| 7fbd882 | feat(05-01): add AccessibilityTreeWalker with ref assignment and JSON output |

## Next Phase Readiness

Plan 05-02 needs to:
1. Update CMakeLists.txt to include the new accessibility/ source files
2. Build and verify compilation
3. The 3 infrastructure classes are ready for ChromeModeApi to consume
