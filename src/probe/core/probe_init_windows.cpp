// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

// Windows DLL entry point for QtMCP probe.
//
// CRITICAL: DllMain runs under the Windows loader lock. NEVER call Qt functions,
// create threads, load libraries, or do any "real" work in DllMain.
// All initialization is deferred until after the DLL load completes.
//
// Initialization paths:
// 1. Build-time linking / LD_PRELOAD: Q_COREAPP_STARTUP_FUNCTION fires automatically
// 2. Runtime injection: launcher calls exported qtmcpProbeInit() via CreateRemoteThread
//
// See: https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices

#ifdef _WIN32

#include "probe.h"

#include <Windows.h>
#include <synchapi.h>

namespace {

// One-time initialization using Windows InitOnce API.
// NEVER use std::call_once - it uses TLS internally on MSVC which breaks
// for dynamically loaded DLLs.
INIT_ONCE g_initOnce = INIT_ONCE_STATIC_INIT;

// Flag indicating DLL was loaded. Only set in DllMain, read elsewhere.
bool g_dllLoaded = false;

// InitOnce callback - this is called at most once, after DLL load completes.
// SAFE to call Qt functions here.
BOOL CALLBACK InitOnceCallback(PINIT_ONCE /*initOnce*/, PVOID /*param*/, PVOID* /*context*/) {
  // Now safe to use Qt
  qtmcp::Probe::instance()->initialize();
  return TRUE;
}

}  // namespace

namespace qtmcp {

/// @brief Ensure the probe is initialized.
///
/// Uses Windows InitOnce API for thread-safe one-time initialization.
/// This function is safe to call from any thread after DLL load completes.
/// The first call will trigger initialization; subsequent calls are no-ops.
void ensureInitialized() {
  if (!g_dllLoaded) {
    return;
  }
  InitOnceExecuteOnce(&g_initOnce, InitOnceCallback, nullptr, nullptr);
}

}  // namespace qtmcp

// Automatic initialization hook using Q_COREAPP_STARTUP_FUNCTION.
// This fires automatically when QCoreApplication starts for the build-time
// linking case. For runtime injection, the launcher calls qtmcpProbeInit()
// which either inits immediately or registers via qAddPreRoutine().
#include <QCoreApplication>

static void qtmcpAutoInit() {
  // Check if probe is disabled via environment
  QByteArray enabled = qgetenv("QTMCP_ENABLED");
  if (enabled == "0") {
    OutputDebugStringA("[QtMCP] Probe disabled via QTMCP_ENABLED=0\n");
    return;
  }

  OutputDebugStringA("[QtMCP] qtmcpAutoInit — calling ensureInitialized()\n");
  qtmcp::ensureInitialized();
}

// Register the startup function with Qt (fallback for build-time linking)
Q_COREAPP_STARTUP_FUNCTION(qtmcpAutoInit)

/// Explicit initialization entry point for runtime injection.
/// Called by the launcher via CreateRemoteThread after LoadLibraryW completes.
/// Has LPTHREAD_START_ROUTINE signature: DWORD WINAPI func(LPVOID).
extern "C" __declspec(dllexport)
DWORD WINAPI qtmcpProbeInit(LPVOID /*param*/) {
  OutputDebugStringA("[QtMCP] qtmcpProbeInit() called by launcher\n");

  g_dllLoaded = true;

  if (QCoreApplication::instance()) {
    // QApp already exists (attached to running process) — init now
    OutputDebugStringA("[QtMCP] QCoreApplication exists, initializing immediately\n");
    qtmcp::ensureInitialized();
  } else {
    // QApp doesn't exist yet (suspended process) — register for later.
    // qAddPreRoutine calls the callback immediately if QApp exists,
    // otherwise adds to the list for QCoreApplication constructor to call.
    OutputDebugStringA("[QtMCP] QCoreApplication not yet created, registering qAddPreRoutine\n");
    qAddPreRoutine(qtmcpAutoInit);
  }

  return 0;
}

// Windows DLL entry point.
//
// RESTRICTIONS (loader lock is held):
// - DO NOT call Qt functions (QString, QDebug, etc.)
// - DO NOT create threads
// - DO NOT call LoadLibrary/GetProcAddress
// - DO NOT use synchronization primitives that might deadlock
// - DO NOT allocate memory via CRT (new, malloc) if possible
//
// ALLOWED:
// - DisableThreadLibraryCalls
// - Set simple flags (bool, int)
// - Return immediately
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      // Optimization: we don't need thread attach/detach notifications
      DisableThreadLibraryCalls(hModule);
      // Mark that DLL is loaded - this is the ONLY work we do here
      g_dllLoaded = true;
      // DO NOT call InitializeProbe() or any Qt functions here!
      break;

    case DLL_PROCESS_DETACH:
      // Only cleanup if process is not terminating (reserved == nullptr).
      // If reserved != nullptr, process is being terminated and we should
      // not access any global data or call cleanup code.
      if (reserved == nullptr) {
        // Normal DLL unload - safe to cleanup
        qtmcp::Probe::instance()->shutdown();
      }
      break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      // Disabled via DisableThreadLibraryCalls
      break;
  }

  return TRUE;
}

#endif  // _WIN32
