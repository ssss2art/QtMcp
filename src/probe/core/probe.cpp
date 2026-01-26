// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "core/probe.h"

#include <QCoreApplication>
#include <QTimer>

#include <spdlog/spdlog.h>

#include "transport/websocket_server.h"

namespace qtmcp {

namespace {
// Global singleton instance
Probe* g_instance = nullptr;
}  // namespace

Probe* Probe::Instance() {
  if (g_instance == nullptr) {
    g_instance = new Probe();
  }
  return g_instance;
}

Probe::Probe() : QObject(nullptr) {
  spdlog::info("QtMCP Probe created");
  ReadConfiguration();
}

Probe::~Probe() {
  Shutdown();
  g_instance = nullptr;
}

void Probe::ReadConfiguration() {
  // Read port from environment
  QByteArray port_env = qgetenv("QTMCP_PORT");
  if (!port_env.isEmpty()) {
    bool ok = false;
    int port = port_env.toInt(&ok);
    if (ok && port > 0 && port < 65536) {
      port_ = port;
    }
  }

  // Read mode from environment
  QByteArray mode_env = qgetenv("QTMCP_MODE");
  if (!mode_env.isEmpty()) {
    QString mode = QString::fromUtf8(mode_env).toLower();
    if (mode == "native" || mode == "computer_use" || mode == "chrome" || mode == "all") {
      mode_ = mode;
    }
  }

  spdlog::info("QtMCP configuration: port={}, mode={}", port_, mode_.toStdString());
}

bool Probe::Initialize(int port) {
  if (running_) {
    spdlog::warn("Probe already initialized");
    return true;
  }

  port_ = port;

  // Check if Qt event loop is available
  if (QCoreApplication::instance() == nullptr) {
    spdlog::info("QCoreApplication not available yet, deferring initialization");
    // We'll need to wait for the application to be created
    // The injector will handle this via QTimer::singleShot when app is ready
    return true;
  }

  DeferredInitialize();
  return running_;
}

void Probe::DeferredInitialize() {
  if (initialized_) {
    return;
  }
  initialized_ = true;

  spdlog::info("QtMCP Probe initializing on port {}", port_);

  // Create WebSocket server
  server_ = std::make_unique<WebSocketServer>(this);

  // Connect signals
  connect(server_.get(), &WebSocketServer::ClientConnected, this, &Probe::ClientConnected);
  connect(server_.get(), &WebSocketServer::ClientDisconnected, this, &Probe::ClientDisconnected);
  connect(server_.get(), &WebSocketServer::ErrorOccurred, this, &Probe::ErrorOccurred);

  // Start server
  if (server_->Start(port_)) {
    running_ = true;
    spdlog::info("QtMCP Probe started successfully on port {}", port_);
  } else {
    spdlog::error("Failed to start QtMCP Probe on port {}", port_);
    emit ErrorOccurred(QString("Failed to start WebSocket server on port %1").arg(port_));
  }
}

void Probe::Shutdown() {
  if (!running_) {
    return;
  }

  spdlog::info("QtMCP Probe shutting down");

  if (server_) {
    server_->Stop();
    server_.reset();
  }

  running_ = false;
  initialized_ = false;
}

bool Probe::IsRunning() const { return running_; }

int Probe::Port() const { return running_ ? port_ : 0; }

QString Probe::Mode() const { return mode_; }

}  // namespace qtmcp
