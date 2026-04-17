---
name: test-mcp-modes
description: Comprehensive end-to-end test of the qtPilot MCP server across all three modes (native, chrome, computer_use), plus logging and recording subsystems, using the test app. Use this skill whenever the user asks to test the MCP server, verify modes work, run a smoke test, check that qtPilot tools are functioning after a build or change, or wants to validate logging/recording functionality.
---

# Test qtPilot MCP Server — Full E2E Suite

Comprehensive end-to-end test of the qtPilot MCP server covering all three interaction modes (native, chrome, computer_use), the message logging subsystem, and the event recording subsystem.

## Prerequisites

The project must be built (`cmake --build build --config Release`). The MCP server `qtpilot` must be configured in `.mcp.json`.

## Procedure

### Phase 1: Launch, Connect, and Start Logging

1. **Find the Qt directory** from `build/CMakeCache.txt` (`QTPILOT_QT_DIR:PATH=...`).
2. **Launch the test app** in the background with Qt DLLs on PATH:
   ```
   QT_DIR=<from cache>
   cmd //c "set PATH=<QT_DIR>\bin;%PATH% && set QT_PLUGIN_PATH=<QT_DIR>\plugins && <abs_path>/build/bin/Release/qtPilot-launcher.exe <abs_path>/build/bin/Release/qtPilot-test-app.exe"
   ```
   Use `run_in_background: true`. Wait 4 seconds for startup.
3. **Discover and connect:**
   - `qtpilot_list_probes()` — expect 1 probe with app_name "qtPilot Test App"
   - `qtpilot_connect_probe(ws_url=...)` — connect using the URL from list
   - `qt_ping()` — expect `pong: true`

Record: probe PID, Qt version, WebSocket URL.

4. **Start verbose logging** to capture all traffic during the test:
   - `qtpilot_log_start(level=3)` — expect `logging: true`, a file path, and `level: 3`
   - Record the log file path for later verification.

5. **Check logging status:**
   - `qtpilot_log_status()` — expect `logging: true`, correct path and level

### Phase 2: Native Mode Tests (qt_* tools)

Verify mode: `qtpilot_get_mode()` — expect `"native"` (default).

Run these tests, recording pass/fail for each:

1. **Object discovery:**
   - `qt_objects_findByClass(className="QLineEdit")` — expect 2 results with hierarchical objectIds containing `nameEdit` and `emailEdit`
   - `qt_objects_findByClass(className="QPushButton")` — expect 3+ results including `submitButton`, `clearButton`
   - `qt_objects_find(name="nameEdit")` — expect 1 result

2. **Properties:**
   - `qt_properties_get(objectId="...", name="placeholderText")` on the nameEdit — expect "Enter your name"
   - `qt_properties_get(objectId="...", name="text")` on submitButton — expect "Submit"

3. **UI interaction:**
   - `qt_ui_click(objectId=<nameEdit>)` — expect success
   - `qt_ui_sendKeys(objectId=<nameEdit>, text="Test User")` — expect success
   - `qt_ui_click(objectId=<emailEdit>)` — expect success
   - `qt_ui_sendKeys(objectId=<emailEdit>, text="test@example.com")` — expect success
   - `qt_properties_get(objectId=<nameEdit>, name="text")` — expect "Test User"

4. **Screenshot:**
   - `qt_ui_screenshot(objectId="MainWindow", fullWindow=true)` — expect base64 image data

5. **Object tree:**
   - `qt_objects_tree(maxDepth=3)` — expect nested structure with className and children

6. **Submit and clear:**
   - `qt_ui_click(objectId=<submitButton>)` — expect success
   - `qt_ui_click(objectId=<clearButton>)` — expect success
   - `qt_properties_get(objectId=<nameEdit>, name="text")` — expect "" (empty after clear)

7. **Methods:**
   - `qt_methods_list(objectId=<nameEdit>)` — expect a list of invocable methods including `clear`, `selectAll`
   - `qt_ui_sendKeys(objectId=<nameEdit>, text="Method Test")` — fill a value first
   - `qt_methods_invoke(objectId=<nameEdit>, method="clear")` — expect success
   - `qt_properties_get(objectId=<nameEdit>, name="text")` — expect "" (cleared by method)

8. **Signals:**
   - `qt_signals_list(objectId=<nameEdit>)` — expect signals including `textChanged`, `textEdited`
   - `qt_signals_subscribe(objectId=<nameEdit>, signal="textChanged")` — expect a subscriptionId
   - `qt_ui_sendKeys(objectId=<nameEdit>, text="Sig")` — trigger the signal
   - `qt_signals_unsubscribe(subscriptionId=<from above>)` — expect success

9. **Named objects (symbolic names):**
   - `qt_names_register(name="myNameField", path=<nameEdit objectId>)` — expect success
   - `qt_names_list()` — expect entry with "myNameField"
   - `qt_names_validate()` — expect all valid
   - `qt_objects_find(name="nameEdit")` using the registered name — verify it resolves
   - `qt_names_unregister(name="myNameField")` — expect success

10. **Models (table data, flat model):**
    - `qt_models_list()` — expect at least 2 models (tableWidget's internal model, treeView's model)
    - Pick the tableWidget model objectId from the list
    - `qt_models_info(objectId=<tableModel>)` — expect rows >= 3, columns >= 3
    - `qt_models_data(objectId=<tableModel>)` — expect a rows array with display cells; the first row should contain "Alpha"

11. **Tree navigation (qt_models_data with parent, qt_models_find, qt_ui_clickItem):**
    - Switch to the Tree tab: `qt_ui_click(objectId="treeTab")` — expect success
    - `qt_models_data(objectId="treeView")` — expect top-level rows (ETC, Martin, BulkManufacturer), each with `hasChildren=true`, each row having a `path` field
    - `qt_models_data(objectId="treeView", parent=[0])` — expect ETC's children (fos4..., ColorSource PAR), `path` starts with `[0, ...]`
    - `qt_models_data(objectId="treeView", parent=[0, 0])` — expect fos4's children (Mode 8ch, Mode 12ch)
    - `qt_models_data(objectId="treeView", parent=[99])` — expect error `kInvalidParentPath` with `failedSegment=0`
    - `qt_models_find(objectId="treeView", value="Aura", match="contains")` — expect at least 1 match with `path=[1, 0]` (under Martin)
    - `qt_models_find(objectId="treeView", value="Fixture 0500", match="exact", parent=[2])` — expect 1 match inside the BulkManufacturer subtree
    - `qt_ui_clickItem(objectId="treeView", itemPath=["Martin","MAC Aura XB","Mode 12ch"], action="select")` — expect `found=true`, `path=[1,0,1]`, ancestors auto-expanded
    - `qt_ui_clickItem(objectId="treeView", path=[0, 0], action="select")` — dual-addressing proof; expect `found=true`
    - Pagination: `qt_models_data(objectId="treeView", parent=[2], offset=1100, limit=100)` — expect 100 rows, `hasMore=false`, first row path `[2, 1100]`

12. **Hit test and geometry:**
    - `qt_ui_geometry(objectId=<submitButton>)` — expect x, y, width, height values
    - `qt_ui_hitTest(x=<center_x>, y=<center_y>)` using submitButton center — expect result identifying the submitButton

13. **Events:**
    - `qt_events_startCapture()` — expect success
    - `qt_ui_click(objectId=<submitButton>)` — generate some events
    - `qt_events_stopCapture()` — expect captured events list with entries

14. **QML inspect** (may return empty if no QML items, but should not error):
    - `qt_qml_inspect(objectId=<nameEdit>)` — expect a result (possibly indicating not a QML item)

### Phase 3: Recording Tests

Test the event recording subsystem before switching modes.

1. **Start recording:**
   - `qtpilot_start_recording(targets=[<nameEdit objectId>])` — expect `recording: true`
   - `qtpilot_recording_status()` — expect `recording: true` with target info

2. **Generate recorded events:**
   - `qt_ui_click(objectId=<nameEdit>)` — click to focus
   - `qt_ui_sendKeys(objectId=<nameEdit>, text="Record Test")` — type to generate signals/events
   - `qt_ui_click(objectId=<clearButton>)` — click clear

3. **Stop recording and verify:**
   - `qtpilot_stop_recording()` — expect `recording: false` with an event list
   - Verify the event list contains entries (timestamps, event types, object IDs)

### Phase 4: Chrome Mode Tests (chr_* tools)

1. **Switch mode:** `qtpilot_set_mode(mode="chrome")` — expect `changed: true`
2. **Verify:** `qtpilot_get_mode()` — expect `"chrome"`

Run these tests:

3. **Page reading:**
   - `chr_readPage()` — expect accessibility tree with roles, names, refs
   - `chr_getPageText()` — expect visible text including labels and button text

4. **Find elements:**
   - `chr_find(query="Submit")` — expect at least 1 result with a ref

5. **Form interaction:**
   - Find a text input ref from `chr_readPage()`
   - `chr_formInput(ref=<input_ref>, value="Chrome Test")` — expect success
   - `chr_click(ref=<submit_ref>)` — expect success

6. **Tab context:**
   - `chr_tabsContext()` — expect window/tab info

7. **Console messages:**
   - `chr_readConsoleMessages()` — expect a result (may be empty list, but should not error)

8. **Navigate:**
   - Find a tab ref from `chr_readPage()`
   - `chr_navigate(ref=<tab_ref>, action="activateTab")` — expect success (switches tab)

### Phase 5: Computer Use Mode Tests (cu_* tools)

1. **Switch mode:** `qtpilot_set_mode(mode="cu")` — expect `changed: true`
2. **Verify:** `qtpilot_get_mode()` — expect `"cu"`

Run these tests:

3. **Screenshot:**
   - `cu_screenshot()` — expect base64 image data with width/height

4. **Cursor:**
   - `cu_cursorPosition()` — expect x, y coordinates

5. **Click and type** (use coordinates from the screenshot or known widget positions):
   - First switch briefly to all mode: `qtpilot_set_mode(mode="all")`
   - `qt_ui_geometry(objectId=<nameEdit>)` — get x, y, width, height
   - Switch back: `qtpilot_set_mode(mode="cu")`
   - `cu_leftClick(x=<center_x>, y=<center_y>)` — expect success
   - `cu_type(text="CU Test")` — expect success
   - `cu_key(key="Tab")` — expect success (moves to next field)

6. **Mouse operations:**
   - `cu_mouseMove(x=100, y=100)` — expect success
   - `cu_cursorPosition()` — expect x ~100, y ~100
   - `cu_rightClick(x=<center_x>, y=<center_y>)` — expect success (may open context menu)
   - `cu_doubleClick(x=<nameEdit center_x>, y=<nameEdit center_y>)` — expect success (selects word)

7. **Scroll:**
   - `cu_scroll(x=200, y=200, direction="down", amount=3)` — expect success

8. **Key combos:**
   - `cu_key(key="Ctrl+a")` — select all
   - `cu_key(key="Delete")` — delete selection
   - `cu_type(text="Final CU Test")` — type replacement text

### Phase 6: Logging Verification

1. **Tail recent log entries:**
   - `qtpilot_log_tail(count=20)` — expect entries with `dir` field (`mcp_in`, `mcp_out`, `req`, `res`)
   - Verify entries include tool calls from the preceding test phases
   - Verify entries have timestamps

2. **Stop logging:**
   - `qtpilot_log_stop()` — expect `logging: false`, a file path, `entries` count > 0, and `duration` > 0

3. **Verify log file on disk:**
   - Read the first and last few lines of the log file (path from qtpilot_log_stop)
   - Verify it is valid JSONL (one JSON object per line)
   - Verify it contains `mcp_in`, `req`, `res`, `mcp_out` entry types

### Phase 7: Cleanup

1. **Switch back to native mode:** `qtpilot_set_mode(mode="native")`
2. **Close the app:** `qt_methods_invoke(objectId="QApplication", method="quit")`
   - Expect WebSocket closed error (normal — app exits)
3. Wait for background launcher process to exit.
4. **Clean up log file:** Delete the test log file (path from Phase 6).

1. **Switch back to native mode:** `qtpilot_set_mode(mode="native")`
2. **Close the app:** `qt_methods_invoke(objectId="QApplication", method="quit")`
   - Expect WebSocket closed error (normal — app exits)
3. Wait for background launcher process to exit.

### Phase 8: Report

Print a summary table:

```
## qtPilot MCP Full E2E Test Results

| Phase | Test | Result |
|-------|------|--------|
| Connect | list_probes | PASS/FAIL |
| Connect | connect_probe | PASS/FAIL |
| Connect | ping | PASS/FAIL |
| Connect | log_start | PASS/FAIL |
| Connect | log_status | PASS/FAIL |
| Native | object_discovery | PASS/FAIL |
| Native | properties | PASS/FAIL |
| Native | ui_interaction | PASS/FAIL |
| Native | screenshot | PASS/FAIL |
| Native | object_tree | PASS/FAIL |
| Native | submit_and_clear | PASS/FAIL |
| Native | methods | PASS/FAIL |
| Native | signals | PASS/FAIL |
| Native | named_objects | PASS/FAIL |
| Native | models_flat_read | PASS/FAIL |
| Native | models_tree_read | PASS/FAIL |
| Native | models_find | PASS/FAIL |
| Native | ui_clickItem | PASS/FAIL |
| Native | hit_test_geometry | PASS/FAIL |
| Native | events | PASS/FAIL |
| Native | qml_inspect | PASS/FAIL |
| Recording | start_recording | PASS/FAIL |
| Recording | generate_events | PASS/FAIL |
| Recording | stop_and_verify | PASS/FAIL |
| Chrome | switch_mode | PASS/FAIL |
| Chrome | read_page | PASS/FAIL |
| Chrome | find_elements | PASS/FAIL |
| Chrome | form_interaction | PASS/FAIL |
| Chrome | tabs_context | PASS/FAIL |
| Chrome | console_messages | PASS/FAIL |
| Chrome | navigate | PASS/FAIL |
| CU | switch_mode | PASS/FAIL |
| CU | screenshot | PASS/FAIL |
| CU | cursor_position | PASS/FAIL |
| CU | click_and_type | PASS/FAIL |
| CU | mouse_operations | PASS/FAIL |
| CU | scroll | PASS/FAIL |
| CU | key_combos | PASS/FAIL |
| Logging | log_tail | PASS/FAIL |
| Logging | log_stop | PASS/FAIL |
| Logging | log_file_verify | PASS/FAIL |
| Cleanup | quit_app | PASS/FAIL |
```

Mark a test as PASS if the tool returned the expected result type without error. Mark FAIL with a brief reason.

Report total: **X/42 tests passed**.

## Error Handling

- If the test app fails to launch (e.g., Qt DLLs not found), report the launcher output and stop.
- If a mode switch fails, skip that mode's tests and continue to the next.
- If an individual test fails, record FAIL and continue — don't abort the whole suite.
- Always attempt cleanup (Phase 5) even if earlier phases failed.
