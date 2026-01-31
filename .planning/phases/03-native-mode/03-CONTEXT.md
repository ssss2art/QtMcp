# Phase 3: Native Mode - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Complete Native Mode API surface exposing all Qt introspection capabilities (built in Phase 2) as a polished, agent-friendly JSON-RPC API. This includes reorganizing the existing 21 methods into a consistent namespace, adding convenience operations, and implementing proper error handling. Discovery, inspection, mutation, interaction, and signal monitoring must all be available through the finalized API.

</domain>

<decisions>
## Implementation Decisions

### API naming & namespaces
- Dotted namespace convention: `qt.objects.find`, `qt.properties.get`, `qt.signals.subscribe`, etc.
- Group methods by domain (objects, properties, methods, signals, ui, etc.)
- Reorganize the existing 21 Phase 2 methods into the new namespace scheme

### Response format
- Uniform envelope for all responses: `{result: ..., meta: {...}}`
- Meta object includes contextual info (objectId, timestamp, etc.)
- Consistent structure across all methods so agents can parse uniformly

### Object referencing (three styles)
- **Hierarchical path IDs** (primary): `QMainWindow/centralWidget/QPushButton#submit`
- **Numeric shorthand IDs**: compact integer refs for brevity during sessions
- **Symbolic names**: user-defined aliases loaded from a JSON name map file (Squish-style object map)
- All three styles accepted anywhere an objectId parameter is expected
- Name map loaded from file at startup AND manageable at runtime via API (register/unregister names)

### Error handling
- Standard JSON-RPC error codes (-32000 to -32099 range) with detailed human-readable messages
- Stale object references return error with hint to re-query the object tree (no auto-resolve)
- Validation errors include the expected parameter schema for that method — agents can self-correct
- Dedicated `qt.ping` health check method — agents opt-in to check event loop responsiveness

### Convenience operations
- Single calls only (no JSON-RPC batch support) — keep it simple
- Combined `qt.objects.inspect` method returning properties + methods + signals in one call
- Individual methods (`qt.properties.list`, `qt.methods.list`, `qt.signals.list`) also available — agents choose
- Rich query method: `qt.objects.query` with filters like `{className: "QPushButton", properties: {enabled: true}}`
- Object tree supports depth limiting (`maxDepth`) and subtree queries (start from specific objectId)

### Claude's Discretion
- Exact namespace groupings and method name choices within the dotted convention
- Meta object fields beyond objectId/timestamp
- Internal caching or optimization strategies
- Health check implementation details (what qt.ping actually measures)

</decisions>

<specifics>
## Specific Ideas

- Symbolic name map inspired by Squish's object naming system (https://doc.qt.io/squish/squish-api.html) — user defines short names for frequently-used widget paths in a JSON file
- Name map is both file-based (version-controllable, loaded at startup) and runtime-manageable (agents can register/unregister names during a session)
- Combined inspect + individual methods pattern: agents doing exploration use inspect, agents doing targeted work use individual methods

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 03-native-mode*
*Context gathered: 2026-01-31*
