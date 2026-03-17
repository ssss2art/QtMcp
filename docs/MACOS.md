# macOS Support for qtPilot

Status of macOS platform support, implementation notes, and known issues.

## Implementation Summary

macOS support was added by creating platform-specific files for probe initialization and launcher injection, and updating CMake, Python, and C++ code to handle macOS framework layouts and environment variables.

### Files Created
- `src/probe/core/probe_init_macos.cpp` — `DYLD_INSERT_LIBRARIES` constructor/destructor + `Q_COREAPP_STARTUP_FUNCTION`
- `src/launcher/injector_macos.cpp` — Fork/exec with `DYLD_INSERT_LIBRARIES` (colon-separated), SIP path warnings

### Files Modified
- `CMakeLists.txt` — Removed macOS warning, enabled launcher, added `-Wno-variadic-macro-arguments-omitted`
- `src/probe/CMakeLists.txt` — Added `APPLE` branch for probe init
- `src/launcher/CMakeLists.txt` — Added `APPLE` branch for injector, framework-aware Qt prefix detection
- `src/launcher/injector_linux.cpp` — Narrowed guard from `#ifndef Q_OS_WIN` to `#if defined(Q_OS_LINUX)`
- `src/launcher/main.cpp` — Added `.dylib` glob patterns for probe discovery
- `src/launcher/qt_env_setup.cpp` — Platform-aware Qt detection (frameworks, `libqcocoa.dylib`, `DYLD_LIBRARY_PATH`)
- `python/src/qtpilot/qt_env.py` — macOS dylib/framework detection, `DYLD_LIBRARY_PATH` support
- `python/src/qtpilot/download.py` — `darwin` platform mapping, `.dylib` extension
- `CMakePresets.json` — `macos-debug` and `macos-release` presets

### Build & Test Fixes for Qt 6.10 + AppleClang
- `src/probe/api/chrome_mode_api.cpp` — Removed `QAccessible::Label` case (now a `RelationFlag` in Qt 6.10)
- `tests/test_object_registry.cpp` — Fixed lambda captures (`t` missing, `objectsPerThread` unnecessary)

## Building on macOS

```bash
# Configure (Qt installed via Qt Online Installer)
cmake -B build -DCMAKE_PREFIX_PATH=/Applications/Qt/6.10.0/macos

# Build
cmake --build build --config Release

# Run unit tests (headless, no display needed)
QT_QPA_PLATFORM=minimal QTPILOT_ENABLED=0 ctest --test-dir build --output-on-failure
```

All 16 tests pass on macOS ARM64 with Qt 6.10.0.

## Injection Mechanism

macOS uses `DYLD_INSERT_LIBRARIES` (analogous to Linux `LD_PRELOAD`):

```bash
DYLD_INSERT_LIBRARIES=/path/to/libqtPilot-probe-qt6.10.dylib \
QTPILOT_PORT=9222 \
/path/to/MyApp.app/Contents/MacOS/MyApp
```

The probe's `__attribute__((constructor))` runs before `main()`, and `Q_COREAPP_STARTUP_FUNCTION` triggers initialization once Qt is ready.

## macOS-Specific Limitations

### System Integrity Protection (SIP)

SIP strips `DYLD_INSERT_LIBRARIES` for binaries in protected paths:
- `/usr/`
- `/System/`
- `/bin/`
- `/sbin/`

**Only user-built or non-SIP-protected executables can be injected.** The injector warns when a target is in a protected path.

### Screen Recording Permission

On macOS 10.15+, `QWidget::grab()` requires the **Screen Recording** permission in System Settings > Privacy & Security. Without it:
- The grab returns a corrupt/empty pixmap
- `QImageWriter::write()` crashes with `EXC_BAD_ACCESS` in `_platform_memmove`
- The probe should use `CGPreflightScreenCaptureAccess()` (macOS 11+) to check before attempting capture

**Important:** The terminal app must be restarted after granting permission for it to take effect.

### App Bundle Resolution

macOS Qt apps are distributed as `.app` bundles. The launcher currently requires the bare executable path:
```
# Must specify the inner executable, not the .app
build/bin/qtPilot-launcher MyApp.app/Contents/MacOS/MyApp
```

Future improvement: accept `.app` paths and resolve via `Contents/Info.plist` `CFBundleExecutable`.

### Third-Party Library Paths

Real-world macOS apps may have rpaths that don't resolve when launched from a different working directory (e.g., worktree builds). Set `DYLD_LIBRARY_PATH` to include additional library directories:

```bash
DYLD_LIBRARY_PATH="/path/to/libs:/another/path" \
DYLD_INSERT_LIBRARIES=/path/to/probe.dylib \
./MyApp.app/Contents/MacOS/MyApp
```

## Qt Framework Layout

macOS Qt uses framework bundles rather than plain shared libraries:

| Component | Windows | Linux | macOS |
|-----------|---------|-------|-------|
| Core library | `bin/Qt6Core.dll` | `lib/libQt6Core.so` | `lib/QtCore.framework/` |
| Platform plugin | `plugins/platforms/qwindows.dll` | `plugins/platforms/libqxcb.so` | `plugins/platforms/libqcocoa.dylib` |
| Lib env var | `PATH` | `LD_LIBRARY_PATH` | `DYLD_LIBRARY_PATH` |
| Injection env var | N/A (DLL injection) | `LD_PRELOAD` | `DYLD_INSERT_LIBRARIES` |
| Probe extension | `.dll` | `.so` | `.dylib` |

Some Qt installations (e.g., Qt 6.9.3 WebEngine-only module) may not have a `bin/` directory at all — only `lib/` and `plugins/`.

## Testing Results (DaVinci CAD App, Qt 6.10)

Tested against a large real-world Qt application (~2300 library types, custom QMainWindow, multiple dock widgets).

**Worked:**
- Probe injection and WebSocket server startup
- `ping`, `echo`, `getVersion`, `getModes`
- `hitTest` — correctly resolved widgets at pixel coordinates
- `getObjectInfo`, `listProperties`, `listMethods`, `listSignals` — all worked with direct objectIds
- `getGeometry` — returned correct coordinates with devicePixelRatio=2 (Retina)

**macOS-specific crashes:**
- `screenshot` — crashes without Screen Recording permission (null pixmap not guarded)
- `findByClassName` — crashes due to stale pointers (not macOS-specific, but all repros were on macOS)
