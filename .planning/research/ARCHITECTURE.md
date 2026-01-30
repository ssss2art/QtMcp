# Architecture Patterns

**Domain:** Qt introspection/injection tools
**Researched:** 2026-01-29

## Recommended Architecture

QtMCP follows the established architecture patterns used by GammaRay, Qt-Inspector, and Qat for Qt introspection tools. The architecture separates into four primary components with clear boundaries:

```
                                    ┌─────────────────────────┐
                                    │     Claude / LLM        │
                                    │    (MCP Client)         │
                                    └───────────┬─────────────┘
                                                │ MCP Protocol
                                                │ (stdio/HTTP)
┌───────────────────────────────────────────────┼──────────────────────────────┐
│  HOST MACHINE                                 │                              │
│                                               ▼                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │  PYTHON MCP SERVER                                                      │ │
│  │  ┌──────────────┐    ┌──────────────┐    ┌──────────────────────────┐  │ │
│  │  │ MCP Protocol │    │ Tool         │    │ WebSocket Client         │  │ │
│  │  │ Handler      │───▶│ Dispatcher   │───▶│ (JSON-RPC)               │  │ │
│  │  └──────────────┘    └──────────────┘    └────────────┬─────────────┘  │ │
│  └───────────────────────────────────────────────────────│─────────────────┘ │
│                                                          │ WebSocket         │
│                                                          │ JSON-RPC 2.0      │
│  ┌───────────────────────────────────────────────────────▼─────────────────┐ │
│  │  TARGET Qt APPLICATION PROCESS                                          │ │
│  │  ┌────────────────────────────────────────────────────────────────────┐ │ │
│  │  │  C++ PROBE (libqtmcp.so / qtmcp.dll)                               │ │ │
│  │  │                                                                     │ │ │
│  │  │  ┌──────────────────────────────────────────────────────────────┐  │ │ │
│  │  │  │  TRANSPORT LAYER                                             │  │ │ │
│  │  │  │  ┌─────────────────┐    ┌────────────────────────────────┐   │  │ │ │
│  │  │  │  │ WebSocket Server│    │ JSON-RPC Handler               │   │  │ │ │
│  │  │  │  │ (QWebSocketSvr) │───▶│ (Request/Response routing)     │   │  │ │ │
│  │  │  │  └─────────────────┘    └────────────────────────────────┘   │  │ │ │
│  │  │  └──────────────────────────────────────────────────────────────┘  │ │ │
│  │  │                                    │                                │ │ │
│  │  │  ┌─────────────────────────────────▼────────────────────────────┐  │ │ │
│  │  │  │  INTROSPECTION LAYER                                         │  │ │ │
│  │  │  │  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐  │  │ │ │
│  │  │  │  │ Object       │  │ Meta         │  │ Widget/A11y        │  │  │ │ │
│  │  │  │  │ Registry     │  │ Inspector    │  │ Tree Builder       │  │  │ │ │
│  │  │  │  └──────────────┘  └──────────────┘  └────────────────────┘  │  │ │ │
│  │  │  └──────────────────────────────────────────────────────────────┘  │ │ │
│  │  │                                    │                                │ │ │
│  │  │  ┌─────────────────────────────────▼────────────────────────────┐  │ │ │
│  │  │  │  HOOKS LAYER (Qt Internal APIs)                              │  │ │ │
│  │  │  │  ┌──────────────────────┐  ┌──────────────────────────────┐  │  │ │ │
│  │  │  │  │ qtHookData Callbacks │  │ Signal Spy Callbacks         │  │  │ │ │
│  │  │  │  │ (AddQObject/Remove)  │  │ (qt_register_signal_spy_cb)  │  │  │ │ │
│  │  │  │  └──────────────────────┘  └──────────────────────────────┘  │  │ │ │
│  │  │  └──────────────────────────────────────────────────────────────┘  │ │ │
│  │  │                                    │                                │ │ │
│  │  │  ┌─────────────────────────────────▼────────────────────────────┐  │ │ │
│  │  │  │  Qt Application Objects (QWidgets, QML Items, etc.)          │  │ │ │
│  │  │  └──────────────────────────────────────────────────────────────┘  │ │ │
│  │  └────────────────────────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Component Boundaries

| Component | Responsibility | Communicates With | Process |
|-----------|---------------|-------------------|---------|
| **Python MCP Server** | MCP protocol handling, tool exposure to Claude | Claude (stdio/HTTP), C++ Probe (WebSocket) | Separate process |
| **WebSocket Client (Python)** | Async communication with probe, request/response correlation | JSON-RPC Handler in probe | Part of MCP Server |
| **WebSocket Server (C++)** | Network transport, connection management | MCP Server, JSON-RPC Handler | In-target process |
| **JSON-RPC Handler (C++)** | Request parsing, method dispatch, response formatting | WebSocket Server, Mode Handlers | In-target process |
| **Mode Handlers (C++)** | API-specific logic (Native/ComputerUse/Chrome) | JSON-RPC Handler, Introspection Layer | In-target process |
| **Object Registry (C++)** | QObject lifecycle tracking, ID assignment | Hooks Layer, Meta Inspector | In-target process |
| **Meta Inspector (C++)** | QMetaObject introspection, property/method access | Object Registry, Qt Objects | In-target process |
| **Widget Tree Builder (C++)** | Hierarchy traversal, accessibility tree generation | Object Registry, Qt Widgets | In-target process |
| **Hooks Layer (C++)** | Qt internal API integration (qtHookData, signal spy) | Qt Core internals | In-target process |

### Data Flow

**Request Flow (Inbound):**
```
1. Claude sends MCP tool call
2. Python MCP Server receives via stdio/HTTP
3. Tool Dispatcher selects appropriate handler
4. WebSocket Client formats JSON-RPC request
5. WebSocket Server receives in Qt event loop
6. JSON-RPC Handler parses and dispatches
7. Mode Handler processes request
8. Introspection Layer queries Qt objects
9. Response travels back up the chain
```

**Event Flow (Outbound - Push Notifications):**
```
1. Qt signal fires or object created/destroyed
2. Hooks Layer callback triggers
3. Object Registry updates internal state
4. JSON-RPC Handler formats notification
5. WebSocket Server pushes to all clients
6. Python MCP Server receives notification
7. (Optionally forwarded to Claude via MCP)
```

## Patterns to Follow

### Pattern 1: Qt Event Loop Integration

**What:** All probe operations must integrate with the target application's Qt event loop, never block it.

**When:** All network communication, async operations, and deferred callbacks.

**Why:** The probe runs inside the target application. Blocking the event loop freezes the UI and prevents signals from firing.

**Example:**
```cpp
// WebSocket server runs in the main Qt event loop
class WebSocketServer : public QObject {
    Q_OBJECT
public:
    WebSocketServer(QObject* parent = nullptr)
        : QObject(parent), server_(new QWebSocketServer("QtMCP",
                                   QWebSocketServer::NonSecureMode, this)) {
        // Signal-based connection handling - non-blocking
        connect(server_, &QWebSocketServer::newConnection,
                this, &WebSocketServer::onNewConnection);
    }

private slots:
    void onNewConnection() {
        QWebSocket* socket = server_->nextPendingConnection();
        connect(socket, &QWebSocket::textMessageReceived,
                this, &WebSocketServer::onTextMessage);
    }

    void onTextMessage(const QString& message) {
        // Process via event loop, never block
        QMetaObject::invokeMethod(this, [this, message]() {
            processMessage(message);
        }, Qt::QueuedConnection);
    }
};
```

### Pattern 2: Object Lifecycle Tracking via qtHookData

**What:** Use Qt's internal hook array to receive callbacks for every QObject creation and destruction.

**When:** Tracking all objects in the application for the Object Registry.

**Why:** This is the only reliable way to know about ALL objects, including those created before probe initialization.

**Example:**
```cpp
#include <private/qhooks_p.h>

namespace {
    QHooks::AddQObjectCallback previousAddCallback = nullptr;
    QHooks::RemoveQObjectCallback previousRemoveCallback = nullptr;
}

void onObjectAdded(QObject* object) {
    // Call previous callback first (daisy-chain)
    if (previousAddCallback)
        previousAddCallback(object);

    // Track in our registry
    ObjectRegistry::instance()->registerObject(object);
}

void onObjectRemoved(QObject* object) {
    if (previousRemoveCallback)
        previousRemoveCallback(object);

    ObjectRegistry::instance()->unregisterObject(object);
}

void installHooks() {
    // Preserve existing callbacks (for tools like GammaRay)
    previousAddCallback = reinterpret_cast<QHooks::AddQObjectCallback>(
        qtHookData[QHooks::AddQObject]);
    previousRemoveCallback = reinterpret_cast<QHooks::RemoveQObjectCallback>(
        qtHookData[QHooks::RemoveQObject]);

    // Install our callbacks
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&onObjectAdded);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&onObjectRemoved);
}
```

### Pattern 3: Signal Spy Callback Registration

**What:** Use `qt_register_signal_spy_callbacks` to monitor all signal emissions.

**When:** Implementing signal subscription and push notifications.

**Why:** This is the standard Qt internal API for signal monitoring, used by GammaRay.

**Example:**
```cpp
#include <private/qobject_p.h>

// Callback set for signal monitoring
static QSignalSpyCallbackSet s_callbacks = {
    nullptr, nullptr, nullptr, nullptr
};

void signalBeginCallback(QObject* caller, int signal_index, void** argv) {
    // Called before signal handlers run
    SignalMonitor::instance()->onSignalEmit(caller, signal_index, argv);
}

void signalEndCallback(QObject* caller, int signal_index) {
    // Called after all handlers complete
}

void installSignalSpy() {
    s_callbacks.signal_begin_callback = signalBeginCallback;
    s_callbacks.signal_end_callback = signalEndCallback;
    qt_register_signal_spy_callbacks(s_callbacks);
}
```

### Pattern 4: QPointer for Safe Object References

**What:** Always use QPointer to hold references to tracked QObjects.

**When:** Storing object references in registries, caches, or pending operations.

**Why:** QObjects can be deleted at any time. QPointer automatically nullifies when the object is destroyed.

**Example:**
```cpp
class ObjectRegistry {
public:
    QObject* findObject(const QString& id) {
        auto it = idToObject_.find(id);
        if (it == idToObject_.end())
            return nullptr;

        // QPointer automatically checks for deletion
        if (it.value().isNull()) {
            idToObject_.erase(it);  // Clean up stale entry
            return nullptr;
        }
        return it.value().data();
    }

private:
    QHash<QString, QPointer<QObject>> idToObject_;
};
```

### Pattern 5: Thread-Safe Cross-Component Communication

**What:** Use Qt's signal/slot mechanism with queued connections for thread safety.

**When:** Communication between components that may be on different threads.

**Why:** The probe runs in the target's main thread, but WebSocket I/O may occur on worker threads.

**Example:**
```cpp
class Probe : public QObject {
    Q_OBJECT
signals:
    void requestReceived(const QString& method, const QJsonObject& params, int requestId);
    void responseReady(int requestId, const QJsonValue& result);

public:
    Probe() {
        // Queued connection ensures main thread execution
        connect(this, &Probe::requestReceived,
                this, &Probe::handleRequest, Qt::QueuedConnection);
    }

private slots:
    void handleRequest(const QString& method, const QJsonObject& params, int requestId) {
        // Always runs on main thread - safe to access Qt objects
        QJsonValue result = processMethod(method, params);
        emit responseReady(requestId, result);
    }
};
```

### Pattern 6: Hierarchical ID Generation

**What:** Generate stable, human-readable object IDs based on parent-child hierarchy.

**When:** Assigning unique identifiers to QObjects for external reference.

**Why:** Hierarchical IDs are debuggable and reflect the actual Qt object structure.

**Example:**
```cpp
QString generateObjectId(QObject* obj) {
    QStringList segments;
    QObject* current = obj;

    while (current) {
        QString segment = current->metaObject()->className();

        // Add objectName if set
        if (!current->objectName().isEmpty()) {
            segment += "#" + current->objectName();
        } else {
            // Add index for disambiguation among siblings
            QObject* parent = current->parent();
            if (parent) {
                int index = 0;
                for (QObject* sibling : parent->children()) {
                    if (sibling == current) break;
                    if (sibling->metaObject()->className() ==
                        current->metaObject()->className())
                        index++;
                }
                if (index > 0)
                    segment += QString("[%1]").arg(index);
            }
        }

        segments.prepend(segment);
        current = current->parent();
    }

    return segments.join("/");
}
```

## Anti-Patterns to Avoid

### Anti-Pattern 1: Blocking the Qt Event Loop

**What:** Performing synchronous operations that block the main thread.

**Why bad:** Freezes the UI, prevents signal delivery, can cause deadlocks with nested event loops.

**Instead:** Use async operations with Qt signals/slots, or `QMetaObject::invokeMethod` with `Qt::QueuedConnection`.

```cpp
// BAD - blocks event loop
void handleRequest(const QJsonObject& request) {
    QThread::msleep(1000);  // NEVER do this
    processRequest(request);
}

// GOOD - deferred execution
void handleRequest(const QJsonObject& request) {
    QTimer::singleShot(0, this, [this, request]() {
        processRequest(request);
    });
}
```

### Anti-Pattern 2: Raw Pointers to QObjects

**What:** Storing raw `QObject*` pointers in data structures.

**Why bad:** QObjects can be deleted at any time by their parent or explicitly. Dangling pointer access causes crashes.

**Instead:** Use `QPointer<QObject>` which automatically nullifies on object destruction.

```cpp
// BAD - dangling pointer risk
QHash<QString, QObject*> objectCache_;

// GOOD - safe references
QHash<QString, QPointer<QObject>> objectCache_;
```

### Anti-Pattern 3: Ignoring Thread Affinity

**What:** Accessing QObjects from threads other than the one they were created on.

**Why bad:** Qt objects are not thread-safe. Accessing from wrong thread causes undefined behavior.

**Instead:** Use `QMetaObject::invokeMethod` with `Qt::QueuedConnection` to dispatch to correct thread.

```cpp
// BAD - direct cross-thread access
void workerThreadFunction(QWidget* widget) {
    widget->setText("Hello");  // UNDEFINED BEHAVIOR
}

// GOOD - marshal to correct thread
void workerThreadFunction(QWidget* widget) {
    QMetaObject::invokeMethod(widget, [widget]() {
        widget->setText("Hello");
    }, Qt::QueuedConnection);
}
```

### Anti-Pattern 4: Hardcoded Object Paths

**What:** Using hardcoded hierarchical paths to find objects.

**Why bad:** Widget hierarchy can change between application versions, breaking automation.

**Instead:** Use `findByObjectName` or `findByClassName` with flexible matching.

```cpp
// BAD - brittle hardcoded path
QObject* button = findObject("QMainWindow/centralWidget/QVBoxLayout/QPushButton[2]");

// GOOD - flexible lookup
QObject* button = findByObjectName("submitButton");
// or
QObject* button = findByClassName("QPushButton", [](QObject* obj) {
    return obj->property("text").toString() == "Submit";
});
```

### Anti-Pattern 5: Synchronous WebSocket Communication

**What:** Blocking on WebSocket responses in the Python MCP server.

**Why bad:** MCP tools should be responsive; blocking makes Claude wait unnecessarily.

**Instead:** Use async/await patterns in Python with proper timeout handling.

```python
# BAD - blocking wait
def call_probe(self, method, params):
    self.ws.send(json.dumps(request))
    return json.loads(self.ws.recv())  # Blocks indefinitely

# GOOD - async with timeout
async def call_probe(self, method, params, timeout=5.0):
    await self.ws.send(json.dumps(request))
    try:
        response = await asyncio.wait_for(
            self.ws.recv(), timeout=timeout
        )
        return json.loads(response)
    except asyncio.TimeoutError:
        raise ProbeTimeoutError(f"Probe did not respond within {timeout}s")
```

## Scalability Considerations

| Concern | At 10 objects | At 1K objects | At 100K objects |
|---------|---------------|---------------|-----------------|
| **Object Registry** | HashMap lookup O(1) | HashMap lookup O(1) | Consider weak references, lazy enumeration |
| **Tree Traversal** | Full traversal OK | Depth-limited traversal | Pagination, on-demand loading |
| **Signal Monitoring** | Monitor all | Selective subscription | Must filter at callback level |
| **Memory Overhead** | Negligible | ~1MB for metadata | Monitor via profiling, consider eviction |

## Build Order (Dependencies Between Components)

Based on the architecture, the recommended implementation order:

```
Phase 1: Foundation
├── 1.1 CMake/vcpkg setup
├── 1.2 Probe entry point (DllMain/constructor)
└── 1.3 Basic WebSocket server (QWebSocketServer)

Phase 2: Hooks Layer (No dependencies on other probe components)
├── 2.1 qtHookData integration
└── 2.2 Signal spy callback registration

Phase 3: Object Registry (Depends on Hooks)
├── 3.1 Object tracking via hooks
├── 3.2 ID generation
└── 3.3 QPointer-based storage

Phase 4: Introspection Layer (Depends on Object Registry)
├── 4.1 Meta Inspector (property/method introspection)
├── 4.2 Widget Tree Builder
└── 4.3 Accessibility tree generation

Phase 5: Transport Layer (Depends on Introspection)
├── 5.1 JSON-RPC request/response handling
├── 5.2 Method dispatch routing
└── 5.3 Push notification support

Phase 6: Mode Handlers (Depends on Transport + Introspection)
├── 6.1 Native mode (full introspection)
├── 6.2 Computer Use mode (screenshot + coordinates)
└── 6.3 Chrome mode (a11y tree + refs)

Phase 7: Python MCP Server (Depends on WebSocket server being complete)
├── 7.1 WebSocket client
├── 7.2 MCP tool definitions
└── 7.3 Claude integration
```

**Key Dependencies:**
- Hooks Layer must exist before Object Registry can track objects
- Object Registry must exist before Introspection can query objects
- Introspection must exist before Mode Handlers can respond to requests
- WebSocket Server must be functional before Python MCP Server can connect

## Sources

### HIGH Confidence (Official Documentation / Direct Source)
- [GammaRay API Documentation](https://kdab.github.io/GammaRay/) - Probe class reference, signal spy callbacks
- [GammaRay Source Code](https://github.com/KDAB/GammaRay) - probe.cpp implementation
- [Qt QWebSocketServer Class](https://doc.qt.io/qt-6/qwebsocketserver.html) - Official API reference
- [QObject Internals Wiki](https://wiki.qt.io/QObject-Internals) - Connection lists, method indexing
- [Qat Architecture](https://qat.readthedocs.io/en/stable/doc/contributing/Architecture.html) - TCP/JSON communication pattern
- [MCP Python SDK](https://github.com/modelcontextprotocol/python-sdk) - FastMCP, transport patterns

### MEDIUM Confidence (WebSearch verified with secondary sources)
- [libjsonrpcwebsocketserver](https://github.com/gustavosbarreto/libjsonrpcwebsocketserver) - Qt JSON-RPC pattern
- [Qt Inspector](https://github.com/robertknight/Qt-Inspector) - LD_PRELOAD injection pattern
- [Qt Signal/Slot Internals](https://woboq.com/blog/how-qt-signals-slots-work.html) - Signal emission mechanism

### LOW Confidence (WebSearch only - requires validation)
- qt_register_signal_spy_callbacks specific behavior (needs testing against Qt 5.15/6.x)
- qtHookData stability across Qt minor versions (community consensus, but undocumented)
