// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "transport/websocket_server.h"

#include <QDebug>

#ifdef QTMCP_HAS_SPDLOG
#include <spdlog/spdlog.h>
#define LOG_INFO(msg) spdlog::info(msg)
#define LOG_WARN(msg) spdlog::warn(msg)
#define LOG_ERROR(msg) spdlog::error(msg)
#define LOG_DEBUG(msg) spdlog::debug(msg)
#else
#define LOG_INFO(msg) qInfo() << msg
#define LOG_WARN(msg) qWarning() << msg
#define LOG_ERROR(msg) qCritical() << msg
#define LOG_DEBUG(msg) qDebug() << msg
#endif

#include <algorithm>

#include "transport/jsonrpc_handler.h"

namespace qtmcp {

WebSocketServer::WebSocketServer(QObject* parent)
    : QObject(parent), rpc_handler_(std::make_unique<JsonRpcHandler>(this)) {}

WebSocketServer::~WebSocketServer() { Stop(); }

bool WebSocketServer::Start(int port) {
  if (server_ && server_->isListening()) {
    qWarning() << "WebSocket server already running on port" << port_;
    return true;
  }

  server_ = std::make_unique<QWebSocketServer>(QStringLiteral("QtMCP"),
                                                QWebSocketServer::NonSecureMode, this);

  if (!server_->listen(QHostAddress::LocalHost, port)) {
    qCritical() << "Failed to start WebSocket server on port" << port << ":" << server_->errorString();
    emit ErrorOccurred(server_->errorString());
    server_.reset();
    return false;
  }

  port_ = server_->serverPort();
  qInfo() << "WebSocket server listening on localhost:" << port_;

  connect(server_.get(), &QWebSocketServer::newConnection, this,
          &WebSocketServer::OnNewConnection);

  return true;
}

void WebSocketServer::Stop() {
  if (!server_) {
    return;
  }

  LOG_INFO("Stopping WebSocket server");

  // Close all client connections
  for (QWebSocket* client : clients_) {
    client->close();
    client->deleteLater();
  }
  clients_.clear();

  // Close server
  server_->close();
  server_.reset();
  port_ = 0;
}

bool WebSocketServer::IsRunning() const { return server_ && server_->isListening(); }

int WebSocketServer::Port() const { return port_; }

int WebSocketServer::ClientCount() const { return static_cast<int>(clients_.size()); }

void WebSocketServer::OnNewConnection() {
  QWebSocket* client = server_->nextPendingConnection();
  if (client == nullptr) {
    return;
  }

  qInfo() << "Client connected from" << client->peerAddress().toString() << ":" << client->peerPort();

  // Connect client signals
  connect(client, &QWebSocket::textMessageReceived, this,
          &WebSocketServer::OnTextMessageReceived);
  connect(client, &QWebSocket::disconnected, this, &WebSocketServer::OnClientDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  connect(client, &QWebSocket::errorOccurred, this,
          &WebSocketServer::OnSocketError);
#else
  connect(client, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
          &WebSocketServer::OnSocketError);
#endif

  clients_.push_back(client);
  emit ClientConnected();
}

void WebSocketServer::OnClientDisconnected() {
  QWebSocket* client = qobject_cast<QWebSocket*>(sender());
  if (client == nullptr) {
    return;
  }

  LOG_INFO("Client disconnected");

  // Remove from client list
  auto it = std::find(clients_.begin(), clients_.end(), client);
  if (it != clients_.end()) {
    clients_.erase(it);
  }

  client->deleteLater();
  emit ClientDisconnected();
}

void WebSocketServer::OnTextMessageReceived(const QString& message) {
  QWebSocket* client = qobject_cast<QWebSocket*>(sender());
  if (client == nullptr) {
    return;
  }

  qDebug() << "Received message:" << message;

  // Process JSON-RPC message and get response
  QString response = rpc_handler_->HandleMessage(message);

  if (!response.isEmpty()) {
    qDebug() << "Sending response:" << response;
    client->sendTextMessage(response);
  }
}

void WebSocketServer::OnSocketError(QAbstractSocket::SocketError error) {
  QWebSocket* client = qobject_cast<QWebSocket*>(sender());
  if (client == nullptr) {
    return;
  }

  qCritical() << "WebSocket error" << static_cast<int>(error) << ":" << client->errorString();
  emit ErrorOccurred(client->errorString());
}

}  // namespace qtmcp
