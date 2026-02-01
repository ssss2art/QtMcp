// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "core/probe.h"

#include <QCoreApplication>
#include <QDebug>
#include <stdexcept>

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

#include "api/computer_use_mode_api.h"
#include "api/native_mode_api.h"
#include "api/symbolic_name_map.h"
#include "core/object_registry.h"
#include "core/object_resolver.h"
#include "introspection/signal_monitor.h"
#include "transport/websocket_server.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QWindow>

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

    // Install object hooks for tracking QObjects
    installObjectHooks();

    // Scan objects that existed before hook installation
    // This includes QCoreApplication and any widgets created early
    ObjectRegistry* registry = ObjectRegistry::instance();

    // Scan QCoreApplication (and all its children)
    if (QCoreApplication::instance()) {
        registry->scanExistingObjects(QCoreApplication::instance());
    }

    // For GUI apps, also scan top-level windows/widgets
    // QGuiApplication::allWindows() returns QWindow*, which are QObjects
    if (auto* guiApp = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        for (QWindow* window : guiApp->allWindows()) {
            registry->scanExistingObjects(window);
        }
    }

    LOG_INFO("[QtMCP] Object hooks installed, tracking " +
             QString::number(registry->objectCount()) + " existing objects");
    fprintf(stderr, "[QtMCP] Object hooks installed, tracking %d existing objects\n",
            registry->objectCount());

    // Create and start WebSocket server
    m_server = new WebSocketServer(m_port, this);

    // Connect server signals to probe signals for external monitoring
    connect(m_server, &WebSocketServer::clientConnected,
            this, &Probe::clientConnected);
    connect(m_server, &WebSocketServer::clientDisconnected,
            this, &Probe::clientDisconnected);
    connect(m_server, &WebSocketServer::errorOccurred,
            this, &Probe::errorOccurred);

    // Start the server
    if (!m_server->start()) {
        LOG_ERROR("[QtMCP] Failed to start WebSocket server");
        fprintf(stderr, "[QtMCP] ERROR: Failed to start WebSocket server on port %u\n",
                static_cast<unsigned>(m_port));
        delete m_server;
        m_server = nullptr;
        m_initialized = false;
        return false;
    }

    m_running = true;

    // Register Native Mode API (qt.* namespaced methods)
    try {
        auto* nativeApi = new NativeModeApi(m_server->rpcHandler(), this);
        Q_UNUSED(nativeApi);
        fprintf(stderr, "[QtMCP] Native Mode API (qt.*) registered\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "[QtMCP] WARNING: Failed to register Native Mode API: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "[QtMCP] WARNING: Failed to register Native Mode API (unknown exception)\n");
    }

    // Register Computer Use Mode API (cu.* namespaced methods)
    try {
        auto* cuApi = new ComputerUseModeApi(m_server->rpcHandler(), this);
        Q_UNUSED(cuApi);
        fprintf(stderr, "[QtMCP] Computer Use Mode API (cu.*) registered\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "[QtMCP] WARNING: Failed to register Computer Use Mode API: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "[QtMCP] WARNING: Failed to register Computer Use Mode API (unknown exception)\n");
    }

    // Auto-load symbolic name map from env var or default file
    QString nameMapPath = qEnvironmentVariable("QTMCP_NAME_MAP");
    if (nameMapPath.isEmpty()) {
        nameMapPath = QDir::currentPath() + QStringLiteral("/qtmcp-names.json");
    }
    if (QFile::exists(nameMapPath)) {
        SymbolicNameMap::instance()->loadFromFile(nameMapPath);
        LOG_INFO("[QtMCP] Loaded symbolic name map from " + nameMapPath);
        fprintf(stderr, "[QtMCP] Loaded name map from %s\n", qPrintable(nameMapPath));
    }

    // Clear numeric IDs on client disconnect to prevent stale references
    connect(m_server, &WebSocketServer::clientDisconnected,
            this, []() {
        ObjectResolver::clearNumericIds();
    });

    // Wire signal notifications to WebSocket client
    connect(SignalMonitor::instance(), &SignalMonitor::signalEmitted,
            this, [this](const QJsonObject& notification) {
        if (m_server) {
            QJsonObject rpcNotification;
            rpcNotification["jsonrpc"] = "2.0";
            rpcNotification["method"] = "qtmcp.signalEmitted";
            rpcNotification["params"] = notification;
            m_server->sendMessage(QString::fromUtf8(
                QJsonDocument(rpcNotification).toJson(QJsonDocument::Compact)));
        }
    });

    connect(SignalMonitor::instance(), &SignalMonitor::objectCreated,
            this, [this](const QJsonObject& notification) {
        if (m_server) {
            QJsonObject rpcNotification;
            rpcNotification["jsonrpc"] = "2.0";
            rpcNotification["method"] = "qtmcp.objectCreated";
            rpcNotification["params"] = notification;
            m_server->sendMessage(QString::fromUtf8(
                QJsonDocument(rpcNotification).toJson(QJsonDocument::Compact)));
        }
    });

    connect(SignalMonitor::instance(), &SignalMonitor::objectDestroyed,
            this, [this](const QJsonObject& notification) {
        if (m_server) {
            QJsonObject rpcNotification;
            rpcNotification["jsonrpc"] = "2.0";
            rpcNotification["method"] = "qtmcp.objectDestroyed";
            rpcNotification["params"] = notification;
            m_server->sendMessage(QString::fromUtf8(
                QJsonDocument(rpcNotification).toJson(QJsonDocument::Compact)));
        }
    });

    LOG_INFO("[QtMCP] Probe initialized successfully");
    fprintf(stderr, "[QtMCP] Probe initialized on port %u\n", static_cast<unsigned>(m_port));

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

    // Uninstall object hooks first (before any Qt objects are destroyed)
    uninstallObjectHooks();

    // Stop and delete WebSocket server
    if (m_server) {
        m_server->stop();
        delete m_server;
        m_server = nullptr;
    }

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

WebSocketServer* Probe::server() const {
    return m_server;
}

}  // namespace qtmcp
