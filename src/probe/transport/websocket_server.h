// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>

#include <memory>
#include <vector>

namespace qtmcp {

class JsonRpcHandler;

/// @brief WebSocket server for JSON-RPC communication.
///
/// This class manages WebSocket connections from clients (e.g., MCP server,
/// test automation tools) and routes JSON-RPC messages to the appropriate
/// handlers.
class WebSocketServer : public QObject {
  Q_OBJECT

 public:
  /// @brief Construct a WebSocket server.
  /// @param parent Parent QObject.
  explicit WebSocketServer(QObject* parent = nullptr);

  /// @brief Destructor.
  ~WebSocketServer() override;

  /// @brief Start the WebSocket server.
  /// @param port Port to listen on.
  /// @return true if server started successfully.
  bool Start(int port);

  /// @brief Stop the WebSocket server.
  void Stop();

  /// @brief Check if server is running.
  /// @return true if server is listening.
  bool IsRunning() const;

  /// @brief Get the current port.
  /// @return Port number, or 0 if not running.
  int Port() const;

  /// @brief Get number of connected clients.
  /// @return Number of active connections.
  int ClientCount() const;

 signals:
  /// @brief Emitted when a client connects.
  void ClientConnected();

  /// @brief Emitted when a client disconnects.
  void ClientDisconnected();

  /// @brief Emitted when an error occurs.
  /// @param message Error description.
  void ErrorOccurred(const QString& message);

 private slots:
  /// @brief Handle new client connection.
  void OnNewConnection();

  /// @brief Handle client disconnection.
  void OnClientDisconnected();

  /// @brief Handle incoming text message.
  /// @param message The received message.
  void OnTextMessageReceived(const QString& message);

  /// @brief Handle WebSocket error.
  /// @param error The error that occurred.
  void OnSocketError(QAbstractSocket::SocketError error);

 private:
  std::unique_ptr<QWebSocketServer> server_;
  std::vector<QWebSocket*> clients_;
  std::unique_ptr<JsonRpcHandler> rpc_handler_;
  int port_ = 0;
};

}  // namespace qtmcp
