// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "core/injector.h"

#ifdef QTMCP_PLATFORM_WINDOWS

#include <QCoreApplication>
#include <QTimer>

#include <spdlog/spdlog.h>

#include <windows.h>

#include <cstdlib>

#include "core/probe.h"

namespace qtmcp {

void InitializeProbe() {
  // Check if probe is disabled
  const char* enabled_env = std::getenv("QTMCP_ENABLED");
  if (enabled_env != nullptr && std::string(enabled_env) == "0") {
    spdlog::info("QtMCP Probe disabled via QTMCP_ENABLED=0");
    return;
  }

  spdlog::info("QtMCP Probe library loaded (Windows)");

  // Get port from environment or use default
  int port = 9999;
  const char* port_env = std::getenv("QTMCP_PORT");
  if (port_env != nullptr) {
    port = std::atoi(port_env);
    if (port <= 0 || port > 65535) {
      port = 9999;
    }
  }

  // Check if QCoreApplication exists
  if (QCoreApplication::instance() != nullptr) {
    // Application exists, initialize via event loop
    QTimer::singleShot(0, [port]() { Probe::Instance()->Initialize(port); });
  } else {
    spdlog::info("QCoreApplication not yet created, will initialize when available");
    // Will be initialized lazily
  }
}

void ShutdownProbe() {
  spdlog::info("QtMCP Probe library unloading (Windows)");
  if (Probe::Instance()->IsRunning()) {
    Probe::Instance()->Shutdown();
  }
}

}  // namespace qtmcp

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      // Disable thread notifications for performance
      DisableThreadLibraryCalls(hinstDLL);
      qtmcp::InitializeProbe();
      break;

    case DLL_PROCESS_DETACH:
      // Only cleanup if process is not terminating
      if (lpvReserved == nullptr) {
        qtmcp::ShutdownProbe();
      }
      break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      // Not used
      break;
  }
  return TRUE;
}

#endif  // QTMCP_PLATFORM_WINDOWS
