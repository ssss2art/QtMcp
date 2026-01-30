# Project Research Summary

**Project:** QtMCP
**Domain:** Qt introspection/injection library with AI automation
**Researched:** 2026-01-29
**Confidence:** HIGH

## Executive Summary

QtMCP is an injection library that enables AI assistants (specifically Claude) to control Qt applications at runtime via the Model Context Protocol. This is a niche domain combining Qt introspection (like GammaRay), test automation patterns (like Squish), and AI tooling (via MCP). The recommended approach follows established patterns from GammaRay and Qt-Inspector: a C++ probe injected via DLL/LD_PRELOAD, using Qt's internal hooks (qtHookData, signal spy callbacks) for object tracking, communicating with a Python MCP server via WebSocket/JSON-RPC.

The core technical challenge is Windows DLL injection stability. Critical pitfalls include CRT mismatch, TLS initialization failures, and DllMain loader lock deadlocks - all of which can cause catastrophic failures that are difficult to debug. The architecture must prioritize thread safety (qtHookData callbacks run on any thread), Qt event loop integration (probe must never block the target app), and careful object lifetime management (QPointer for everything). Linux injection via LD_PRELOAD is straightforward by comparison.

The roadmap should proceed foundation-first: establish injection + WebSocket transport, then build introspection layers (Object Registry -> Meta Inspector -> Mode Handlers). The three API modes (Native/Computer Use/Chrome) can be developed in parallel once core introspection exists. QML support is complex and should be deferred post-MVP. Stack is mature: Qt 5.15 LTS + C++17, Qt WebSockets, official MCP Python SDK. No exotic dependencies required.

## Key Findings

### Recommended Stack

QtMCP requires a carefully chosen stack balancing Qt integration, cross-platform support (Windows + Linux), and minimal dependencies. The core constraint is Qt 5.15 LTS as primary target with C++17, while the Python MCP server uses the official Anthropic SDK.

**Core technologies:**
- **Qt 5.15 LTS / Qt 6.3+** (C++17): Primary framework - Qt 5.15 is industry LTS standard supported until 2025+, most deployed Qt apps use 5.15. GammaRay targets 5.15+. Qt 6 requires C++17 minimum.
- **Qt WebSockets**: WebSocket server in probe - native Qt integration, no external deps, signal/slot based. Preferred over websocketpp (requires Boost), Boost.Beast (heavyweight), or libwebsockets (C library, poor Qt integration).
- **QJsonDocument**: JSON-RPC parsing - part of Qt Core, zero external deps, sufficient for protocol needs. Only consider nlohmann/json if JSON manipulation becomes unwieldy.
- **vcpkg manifest mode**: Dependency management - cross-platform, CMake integration, tracks deps in git. Do NOT include Qt in vcpkg (slow builds, use official Qt installer).
- **CMake 3.21+ with presets**: Build system - CMakePresets.json support (matured 3.21), better Qt6 integration, reliable toolchain file handling.
- **Python 3.10+ with official MCP SDK 1.26.0**: MCP server - requires Python 3.10+, production-ready v1.x. Use official SDK not FastMCP 2.0/3.0 (unnecessary complexity).
- **websockets 16.0**: WebSocket client in Python - asyncio-native, simpler than aiohttp for WebSocket-only use case.
- **Qt Test**: Testing framework - native signal/slot testing, QSignalSpy, Qt Creator integration, zero config. Catch2 optional for pure C++ utilities.

**Critical version requirements:**
- MSVC 2022 (17.8+) on Windows for Qt 6.10 support
- GCC 9+ on Linux (GCC 11+ recommended for C++17 conformance)
- Never mix Qt 5 and Qt 6 binaries (completely ABI-incompatible)
- Match CRT version between probe DLL and target app on Windows

**What NOT to use:**
- Boost (heavyweight, Qt provides equivalent)
- RapidJSON (performance overkill)
- websocketpp (requires Boost/ASIO)
- GoogleTest (Qt Test better for Qt-specific testing)
- FastMCP 2.0/3.0 (unnecessary complexity)
- Clang on Windows (MSVC better tested/debugged with Qt)
- C++20 (inconsistent compiler support)

### Expected Features

Qt introspection and automation tools span debugging (GammaRay), test automation (Squish, TestComplete), and lightweight testing (Qt Test). For QtMCP's AI automation use case, features must bridge introspection capabilities with MCP-compatible interfaces across three API modes.

**Must have (table stakes):**
- QObject tree traversal - every tool does this via QObject::children(), findChild()
- Property read/write - fundamental introspection, GammaRay/Squish all support
- Signal/slot inspection - must list connections, not just definitions
- Widget geometry access - required for any UI automation
- Screenshot capture - Computer Use mode requires this (QWidget::grab(), QQuickWindow)
- Mouse/keyboard event simulation - basic automation requirement (QTest patterns)
- Element identification by name - stable automation requires addressability (objectName)
- JSON-RPC transport - MCP protocol requirement
- Process injection/attachment - DLL/LD_PRELOAD like GammaRay, Squish, Qat
- Cross-platform support - Windows + Linux minimum, macOS nice-to-have
- QML/Qt Quick support - modern Qt apps use QML, can't be Widgets-only in 2026

**Should have (competitive differentiators):**
- Accessibility tree export (Chrome mode) - enables AI-native interaction via refs, matches Claude's browser API. Unique - no Qt tool exposes this format.
- MCP-native protocol - direct AI agent integration without translation layer. Primary differentiator.
- Three API modes (Native/Computer Use/Chrome) - flexibility for different AI capabilities. No competitor offers this versatility.
- Real-time object change notifications - AI can react to state changes. GammaRay has this; automation tools don't expose it.
- Model/View data access - navigate QAbstractItemModel programmatically. GammaRay has this; critical for data-driven apps.
- Coordinate-to-element mapping - click coords -> QObject for Computer Use mode. Required for Computer Use; not in introspection tools.

**Defer (v2+):**
- State machine visualization - GammaRay exclusive feature, complex, niche
- Layout diagnostic overlay - visual debugging, useful but not essential
- Semantic element descriptions - AI-friendly descriptions beyond raw properties
- Action recording for learning - Squish does this for tests, novel for AI learning
- Async operation support - long-running AI commands with progress
- QML context/binding access - GammaRay-level depth, requires private APIs
- Multi-process support - enterprise feature

**Anti-features (explicitly avoid):**
- Image-based element recognition - brittle, slow, unreliable. Use Qt's native object model and accessibility APIs.
- Coordinate-only automation - breaks on resize/DPI/layouts. Use object references; coords as fallback.
- Recording-only test creation - Squish pain point, hard to maintain. Expose structured API.
- Blocking synchronous API - Qt apps freeze, AI agents timeout. Use async JSON-RPC.
- Custom scripting language - maintenance burden. Use JSON-RPC, let AI use native reasoning.
- Widget-only support - modern Qt is QML-heavy. Support both from day one.
- Relying on objectName everywhere - many apps don't set names. Multiple identification strategies.

### Architecture Approach

QtMCP follows established architecture patterns from GammaRay, Qt-Inspector, and Qat: separate into four primary components with clear boundaries. Python MCP Server (MCP protocol handling, separate process) -> WebSocket Client (async communication) -> WebSocket Server (C++, in-target process) -> JSON-RPC Handler (request parsing, method dispatch) -> Mode Handlers (Native/ComputerUse/Chrome logic) -> Introspection Layer (Object Registry, Meta Inspector, Tree Builder) -> Hooks Layer (qtHookData, signal spy callbacks) -> Qt Application Objects.

**Major components:**
1. **Python MCP Server** - MCP protocol handling, tool exposure to Claude, WebSocket client for probe communication. Separate process.
2. **C++ Probe (injected DLL/SO)** - Runs in-target process. Contains WebSocket server, JSON-RPC handler, mode handlers, introspection layer, hooks layer.
3. **Object Registry** - QObject lifecycle tracking via qtHookData callbacks, hierarchical ID assignment, QPointer-based storage. Foundation for all introspection.
4. **Introspection Layer** - Meta Inspector (QMetaObject introspection), Widget Tree Builder (hierarchy traversal), Accessibility Tree Generator. Queries Qt objects on behalf of Mode Handlers.
5. **Hooks Layer** - qtHookData (AddQObject/RemoveQObject callbacks), signal spy callbacks (qt_register_signal_spy_callbacks). Interface to Qt internals.

**Key patterns:**
- **Qt Event Loop Integration** - All probe operations must integrate with target app's event loop, never block it. WebSocket handlers use signal/slot, QMetaObject::invokeMethod with Qt::QueuedConnection.
- **qtHookData Callbacks** - Use Qt's internal hook array to receive callbacks for every QObject creation/destruction. Only reliable way to know about ALL objects.
- **Signal Spy Callbacks** - Use qt_register_signal_spy_callbacks to monitor all signal emissions. Standard Qt internal API used by GammaRay.
- **QPointer for Safe References** - Always use QPointer to hold references to tracked QObjects. Automatically nullifies when object destroyed.
- **Thread-Safe Communication** - Use Qt signal/slot with queued connections for thread safety. qtHookData callbacks run on creating thread.
- **Hierarchical ID Generation** - Generate stable, human-readable object IDs based on parent-child hierarchy. Debuggable and reflects actual Qt structure.

**Data flow:**
- Request: Claude -> MCP Server (stdio/HTTP) -> WebSocket Client (Python) -> WebSocket Server (C++) -> JSON-RPC Handler -> Mode Handler -> Introspection Layer -> Qt Objects -> Response back up chain
- Events: Qt signal fires -> Hooks Layer callback -> Object Registry updates -> JSON-RPC notification -> WebSocket push -> Python MCP Server (optionally to Claude)

### Critical Pitfalls

**Windows DLL injection pitfalls (CATASTROPHIC if ignored):**

1. **CRT Mismatch** - Injecting DLL compiled with different C Runtime version causes memory corruption. Allocating with msvcr110.dll!malloc, freeing with msvcr120.dll!free corrupts heap. Crashes in free(), delete, silent data corruption in containers. MUST match build configurations exactly (MSVC version, CRT linkage, Debug/Release). Never pass STL containers across DLL boundary. Allocate and free in same module. Document build requirements clearly. **Phase 1 Foundation**.

2. **TLS Initialization Failure** - DLLs using thread_local or __declspec(thread) fail catastrophically when loaded via LoadLibrary(). Loader doesn't initialize TLS slots for dynamically loaded DLLs. Module A tramples Module B's thread-local storage. One of the worst debugging scenarios. AVOID thread_local and __declspec(thread) in probe DLL entirely. Use explicit TLS APIs (FLS, TlsAlloc/Get/SetValue). Replace std::call_once with InitOnceBeginInitialize. **Phase 1 Foundation**.

3. **DllMain Loader Lock Deadlock** - Performing forbidden operations inside DllMain causes deadlock. Loader holds global lock during DLL init. Calling LoadLibrary, CreateThread, COM, registry, sync primitives from DllMain deadlocks entire process. Process hangs during DLL load with no error. Do MINIMAL work in DllMain. Defer initialization. Never call LoadLibrary, CreateThread, COM, registry, sync waits, or Qt functions from DllMain. **Phase 1 Core Probe Skeleton**.

**Qt-specific pitfalls:**

4. **Qt Version Binary Incompatibility** - Probe compiled against Qt 5.15.2 crashes with Qt 5.15.3 or Qt 6. Even minor version differences cause ABI issues. Qt 5 and Qt 6 completely ABI-incompatible. Crash on first Qt function call, missing symbols on Linux, garbage from metaObject(). Build probes for each Qt minor version. Runtime version check via qVersion(). Ship build documentation. Qt 6 requires separate build. **Phase 1 Foundation**.

5. **qtHookData Callback Thread Safety** - qtHookData[QHooks::AddQObject] callback invoked from thread creating the QObject. Without synchronization, object registry corrupts when objects created from multiple threads simultaneously. Corrupted hash tables, missing objects, dangling pointers, race conditions. Mutex-protect ALL registry operations from start. Consider lock-free structures. Minimize work in callback. **Phase 2 Object Registry**.

6. **QObject Parent-Thread Affinity Violation** - Setting parent on QObject when parent lives in different thread causes crashes. Qt requires parent-child same thread affinity. "Cannot set parent, new parent is in different thread" warning. Segfaults on events. Never parent objects across threads. Use moveToThread() carefully (can only push, not pull; object cannot have parent). **Phase 2 Object Registry, Phase 3 Signal Monitoring**.

**Other notable pitfalls:**
- LD_PRELOAD blocked by -Bsymbolic-functions on Linux - don't rely on symbol interposition, use qtHookData instead
- QSignalSpy uses DirectConnection (thread unsafe) - custom implementation required, don't use QSignalSpy for production
- WebSocket blocking Qt event loop - process messages asynchronously, chunk large responses
- QMetaObject::invokeMethod object lifetime - use QPointer to guard references in async operations
- QML introspection requires private Qt APIs - stick to public APIs where possible, accept limitations for MVP

## Implications for Roadmap

Based on research, suggested phase structure follows dependency order discovered in architecture:

### Phase 1: Foundation (Injection + Transport)
**Rationale:** Nothing works without getting code into target process and establishing communication. Windows injection is the highest technical risk (CRT mismatch, TLS, DllMain deadlocks) so must be addressed first with proper architecture.

**Delivers:**
- Working injection on Windows (DLL) and Linux (LD_PRELOAD)
- Basic WebSocket server in probe responding to ping/pong
- Python MCP server skeleton connecting to probe
- CMake/vcpkg build system with proper CRT configuration

**Addresses:**
- Process injection (table stakes feature)
- JSON-RPC transport (table stakes feature)
- Cross-platform support (table stakes feature)

**Avoids:**
- CRT mismatch (CRITICAL - Phase 1 pitfall)
- TLS initialization failure (CRITICAL - Phase 1 pitfall)
- DllMain loader lock deadlock (CRITICAL - Phase 1 pitfall)

**Research flag:** Standard patterns documented, skip research-phase.

### Phase 2: Object Registry (Hooks + Tracking)
**Rationale:** Foundation for all introspection. Must exist before any introspection queries can work. qtHookData callbacks provide only reliable way to track ALL objects including those created before probe init.

**Delivers:**
- qtHookData integration (AddQObject/RemoveQObject callbacks)
- Object Registry with thread-safe tracking
- Hierarchical ID generation
- QPointer-based storage

**Addresses:**
- QObject tree traversal (table stakes)
- Element identification by name (table stakes)

**Avoids:**
- qtHookData callback thread safety (CRITICAL - Phase 2 pitfall)
- QObject parent-thread affinity violation (CRITICAL - Phase 2 pitfall)

**Uses:**
- Qt Core meta-object system (from STACK.md)
- QPointer pattern (from ARCHITECTURE.md)

**Research flag:** Standard patterns in GammaRay source, skip research-phase.

### Phase 3: Native Mode (Core Introspection)
**Rationale:** Primary differentiator and most useful for capable AI agents. Builds on Object Registry to provide full Qt introspection. Enables basic automation before visual modes.

**Delivers:**
- Meta Inspector (property/method introspection via QMetaObject)
- Signal/slot inspection with signal spy callbacks
- Widget geometry access
- Mouse/keyboard event simulation (QTest patterns)
- Native Mode JSON-RPC API

**Addresses:**
- Property read/write (table stakes)
- Signal/slot inspection (table stakes)
- Widget geometry access (table stakes)
- Mouse/keyboard simulation (table stakes)
- Real-time object change notifications (differentiator)

**Avoids:**
- Signal spy thread safety issues (custom implementation, not QSignalSpy)
- Event loop blocking (async processing)

**Implements:**
- Introspection Layer (ARCHITECTURE.md)
- Mode Handlers (ARCHITECTURE.md)

**Research flag:** Skip research-phase. Well-documented Qt APIs and GammaRay patterns.

### Phase 4: Computer Use Mode
**Rationale:** Second API mode, simpler than Chrome mode. Required for visual AI agents. Builds on existing introspection for coordinate-to-element mapping.

**Delivers:**
- Screenshot capture (QWidget::grab, QQuickWindow)
- Coordinate-to-element hit testing
- Computer Use Mode JSON-RPC API

**Addresses:**
- Screenshot capture (table stakes)
- Coordinate-to-element mapping (differentiator)

**Uses:**
- Qt GUI module (from STACK.md)
- Widget Tree Builder (from ARCHITECTURE.md)

**Research flag:** Skip research-phase. Standard Qt APIs well-documented.

### Phase 5: Chrome Mode (Accessibility Tree)
**Rationale:** Third API mode, highest complexity due to accessibility tree generation. Primary differentiator - matches Claude's browser automation API. Can be developed in parallel with Phase 4 if resources allow.

**Delivers:**
- Accessibility tree export (QAccessible)
- Ref-based element interaction
- Chrome Mode JSON-RPC API

**Addresses:**
- Accessibility tree export (differentiator)
- MCP-native protocol (differentiator)
- Three API modes (differentiator)

**Avoids:**
- Object destruction during action (QPointer validity checks)

**Research flag:** NEEDS research-phase. QAccessible hierarchy exposure is novel, no existing examples. Need to research QAccessibleInterface API patterns, tree structure mapping to Chrome accessibility tree format, ref stability guarantees.

### Phase 6: QML Support (Deferred)
**Rationale:** Modern Qt apps heavily use QML, but requires private Qt APIs and adds significant complexity. Defer to post-MVP to validate core value first.

**Delivers:**
- QML item tree traversal (QQuickItem)
- QML property access
- QML context/binding inspection (limited without private APIs)

**Addresses:**
- QML/Qt Quick support (table stakes - but deferred)

**Avoids:**
- QML introspection requires private Qt APIs (MODERATE pitfall)

**Research flag:** NEEDS research-phase. Private API usage patterns, version compatibility strategy, which features are possible with public APIs only.

### Phase Ordering Rationale

**Dependency-driven order:**
1. Foundation MUST exist before any other phase (nothing works without injection + transport)
2. Object Registry MUST exist before introspection (hooks track objects)
3. Introspection MUST exist before Mode Handlers (modes query introspection layer)
4. Mode Handlers can be developed in parallel once registry + basic introspection exist

**Risk-driven order:**
- Windows injection risks addressed first (highest technical risk per pitfalls research)
- Thread safety designed into registry from start (critical pitfall)
- Visual modes (Computer Use, Chrome) after core introspection working (validate value early)

**Value-driven order:**
- Native Mode first (primary differentiator, most useful for AI)
- Computer Use second (simpler than Chrome, immediate value)
- Chrome Mode third (complex accessibility tree, research needed)
- QML deferred (complex, requires private APIs, validate core value first)

### Research Flags

**Phases needing deeper research during planning:**
- **Phase 5 (Chrome Mode)**: QAccessible tree structure mapping to Chrome accessibility tree format is novel. No existing examples of exporting Qt accessibility as Chrome-compatible tree. Need to research: QAccessibleInterface API patterns, tree traversal strategies, ref stability guarantees, which Qt accessibility features map to Chrome roles/states/properties.
- **Phase 6 (QML Support)**: Private API usage requires version compatibility strategy. Need to research: which QML introspection features are possible with public APIs only (QQmlEngine, QQmlContext, QQmlProperty), which require private headers (Qt::Qml_PRIVATE), how GammaRay handles QML version compatibility, fallback strategies when private APIs change.

**Phases with standard patterns (skip research-phase):**
- **Phase 1 (Foundation)**: DLL injection patterns well-documented in GammaRay, kubo/injector. WebSocket + JSON-RPC are standard protocols.
- **Phase 2 (Object Registry)**: qtHookData usage directly observable in GammaRay source code. Qt threading patterns well-documented.
- **Phase 3 (Native Mode)**: QMetaObject introspection is public Qt API with extensive documentation. Signal spy callbacks used by GammaRay with available source.
- **Phase 4 (Computer Use Mode)**: QWidget::grab() and hit testing are standard Qt APIs with clear documentation.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All technologies verified against official docs and current releases. Qt 5.15 LTS + Qt WebSockets + official MCP SDK are mature, production-ready choices. vcpkg + CMake patterns well-established. |
| Features | HIGH | Table stakes features consistent across all researched tools (GammaRay, Squish, Qt Test). Differentiators (MCP protocol, three API modes) are sound based on Claude's existing browser automation patterns. |
| Architecture | HIGH | Follows proven patterns from GammaRay (probe injection, qtHookData hooks, signal spy callbacks) and Qat (WebSocket/JSON-RPC transport). Component boundaries match successful Qt introspection tools. |
| Pitfalls | HIGH | Critical Windows pitfalls (CRT mismatch, TLS, DllMain) well-documented in Microsoft Learn. Qt threading pitfalls documented in KDAB Eight Rules and Qt official docs. GammaRay source code demonstrates successful mitigation. |

**Overall confidence:** HIGH

Research drew from authoritative sources: Qt official documentation, Microsoft Learn, GammaRay source code and API docs, MCP Python SDK, Squish documentation. Architecture patterns validated against multiple successful Qt introspection tools (GammaRay, Qt-Inspector, Qat). Stack choices verified against official platform support matrices and current releases.

### Gaps to Address

**Chrome Mode accessibility tree mapping:** QAccessible API is well-documented, but mapping to Chrome's accessibility tree format is novel. No existing examples found. Will need experimentation during Phase 5 to determine: exact role/state/property mappings, how to generate stable refs, whether QAccessibleInterface hierarchy matches expectations. Consider consulting Chrome DevTools Protocol documentation during Phase 5 planning.

**Qt version compatibility strategy for deployment:** Research established probe must match target app's Qt version (ABI incompatibility), but deployment strategy unclear. Gap: how to detect target app's Qt version before injection? How to ship multiple probe binaries? Consider: environment variable override, version detection launcher, runtime Qt version query before probe init. Address during Phase 1 detailed planning.

**QML support feasibility with public APIs only:** Research identified QML introspection requires private Qt APIs for deep features. Gap: which features are actually achievable with public APIs? Can we provide useful QML introspection without private headers? Consider: documenting "QML Lite" feature set using only public APIs, clear documentation of limitations, user feedback to validate if public API subset provides sufficient value. Validate during Phase 6 research-phase.

**Performance impact on target application:** Probe runs in-target process with qtHookData callbacks on every object creation/destruction, signal spy on every signal. Gap: what is actual performance overhead in large Qt applications? Will it be acceptable? Consider: profiling with real-world Qt apps during Phase 2-3, implementing toggleable monitoring, lazy introspection loading. Add performance testing task to Phase 3.

## Sources

### Primary (HIGH confidence)
- STACK.md: Qt 6.10 supported platforms, Qt WebSockets docs, MCP Python SDK PyPI, vcpkg CMake integration, GammaRay architecture
- FEATURES.md: GammaRay GitHub and user manual, Qt accessibility docs, Qt Test overview, Squish object identification docs, QWidget::grab docs
- ARCHITECTURE.md: GammaRay API docs and source code, Qt QWebSocketServer class, QObject internals wiki, Qat architecture docs, MCP Python SDK
- PITFALLS.md: Microsoft Learn (CRT objects, DLL best practices, TLS in DLL), Qt docs (threads and QObjects, Qt version compatibility), KDAB Eight Rules of Multithreaded Qt

### Secondary (MEDIUM confidence)
- STACK.md: Qt Wiki supported C++ versions, kubo/injector library, Qat architecture reference
- FEATURES.md: Squish product page, MCP protocol specification, Qt GUI testing pitfalls blog
- ARCHITECTURE.md: libjsonrpcwebsocketserver, Qt Inspector LD_PRELOAD pattern, Qt signal/slot internals (Woboq blog)
- PITFALLS.md: Nynaeve TLS design problems, Woboq signals blog, Qt forums (invokeMethod, signal spy)

### Tertiary (LOW confidence)
- FEATURES.md: Test automation anti-patterns (general patterns, not Qt-specific)
- ARCHITECTURE.md: qt_register_signal_spy_callbacks specific behavior needs testing
- PITFALLS.md: qtHookData stability across Qt minor versions (community consensus, undocumented)

---
*Research completed: 2026-01-29*
*Ready for roadmap: yes*
