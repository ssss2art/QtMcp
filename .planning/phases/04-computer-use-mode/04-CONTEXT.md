# Phase 4: Computer Use Mode - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

AI agents can control Qt applications using screenshots and pixel coordinates. Provides screenshot capture, mouse actions (click, right-click, double-click, drag, move), text typing, key combinations, scrolling, and cursor position querying. This is the visual automation API — no object tree or accessibility knowledge required.

</domain>

<decisions>
## Implementation Decisions

### Screenshot behavior
- Default capture scope: active Qt window
- Optional parameter to capture full screen or a specific region
- Region specified as bounding rect (x, y, w, h)
- Cursor not included by default; optional flag to include it
- Default to logical pixels (coordinates match screenshot 1:1); optional flag for native/physical pixel resolution on HiDPI displays

### Mouse action model
- Coordinate system: window-relative by default (origin at top-left of Qt window); optional flag for screen-absolute
- High-level drag command: single call with start + end coordinates
- Low-level mouse control also available: separate mouseDown/mouseMove/mouseUp for complex drag scenarios
- Actions execute instantly by default; optional delay_ms parameter for timing-sensitive actions
- Scrolling: discrete ticks (like mouse wheel clicks) at a given coordinate, with direction parameter

### Keyboard input model
- Separate actions: `type` for text entry, `key` for special keys and shortcuts
- Modifier combos expressed as string format: "ctrl+c", "ctrl+shift+s", etc.
- Keys always sent to currently focused widget — agent must click to focus first
- Special key naming: accept both Chrome API names AND Qt key names (e.g., both "Enter" and "Return" work, both "ArrowUp" and "Up" work)

### API response design
- Chrome-compatible API baseline with Qt-specific extensions where it makes sense
- Minimal responses by default: success confirmation or structured error
- Optional flag to include auto-screenshot in action responses (avoids separate screenshot call)
- Out-of-bounds coordinates return an error (strict — no silent clamping)
- Cursor position query returns (x, y) coordinates plus widget ID and class name under cursor

### Claude's Discretion
- Exact Chrome API action name mappings (which names to match vs extend)
- Internal implementation of screenshot rendering pipeline
- Double-click timing interval
- How to map Chrome key names to Qt key enums internally

</decisions>

<specifics>
## Specific Ideas

- API shape should feel familiar to agents that already know Claude-in-Chrome's computer tool
- Both Chrome and Qt key naming conventions accepted — agents don't need to learn Qt-specific names
- Window-relative coordinates as default ensures screenshots and click targets always align without offset math

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 04-computer-use-mode*
*Context gathered: 2026-01-31*
