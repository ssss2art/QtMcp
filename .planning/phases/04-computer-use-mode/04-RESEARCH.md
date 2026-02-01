# Phase 4: Computer Use Mode - Research

**Researched:** 2026-01-31
**Domain:** Chrome-compatible computer use API over JSON-RPC, wrapping existing Qt interaction primitives
**Confidence:** HIGH

## Summary

Phase 4 creates a `ComputerUseModeApi` class that registers `cu.*` JSON-RPC methods mimicking Anthropic's Claude Computer Use tool API. The existing Phase 2 classes (`InputSimulator`, `Screenshot`, `HitTest`) provide all low-level primitives. Phase 4's job is purely an API layer: accepting Chrome-style action parameters (coordinate arrays, xdotool-style key names), translating them to Qt calls, and returning Chrome-compatible responses.

The Claude Computer Use tool has three versions (`computer_20241022`, `computer_20250124`, `computer_20251124`) with progressively more actions. The `computer_20250124` version is the primary target, as it covers all requirements (CU-01 through CU-10) including scroll, fine-grained mouse control, and all click variants. The newest `computer_20251124` only adds `zoom` which is out of scope.

Key engineering challenges: (1) mapping Chrome/xdotool key names to Qt::Key enums, (2) coordinate translation between window-relative and screen-absolute, (3) scroll simulation via manual QWheelEvent construction (QTest has no wheel support), (4) low-level mouse drag with QTest::mousePress/mouseMove/mouseRelease sequence.

**Primary recommendation:** Create a `ComputerUseModeApi` class following the same pattern as `NativeModeApi`, registering `cu.*` methods. Include a `KeyNameMapper` utility class for Chrome-to-Qt key translation, and extend `InputSimulator` with mousePress/mouseRelease/mouseMove/scroll methods.

## Standard Stack

### Core (already in project)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6::Widgets | 6.x | Widget manipulation, QApplication::widgetAt | Already linked |
| Qt6::Test | 6.x | QTest mouse/keyboard simulation | Already linked for InputSimulator |
| Qt6::Gui | 6.x | QWheelEvent, QCursor, QKeySequence | Already linked |

### Supporting (no new dependencies needed)
| Class | Location | Purpose | Reuse Strategy |
|-------|----------|---------|----------------|
| InputSimulator | interaction/input_simulator.h | Mouse click, double-click, text, key sequences | Extend with mousePress/mouseRelease/mouseMove/scroll |
| Screenshot | interaction/screenshot.h | captureWidget/captureWindow/captureRegion | Use directly, add full-screen capture |
| HitTest | interaction/hit_test.h | widgetAt, widgetIdAt, widgetGeometry | Use directly for cursor position query |
| ResponseEnvelope | api/response_envelope.h | wrap() for uniform responses | Use directly |
| ErrorCode | api/error_codes.h | Structured error codes (-32040 range) | Add CU-specific codes if needed |
| JsonRpcHandler | transport/jsonrpc_handler.h | RegisterMethod for cu.* methods | Use directly |

### New Classes to Create
| Class | Location | Purpose |
|-------|----------|---------|
| ComputerUseModeApi | api/computer_use_mode_api.h/.cpp | Registers all cu.* JSON-RPC methods |
| KeyNameMapper | interaction/key_name_mapper.h/.cpp | Chrome/xdotool key name to Qt::Key mapping |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Extending InputSimulator | New CuInputSimulator class | Violates DRY; existing class already has the right abstractions |
| Static key map | QKeySequence::fromString | QKeySequence doesn't handle Chrome names like "ArrowUp", "BackSpace" (with capital S) |

**Installation:** No new dependencies. All Qt modules already linked.

## Architecture Patterns

### Recommended Project Structure
```
src/probe/
  api/
    computer_use_mode_api.h    # New: cu.* method registration
    computer_use_mode_api.cpp  # New: all cu.* handlers
  interaction/
    input_simulator.h          # Extended: add mousePress/mouseRelease/mouseMove/scroll
    input_simulator.cpp        # Extended: implementations
    key_name_mapper.h          # New: Chrome key name -> Qt::Key
    key_name_mapper.cpp        # New: mapping table + parser
    screenshot.h               # Extended: add captureScreen (full screen)
    screenshot.cpp             # Extended: implementation
```

### Pattern 1: API Registration (follow NativeModeApi pattern)
**What:** Single class registers all cu.* methods on JsonRpcHandler
**When to use:** This is the established pattern from Phase 3
**Example:**
```cpp
// Source: existing NativeModeApi pattern
class ComputerUseModeApi : public QObject {
    Q_OBJECT
public:
    explicit ComputerUseModeApi(JsonRpcHandler* handler, QObject* parent = nullptr);
private:
    void registerScreenshotMethods();   // cu.screenshot
    void registerMouseMethods();        // cu.click, cu.rightClick, cu.doubleClick, cu.mouseMove, cu.drag, cu.mouseDown, cu.mouseUp
    void registerKeyboardMethods();     // cu.type, cu.key
    void registerScrollMethod();        // cu.scroll
    void registerQueryMethods();        // cu.cursorPosition
    JsonRpcHandler* m_handler;
};
```

### Pattern 2: Chrome-Compatible Action Naming
**What:** Map Claude Computer Use tool action names to cu.* JSON-RPC methods
**When to use:** All method names should feel natural to agents familiar with the Chrome tool

| Chrome Action | JSON-RPC Method | Notes |
|---------------|-----------------|-------|
| `screenshot` | `cu.screenshot` | Returns base64 PNG |
| `left_click` | `cu.click` | Default button=left |
| `right_click` | `cu.rightClick` | |
| `middle_click` | `cu.middleClick` | |
| `double_click` | `cu.doubleClick` | |
| `mouse_move` | `cu.mouseMove` | |
| `left_click_drag` | `cu.drag` | High-level: start + end |
| `left_mouse_down` | `cu.mouseDown` | Low-level |
| `left_mouse_up` | `cu.mouseUp` | Low-level |
| `type` | `cu.type` | Text entry |
| `key` | `cu.key` | Key combos |
| `scroll` | `cu.scroll` | Direction + amount |
| `cursor_position` | `cu.cursorPosition` | Returns coords + widget info |

### Pattern 3: Coordinate Translation
**What:** Window-relative coordinates by default, optional screen-absolute
**When to use:** All mouse/click/scroll actions

```cpp
// Pseudocode for coordinate resolution
QWidget* targetWindow = getActiveQtWindow();
QPoint resolveCoordinate(int x, int y, bool screenAbsolute, QWidget* window) {
    if (screenAbsolute) {
        return QPoint(x, y);  // Used directly with QApplication::widgetAt
    }
    // Window-relative: convert to widget-local for QTest
    return QPoint(x, y);  // Already relative to window origin
}

// For QTest functions, need widget-local coordinates:
QWidget* widgetAtPoint = window->childAt(QPoint(x, y));
if (!widgetAtPoint) widgetAtPoint = window;
QPoint localPos = widgetAtPoint->mapFrom(window, QPoint(x, y));
```

### Pattern 4: Response Format
**What:** Minimal success responses with optional auto-screenshot
**When to use:** All cu.* method responses

```cpp
// Action response (minimal by default)
{
    "result": { "success": true },
    "meta": { "timestamp": 1706745600000 }
}

// With include_screenshot=true
{
    "result": {
        "success": true,
        "screenshot": "<base64 PNG>"
    },
    "meta": { "timestamp": 1706745600000 }
}

// Screenshot response
{
    "result": {
        "image": "<base64 PNG>",
        "width": 1024,
        "height": 768
    },
    "meta": { "timestamp": 1706745600000 }
}

// Cursor position response
{
    "result": {
        "x": 150,
        "y": 200,
        "widgetId": "QMainWindow/centralWidget/QPushButton(saveBtn)",
        "className": "QPushButton"
    },
    "meta": { "timestamp": 1706745600000 }
}
```

### Anti-Patterns to Avoid
- **Passing screen-absolute coordinates to QTest functions:** QTest mouse functions expect widget-local coordinates. Always convert.
- **Using QKeySequence alone for key parsing:** It does not understand Chrome names like "ArrowUp", "BackSpace" (different capitalization). Need custom mapper.
- **Silently clamping out-of-bounds coordinates:** CONTEXT.md explicitly says return error for OOB.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Key sequence parsing | Custom string parser for "ctrl+shift+a" | QKeySequence (for Qt names) + custom ChromeToQt table (for Chrome aliases) | QKeySequence handles modifier+key combos well, just need alias layer |
| Widget-at-coordinate lookup | Manual widget tree walk | QApplication::widgetAt() + HitTest::widgetAt() | Already implemented, handles Z-order correctly |
| Screenshot encoding | Manual PNG encoding | Screenshot::captureWidget/Window/Region | Already base64 PNG, tested |
| JSON-RPC registration | Custom dispatch table | JsonRpcHandler::RegisterMethod | Established pattern |
| Response wrapping | Custom JSON builders | ResponseEnvelope::wrap() | Consistent with Phase 3 API |

**Key insight:** Phase 4 is almost entirely an API translation layer. The hard work (QTest simulation, screenshot capture, hit testing) is done. The new work is mapping Chrome conventions to existing Qt primitives.

## Common Pitfalls

### Pitfall 1: QTest Mouse Functions Require Widget-Local Coordinates
**What goes wrong:** Passing window-relative (x, y) directly to QTest::mouseClick when the target is a child widget.
**Why it happens:** QTest::mouseClick(widget, ..., pos) expects `pos` relative to `widget`, not relative to the window.
**How to avoid:** Always: (1) find widget at window coordinate using window->childAt(x, y), (2) convert window coordinate to widget-local using widget->mapFrom(window, QPoint(x, y)), (3) pass local coordinate to QTest.
**Warning signs:** Clicks landing on wrong widgets, especially in nested layouts.

### Pitfall 2: QTest Has No Wheel/Scroll Simulation
**What goes wrong:** Looking for QTest::mouseWheel() or similar -- it does not exist.
**Why it happens:** Known Qt limitation (QTBUG-71449, filed 2018, still open).
**How to avoid:** Manually construct QWheelEvent and send via QCoreApplication::sendEvent(widget, &event). Standard angleDelta is 120 units per tick (15 degrees). Vertical scroll: angleDelta(0, +/-120). Horizontal scroll: angleDelta(+/-120, 0).
**Warning signs:** Compilation errors looking for nonexistent QTest scroll functions.

### Pitfall 3: QTest Drag (mousePress + mouseMove + mouseRelease) Sequence Issues
**What goes wrong:** QTest::mousePress followed by QTest::mouseMove does not reliably maintain button-held state in some configurations.
**Why it happens:** Known QTest issue -- mouse state tracking across separate calls can be unreliable.
**How to avoid:** For drag operations, construct QMouseEvent objects manually and use QCoreApplication::sendEvent(). Sequence: MouseButtonPress at start -> MouseMove events along path -> MouseButtonRelease at end. Process events between steps.
**Warning signs:** Drag not being recognized by target widget, drop not completing.

### Pitfall 4: Chrome vs Qt Key Name Differences
**What goes wrong:** Agent sends "Enter" expecting main keyboard Enter, but Qt maps "Enter" to numpad Enter (Qt::Key_Enter = 0x1005). Main Enter is "Return" (Qt::Key_Return = 0x1004).
**Why it happens:** Chrome/xdotool uses "Return" for main Enter key. Qt's QKeySequence::fromString("Enter") maps to numpad Enter.
**How to avoid:** Build explicit alias table. Map both "Enter" and "Return" to Qt::Key_Return. Map Chrome "ArrowUp" to Qt "Up", "ArrowDown" to "Down", etc.
**Warning signs:** Key events not being received by widgets, wrong key being sent.

### Pitfall 5: HiDPI Coordinate Mismatch
**What goes wrong:** Screenshot at 2x DPI shows 2000x1500 pixels but widget reports 1000x750 logical size.
**Why it happens:** QWidget::grab() returns physical pixels, coordinates are logical.
**How to avoid:** Per CONTEXT.md decision: default to logical pixels (coordinates match screenshot 1:1). Use QWidget::grab() which respects devicePixelRatio -- but the returned QPixmap has devicePixelRatio set, so when saved as PNG it may be larger. Need to explicitly set the pixmap's devicePixelRatio to 1.0 for logical-pixel screenshots, or scale down.
**Warning signs:** Agent clicks at (500, 300) in screenshot but hits wrong spot because screenshot was 2x resolution.

### Pitfall 6: No Active Window
**What goes wrong:** cu.screenshot or cu.click called but no Qt window is active/visible.
**Why it happens:** Application might not have a visible window, or window was closed.
**How to avoid:** Check QApplication::activeWindow() and QApplication::topLevelWidgets(). Return clear error with ErrorCode::kWidgetNotVisible.
**Warning signs:** Null pointer dereference, empty screenshots.

## Code Examples

### Chrome Key Name to Qt::Key Mapping Table
```cpp
// Source: Anthropic computer use reference (xdotool key names) + Qt::Key enum
// This is the critical translation table

struct KeyMapping {
    const char* chromeName;
    Qt::Key qtKey;
};

static const KeyMapping KEY_MAP[] = {
    // Chrome/xdotool name     Qt::Key
    // Navigation
    {"Return",                  Qt::Key_Return},
    {"Enter",                   Qt::Key_Return},     // Alias: agents often use "Enter"
    {"Tab",                     Qt::Key_Tab},
    {"Escape",                  Qt::Key_Escape},
    {"Esc",                     Qt::Key_Escape},     // Short alias
    {"BackSpace",               Qt::Key_Backspace},
    {"Backspace",               Qt::Key_Backspace},  // Case-insensitive alias
    {"Delete",                  Qt::Key_Delete},
    {"space",                   Qt::Key_Space},
    {"Space",                   Qt::Key_Space},

    // Arrow keys (Chrome uses "ArrowUp", xdotool uses "Up")
    {"Up",                      Qt::Key_Up},
    {"ArrowUp",                 Qt::Key_Up},
    {"Down",                    Qt::Key_Down},
    {"ArrowDown",               Qt::Key_Down},
    {"Left",                    Qt::Key_Left},
    {"ArrowLeft",               Qt::Key_Left},
    {"Right",                   Qt::Key_Right},
    {"ArrowRight",              Qt::Key_Right},

    // Page navigation
    {"Home",                    Qt::Key_Home},
    {"End",                     Qt::Key_End},
    {"Page_Up",                 Qt::Key_PageUp},
    {"PageUp",                  Qt::Key_PageUp},
    {"Page_Down",               Qt::Key_PageDown},
    {"PageDown",                Qt::Key_PageDown},
    {"Insert",                  Qt::Key_Insert},

    // Function keys
    {"F1",  Qt::Key_F1},  {"F2",  Qt::Key_F2},  {"F3",  Qt::Key_F3},
    {"F4",  Qt::Key_F4},  {"F5",  Qt::Key_F5},  {"F6",  Qt::Key_F6},
    {"F7",  Qt::Key_F7},  {"F8",  Qt::Key_F8},  {"F9",  Qt::Key_F9},
    {"F10", Qt::Key_F10}, {"F11", Qt::Key_F11}, {"F12", Qt::Key_F12},

    // Modifiers (when used as standalone keys)
    {"Shift_L",                 Qt::Key_Shift},
    {"Shift",                   Qt::Key_Shift},
    {"Control_L",               Qt::Key_Control},
    {"Control",                 Qt::Key_Control},
    {"Alt_L",                   Qt::Key_Alt},
    {"Alt",                     Qt::Key_Alt},
    {"Super_L",                 Qt::Key_Super_L},
    {"Super",                   Qt::Key_Super_L},
    {"Meta",                    Qt::Key_Meta},

    // Misc
    {"Print",                   Qt::Key_Print},
    {"Scroll_Lock",             Qt::Key_ScrollLock},
    {"Pause",                   Qt::Key_Pause},
    {"Caps_Lock",               Qt::Key_CapsLock},
    {"Num_Lock",                Qt::Key_NumLock},
    {"Menu",                    Qt::Key_Menu},
};
```

### Scroll Simulation via QWheelEvent
```cpp
// Source: Qt 6 QWheelEvent documentation + QTBUG-71449 workaround
void InputSimulator::scroll(QWidget* widget, const QPoint& pos,
                            int dx, int dy, Qt::KeyboardModifiers mods) {
    QPoint localPos = pos.isNull() ? widget->rect().center() : pos;
    QPoint globalPos = widget->mapToGlobal(localPos);

    // 120 units = 1 standard mouse wheel tick (15 degrees)
    QPoint angleDelta(dx * 120, dy * 120);
    QPoint pixelDelta(0, 0);  // Not available on most platforms

    QWheelEvent event(
        QPointF(localPos),
        QPointF(globalPos),
        pixelDelta,
        angleDelta,
        Qt::NoButton,
        mods,
        Qt::NoScrollPhase,
        false  // not inverted
    );
    QCoreApplication::sendEvent(widget, &event);
}
```

### Window-Relative to Widget-Local Coordinate Conversion
```cpp
// Source: Qt coordinate system documentation
// Given window-relative (x, y), find target widget and local coords

struct ResolvedTarget {
    QWidget* widget;
    QPoint localPos;
};

ResolvedTarget resolveWindowCoordinate(QWidget* window, int x, int y) {
    QPoint windowPos(x, y);

    // Bounds check (CONTEXT.md: strict, no clamping)
    if (x < 0 || y < 0 || x >= window->width() || y >= window->height()) {
        throw JsonRpcException(
            JsonRpcError::kInvalidParams,
            QString("Coordinates (%1, %2) out of bounds for window (%3 x %4)")
                .arg(x).arg(y).arg(window->width()).arg(window->height()));
    }

    // Find deepest child widget at this position
    QWidget* target = window->childAt(windowPos);
    if (!target) target = window;

    // Convert to widget-local coordinates
    QPoint local = target->mapFrom(window, windowPos);
    return {target, local};
}
```

### Low-Level Mouse Drag via QMouseEvent
```cpp
// Source: QTest limitations workaround, manual event construction
void InputSimulator::mouseDrag(QWidget* window,
                                const QPoint& startPos, const QPoint& endPos,
                                Qt::MouseButton button) {
    // Resolve start widget
    QWidget* startWidget = window->childAt(startPos);
    if (!startWidget) startWidget = window;
    QPoint localStart = startWidget->mapFrom(window, startPos);
    QPoint globalStart = startWidget->mapToGlobal(localStart);

    // Press at start
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(localStart),
                           QPointF(globalStart), button, button, Qt::NoModifier);
    QCoreApplication::sendEvent(startWidget, &pressEvent);
    QApplication::processEvents();

    // Move to end (may need intermediate points for smooth drag)
    QWidget* endWidget = window->childAt(endPos);
    if (!endWidget) endWidget = window;
    QPoint localEnd = endWidget->mapFrom(window, endPos);
    QPoint globalEnd = endWidget->mapToGlobal(localEnd);

    QMouseEvent moveEvent(QEvent::MouseMove, QPointF(localEnd),
                          QPointF(globalEnd), Qt::NoButton, button, Qt::NoModifier);
    QCoreApplication::sendEvent(endWidget, &moveEvent);
    QApplication::processEvents();

    // Release at end
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPointF(localEnd),
                             QPointF(globalEnd), button, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(endWidget, &releaseEvent);
    QApplication::processEvents();
}
```

### Full Screenshot with Optional Region
```cpp
// Source: existing Screenshot class + CONTEXT.md decisions
// cu.screenshot handler pseudocode

QString handleScreenshot(const QJsonObject& params) {
    // Determine capture scope
    QJsonObject region = params["region"].toObject();
    bool fullScreen = params["fullScreen"].toBool(false);
    bool includePhysicalPixels = params["physicalPixels"].toBool(false);

    QWidget* window = getActiveQtWindow();  // helper
    if (!window) {
        throw JsonRpcException(ErrorCode::kWidgetNotVisible,
            "No active Qt window found");
    }

    QByteArray base64;
    if (fullScreen) {
        // Capture entire screen
        QScreen* screen = window->screen();
        QPixmap pixmap = screen->grabWindow(0);  // 0 = entire screen
        // encode...
    } else if (!region.isEmpty()) {
        QRect rect(region["x"].toInt(), region["y"].toInt(),
                   region["width"].toInt(), region["height"].toInt());
        base64 = Screenshot::captureRegion(window, rect);
    } else {
        base64 = Screenshot::captureWindow(window);
    }

    QJsonObject result;
    result["image"] = QString::fromLatin1(base64);
    result["width"] = window->width();
    result["height"] = window->height();
    return envelopeToString(ResponseEnvelope::wrap(result));
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| computer_20241022 (no scroll action) | computer_20250124 (scroll, fine mouse control) | Jan 2025 | Our target version; must support scroll |
| computer_20250124 (no zoom) | computer_20251124 (zoom action) | Nov 2025 | Zoom is Opus 4.5 only; can defer |
| Single click type only | left_mouse_down/left_mouse_up for fine control | Jan 2025 | Must support for CONTEXT.md drag requirement |

**Deprecated/outdated:**
- `computer_20241022`: Lacks scroll, fine mouse control. Still works but agents on newer models use 20250124.

## Open Questions

1. **Active window detection strategy**
   - What we know: QApplication::activeWindow() returns the focused window; QApplication::topLevelWidgets() lists all.
   - What's unclear: If the Qt app has multiple top-level windows, which should "default" target? The focused one seems right, but what if focus is elsewhere (e.g., on the agent's terminal)?
   - Recommendation: Use QApplication::activeWindow() first, fall back to first visible top-level widget. Document behavior. Consider optional `windowId` parameter.

2. **QTest::mouseMove without widget**
   - What we know: QTest::mouseMove(widget, pos) moves the cursor relative to a widget. There is no global mouse move in QTest.
   - What's unclear: For cu.mouseMove with screen-absolute coordinates, we need to move the mouse globally.
   - Recommendation: Use QCursor::setPos(QPoint) for global mouse positioning, then QTest for widget-relative operations. QCursor::setPos works cross-platform.

3. **Screenshot devicePixelRatio handling**
   - What we know: QWidget::grab() returns a QPixmap with devicePixelRatio set. On 2x displays, a 400x300 widget produces an 800x600 pixel image.
   - What's unclear: Whether to return the raw physical pixels or scale down to logical pixels by default.
   - Recommendation: Per CONTEXT.md, default to logical pixels. Scale the captured QPixmap to logical size (divide by devicePixelRatio) before encoding. Offer `physicalPixels: true` flag for native resolution.

4. **Modifier key names in key combos**
   - What we know: Chrome uses "ctrl+c" (lowercase), xdotool uses "ctrl+c" or "Control_L+c". Qt QKeySequence uses "Ctrl+C" (title case).
   - What's unclear: Exact case sensitivity requirements.
   - Recommendation: KeyNameMapper should be case-insensitive for modifier names. Accept "ctrl", "Ctrl", "CTRL", "Control", "Control_L" all mapping to Qt::ControlModifier.

## Sources

### Primary (HIGH confidence)
- [Anthropic Computer Use Tool Documentation](https://platform.claude.com/docs/en/docs/agents-and-tools/computer-use) - Official API specification, action types, parameters, response format
- [Anthropic computer-use-demo reference implementation](https://github.com/anthropics/claude-quickstarts/blob/main/computer-use-demo/computer_use_demo/tools/computer.py) - Complete action list, parameter validation, key name conventions, coordinate scaling
- [QTest Namespace Qt 6](https://doc.qt.io/qt-6/qtest.html) - mouseClick, mousePress, mouseRelease, mouseMove, mouseDClick, keyClick, keyClicks
- [QWheelEvent Qt 6](https://doc.qt.io/qt-6/qwheelevent.html) - Constructor parameters, angleDelta conventions (120 units per tick)
- [QKeySequence Qt 6](https://doc.qt.io/qt-6/qkeysequence.html) - PortableText format, key name strings
- Existing codebase: InputSimulator, Screenshot, HitTest, NativeModeApi (read directly)

### Secondary (MEDIUM confidence)
- [QTBUG-71449](https://bugreports.qt.io/browse/QTBUG-71449) - Confirms QTest has no wheel event simulation; manual QWheelEvent is standard workaround
- [xdotool key names (X11 keysym)](https://gitlab.com/nokun/gestures/-/wikis/xdotool-list-of-key-codes) - Key name conventions used by Chrome Computer Use tool
- [QTest mouse move with button pressed issues](https://www.qtcentre.org/threads/71770-QTest-simulating-a-mouse-move-with-the-pressed-button) - Known issue with drag simulation

### Tertiary (LOW confidence)
- None. All findings verified with official sources.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - No new dependencies, all Qt modules already in use
- Architecture: HIGH - Following established NativeModeApi pattern, Chrome API shape verified from official docs
- Key mapping: HIGH - Chrome reference implementation uses xdotool names, Qt key names documented
- Pitfalls: HIGH - QTest limitations confirmed via Qt bug tracker and documentation
- Scroll simulation: HIGH - QWheelEvent manual construction is documented workaround
- Drag simulation: MEDIUM - QTest drag issues reported by community; manual QMouseEvent approach is standard but less tested in this codebase

**Research date:** 2026-01-31
**Valid until:** 2026-03-31 (stable; Chrome API versioned, Qt 6 API stable)
