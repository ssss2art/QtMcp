# Domain Pitfalls

**Domain:** Qt introspection/injection library
**Project:** QtMCP
**Researched:** 2026-01-29
**Confidence:** HIGH (verified against multiple authoritative sources)

---

## Critical Pitfalls

Mistakes that cause rewrites, crashes, or fundamental architectural problems.

---

### Pitfall 1: CRT Mismatch in Windows DLL Injection

**What goes wrong:** When injecting a DLL compiled with one version of the C Runtime into a Qt application compiled with a different CRT version, memory corruption occurs. Allocating memory with `msvcr110.dll!malloc` and freeing with `msvcr120.dll!free` causes heap corruption. STL containers crossing DLL boundaries crash or corrupt data silently.

**Why it happens:** Qt applications are typically built with specific MSVC versions. The probe DLL may be built with a different compiler/CRT. The C++ standard library detects mismatches via `#pragma detect_mismatch` but only at link time - runtime loading bypasses this check.

**Consequences:**
- Random crashes in memory allocation/deallocation
- Silent data corruption in containers
- Crashes that only appear under specific conditions
- Extremely difficult to debug (symptoms far from cause)

**Warning signs:**
- Crashes in `free()` or `delete` with corrupted heap messages
- Application works in Debug but crashes in Release (or vice versa)
- Crashes when returning strings or containers across DLL boundary
- Different behavior with static vs dynamic CRT linking

**Prevention:**
1. **Match build configurations exactly:** Build the probe DLL with the same MSVC version, CRT linkage (static vs dynamic), and Debug/Release configuration as the target Qt application
2. **Use C-style interfaces across DLL boundary:** Never pass STL containers, `std::string`, or allocated pointers across the DLL boundary
3. **Allocate and free in the same module:** If DLL allocates memory, DLL must also free it
4. **Consider static CRT linking:** Reduces version dependency but increases DLL size
5. **Document build requirements clearly:** Specify exact MSVC version in build docs

**Phase mapping:** Phase 1 (Foundation) - Must be addressed in CMake configuration and documented before any code is written.

**Sources:**
- [Microsoft Learn: Potential Errors Passing CRT Objects Across DLL Boundaries](https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries)

---

### Pitfall 2: TLS (Thread Local Storage) Initialization Failure in Injected DLLs

**What goes wrong:** DLLs using `__declspec(thread)` or `thread_local` variables fail catastrophically when loaded via `LoadLibrary()` rather than at process startup. The loader doesn't initialize TLS slots for dynamically loaded DLLs, causing one module to trample another module's thread-local storage.

**Why it happens:** On Windows (especially pre-Vista), the loader only allocates TLS slots for DLLs present at process creation. Dynamically loaded DLLs using implicit TLS get an uninitialized `_tls_index` of 0, causing them to corrupt the main executable's TLS data.

**Consequences:**
- One of the worst debugging scenarios: Module A corrupts Module B's state
- Crashes that appear random and unrelated to TLS
- Works in some scenarios, fails in others
- May work during testing but fail in production

**Warning signs:**
- Strange crashes after probe DLL is loaded
- Global state mysteriously changing
- Variables resetting to unexpected values
- Errors only occurring with certain threading patterns

**Prevention:**
1. **Avoid `thread_local` and `__declspec(thread)` in probe DLL entirely**
2. **Use explicit TLS APIs instead:** Windows FLS (Fiber Local Storage) API or `TlsAlloc()`/`TlsGetValue()`/`TlsSetValue()`
3. **Replace `std::call_once`** with `InitOnceBeginInitialize`/`InitOnceComplete`
4. **Audit all dependencies** for implicit TLS usage (some standard library features use it)
5. **Test injection scenarios specifically** - don't rely only on linked-at-startup tests

**Phase mapping:** Phase 1 (Foundation) - Audit code patterns before implementation; Phase 2 (Introspection) - Verify registry doesn't use TLS.

**Sources:**
- [Nynaeve: Thread Local Storage Design Problems](http://www.nynaeve.net/?p=187)
- [Microsoft Learn: Using TLS in a DLL](https://learn.microsoft.com/en-us/windows/win32/dlls/using-thread-local-storage-in-a-dynamic-link-library)
- [A guest in another process - remote thread crash](https://m417z.com/A-guest-in-another-process-a-story-of-a-remote-thread-crash/)

---

### Pitfall 3: DllMain Loader Lock Deadlock

**What goes wrong:** Performing forbidden operations inside `DllMain` causes deadlock or crash. The Windows loader holds a global lock during DLL initialization, and calling certain functions (LoadLibrary, thread creation, COM, etc.) from DllMain can deadlock the entire process.

**Why it happens:** `DllMain` runs under the loader lock. Many operations require the loader lock internally. If `DllMain` calls such an operation, deadlock occurs. This includes seemingly innocent operations.

**Consequences:**
- Process hangs during DLL load (no crash, just freeze)
- Intermittent hangs depending on thread timing
- Process appears frozen with no error message
- Difficult to diagnose without understanding loader lock

**Warning signs:**
- Application freezes when probe loads
- Hang on startup with no error
- Works sometimes, hangs other times
- Deadlock in debugger showing loader lock held

**Prevention:**
1. **Do minimal work in DllMain:** Only set flags, no real initialization
2. **Defer initialization:** Use a separate `Initialize()` function called after DLL load
3. **Never call from DllMain:**
   - `LoadLibrary()` / `LoadLibraryEx()`
   - `CreateThread()` or thread pool functions
   - Registry functions
   - COM functions (`CoInitialize`, etc.)
   - Synchronization primitives that wait
   - Any Qt functions (they may do forbidden things internally)
4. **Use constructor attribute carefully on Windows:** `__attribute__((constructor))` runs from DllMain context

**Phase mapping:** Phase 1 (Core Probe Skeleton) - DllMain implementation must be minimal.

**Sources:**
- [Microsoft Learn: Dynamic-Link Library Best Practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices)

---

### Pitfall 4: Qt Version Binary Incompatibility

**What goes wrong:** Probe DLL compiled against Qt 5.15.2 crashes or behaves incorrectly when loaded into application using Qt 5.15.3 or Qt 6.x. Even minor version differences can cause subtle ABI issues.

**Why it happens:** Qt maintains backwards binary compatibility within major versions but not forwards compatibility. An app built with Qt 5.15.3 may have symbols that don't exist in Qt 5.15.2 headers. Qt 5 and Qt 6 are completely ABI-incompatible.

**Consequences:**
- Crash on first Qt function call
- Undefined behavior with mismatched vtables
- "Missing symbol" errors on Linux
- Silent corruption with mismatched struct layouts

**Warning signs:**
- Immediate crash when probe loads into Qt app
- "undefined symbol" errors
- `QObject::metaObject()` returns garbage
- Version mismatch warnings in debug output

**Prevention:**
1. **Build probes for each Qt minor version** users might have
2. **Use dlopen/GetProcAddress** for Qt symbols instead of linking directly (advanced)
3. **Runtime version check:** Query `qVersion()` and refuse to initialize if mismatched
4. **Ship build documentation:** Clearly state required Qt version
5. **Consider header-only introspection** where possible to reduce ABI dependency
6. **Qt 6 requires separate build:** Cannot support both with same binary

**Phase mapping:** Phase 1 (Foundation) - Document Qt version requirements; Post-MVP - Consider multi-version support strategy.

**Sources:**
- [Qt Wiki: Qt Version Compatibility](https://wiki.qt.io/Qt-Version-Compatibility)
- [Qt Documentation: Porting to Qt 6](https://doc.qt.io/qt-6/portingguide.html)

---

### Pitfall 5: qtHookData Callback Thread Safety

**What goes wrong:** The `qtHookData[QHooks::AddQObject]` callback is invoked from the thread that creates the QObject. Without proper synchronization, the object registry data structures get corrupted when objects are created from multiple threads simultaneously.

**Why it happens:** Qt's hook mechanism calls the registered callback directly in the QObject constructor, on whatever thread is constructing the object. There's no built-in synchronization.

**Consequences:**
- Corrupted object registry hash tables
- Missing objects in registry
- Dangling pointers
- Random crashes during object lookup
- Race conditions that appear intermittently

**Warning signs:**
- Crashes in registry operations under multithreaded load
- Objects "disappearing" from registry
- Duplicate object IDs
- Inconsistent registry state

**Prevention:**
1. **Mutex-protect all registry operations:** Use `QMutex` or `std::mutex` around registry data structures
2. **Consider lock-free structures:** For high-frequency object creation scenarios
3. **Minimize work in callback:** Just record the pointer, defer processing
4. **Thread-local buffering:** Buffer objects per-thread, merge periodically
5. **Test with aggressive multithreading:** Create objects from many threads simultaneously

**Phase mapping:** Phase 2 (Object Registry) - Thread safety must be designed in from the start.

**Sources:**
- [Qt Documentation: Threads and QObjects](https://doc.qt.io/qt-6/threads-qobject.html)
- [Qt Source: QCoreApplication.cpp](https://github.com/qt/qtbase/blob/dev/src/corelib/kernel/qcoreapplication.cpp)

---

### Pitfall 6: QObject Parent-Thread Affinity Violation

**What goes wrong:** Attempting to set a parent on a QObject when the parent lives in a different thread causes crashes or warnings, and can lead to event delivery to destroyed objects.

**Why it happens:** Qt requires all QObjects in a parent-child relationship to have the same thread affinity. The child receives events on the parent's thread. Violating this invariant causes undefined behavior.

**Consequences:**
- "QObject::setParent: Cannot set parent, new parent is in a different thread" warning
- Segfaults on mouse/keyboard events
- Events delivered to wrong thread
- Crashes during object destruction

**Warning signs:**
- Warning messages about thread affinity in debug output
- Crashes during UI interaction
- Objects not receiving events
- Destruction crashes

**Prevention:**
1. **Never parent objects across threads**
2. **Use `moveToThread()` carefully:** Can only push, not pull; object cannot have a parent
3. **Create worker objects without parents:** Use explicit deletion or `deleteLater()`
4. **Verify thread affinity before operations:** Check `obj->thread()` before parenting
5. **Use signals/slots for cross-thread communication:** Not direct function calls

**Phase mapping:** Phase 2 (Object Registry), Phase 3 (Signal Monitoring) - Must understand thread context of all operations.

**Sources:**
- [KDAB: The Eight Rules of Multithreaded Qt](https://www.kdab.com/the-eight-rules-of-multithreaded-qt/)
- [Qt Documentation: Threads and QObjects](https://doc.qt.io/qt-5/threads-qobject.html)

---

## Moderate Pitfalls

Mistakes that cause delays, debugging sessions, or technical debt.

---

### Pitfall 7: LD_PRELOAD Blocked by -Bsymbolic-functions

**What goes wrong:** On Linux, symbol interposition via `LD_PRELOAD` fails when the Qt libraries are built with `-Bsymbolic-functions` linker flag. The preloaded library's symbols are ignored.

**Why it happens:** The `-Bsymbolic-functions` flag tells the linker to resolve function references within the library itself, bypassing the normal symbol lookup that would find `LD_PRELOAD` overrides.

**Warning signs:**
- `LD_PRELOAD` hooks not being called
- Probe loads but doesn't intercept expected functions
- Works on some distros but not others

**Prevention:**
1. **Don't rely on symbol interposition for core functionality**
2. **Use Qt's hook mechanism (qtHookData) instead** for object tracking
3. **Check if target Qt uses -Bsymbolic-functions:** `readelf -d libQt5Core.so | grep SYMBOLIC`
4. **Use `--dynamic-list` whitelist** if you must intercept specific symbols

**Phase mapping:** Phase 1 (Linux Injector) - Design injection strategy that doesn't rely on symbol interposition.

**Sources:**
- [Qt Wiki: Performance Tip Startup Time](https://wiki.qt.io/Performance_Tip_Startup_Time)

---

### Pitfall 8: QSignalSpy Uses Direct Connection (Thread Unsafe)

**What goes wrong:** `QSignalSpy` internally uses `Qt::DirectConnection`, which means the spy's slot runs in the emitting thread. When spying on signals from other threads, this causes race conditions and "timers cannot be stopped from another thread" warnings.

**Why it happens:** `QSignalSpy` was designed for unit testing single-threaded scenarios. Its direct connection means the spy is invoked in whatever thread emits the signal.

**Warning signs:**
- Warnings about timers and threads
- Race conditions in signal argument capture
- Test hangs with `QSignalSpy::wait()`
- Inconsistent signal capture results

**Prevention:**
1. **Don't use QSignalSpy for production monitoring** - implement your own signal spy
2. **Use `Qt::QueuedConnection` explicitly** for cross-thread signal monitoring
3. **Protect spy data with mutex** when implementing custom monitoring
4. **Consider signal proxy objects** that live in the right thread

**Phase mapping:** Phase 3 (Signal Monitoring) - Custom implementation required, don't use QSignalSpy.

**Sources:**
- [Qt Development Mailing List: Why is QSignalSpy using Qt::DirectConnection?](https://development.qt-project.narkive.com/BYQwNSvQ/why-is-qsignalspy-using-qt-directconnection)
- [Qt Documentation: QSignalSpy](https://doc.qt.io/qt-6/qsignalspy.html)

---

### Pitfall 9: WebSocket Blocking the Qt Event Loop

**What goes wrong:** Large WebSocket message processing blocks the Qt event loop, causing UI freezes in the target application. The probe becomes a performance problem for the app it's monitoring.

**Why it happens:** `QWebSocket` message handlers run on the main event loop by default. Processing large JSON payloads (like full object trees) synchronously blocks all Qt event processing.

**Warning signs:**
- Target app becomes sluggish when probe is active
- UI freezes during introspection operations
- Mouse/keyboard input delayed
- Warnings about slow event processing

**Prevention:**
1. **Process WebSocket messages asynchronously:** Post work to a separate thread
2. **Chunk large responses:** Don't send entire object tree at once
3. **Use incremental/streaming JSON:** For large payloads
4. **Profile event loop latency:** Monitor `QTimer` precision as indicator
5. **Consider separate WebSocket thread** with `moveToThread()`

**Phase mapping:** Phase 1 (WebSocket Server) - Architecture decision; Phase 8 (Testing) - Performance testing.

**Sources:**
- [Qt Wiki: Threads Events QObjects](https://wiki.qt.io/Threads_Events_QObjects)
- [QTBUG-63719: QML WebSockets blocking main thread](https://github.com/ChALkeR/qmlwebsockets_threaded)

---

### Pitfall 10: QMetaObject::invokeMethod Object Lifetime

**What goes wrong:** Using `QMetaObject::invokeMethod` with `Qt::QueuedConnection` on an object that gets deleted before the event is processed causes a crash or accesses freed memory.

**Why it happens:** The queued call posts an event that references the object. If the object is deleted before the event processes, the reference becomes dangling.

**Warning signs:**
- Crashes in `qt_static_metacall`
- "Pure virtual method called" errors
- Crashes during application shutdown
- Intermittent crashes during rapid object creation/destruction

**Prevention:**
1. **Use `QPointer` to guard object references** in async operations
2. **Check object validity before invoking:** Don't queue calls to soon-to-be-deleted objects
3. **Use shared pointers** for objects passed between threads
4. **Implement clean shutdown:** Ensure all pending events processed before object deletion
5. **Consider `deleteLater()`** to ensure event queue drains first

**Phase mapping:** Phase 3 (Native Mode) - Method invocation implementation; Phase 5 (Chrome Mode) - Click/interact actions.

**Sources:**
- [Qt Forum: QMetaObject::invokeMethod thread safety](https://forum.qt.io/topic/97683/thread-safety-of-qmetaobject-invokemethod-e-g-slot-invoking-from-random-threads)
- [Qt Forum: invokeMethod crash](https://forum.qt.io/topic/28456/qmetaobject-invokemethod-crash)

---

### Pitfall 11: Static Initialization Order Fiasco with Qt MetaObjects

**What goes wrong:** Accessing `staticMetaObject` from static initializers causes crashes because the meta object data may not be initialized yet.

**Why it happens:** C++ doesn't guarantee initialization order of static objects across translation units. Qt's moc-generated `staticMetaObject` may not be initialized when your static object tries to use `QObject::connect()`.

**Warning signs:**
- Crash in `staticMetaObject.className()` returning garbage
- Crash during static object construction
- Works on some compilers/platforms but not others
- Works in Release but crashes in Debug (or vice versa)

**Prevention:**
1. **Never use `QObject::connect()` in static initializers**
2. **Defer initialization to main() or later**
3. **Use construct-on-first-use idiom** for global objects
4. **Avoid global QObject instances entirely**
5. **Initialize probe from `DllMain`/constructor attribute,** not from static constructors

**Phase mapping:** Phase 1 (Foundation) - Avoid static QObjects in probe design.

**Sources:**
- [Qt Bug: QTBUG-62693 - staticMetaObject initialization](https://bugreports.qt.io/browse/QTBUG-62693)
- [Qt Forum: Crash in static object constructor](https://forum.qt.io/topic/16201/solved-crash-qobject-connect-in-a-constructor-of-a-static-object-instance)

---

### Pitfall 12: QML Introspection Requires Private Qt APIs

**What goes wrong:** Accessing QML internals (scene graph, private properties, internal types) requires Qt private headers that change without notice between versions.

**Why it happens:** Qt's QML implementation keeps many internals private. Tools like GammaRay must use `Qt::Qml_PRIVATE` targets which have no stability guarantees.

**Warning signs:**
- Compilation errors after Qt patch update
- Missing symbols at runtime
- Different behavior between Qt versions
- "Private API" warnings

**Prevention:**
1. **Stick to public QML APIs where possible:** `QQmlEngine`, `QQmlContext`, `QQmlProperty`
2. **Isolate private API usage:** Wrap in version-checked code
3. **Document Qt version requirements precisely**
4. **Test against multiple Qt patch versions**
5. **Accept limited QML introspection for MVP:** Full introspection requires private APIs

**Phase mapping:** Post-MVP - QML support is explicitly out of MVP scope; document limitations.

**Sources:**
- [Qt Blog: QML Type Compilation](https://www.qt.io/blog/qml-type-compilation)
- [KDAB GammaRay](https://www.kdab.com/software-technologies/developer-tools/gammaray/)

---

## Minor Pitfalls

Mistakes that cause annoyance but are quickly fixable.

---

### Pitfall 13: Qt::AutoConnection Race During Object moveToThread

**What goes wrong:** Connecting a signal with `Qt::AutoConnection` (the default) then calling `moveToThread()` can result in unexpected connection types, as the connection type is determined at emit time, not connect time.

**Prevention:**
1. **Be explicit about connection type** when cross-thread communication is involved
2. **Establish connections after `moveToThread()`** when possible
3. **Prefer `Qt::QueuedConnection` explicitly** for thread-crossing signals

**Phase mapping:** Phase 3 (Signal Monitoring) - Be explicit in connection types.

---

### Pitfall 14: qRegisterMetaType Not Called for Signal Arguments

**What goes wrong:** Queued signal connections with custom types fail silently or crash because the type isn't registered with Qt's meta-type system.

**Prevention:**
1. **Call `qRegisterMetaType<CustomType>()` at startup** for all custom signal argument types
2. **Use `Q_DECLARE_METATYPE(CustomType)`** in headers
3. **Check return value of connections** in debug builds

**Phase mapping:** Phase 3 (Signal Monitoring) - Register types before monitoring signals.

---

### Pitfall 15: QWebSocket Moved to Different Thread Without Event Loop

**What goes wrong:** Moving `QWebSocket` to a worker thread but forgetting to start an event loop in that thread causes the socket to never process any events.

**Prevention:**
1. **Always call `exec()` in worker thread's `run()`** if using QObjects there
2. **Or use `moveToThread()` with objects that run their own event processing**
3. **Verify with simple ping/pong test** that communication works after thread setup

**Phase mapping:** Phase 1 (WebSocket Server) - If using threaded WebSocket, ensure event loop.

---

## Phase-Specific Warning Summary

| Phase | Topic | Likely Pitfall | Mitigation |
|-------|-------|---------------|------------|
| Phase 1 | DllMain | Loader lock deadlock | Minimal DllMain, defer init |
| Phase 1 | TLS | TLS corruption in injected DLL | Use FLS API, avoid thread_local |
| Phase 1 | CRT | CRT version mismatch | Match build config, C interfaces |
| Phase 1 | WebSocket | Event loop blocking | Async processing, chunking |
| Phase 2 | Registry | Thread safety in AddQObject hook | Mutex protection from start |
| Phase 2 | Registry | Object lifetime tracking | QPointer everywhere |
| Phase 3 | Signals | QSignalSpy thread issues | Custom spy implementation |
| Phase 3 | Signals | Connection type races | Explicit Qt::QueuedConnection |
| Phase 4 | Computer Use | High-DPI screenshot issues | Device pixel ratio handling |
| Phase 5 | Chrome Mode | Object destruction during action | QPointer, validity checks |
| All | Qt Version | Binary compatibility | Version checking at init |

---

## Sources

### Authoritative (HIGH confidence)
- [Microsoft Learn: CRT Object Boundaries](https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries)
- [Microsoft Learn: DLL Best Practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices)
- [Qt Documentation: Threads and QObjects](https://doc.qt.io/qt-6/threads-qobject.html)
- [Qt Wiki: Qt Version Compatibility](https://wiki.qt.io/Qt-Version-Compatibility)
- [KDAB: Eight Rules of Multithreaded Qt](https://www.kdab.com/the-eight-rules-of-multithreaded-qt/)
- [Qt Documentation: QMetaObject](https://doc.qt.io/qt-6/qmetaobject.html)

### Community/Analysis (MEDIUM confidence)
- [Nynaeve: TLS Design Problems](http://www.nynaeve.net/?p=187)
- [Woboq: How Qt Signals Work](https://woboq.com/blog/how-qt-signals-slots-work-part3-queuedconnection.html)
- [Qt Forum discussions on invokeMethod thread safety](https://forum.qt.io/topic/97683/thread-safety-of-qmetaobject-invokemethod-e-g-slot-invoking-from-random-threads)
- [KDAB GammaRay implementation insights](https://github.com/KDAB/GammaRay)
