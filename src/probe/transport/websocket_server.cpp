// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "transport/websocket_server.h"

#include "transport/jsonrpc_handler.h"

#include <QDebug>
#include <QWebSocket>
#include <QWebSocketServer>

namespace qtmcp {

WebSocketServer::WebSocketServer(quint16 port, QObject* parent)
    : QObject(parent),
      m_server(
          new QWebSocketServer(QStringLiteral("QtMCP"), QWebSocketServer::NonSecureMode, this)),
      m_activeClient(nullptr),
      m_rpcHandler(new JsonRpcHandler(this)),
      m_port(port) {
  connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

WebSocketServer::~WebSocketServer() {
  stop();
}

bool WebSocketServer::start() {
  if (m_server->isListening()) {
    qWarning() << "[QtMCP] WebSocket server already listening on port" << m_port;
    return true;
  }

  if (!m_server->listen(QHostAddress::Any, m_port)) {
    QString error = m_server->errorString();
    qCritical() << "[QtMCP] Failed to start WebSocket server on port" << m_port << ":" << error;
    emit errorOccurred(error);
    return false;
  }

  // Print startup message to stderr as specified in CONTEXT.md
  fprintf(stderr, "QtMCP listening on ws://0.0.0.0:%u\n", static_cast<unsigned>(m_port));
  fflush(stderr);

  return true;
}

void WebSocketServer::stop() {
  // Close active client connection if any
  if (m_activeClient) {
    m_activeClient->close();
    m_activeClient->deleteLater();
    m_activeClient = nullptr;
  }

  // Close the server
  if (m_server->isListening()) {
    m_server->close();
  }
}

bool WebSocketServer::isListening() const {
  return m_server && m_server->isListening();
}

quint16 WebSocketServer::port() const {
  return m_port;
}

bool WebSocketServer::hasActiveClient() const {
  return m_activeClient != nullptr;
}

JsonRpcHandler* WebSocketServer::rpcHandler() const {
  return m_rpcHandler;
}

bool WebSocketServer::sendMessage(const QString& message) {
  if (!m_activeClient) {
    return false;
  }
  qDebug() << "[QtMCP] Sending notification:" << message;
  m_activeClient->sendTextMessage(message);
  return true;
}

void WebSocketServer::onNewConnection() {
  QWebSocket* socket = m_server->nextPendingConnection();
  if (!socket) {
    return;
  }

  // Single-client semantics: reject if we already have a client
  if (m_activeClient) {
    qWarning() << "[QtMCP] Rejecting connection from" << socket->peerAddress().toString()
               << "- another client is already connected";
    socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                  QStringLiteral("Another client is already connected"));
    socket->deleteLater();
    return;
  }

  // Accept this client
  m_activeClient = socket;
  qInfo() << "[QtMCP] Client connected from" << socket->peerAddress().toString() << ":"
          << socket->peerPort();

  connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessage);
  connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onClientDisconnected);

  emit clientConnected();
}

void WebSocketServer::onTextMessage(const QString& message) {
  if (!m_activeClient) {
    return;
  }

  qDebug() << "[QtMCP] Received:" << message;
  emit messageReceived(message);

  // Process message through JSON-RPC handler
  QString response = m_rpcHandler->HandleMessage(message);

  // Send response if not a notification (notifications return empty response)
  if (!response.isEmpty()) {
    qDebug() << "[QtMCP] Sending:" << response;
    m_activeClient->sendTextMessage(response);
  }
}

void WebSocketServer::onClientDisconnected() {
  if (m_activeClient) {
    qInfo() << "[QtMCP] Client disconnected";
    m_activeClient->deleteLater();
    m_activeClient = nullptr;
    emit clientDisconnected();
  }
  // Server keeps listening for new connections - do NOT stop!
}

}  // namespace qtmcp
