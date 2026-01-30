// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "core/probe.h"

#include <QCoreApplication>
#include <QDebug>

// Logging macros - use spdlog if available, otherwise Qt logging
#ifdef QTMCP_HAS_SPDLOG
#include <spdlog/spdlog.h>
#define LOG_INFO(msg) spdlog::info(msg)
#define LOG_WARN(msg) spdlog::warn(msg)
#define LOG_ERROR(msg) spdlog::error(msg)
#else
#define LOG_INFO(msg) qInfo() << msg
#define LOG_WARN(msg) qWarning() << msg
#define LOG_ERROR(msg) qCritical() << msg
#endif

// Forward declaration - WebSocketServer not yet implemented
// #include "transport/websocket_server.h"

namespace qtmcp {

// Thread-safe singleton storage using Q_GLOBAL_STATIC
// This is safe for dynamically loaded DLLs (no TLS issues)
Q_GLOBAL_STATIC(Probe, s_probeInstance)

Probe* Probe::instance() {
    return s_probeInstance();
}

Probe::Probe()
    : QObject(nullptr)
    , m_server(nullptr)
    , m_port(9222)
    , m_mode(QStringLiteral("all"))
    , m_initialized(false)
    , m_running(false)
{
    // Read configuration from environment variables
    readConfiguration();

    // Log creation to stderr for debugging injection issues
    // Using fprintf because qDebug may not work before Qt is fully initialized
    fprintf(stderr, "[QtMCP] Probe singleton created (port=%u, mode=%s)\n",
            static_cast<unsigned>(m_port), qPrintable(m_mode));
}

Probe::~Probe() {
    shutdown();
    fprintf(stderr, "[QtMCP] Probe singleton destroyed\n");
}

void Probe::readConfiguration() {
    // Read port from environment
    QByteArray portEnv = qgetenv("QTMCP_PORT");
    if (!portEnv.isEmpty()) {
        bool ok = false;
        int envPort = portEnv.toInt(&ok);
        if (ok && envPort > 0 && envPort <= 65535) {
            m_port = static_cast<quint16>(envPort);
        }
    }

    // Read mode from environment
    QByteArray modeEnv = qgetenv("QTMCP_MODE");
    if (!modeEnv.isEmpty()) {
        QString modeStr = QString::fromUtf8(modeEnv).toLower();
        if (modeStr == QLatin1String("native") ||
            modeStr == QLatin1String("computer_use") ||
            modeStr == QLatin1String("chrome") ||
            modeStr == QLatin1String("all")) {
            m_mode = modeStr;
        }
    }
}

bool Probe::initialize() {
    // Guard against multiple initialization
    if (m_initialized) {
        LOG_WARN("[QtMCP] Probe::initialize() called but already initialized");
        return true;
    }

    // CRITICAL: Verify QCoreApplication exists before using any Qt functionality
    // This is the main safety check for deferred initialization
    if (!QCoreApplication::instance()) {
        LOG_ERROR("[QtMCP] Probe::initialize() called but QCoreApplication does not exist!");
        fprintf(stderr, "[QtMCP] ERROR: Cannot initialize - QCoreApplication not created yet\n");
        return false;
    }

    // Mark as initialized first to prevent re-entry
    m_initialized = true;

    // Log initialization
    LOG_INFO("[QtMCP] Probe initializing...");
    fprintf(stderr, "[QtMCP] Probe initialized (port=%u)\n", static_cast<unsigned>(m_port));

    // TODO: Start WebSocket server here in future plan (01-03 or 01-04)
    // For now, just mark as "running" since we have no server yet
    // m_server = new WebSocketServer(m_port, this);
    // m_running = m_server->isListening();

    // Placeholder: will be replaced when WebSocketServer is implemented
    m_running = true;

    return true;
}

bool Probe::isInitialized() const {
    return m_initialized;
}

void Probe::shutdown() {
    if (!m_initialized) {
        return;
    }

    LOG_INFO("[QtMCP] Probe shutting down...");
    fprintf(stderr, "[QtMCP] Probe shutting down\n");

    // TODO: Stop WebSocket server when implemented
    // if (m_server) {
    //     delete m_server;
    //     m_server = nullptr;
    // }

    m_running = false;
    m_initialized = false;
}

void Probe::setPort(quint16 port) {
    if (m_initialized) {
        LOG_WARN("[QtMCP] Cannot change port after initialization");
        return;
    }
    if (port > 0) {
        m_port = port;
    }
}

quint16 Probe::port() const {
    return m_port;
}

QString Probe::mode() const {
    return m_mode;
}

bool Probe::isRunning() const {
    return m_running;
}

}  // namespace qtmcp
