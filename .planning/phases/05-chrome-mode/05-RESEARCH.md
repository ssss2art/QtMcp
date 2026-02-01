# Phase 5: Chrome Mode - Research

**Researched:** 2026-01-31
**Domain:** Qt Accessibility (QAccessible) to Chrome-style accessibility tree mapping; ref-based element interaction
**Confidence:** MEDIUM (novel mapping domain, but both APIs are well-documented)

## Summary

Phase 5 implements the "Chrome Mode" API that lets AI agents interact with Qt applications using an accessibility tree with numbered element references, mirroring the Claude Chrome extension's tool interface. The core challenge is mapping Qt's `QAccessibleInterface` hierarchy to a Chrome-style output format with `ref_1`, `ref_2` style identifiers for interactive elements.

Qt provides a mature accessibility framework via `QAccessibleInterface` with `child()`/`childCount()` traversal, role enums, state bitfields, and specialized sub-interfaces (`QAccessibleValueInterface`, `QAccessibleActionInterface`, `QAccessibleEditableTextInterface`). Chrome's CDP uses ARIA-style role names (`button`, `textbox`, `slider`, etc.) which map naturally to Qt's role enum. The main engineering work is: (1) building the tree walker, (2) mapping roles/states, (3) managing ephemeral refs, (4) implementing form input via accessibility sub-interfaces, and (5) intercepting qDebug messages.

**Primary recommendation:** Build a `ChromeModeApi` class following the exact pattern of `NativeModeApi`/`ComputerUseModeApi` (register methods on `JsonRpcHandler`, use `ResponseEnvelope`, use anonymous namespace helpers). The accessibility tree walker should be a separate `AccessibilityTreeWalker` utility class that `ChromeModeApi` calls. Refs should be ephemeral, incrementing integers, stored in a `QHash<QString, QAccessibleInterface*>` that is rebuilt on each `read_page` call.

## Standard Stack

### Core (all already in project)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Accessibility (`QAccessible`) | Qt 6.x | Access tree, roles, states, text, actions | Built into QtGui, no extra dependency |
| `QAccessibleInterface` | Qt 6.x | Per-object accessibility API | Core traversal mechanism |
| `QAccessibleValueInterface` | Qt 6.x | Numeric value get/set (spinbox, slider, dial) | Built-in, type-safe |
| `QAccessibleActionInterface` | Qt 6.x | Action execution (press, toggle, focus) | Built-in action dispatch |
| `QAccessibleEditableTextInterface` | Qt 6.x | Text insert/replace/delete | Built-in text manipulation |
| `QJsonDocument` / `QJsonObject` | Qt 6.x | JSON serialization | Already used throughout project |
| `JsonRpcHandler` | internal | Method registration | Existing infrastructure |
| `ResponseEnvelope` | internal | Response wrapping | Existing infrastructure |
| `InputSimulator` | internal | Click/type/key actions by widget | Existing Phase 2 infrastructure |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `ObjectRegistry` | internal | Get hierarchical IDs for Qt extras | Enriching tree nodes with objectName/className/hierarchicalId |
| `KeyNameMapper` | internal | Chrome key name parsing | For keyboard actions in `computer` method |
| `Screenshot` | internal | Optional screenshot inclusion | For `include_screenshot` on action responses |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| QAccessible tree walk | QObject tree walk | QAccessible includes non-QObject items (scrollbar handles, etc.) and provides roles/states. Use QAccessible. |
| Direct widget manipulation for form_input | QAccessibleValueInterface/QAccessibleEditableTextInterface | Accessibility interfaces are widget-type-agnostic and work uniformly. Use accessibility interfaces. |
| String matching for find | LLM-based matching | String matching is fast, deterministic, and sufficient. Chrome's find internally uses Sonnet but for a local tool, fuzzy string matching on name/role/label is appropriate. |

## Architecture Patterns

### Recommended Project Structure
```
src/probe/
  accessibility/
    accessibility_tree_walker.h     # Tree traversal + ref assignment
    accessibility_tree_walker.cpp
    role_mapper.h                   # QAccessible::Role -> Chrome role string
    role_mapper.cpp
    console_message_capture.h       # qInstallMessageHandler wrapper
    console_message_capture.cpp
  api/
    chrome_mode_api.h               # chr.* method registration (existing pattern)
    chrome_mode_api.cpp
```

### Pattern 1: Accessibility Tree Walker
**What:** Recursive traversal of QAccessibleInterface tree, building JSON output with ref assignment
**When to use:** Every `read_page` and `find` call
**Example:**
```cpp
// Source: Qt 6 QAccessibleInterface docs + project patterns
class AccessibilityTreeWalker {
public:
    struct WalkOptions {
        QString filter;       // "interactive" or "all"
        int maxDepth = 15;
        int maxChars = 50000;
        QString scopeRef;     // ref_id to start from (subtree)
    };

    struct WalkResult {
        QJsonObject tree;                              // The accessibility tree as JSON
        QHash<QString, QAccessibleInterface*> refMap;  // ref_N -> interface mapping
        int totalNodes = 0;
        bool truncated = false;
    };

    static WalkResult walk(QWidget* window, const WalkOptions& opts);

private:
    static QJsonObject walkNode(QAccessibleInterface* iface,
                                int depth, int maxDepth,
                                const QString& filter,
                                int& refCounter,
                                QHash<QString, QAccessibleInterface*>& refMap,
                                int& charCount, int maxChars,
                                bool& truncated);
};
```

### Pattern 2: Ephemeral Ref Map (File-Scope Static)
**What:** Refs are rebuilt on each `read_page` call. A file-scope static `QHash` stores the current mapping.
**When to use:** Following the same pattern as `s_lastSimulatedPosition` in `ComputerUseModeApi`.
**Example:**
```cpp
// Source: Project pattern from computer_use_mode_api.cpp
namespace {
    static QHash<QString, QPointer<QObject>> s_refToObject;
    static QHash<QString, QAccessibleInterface*> s_refToAccessible;

    void clearRefs() {
        s_refToObject.clear();
        s_refToAccessible.clear();
    }

    QAccessibleInterface* resolveRef(const QString& ref) {
        auto it = s_refToAccessible.find(ref);
        if (it == s_refToAccessible.end() || !it.value() || !it.value()->isValid()) {
            throw JsonRpcException(
                ErrorCode::kRefNotFound,
                QStringLiteral("Element ref not found: %1. Call read_page to get fresh refs.").arg(ref),
                QJsonObject{{QStringLiteral("ref"), ref}});
        }
        return it.value();
    }
}
```

### Pattern 3: Chrome API Method Registration
**What:** Follow exact NativeModeApi/ComputerUseModeApi pattern for ChromeModeApi
**When to use:** All chr.* method registration
**Example:**
```cpp
// Source: Project pattern
class ChromeModeApi : public QObject {
    Q_OBJECT
public:
    explicit ChromeModeApi(JsonRpcHandler* handler, QObject* parent = nullptr);
private:
    void registerReadPageMethod();     // chr.readPage
    void registerClickMethod();        // chr.click
    void registerFormInputMethod();    // chr.formInput
    void registerGetPageTextMethod();  // chr.getPageText
    void registerFindMethod();         // chr.find
    void registerNavigateMethod();     // chr.navigate
    void registerTabsContextMethod();  // chr.tabsContext
    void registerConsoleMethod();      // chr.readConsoleMessages
    JsonRpcHandler* m_handler;
};
```

### Pattern 4: Console Message Capture
**What:** Install `qInstallMessageHandler` to intercept qDebug/qWarning/qCritical/qFatal and store in a ring buffer.
**When to use:** On probe initialization, before any application code runs.
**Example:**
```cpp
// Source: Qt 6 QtLogging docs (https://doc.qt.io/qt-6/qtlogging.html)
struct ConsoleMessage {
    QtMsgType type;        // QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtInfoMsg
    QString message;
    QString file;
    int line;
    QString function;
    qint64 timestamp;      // QDateTime::currentMSecsSinceEpoch()
};

class ConsoleMessageCapture {
public:
    static ConsoleMessageCapture* instance();
    void install();     // calls qInstallMessageHandler
    QList<ConsoleMessage> messages(const QString& pattern = QString(),
                                    bool onlyErrors = false,
                                    int limit = 100) const;
    void clear();
private:
    static void messageHandler(QtMsgType type,
                                const QMessageLogContext& context,
                                const QString& msg);
    QList<ConsoleMessage> m_messages;
    QtMessageHandler m_previousHandler = nullptr;
    mutable QMutex m_mutex;
    static constexpr int MAX_MESSAGES = 1000;  // Ring buffer limit
};
```

### Anti-Patterns to Avoid
- **Walking QObject tree instead of QAccessible tree:** The QAccessible tree includes non-QObject elements (scrollbar handles, etc.) and excludes invisible containers. Always use QAccessibleInterface for tree traversal.
- **Persistent refs across calls:** Chrome uses ephemeral refs that reset each call. Persistent refs would require tracking UI changes, adding massive complexity for no benefit.
- **Capturing QAccessibleInterface pointers long-term:** Accessibility interfaces can become invalid when widgets are destroyed. Use `isValid()` checks, and only hold refs within a single request/response cycle or use QPointer for the underlying QObject.
- **Using QTest functions for form input when accessibility interfaces exist:** For form_input, use `QAccessibleValueInterface::setCurrentValue()` for numeric widgets and `QAccessibleEditableTextInterface::replaceText()` for text widgets. This is widget-type-agnostic.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Tree traversal | Custom QObject tree walk | `QAccessibleInterface::child()`/`childCount()` | Handles non-widget accessible objects, respects visibility |
| Role name mapping | Ad-hoc string switch | Static `QHash<QAccessible::Role, QString>` lookup table | Centralized, testable, complete coverage |
| Setting spinbox/slider values | `qobject_cast<QSpinBox*>` + `setValue()` | `QAccessibleValueInterface::setCurrentValue()` | Works for ANY widget with value semantics |
| Setting text input values | `qobject_cast<QLineEdit*>` + `setText()` | `QAccessibleEditableTextInterface::replaceText()` | Works for QLineEdit, QTextEdit, QPlainTextEdit uniformly |
| Pressing buttons/toggling checkboxes | Direct widget method calls | `QAccessibleActionInterface::doAction(pressAction())` | Widget-type-agnostic |
| Message interception | Redirect stderr/stdout | `qInstallMessageHandler()` | Official Qt API, gets structured context (file, line, function) |
| Key name parsing | New key mapper | Existing `KeyNameMapper` | Already built in Phase 4 |

**Key insight:** Qt's accessibility sub-interfaces (`QAccessibleValueInterface`, `QAccessibleActionInterface`, `QAccessibleEditableTextInterface`) eliminate the need for widget-type-specific code. The Chrome Mode API can interact with ANY widget through its accessibility interface, without knowing whether it's a QSpinBox, QDial, or custom widget.

## Common Pitfalls

### Pitfall 1: QAccessibleInterface Lifetime
**What goes wrong:** Storing `QAccessibleInterface*` pointers that become dangling when widgets are destroyed.
**Why it happens:** QAccessibleInterface objects are not QObjects and don't have QPointer safety. They become invalid when their underlying widget is deleted.
**How to avoid:** Always call `isValid()` before using a cached interface. For the ref map, also store a `QPointer<QObject>` to the underlying QObject (via `iface->object()`) as a validity check. Clear the ref map on each `read_page` call (ephemeral refs).
**Warning signs:** Crashes in `role()` or `text()` calls on stale interfaces.

### Pitfall 2: QAccessible Not Initialized
**What goes wrong:** `QAccessible::queryAccessibleInterface()` returns nullptr for all widgets.
**Why it happens:** Qt's accessibility framework is lazy-initialized. On some platforms, it only activates when an assistive technology client connects.
**How to avoid:** Call `QAccessible::setActive(true)` early in probe initialization to force accessibility activation. This is critical on Linux (AT-SPI) where accessibility may be disabled by default.
**Warning signs:** `queryAccessibleInterface()` returning nullptr for standard Qt widgets.

### Pitfall 3: Infinite/Deep Tree Recursion
**What goes wrong:** Tree walker runs forever or generates massive output on complex UIs.
**Why it happens:** Some widgets (QTableView with 10000 rows) expose every cell as an accessible child. Deep nesting (QTabWidget > QScrollArea > QTreeView > items) compounds the problem.
**How to avoid:** Enforce `maxDepth` (default 15), `maxChars` (default 50000), and node count limits. For tables/trees, summarize rather than expand all children. Prune invisible/offscreen children.
**Warning signs:** read_page taking >1 second or returning >100KB.

### Pitfall 4: Platform-Specific Accessibility Differences
**What goes wrong:** Accessibility tree structure differs between Windows (UI Automation) and Linux (AT-SPI).
**Why it happens:** Qt's accessibility backends have slightly different behaviors. On Windows, `QAccessible::queryAccessibleInterface()` works reliably. On Linux, AT-SPI may not activate without `QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1`.
**How to avoid:** Test on both platforms. Use `QAccessible::setActive(true)` unconditionally. Note: this project already targets Windows primarily with Linux support.
**Warning signs:** Tests pass on one platform but fail on another.

### Pitfall 5: Missing Accessible Names
**What goes wrong:** Many elements show empty names in the tree, making it useless for agents.
**Why it happens:** Qt widgets only have accessible names if: (a) text is set (QPushButton::setText), (b) QLabel buddy is set, (c) explicit `setAccessibleName()` is called, or (d) the accessibility implementation derives it.
**How to avoid:** Fall back to multiple sources for names: `text(QAccessible::Name)`, then `objectName()`, then `className()`, then widget text properties. Include all available identifiers in output.
**Warning signs:** Tree full of unnamed elements.

### Pitfall 6: ComboBox Form Input Complexity
**What goes wrong:** Setting a combo box value via `form_input` is not as simple as setting a text value.
**Why it happens:** QComboBox doesn't implement `QAccessibleValueInterface` or `QAccessibleEditableTextInterface`. It's a composite widget with a popup list.
**How to avoid:** For combo boxes, find the matching item by text and call `QComboBox::setCurrentIndex()` or `setCurrentText()` directly. Check if the accessible object's underlying QObject is a QComboBox and handle it specially.
**Warning signs:** form_input silently failing on combo boxes.

## Code Examples

### Walking the Accessibility Tree
```cpp
// Source: Qt 6 QAccessibleInterface docs
void walkTree(QAccessibleInterface* iface, int depth, int maxDepth,
              QJsonArray& nodes, int& refCounter,
              QHash<QString, QAccessibleInterface*>& refMap,
              const QString& filter) {
    if (!iface || !iface->isValid() || depth > maxDepth)
        return;

    QAccessible::Role role = iface->role();
    QAccessible::State state = iface->state();

    // Skip invisible elements unless filter is "all"
    if (state.invisible && filter != QStringLiteral("all"))
        return;

    bool isInteractive = RoleMapper::isInteractive(role);

    // For "interactive" filter, only assign refs to interactive elements
    // but still include structural parents for context
    QJsonObject node;
    if (isInteractive || filter == QStringLiteral("all")) {
        QString ref = QStringLiteral("ref_%1").arg(++refCounter);
        node[QStringLiteral("ref")] = ref;
        refMap.insert(ref, iface);
    }

    node[QStringLiteral("role")] = RoleMapper::toChromeName(role);
    node[QStringLiteral("name")] = iface->text(QAccessible::Name);

    // States
    QJsonObject states;
    if (state.focused) states[QStringLiteral("focused")] = true;
    if (state.disabled) states[QStringLiteral("disabled")] = true;
    if (state.checked) states[QStringLiteral("checked")] = true;
    if (state.expanded) states[QStringLiteral("expanded")] = true;
    if (state.selected) states[QStringLiteral("selected")] = true;
    if (!states.isEmpty()) node[QStringLiteral("states")] = states;

    // Bounding rect
    QRect rect = iface->rect();
    if (rect.isValid()) {
        node[QStringLiteral("bounds")] = QJsonObject{
            {QStringLiteral("x"), rect.x()},
            {QStringLiteral("y"), rect.y()},
            {QStringLiteral("width"), rect.width()},
            {QStringLiteral("height"), rect.height()}
        };
    }

    // Qt extras
    QObject* obj = iface->object();
    if (obj) {
        node[QStringLiteral("objectName")] = obj->objectName();
        node[QStringLiteral("className")] =
            QString::fromUtf8(obj->metaObject()->className());
        // hierarchicalId from ObjectRegistry
        node[QStringLiteral("objectId")] =
            ObjectRegistry::instance()->objectId(obj);
    }

    // Children
    QJsonArray children;
    for (int i = 0; i < iface->childCount(); ++i) {
        QAccessibleInterface* child = iface->child(i);
        walkTree(child, depth + 1, maxDepth, children,
                 refCounter, refMap, filter);
    }
    if (!children.isEmpty())
        node[QStringLiteral("children")] = children;

    nodes.append(node);
}
```

### Role Mapping Table
```cpp
// Source: QAccessible::Role enum (doc.qt.io/qt-6/qaccessible.html#Role-enum)
//         + ARIA role names (developer.mozilla.org/en-US/docs/Web/Accessibility/ARIA/Reference/Roles)
static const QHash<QAccessible::Role, QString> s_roleMap = {
    // Interactive widget roles
    {QAccessible::Button,         QStringLiteral("button")},
    {QAccessible::CheckBox,       QStringLiteral("checkbox")},
    {QAccessible::RadioButton,    QStringLiteral("radio")},
    {QAccessible::ComboBox,       QStringLiteral("combobox")},
    {QAccessible::SpinBox,        QStringLiteral("spinbutton")},
    {QAccessible::Slider,         QStringLiteral("slider")},
    {QAccessible::Dial,           QStringLiteral("slider")},        // Dial has no ARIA equiv, map to slider
    {QAccessible::ProgressBar,    QStringLiteral("progressbar")},
    {QAccessible::EditableText,   QStringLiteral("textbox")},
    {QAccessible::Link,           QStringLiteral("link")},

    // Menu roles
    {QAccessible::MenuBar,        QStringLiteral("menubar")},
    {QAccessible::PopupMenu,      QStringLiteral("menu")},
    {QAccessible::MenuItem,       QStringLiteral("menuitem")},
    {QAccessible::ButtonMenu,     QStringLiteral("button")},        // Menu button
    {QAccessible::ButtonDropDown, QStringLiteral("button")},

    // Container/structure roles
    {QAccessible::Window,         QStringLiteral("window")},
    {QAccessible::Dialog,         QStringLiteral("dialog")},
    {QAccessible::ToolBar,        QStringLiteral("toolbar")},
    {QAccessible::StatusBar,      QStringLiteral("status")},
    {QAccessible::Grouping,       QStringLiteral("group")},
    {QAccessible::Separator,      QStringLiteral("separator")},
    {QAccessible::Splitter,       QStringLiteral("separator")},     // Splitter ~ separator
    {QAccessible::Pane,           QStringLiteral("region")},
    {QAccessible::Client,         QStringLiteral("generic")},       // Central widget area
    {QAccessible::Application,    QStringLiteral("application")},
    {QAccessible::Document,       QStringLiteral("document")},
    {QAccessible::WebDocument,    QStringLiteral("document")},
    {QAccessible::Section,        QStringLiteral("section")},
    {QAccessible::Heading,        QStringLiteral("heading")},
    {QAccessible::Paragraph,      QStringLiteral("paragraph")},
    {QAccessible::Form,           QStringLiteral("form")},
    {QAccessible::Notification,   QStringLiteral("alert")},
    {QAccessible::Note,           QStringLiteral("note")},
    {QAccessible::ComplementaryContent, QStringLiteral("complementary")},
    {QAccessible::Footer,         QStringLiteral("contentinfo")},

    // Tab roles
    {QAccessible::PageTab,        QStringLiteral("tab")},
    {QAccessible::PageTabList,    QStringLiteral("tablist")},
    {QAccessible::PropertyPage,   QStringLiteral("tabpanel")},

    // Table/list/tree roles
    {QAccessible::Table,          QStringLiteral("table")},
    {QAccessible::ColumnHeader,   QStringLiteral("columnheader")},
    {QAccessible::RowHeader,      QStringLiteral("rowheader")},
    {QAccessible::Row,            QStringLiteral("row")},
    {QAccessible::Cell,           QStringLiteral("cell")},
    {QAccessible::List,           QStringLiteral("list")},
    {QAccessible::ListItem,       QStringLiteral("listitem")},
    {QAccessible::Tree,           QStringLiteral("tree")},
    {QAccessible::TreeItem,       QStringLiteral("treeitem")},

    // Text/display roles
    {QAccessible::StaticText,     QStringLiteral("text")},          // Chrome uses "text" for static text
    {QAccessible::Graphic,        QStringLiteral("img")},
    {QAccessible::Chart,          QStringLiteral("img")},           // Chart ~ image
    {QAccessible::Canvas,         QStringLiteral("img")},
    {QAccessible::Animation,      QStringLiteral("img")},

    // Misc
    {QAccessible::TitleBar,       QStringLiteral("banner")},
    {QAccessible::ScrollBar,      QStringLiteral("scrollbar")},
    {QAccessible::ToolTip,        QStringLiteral("tooltip")},
    {QAccessible::HelpBalloon,    QStringLiteral("tooltip")},
    {QAccessible::AlertMessage,   QStringLiteral("alert")},
    {QAccessible::Indicator,      QStringLiteral("status")},
    {QAccessible::Whitespace,     QStringLiteral("none")},
    {QAccessible::LayeredPane,    QStringLiteral("region")},
    {QAccessible::Terminal,       QStringLiteral("log")},
    {QAccessible::Desktop,        QStringLiteral("application")},
    {QAccessible::ColorChooser,   QStringLiteral("dialog")},
    {QAccessible::Clock,          QStringLiteral("timer")},
    {QAccessible::BlockQuote,     QStringLiteral("blockquote")},
};

// Roles that are "interactive" (get refs in interactive filter mode)
static const QSet<QAccessible::Role> s_interactiveRoles = {
    QAccessible::Button, QAccessible::CheckBox, QAccessible::RadioButton,
    QAccessible::ComboBox, QAccessible::SpinBox, QAccessible::Slider,
    QAccessible::Dial, QAccessible::EditableText, QAccessible::Link,
    QAccessible::MenuItem, QAccessible::PageTab, QAccessible::ListItem,
    QAccessible::TreeItem, QAccessible::Cell,
    QAccessible::ButtonMenu, QAccessible::ButtonDropDown,
    QAccessible::ScrollBar, QAccessible::HotkeyField,
};
```

### Form Input by Widget Type via Accessibility Interfaces
```cpp
// Source: Qt 6 QAccessibleValueInterface/QAccessibleEditableTextInterface docs
void setFormValue(QAccessibleInterface* iface, const QJsonValue& value) {
    // Try value interface first (spinbox, slider, dial, progress bar)
    auto* valueIface = iface->interface_cast<QAccessibleValueInterface>();
    if (valueIface) {
        QVariant v;
        if (value.isDouble()) v = value.toDouble();
        else if (value.isString()) v = value.toString().toDouble();
        else v = value.toVariant();
        valueIface->setCurrentValue(v);
        return;
    }

    // Try editable text interface (QLineEdit, QTextEdit, QPlainTextEdit)
    auto* editIface = iface->interface_cast<QAccessibleEditableTextInterface>();
    if (editIface) {
        auto* textIface = iface->interface_cast<QAccessibleTextInterface>();
        int len = textIface ? textIface->characterCount() : 0;
        editIface->replaceText(0, len, value.toString());
        return;
    }

    // Special case: ComboBox - no standard accessibility interface for selection
    QObject* obj = iface->object();
    if (auto* combo = qobject_cast<QComboBox*>(obj)) {
        QString text = value.toString();
        int idx = combo->findText(text);
        if (idx >= 0) combo->setCurrentIndex(idx);
        else combo->setCurrentText(text); // For editable combo
        return;
    }

    // Special case: CheckBox/RadioButton via action interface
    auto* actionIface = iface->interface_cast<QAccessibleActionInterface>();
    if (actionIface && value.isBool()) {
        QAccessible::State st = iface->state();
        bool checked = st.checked;
        if (checked != value.toBool()) {
            actionIface->doAction(QAccessibleActionInterface::toggleAction());
        }
        return;
    }

    // Fallback error
    throw JsonRpcException(ErrorCode::kFormInputUnsupported,
        QStringLiteral("Widget does not support form input"),
        QJsonObject{{QStringLiteral("role"),
            RoleMapper::toChromeName(iface->role())}});
}
```

### Console Message Capture
```cpp
// Source: Qt 6 QtLogging docs (https://doc.qt.io/qt-6/qtlogging.html)
void ConsoleMessageCapture::messageHandler(
    QtMsgType type,
    const QMessageLogContext& context,
    const QString& msg)
{
    auto* self = instance();
    QMutexLocker lock(&self->m_mutex);

    ConsoleMessage entry;
    entry.type = type;
    entry.message = msg;
    entry.file = context.file ? QString::fromUtf8(context.file) : QString();
    entry.line = context.line;
    entry.function = context.function ? QString::fromUtf8(context.function) : QString();
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();

    self->m_messages.append(entry);

    // Trim ring buffer
    while (self->m_messages.size() > MAX_MESSAGES)
        self->m_messages.removeFirst();

    // Chain to previous handler (critical: don't swallow messages)
    if (self->m_previousHandler)
        self->m_previousHandler(type, context, msg);
}
```

### Text Extraction (get_page_text)
```cpp
// Source: QAccessible::Text enum, QAccessibleInterface docs
QStringList extractVisibleText(QAccessibleInterface* iface, int depth = 0) {
    QStringList result;
    if (!iface || !iface->isValid() || depth > 20)
        return result;

    QAccessible::State state = iface->state();
    if (state.invisible || state.offscreen)
        return result;

    // Extract text from this node
    QString name = iface->text(QAccessible::Name);
    QString value = iface->text(QAccessible::Value);
    QAccessible::Role role = iface->role();

    // Include text for text-bearing roles
    if (role == QAccessible::StaticText ||
        role == QAccessible::EditableText ||
        role == QAccessible::Heading ||
        role == QAccessible::Paragraph ||
        role == QAccessible::Document ||
        role == QAccessible::Button ||
        role == QAccessible::Link ||
        role == QAccessible::Label ||
        role == QAccessible::ListItem ||
        role == QAccessible::TreeItem ||
        role == QAccessible::Cell ||
        role == QAccessible::MenuItem) {
        if (!name.isEmpty()) result << name;
        if (!value.isEmpty() && value != name) result << value;
    }

    // Recurse into children
    for (int i = 0; i < iface->childCount(); ++i) {
        result << extractVisibleText(iface->child(i), depth + 1);
    }
    return result;
}
```

## QAccessible::Role to Chrome Role Name - Complete Mapping

**Confidence: HIGH** - Both enum lists are from official documentation.

The complete mapping is shown in the Code Examples section above. Key mapping decisions:

| Qt Role | Chrome/ARIA Role | Rationale |
|---------|-----------------|-----------|
| `Button` (0x2B) | `button` | Direct match |
| `CheckBox` (0x2C) | `checkbox` | Direct match |
| `RadioButton` (0x2D) | `radio` | ARIA uses "radio" |
| `ComboBox` (0x2E) | `combobox` | Direct match |
| `SpinBox` (0x34) | `spinbutton` | ARIA uses "spinbutton" |
| `Slider` (0x33) | `slider` | Direct match |
| `Dial` (0x31) | `slider` | No ARIA "dial" role, closest is slider |
| `EditableText` (0x2A) | `textbox` | ARIA uses "textbox" |
| `StaticText` (0x29) | `text` | Chrome uses "text" |
| `PageTab` (0x25) | `tab` | ARIA uses "tab" |
| `PageTabList` (0x3C) | `tablist` | ARIA uses "tablist" |
| `Tree` (0x23) | `tree` | Direct match |
| `TreeItem` (0x24) | `treeitem` | ARIA uses "treeitem" |
| Unmapped roles | `generic` | Fallback for unknown roles |

## QAccessible::State to Chrome States - Key Mappings

**Confidence: HIGH** - State struct from Qt 6 source (qaccessible_base.h).

The `QAccessible::State` struct has 37 bitfield members. Only a subset are relevant for Chrome Mode output:

| Qt State | Chrome State | When Included |
|----------|-------------|---------------|
| `focused` | `"focused": true` | Element has keyboard focus |
| `disabled` | `"disabled": true` | Element cannot be interacted with |
| `checked` | `"checked": true` | Checkbox/radio is checked |
| `checkStateMixed` | `"checked": "mixed"` | Tri-state checkbox |
| `expanded` | `"expanded": true` | Tree/accordion item is expanded |
| `collapsed` | `"expanded": false` | Tree/accordion item is collapsed |
| `selected` | `"selected": true` | List/tree item is selected |
| `readOnly` | `"readonly": true` | Text field is read-only |
| `pressed` | `"pressed": true` | Button is being pressed |
| `hasPopup` | `"hasPopup": true` | Element has a popup (combo, menu button) |
| `modal` | `"modal": true` | Dialog is modal |
| `editable` | `"editable": true` | Text is editable |
| `multiLine` | `"multiline": true` | Text edit is multiline |
| `passwordEdit` | `"password": true` | Password input field |
| `required` | `"required": true` | Form field is required |
| `invisible` | (excluded from tree) | Node not included |
| `offscreen` | (excluded from tree) | Node not included by default |

## Chrome Tool API Mapping

**Confidence: HIGH** - Tool definitions from reverse-engineered Claude Chrome extension.

| Chrome Tool | Qt Method Name | Parameters | Notes |
|-------------|---------------|------------|-------|
| `read_page` | `chr.readPage` | `filter`, `depth`, `ref_id` | No `tabId` needed (single app, not browser) |
| `find` | `chr.find` | `query` | String match against names, roles, labels, tooltips |
| `form_input` | `chr.formInput` | `ref`, `value` | value type: string, boolean, or number |
| `get_page_text` | `chr.getPageText` | (none) | All visible text from active window |
| `navigate` | `chr.navigate` | `action`, `target` | "activateTab", "activateMenuItem", "back", "forward" |
| `tabs_context` | `chr.tabsContext` | (none) | List top-level windows with active window indicator |
| `read_console_messages` | `chr.readConsoleMessages` | `pattern`, `onlyErrors`, `clear`, `limit` | Pattern is regex filter |
| click/type/key | Reuse `cu.*` methods | ref-based overloads or translate ref to coords | See Open Questions |

### Method Namespace Decision
Use `chr.*` prefix to distinguish from `qt.*` (native) and `cu.*` (computer use). This follows the established pattern and makes the three modes clearly distinct.

## Error Codes

Following the established pattern (error codes in ranges of 10):

```cpp
// Chrome Mode errors (-32070 to -32079)
constexpr int kRefNotFound = -32070;         // Ref from read_page not found / stale
constexpr int kRefStale = -32071;            // Ref exists but widget was destroyed
constexpr int kFormInputUnsupported = -32072; // Widget doesn't support form input
constexpr int kTreeTooLarge = -32073;         // Accessibility tree exceeds maxChars
constexpr int kFindTooManyResults = -32074;   // Find returned >20 results
constexpr int kNavigateInvalid = -32075;      // Invalid navigation target
constexpr int kConsoleNotAvailable = -32076;  // Console capture not installed
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Coordinate-based interaction only | Accessibility tree + refs | Chrome extension 2025 | Agents can reason about UI semantically |
| Screenshot-only page understanding | Accessibility tree as primary mode | Chrome extension 2025 | 100x smaller context, faster, cheaper |
| LLM-based element finding | String matching on accessible names/roles | Practical for local tools | No API call needed, deterministic |

## Open Questions

1. **Click-by-ref: new method or extend cu.click?**
   - What we know: Chrome extension uses a single `computer` tool with action parameter for click/type/key/screenshot. Our project has separate `cu.*` methods.
   - What's unclear: Should `chr.click` translate the ref to widget+coordinates and delegate to `InputSimulator`, or should it use `QAccessibleActionInterface::doAction(pressAction())`?
   - Recommendation: Use `QAccessibleActionInterface::doAction(pressAction())` for `chr.click` since it's the accessibility-native approach. Fall back to `InputSimulator::mouseClick()` at widget center if no action interface exists. This keeps Chrome Mode independent of Computer Use Mode.

2. **Navigate: What does this mean for Qt apps?**
   - What we know: Chrome's `navigate` does URL navigation or history back/forward. Qt apps don't have URLs.
   - What's unclear: How to map this. Context says "tabs, menus, back/forward" but was deferred from detailed discussion.
   - Recommendation: Support: (a) activate a tab by ref, (b) trigger a menu item by ref, (c) back/forward via QAction undo/redo if available. Keep it simple; this can be extended later.

3. **Find: How sophisticated does matching need to be?**
   - What we know: Chrome's `find` uses Sonnet (an LLM) internally. Our tool should use string matching.
   - What's unclear: Exact matching algorithm (substring? fuzzy? regex?).
   - Recommendation: Case-insensitive substring match against: accessible name, role name, tooltip text, objectName, className. Score by relevance (exact > starts-with > contains). Return top 20. This is simple, fast, and sufficient.

4. **Tree output format: JSON tree or text tree?**
   - What we know: Chrome extension's read_page format is not publicly documented in detail. The open-source `agent-browser` uses a text format like `heading "Title" [ref=e1]`.
   - What's unclear: Whether Claude expects JSON or text.
   - Recommendation: Return JSON (consistent with all other methods in this project). The tree should be a nested JSON structure with `children` arrays. Agents parse JSON natively.

## Sources

### Primary (HIGH confidence)
- Qt 6 QAccessible docs: https://doc.qt.io/qt-6/qaccessible.html - Role enum, State struct, Text enum
- Qt 6 QAccessibleInterface: https://doc.qt.io/qt-6/qaccessibleinterface.html - Navigation, child/parent methods
- Qt 6 qaccessible_base.h source: https://codebrowser.dev/qt6/qtbase/src/gui/accessible/qaccessible_base.h.html - Complete State bitfield (37 fields)
- Qt 6 QAccessibleValueInterface: https://doc.qt.io/qt-6/qaccessiblevalueinterface.html - setCurrentValue, minimumValue, maximumValue
- Qt 6 QAccessibleEditableTextInterface: https://doc.qt.io/qt-6/qaccessibleeditabletextinterface.html - insertText, replaceText, deleteText
- Qt 6 QAccessibleActionInterface: https://doc.qt.io/qt-6/qaccessibleactioninterface.html - 12 predefined actions (press, toggle, focus, etc.)
- Qt 6 QtLogging: https://doc.qt.io/qt-6/qtlogging.html - qInstallMessageHandler, QtMsgType, QMessageLogContext
- Chrome DevTools Protocol Accessibility: https://chromedevtools.github.io/devtools-protocol/tot/Accessibility/ - AXNode structure
- MDN ARIA Roles: https://developer.mozilla.org/en-US/docs/Web/Accessibility/ARIA/Reference/Roles - Complete ARIA role taxonomy

### Secondary (MEDIUM confidence)
- Claude Chrome extension tool definitions (reverse-engineered): https://github.com/AIPexStudio/system-prompts-of-claude-chrome - read_page.json, find.json, form_input.json, navigate.json, tabs_context.json, read_console_messages.json, get_page_text.json
- Qt Accessibility for QWidget Applications: https://doc.qt.io/qt-6/accessible-qwidget.html - Implementation guidance
- Qt source qaccessiblewidgetfactory.cpp: https://github.com/qt/qtbase/blob/dev/src/widgets/accessible/qaccessiblewidgetfactory.cpp - Which widgets get which accessible class

### Tertiary (LOW confidence)
- Claude Chrome extension operation details: https://medium.com/@buttercanbentley/how-claude-in-chrome-works-45b86b06f689 - General architecture description
- agent-browser ref format: https://github.com/vercel-labs/agent-browser - Alternative implementation showing `[ref=e1]` text format

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All Qt built-in, no new dependencies
- Architecture: HIGH - Follows established project patterns exactly
- Role/State mapping: HIGH - Both Qt and ARIA are well-documented standards
- Form input via accessibility interfaces: MEDIUM - Verified APIs exist, ComboBox needs special handling
- Console capture: HIGH - qInstallMessageHandler is well-documented official API
- Chrome tool format parity: MEDIUM - Tool schemas obtained from reverse-engineered repo, not official docs
- Find algorithm: MEDIUM - Simple string matching is a design choice, not verified against Chrome behavior
- Pitfalls: HIGH - Based on Qt accessibility documentation and common Qt development experience

**Research date:** 2026-01-31
**Valid until:** 2026-03-01 (stable domain - Qt accessibility API rarely changes)
