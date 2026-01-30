# Phase 2: Core Introspection - Context

**Gathered:** 2026-01-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Probe can discover, inspect, and interact with any QObject in the target application. This includes:
- Object registry and tree discovery (find by name/class, get hierarchy)
- Property read/write and method invocation
- Signal monitoring with push notifications
- UI interaction primitives (clicks, keystrokes, screenshots, hit testing)

API mode exposure (Native/Computer Use/Chrome) is handled in later phases.

</domain>

<decisions>
## Implementation Decisions

### Object identification
- Path format: objectNames preferred, fall back to text property if unnamed, then class name
- Multiple unnamed instances: use tree position suffix (e.g., `QPushButton#1`, `QPushButton#2`)
- Example path: `mainWindow/central/submitBtn` or `mainWindow/toolbar/QPushButton#1`
- ID stability: stable when based on objectName/text; position-based IDs may change if UI restructured
- Tree operations: accept optional root parameter - return subtree from specified object, full tree if omitted
- Find methods: provide both `findByX` (first match) and `findAllByX` (all matches) variants

### UI interaction semantics
- Coordinate system: widget-local (relative to target widget's top-left corner)
- Sync model: async by default, optional `wait` parameter for synchronous behavior
- Key modifiers: accept both formats
  - String: `'Ctrl+Shift+A'` (parsed like QKeySequence)
  - Explicit array: `modifiers=['ctrl', 'shift']`
  - Explicit array takes precedence if both provided
- Screenshots: support widget capture, window capture, or arbitrary rectangle region

### Error handling & edge cases
- Stale references: return JSON-RPC error with specific code indicating object not found
- Read-only properties: return error explaining property is read-only, include property name
- Method arguments: let Qt's meta-object system attempt type coercion - error only if conversion fails
- Signal subscriptions: auto-unsubscribe when object destroyed, push notification sent to client

### Claude's Discretion
- Exact JSON-RPC error codes and messages
- Internal object registry data structures
- Thread safety implementation details
- Performance optimizations for large object trees

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-core-introspection*
*Context gathered: 2026-01-30*
