// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

#include <memory>

// Export macro for Windows DLL
#if defined(QTMCP_PROBE_LIBRARY)
#if defined(_WIN32)
#define QTMCP_EXPORT __declspec(dllexport)
#else
#define QTMCP_EXPORT __attribute__((visibility("default")))
#endif
#else
#if defined(_WIN32)
#define QTMCP_EXPORT __declspec(dllimport)
#else
#define QTMCP_EXPORT
#endif
#endif

namespace qtmcp {

class WebSocketServer;

/// @brief Main probe class that manages the QtMCP introspection system.
///
/// The Probe is a singleton that initializes when the library is loaded into
/// a Qt application. It starts a WebSocket server that accepts JSON-RPC
/// connections for introspection and automation commands.
///
/// Configuration is done via environment variables:
/// - QTMCP_PORT: WebSocket server port (default: 9999)
/// - QTMCP_MODE: API mode - "native", "computer_use", "chrome", or "all"
/// - QTMCP_ENABLED: Set to "0" to disable the probe
class QTMCP_EXPORT Probe : public QObject {
  Q_OBJECT

 public:
  /// @brief Get the singleton instance.
  /// @return Pointer to the global Probe instance.
  static Probe* Instance();

  /// @brief Initialize the probe with the specified port.
  /// @param port WebSocket server port (default: 9999).
  /// @return true if initialization succeeded, false otherwise.
  bool Initialize(int port = 9999);

  /// @brief Shutdown the probe and release resources.
  void Shutdown();

  /// @brief Check if the probe is currently running.
  /// @return true if the probe is initialized and running.
  bool IsRunning() const;

  /// @brief Get the WebSocket server port.
  /// @return The port number the server is listening on, or 0 if not running.
  int Port() const;

  /// @brief Get the current API mode.
  /// @return The API mode string ("native", "computer_use", "chrome", or "all").
  QString Mode() const;

 signals:
  /// @brief Emitted when a client connects to the WebSocket server.
  void ClientConnected();

  /// @brief Emitted when a client disconnects from the WebSocket server.
  void ClientDisconnected();

  /// @brief Emitted when the probe encounters an error.
  /// @param message Error description.
  void ErrorOccurred(const QString& message);

 private:
  /// @brief Private constructor for singleton pattern.
  Probe();

  /// @brief Destructor.
  ~Probe() override;

  // Prevent copying
  Probe(const Probe&) = delete;
  Probe& operator=(const Probe&) = delete;

  /// @brief Read configuration from environment variables.
  void ReadConfiguration();

  /// @brief Internal initialization called once Qt event loop is available.
  void DeferredInitialize();

  std::unique_ptr<WebSocketServer> server_;
  int port_ = 9999;
  QString mode_ = "all";
  bool running_ = false;
  bool initialized_ = false;
};

}  // namespace qtmcp
