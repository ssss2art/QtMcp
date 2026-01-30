// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "core/object_registry.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGlobalStatic>
#include <QMutexLocker>

// Qt private header for hook access
#include <private/qhooks_p.h>

namespace {

// Store previous callbacks for daisy-chaining (preserve GammaRay coexistence)
QHooks::AddQObjectCallback g_previousAddCallback = nullptr;
QHooks::RemoveQObjectCallback g_previousRemoveCallback = nullptr;

// Flag to track if hooks are installed
bool g_hooksInstalled = false;

}  // namespace

// Hook callbacks - these are called by Qt for every QObject creation/destruction
// They must be minimal and thread-safe.

void qtmcpAddObjectHook(QObject* obj) {
    // Register the object first
    qtmcp::ObjectRegistry::instance()->registerObject(obj);

    // Chain to previous callback (e.g., GammaRay)
    if (g_previousAddCallback) {
        g_previousAddCallback(obj);
    }
}

void qtmcpRemoveObjectHook(QObject* obj) {
    // Unregister the object first
    qtmcp::ObjectRegistry::instance()->unregisterObject(obj);

    // Chain to previous callback
    if (g_previousRemoveCallback) {
        g_previousRemoveCallback(obj);
    }
}

namespace qtmcp {

// Thread-safe singleton storage using Q_GLOBAL_STATIC
Q_GLOBAL_STATIC(ObjectRegistry, s_objectRegistryInstance)

ObjectRegistry* ObjectRegistry::instance() {
    return s_objectRegistryInstance();
}

ObjectRegistry::ObjectRegistry()
    : QObject(nullptr)
{
    // Log creation for debugging
    qDebug() << "[QtMCP] ObjectRegistry created";
}

ObjectRegistry::~ObjectRegistry() {
    qDebug() << "[QtMCP] ObjectRegistry destroyed";
}

void ObjectRegistry::registerObject(QObject* obj) {
    if (!obj) {
        return;
    }

    {
        QMutexLocker lock(&m_mutex);
        m_objects.insert(obj);
    }

    // Emit signal on main thread to avoid threading issues with slots
    // Use QueuedConnection to ensure the signal is delivered asynchronously
    QMetaObject::invokeMethod(this, [this, obj]() {
        // Double-check object still exists before emitting
        // (it could have been destroyed between hook and signal delivery)
        {
            QMutexLocker lock(&m_mutex);
            if (!m_objects.contains(obj)) {
                return;
            }
        }
        emit objectAdded(obj);
    }, Qt::QueuedConnection);
}

void ObjectRegistry::unregisterObject(QObject* obj) {
    if (!obj) {
        return;
    }

    {
        QMutexLocker lock(&m_mutex);
        m_objects.remove(obj);
    }

    // Emit signal on main thread
    QMetaObject::invokeMethod(this, [this, obj]() {
        emit objectRemoved(obj);
    }, Qt::QueuedConnection);
}

QObject* ObjectRegistry::findByObjectName(const QString& name, QObject* root) {
    QMutexLocker lock(&m_mutex);

    if (root) {
        // Search within root's subtree
        if (root->objectName() == name) {
            return root;
        }
        // Use Qt's built-in recursive search
        QList<QObject*> matches = root->findChildren<QObject*>(name, Qt::FindChildrenRecursively);
        return matches.isEmpty() ? nullptr : matches.first();
    }

    // Search all tracked objects
    for (QObject* obj : std::as_const(m_objects)) {
        if (obj && obj->objectName() == name) {
            return obj;
        }
    }
    return nullptr;
}

QList<QObject*> ObjectRegistry::findAllByClassName(const QString& className, QObject* root) {
    QMutexLocker lock(&m_mutex);
    QList<QObject*> result;

    if (root) {
        // Search within root's subtree
        if (QString::fromLatin1(root->metaObject()->className()) == className) {
            result.append(root);
        }
        for (QObject* child : root->children()) {
            result.append(findAllByClassName(className, child));
        }
        return result;
    }

    // Search all tracked objects
    for (QObject* obj : std::as_const(m_objects)) {
        if (obj && QString::fromLatin1(obj->metaObject()->className()) == className) {
            result.append(obj);
        }
    }
    return result;
}

QList<QObject*> ObjectRegistry::allObjects() {
    QMutexLocker lock(&m_mutex);
    return m_objects.values();
}

int ObjectRegistry::objectCount() const {
    QMutexLocker lock(&m_mutex);
    return m_objects.size();
}

bool ObjectRegistry::contains(QObject* obj) const {
    QMutexLocker lock(&m_mutex);
    return m_objects.contains(obj);
}

void ObjectRegistry::scanExistingObjects(QObject* root) {
    if (!root) {
        return;
    }

    // Register this object if not already tracked
    {
        QMutexLocker lock(&m_mutex);
        if (!m_objects.contains(root)) {
            m_objects.insert(root);
            // Don't emit signal for pre-existing objects to avoid noise
            // during initialization
        }
    }

    // Recursively process children
    for (QObject* child : root->children()) {
        scanExistingObjects(child);
    }
}

void installObjectHooks() {
    if (g_hooksInstalled) {
        qWarning() << "[QtMCP] Object hooks already installed";
        return;
    }

    // Verify hook version compatibility
    // qtHookData[QHooks::HookDataVersion] contains the version number
    quintptr hookVersion = qtHookData[QHooks::HookDataVersion];
    if (hookVersion < 1) {
        qWarning() << "[QtMCP] qtHookData version too old:" << hookVersion;
        return;
    }

    qDebug() << "[QtMCP] Installing object hooks (qtHookData version:" << hookVersion << ")";

    // Save existing callbacks for daisy-chaining
    g_previousAddCallback = reinterpret_cast<QHooks::AddQObjectCallback>(
        qtHookData[QHooks::AddQObject]);
    g_previousRemoveCallback = reinterpret_cast<QHooks::RemoveQObjectCallback>(
        qtHookData[QHooks::RemoveQObject]);

    if (g_previousAddCallback) {
        qDebug() << "[QtMCP] Daisy-chaining to existing AddQObject hook";
    }
    if (g_previousRemoveCallback) {
        qDebug() << "[QtMCP] Daisy-chaining to existing RemoveQObject hook";
    }

    // Install our callbacks
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&qtmcpAddObjectHook);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&qtmcpRemoveObjectHook);

    g_hooksInstalled = true;
    qDebug() << "[QtMCP] Object hooks installed successfully";
}

void uninstallObjectHooks() {
    if (!g_hooksInstalled) {
        return;
    }

    qDebug() << "[QtMCP] Uninstalling object hooks";

    // Restore previous callbacks (or nullptr)
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(g_previousAddCallback);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(g_previousRemoveCallback);

    g_previousAddCallback = nullptr;
    g_previousRemoveCallback = nullptr;
    g_hooksInstalled = false;

    qDebug() << "[QtMCP] Object hooks uninstalled";
}

}  // namespace qtmcp
