// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QGlobalStatic>

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

// Forward declaration - WebSocketServer will be implemented in a later plan
class WebSocketServer;

/// @brief Main probe class that manages the QtMCP introspection system.
///
/// The Probe is a singleton that initializes when the library is loaded into
/// a Qt application. It starts a WebSocket server that accepts JSON-RPC
/// connections for introspection and automation commands.
///
/// Configuration is done via environment variables:
/// - QTMCP_PORT: WebSocket server port (default: 9222)
/// - QTMCP_MODE: API mode - "native", "computer_use", "chrome", or "all"
/// - QTMCP_ENABLED: Set to "0" to disable the probe
///
/// IMPORTANT: This class uses Q_GLOBAL_STATIC for thread-safe singleton
/// storage. Do NOT use thread_local or std::call_once in this codebase -
/// they cause issues with dynamically loaded DLLs on Windows.
class QTMCP_EXPORT Probe : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Probe)

public:
    /// @brief Get the singleton instance.
    /// @return Pointer to the global Probe instance (never null).
    ///
    /// Uses Q_GLOBAL_STATIC internally for thread-safe lazy initialization.
    static Probe* instance();

    /// @brief Initialize the probe.
    ///
    /// Must be called AFTER QCoreApplication is created. On Windows, this is
    /// called via InitOnceExecuteOnce to ensure thread-safe one-time init.
    /// On Linux, this is called via QTimer::singleShot from the constructor.
    ///
    /// @return true if initialization succeeded or already initialized.
    bool initialize();

    /// @brief Check if the probe has been initialized.
    /// @return true if initialize() has been successfully called.
    bool isInitialized() const;

    /// @brief Shutdown the probe and release resources.
    ///
    /// Safe to call multiple times. Called automatically on DLL unload.
    void shutdown();

    /// @brief Set the WebSocket server port.
    /// @param port Port number (1-65535).
    ///
    /// Must be called BEFORE initialize(). Has no effect after initialization.
    void setPort(quint16 port);

    /// @brief Get the configured WebSocket server port.
    /// @return The port number (default: 9222).
    quint16 port() const;

    /// @brief Get the current API mode.
    /// @return The API mode string ("native", "computer_use", "chrome", or "all").
    QString mode() const;

    /// @brief Check if the probe is currently running with an active server.
    /// @return true if initialized and server is listening.
    bool isRunning() const;

signals:
    /// @brief Emitted when a client connects to the WebSocket server.
    void clientConnected();

    /// @brief Emitted when a client disconnects from the WebSocket server.
    void clientDisconnected();

    /// @brief Emitted when the probe encounters an error.
    /// @param message Error description.
    void errorOccurred(const QString& message);

public:
    // Constructor/destructor are public for Q_GLOBAL_STATIC compatibility
    // Use instance() to get the singleton - do not construct directly
    Probe();
    ~Probe() override;

private:

    /// @brief Read configuration from environment variables.
    void readConfiguration();

    // WebSocket server - will be created in initialize()
    // Using raw pointer because WebSocketServer is not yet implemented
    WebSocketServer* m_server = nullptr;

    // Configuration
    quint16 m_port = 9222;
    QString m_mode = QStringLiteral("all");

    // State flags
    bool m_initialized = false;
    bool m_running = false;
};

}  // namespace qtmcp
