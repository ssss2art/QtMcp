# Phase 5: Chrome Mode - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Accessibility tree with numbered refs API — AI agents can control Qt applications using element references instead of pixel coordinates or object paths. Mirrors the Claude Chrome extension's read_page/find/form_input/computer model. Does NOT include QML-specific introspection (Phase 6) or Python MCP wrapping (Phase 7).

</domain>

<decisions>
## Implementation Decisions

### Accessibility tree shape
- Chrome-style mixed hierarchy: structural containers shown for context, interactive elements highlighted with refs
- Configurable depth (default ~15), pruned not flat
- Qt accessibility roles translated to Chrome equivalents (QAccessible::PushButton → "button", EditableText → "textbox", etc.)
- Each element includes: ref, role (Chrome name), name/label, states (focused, disabled, checked, expanded), bounding rect
- PLUS Qt extras: objectName, className, hierarchical object ID from Native Mode
- Full Chrome parity for filtering: "all" vs "interactive" filter, configurable depth, scope to subtree by ref_id, max_chars limit

### Ref lifecycle & stability
- Ephemeral refs (Chrome-style): refs reset each read_page call, simple incrementing format (ref_1, ref_2, ref_3)
- Stale ref handling: match Chrome plugin behavior (clear error when ref not found)
- Post-action responses: action result only, no auto-included subtree. Agent calls read_page again if needed.
- Find results use same ref system as read_page — agents can immediately act on found elements

### Element finding & text extraction
- Find method: Chrome parity — match query against accessible names, roles, labels, tooltip text
- Find returns up to 20 matching elements with refs, asks for more specific query if >20
- Text extraction: Chrome parity — extract all visible text from the application (all rendered text the user can see)

### Console message capture
- Install qInstallMessageHandler to intercept qDebug/qWarning/qCritical/qFatal
- Also capture application stdout/stderr
- Agent can read with pattern filtering (regex support like Chrome's read_console_messages)

### Claude's Discretion
- Exact QAccessible::Role → Chrome role name mapping table (research which roles exist in practice)
- How to detect "main content area" for text extraction if needed
- Tree pruning heuristics (which non-interactive containers to keep for structure)
- stdout/stderr capture mechanism (platform-specific)
- Error code assignments for Chrome Mode methods

</decisions>

<specifics>
## Specific Ideas

- "Chrome parity" is the guiding principle — when in doubt, match how the Claude Chrome extension does it
- The user explicitly wants the probe to feel like controlling a browser tab, but for Qt apps
- Refs from find and read_page should be interchangeable — same ref system, same format
- Qt extras (objectName, className, hierarchical ID) are a value-add over Chrome — include them per element

</specifics>

<deferred>
## Deferred Ideas

- Navigation actions for tabs/menus were in the original gray areas but not selected for discussion — researcher should investigate Qt menu/tab accessibility patterns
- Console message capture was discussed but the "navigation surface" (tab/menu control) aspect was deferred from detailed discussion

</deferred>

---

*Phase: 05-chrome-mode*
*Context gathered: 2026-01-31*
