---
status: complete
phase: 04-computer-use-mode
source: 04-01-SUMMARY.md, 04-02-SUMMARY.md, 04-03-SUMMARY.md
started: 2026-01-31T21:00:00Z
updated: 2026-01-31T21:30:00Z
---

## Current Test

[testing complete]

## Tests

### 1. Take Screenshot
expected: Send cu.screenshot via WebSocket. Response contains result.image (base64 PNG), result.width and result.height as integers.
result: pass

### 2. Click a Button
expected: Send cu.click with x,y coordinates targeting a visible button in the test app. The button's clicked signal fires (observable effect like text change or state toggle). Response contains result.success: true.
result: pass

### 3. Type Text into Input Field
expected: Click a QLineEdit to focus it, then send cu.type with text "hello world". The text appears in the input field. Response contains result.success: true.
result: pass

### 4. Send Key Combination
expected: With text in a QLineEdit, send cu.key with key "ctrl+a" then cu.key with key "Delete". The text is selected then deleted, leaving the field empty.
result: pass

### 5. Right-Click
expected: Send cu.rightClick with x,y coordinates on a widget. Response contains result.success: true (context menu may appear depending on widget).
result: pass

### 6. Double-Click
expected: Send cu.doubleClick with x,y on a QLineEdit containing text. The word under the cursor gets selected.
result: pass

### 7. Mouse Drag
expected: Send cu.drag with startX, startY, endX, endY coordinates. The drag operation executes (e.g., moving a slider or selecting text). Response contains result.success: true.
result: pass

### 8. Scroll
expected: Send cu.scroll with x, y, direction ("down"), and amount (3). Response contains result.success: true. Scrollable content moves in the specified direction.
result: pass

### 9. Query Cursor Position
expected: Send cu.cursorPosition. Response contains result.x, result.y (integers) and result.className (string identifying the widget under cursor).
result: pass

### 10. Screenshot with Action
expected: Send cu.click with include_screenshot: true. Response contains both result.success: true AND result.screenshot (base64 PNG showing state after the click).
result: pass

## Summary

total: 10
passed: 10
issues: 3
pending: 0
skipped: 0

## Gaps

- truth: "qt.* methods (NativeModeApi) should be registered alongside cu.* methods"
  status: failed
  reason: "All qt.* methods return -32601 Method not found. Only legacy qtmcp.* and cu.* methods are registered. NativeModeApi is not being wired into the probe at runtime despite being compiled."
  severity: major
  test: discovered during UAT setup
  root_cause: ""
  artifacts: []
  missing: []
  debug_session: ""

- truth: "qtmcp.* object IDs returned by findByClassName should be resolvable by getObjectInfo and getGeometry"
  status: failed
  reason: "qtmcp.findByClassName returns IDs like QObject~20, but qtmcp.getObjectInfo and qtmcp.getGeometry return 'Object not found' for those same IDs. The ID resolution path is broken for legacy methods."
  severity: major
  test: discovered during UAT setup
  root_cause: ""
  artifacts: []
  missing: []
  debug_session: ""

- truth: "cu.cursorPosition should report the widget under the coordinate used in the most recent cu.mouseMove, not the physical OS cursor"
  status: failed
  reason: "cu.cursorPosition uses QCursor::pos() which reads the physical system cursor. cu.mouseMove sends QMouseEvents but does not move the OS cursor. So cursorPosition always returns wherever the physical mouse is, not where CU interactions are happening. This makes cursorPosition useless for CU-mode workflows."
  severity: minor
  test: 9
  root_cause: ""
  artifacts: []
  missing: []
  debug_session: ""
