// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "core/probe.h"

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#ifdef QTMCP_HAS_SPDLOG
#include <spdlog/spdlog.h>
#define LOG_INFO(msg) spdlog::info(msg)
#define LOG_WARN(msg) spdlog::warn(msg)
#define LOG_ERROR(msg) spdlog::error(msg)
#define LOG_INFO_FMT(fmt, ...) spdlog::info(fmt, __VA_ARGS__)
#define LOG_WARN_FMT(fmt, ...) spdlog::warn(fmt, __VA_ARGS__)
#define LOG_ERROR_FMT(fmt, ...) spdlog::error(fmt, __VA_ARGS__)
#else
#define LOG_INFO(msg) qInfo() << msg
#define LOG_WARN(msg) qWarning() << msg
#define LOG_ERROR(msg) qCritical() << msg
#define LOG_INFO_FMT(fmt, ...) qInfo() << QString::asprintf(fmt, __VA_ARGS__)
#define LOG_WARN_FMT(fmt, ...) qWarning() << QString::asprintf(fmt, __VA_ARGS__)
#define LOG_ERROR_FMT(fmt, ...) qCritical() << QString::asprintf(fmt, __VA_ARGS__)
#endif

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
  LOG_INFO("QtMCP Probe created");
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

  qInfo() << "QtMCP configuration: port=" << port_ << ", mode=" << mode_;
}

bool Probe::Initialize(int port) {
  if (running_) {
    LOG_WARN("Probe already initialized");
    return true;
  }

  port_ = port;

  // Check if Qt event loop is available
  if (QCoreApplication::instance() == nullptr) {
    LOG_INFO("QCoreApplication not available yet, deferring initialization");
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

  qInfo() << "QtMCP Probe initializing on port" << port_;

  // Create WebSocket server
  server_ = std::make_unique<WebSocketServer>(this);

  // Connect signals
  connect(server_.get(), &WebSocketServer::ClientConnected, this, &Probe::ClientConnected);
  connect(server_.get(), &WebSocketServer::ClientDisconnected, this, &Probe::ClientDisconnected);
  connect(server_.get(), &WebSocketServer::ErrorOccurred, this, &Probe::ErrorOccurred);

  // Start server
  if (server_->Start(port_)) {
    running_ = true;
    qInfo() << "QtMCP Probe started successfully on port" << port_;
  } else {
    qCritical() << "Failed to start QtMCP Probe on port" << port_;
    emit ErrorOccurred(QString("Failed to start WebSocket server on port %1").arg(port_));
  }
}

void Probe::Shutdown() {
  if (!running_) {
    return;
  }

  LOG_INFO("QtMCP Probe shutting down");

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
