# Phase 6: Extended Introspection - Context

**Gathered:** 2026-02-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Extend the probe's introspection capabilities to cover QML items (QQuickItem hierarchy, properties, IDs) and QAbstractItemModel data access (discovery, navigation, data retrieval). This phase adds QML awareness to the existing object tree and new model/view query methods to the qt.* API. No new API modes — extends existing Native Mode.

</domain>

<decisions>
## Implementation Decisions

### QML Tree Integration
- QML items appear naturally in the existing QObject tree with extra metadata fields (qmlId, qmlFile, isQmlItem)
- No separate QML subtree endpoint — single unified tree
- When a QML item has an `id`, the QML id **replaces** the className in the hierarchical path (e.g., `QQuickWindow/contentItem/submitButton` not `QQuickWindow/contentItem/QQuickRectangle`)
- Anonymous QML items (no id) use **short QML type names** with QQuick prefix stripped (e.g., `Rectangle`, `Text`, `Item` instead of `QQuickRectangle`)
- Tree walk **seamlessly crosses** widget-to-QML boundaries (QQuickWidget → QML items) — agent doesn't need to know about the boundary
- QML detail (qmlId, short type names) always included in getObjectTree — no opt-in flag

### QML Property Depth
- **Public API only** — no private Qt headers, no QQmlPrivate
- Properties accessible through QObject::property() and QMetaObject are exposed
- Context properties (QQmlContext) and binding expressions are **out of scope**
- Return **evaluated values only** — no binding expressions, no binding hints
- Attached properties and grouped properties included if naturally accessible via QMetaObject/QObject::property() — no special handling
- QML properties are **read and write** — support setProperty for QML properties (same QObject::setProperty() mechanism)

### Model/View Navigation
- **Smart pagination**: models with <100 rows return all data; larger models return first page with totalRows count, agent fetches more pages via offset/limit
- **DisplayRole by default**: return Qt::DisplayRole data. Agent can request specific roles or all roles via parameter
- **Discovery via ObjectRegistry filter**: use existing ObjectRegistry to find QAbstractItemModel subclasses — no separate model tracking infrastructure
- **Full hierarchy support**: support parent(), index(), rowCount(parent) for tree models — agent can walk hierarchical model data (file browsers, org charts, etc.)

### API Surface Design
- **Extend qt.* namespace** — QML and Model/View methods added to existing Native Mode (no new qml.* or mv.* namespaces)
- **Chrome Mode automatic inclusion**: QML items that expose QAccessible interfaces appear in chr.readPage automatically — QAccessible doesn't distinguish widget vs QML
- **Model identification accepts view or model objectId**: if agent passes a view's objectId, resolve its model() automatically — agent doesn't need to know model vs view distinction

### Claude's Discretion
- Exact new qt.* method names and signatures
- QML type name stripping logic (handling custom QML types vs built-in)
- Model pagination page size (100 row threshold is a guideline)
- How to handle QML items that lack QAccessible interfaces
- Error handling for model index out-of-bounds
- Role name resolution strategy (roleNames() vs hardcoded Qt role IDs)

</decisions>

<specifics>
## Specific Ideas

- QML hierarchical paths should read like QML source: `Window/contentItem/submitButton` not `QQuickWindow/QQuickItem/QQuickRectangle`
- Model data fetch should feel like a REST API: offset/limit pagination, role filtering
- Agent should be able to go from "I see a table view" → get its data without manually resolving the model object

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 06-extended-introspection*
*Context gathered: 2026-02-01*
