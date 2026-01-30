// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QSet>
#include <QRecursiveMutex>

#include "probe.h"  // For QTMCP_EXPORT

// Forward declarations for hook callbacks (global functions)
void qtmcpAddObjectHook(QObject*);
void qtmcpRemoveObjectHook(QObject*);

namespace qtmcp {

/// @brief Registry that tracks all QObjects in the target application.
///
/// Uses Qt's private qtHookData hooks (AddQObject/RemoveQObject) to track
/// object creation and destruction. This is the same mechanism used by
/// KDAB's GammaRay, ensuring compatibility and proven reliability.
///
/// Thread Safety: All public methods are thread-safe. Hook callbacks may be
/// called from any thread, so the registry uses QRecursiveMutex for protection.
///
/// Usage: Call installObjectHooks() after QCoreApplication is created to
/// start tracking objects. Call uninstallObjectHooks() before shutdown.
///
/// Note: Object IDs will be added in Plan 02-02. This class provides basic
/// lookup by objectName and className.
class QTMCP_EXPORT ObjectRegistry : public QObject {
    Q_OBJECT

public:
    /// @brief Get the singleton instance.
    /// @return Pointer to the global ObjectRegistry instance.
    static ObjectRegistry* instance();

    /// @brief Find object by objectName.
    /// @param name The objectName to search for.
    /// @param root Optional root object to search within (nullptr = all objects).
    /// @return The first matching object, or nullptr if not found.
    QObject* findByObjectName(const QString& name, QObject* root = nullptr);

    /// @brief Find all objects of a given class name.
    /// @param className The class name to search for (e.g., "QPushButton").
    /// @param root Optional root object to search within (nullptr = all objects).
    /// @return List of all matching objects.
    QList<QObject*> findAllByClassName(const QString& className, QObject* root = nullptr);

    /// @brief Get all tracked objects.
    /// @return List of all objects currently in the registry.
    QList<QObject*> allObjects();

    /// @brief Get the number of tracked objects.
    /// @return Object count.
    int objectCount() const;

    /// @brief Check if an object is tracked.
    /// @param obj The object to check.
    /// @return true if the object is in the registry.
    bool contains(QObject* obj) const;

signals:
    /// @brief Emitted when a new object is registered.
    /// @param obj The newly tracked object.
    /// @note Emitted on the main thread via QueuedConnection.
    void objectAdded(QObject* obj);

    /// @brief Emitted when an object is unregistered (about to be destroyed).
    /// @param obj The object being removed.
    /// @note Emitted on the main thread via QueuedConnection.
    void objectRemoved(QObject* obj);

private:
    // Hook callback friends - these are called from Qt's internal hooks
    // Note: These are global functions (not in namespace) for Qt hook compatibility
    friend void ::qtmcpAddObjectHook(QObject*);
    friend void ::qtmcpRemoveObjectHook(QObject*);

    /// @brief Register an object (called from hook).
    /// @param obj The object to register.
    void registerObject(QObject* obj);

    /// @brief Unregister an object (called from hook).
    /// @param obj The object to unregister.
    void unregisterObject(QObject* obj);

    /// @brief Set of tracked objects.
    /// Simple set for now - IDs will be added in Plan 02-02.
    QSet<QObject*> m_objects;

    /// @brief Mutex for thread-safe access.
    /// Must be recursive because hook callbacks may nest.
    mutable QRecursiveMutex m_mutex;

public:
    // Constructor/destructor are public for Q_GLOBAL_STATIC compatibility
    // Use instance() to get the singleton - do not construct directly
    ObjectRegistry();
    ~ObjectRegistry() override;
};

/// @brief Install Qt object lifecycle hooks.
///
/// Installs AddQObject and RemoveQObject hooks into Qt's qtHookData array.
/// Existing hooks are preserved via daisy-chaining for tool coexistence
/// (e.g., GammaRay can run alongside QtMCP).
///
/// Must be called AFTER QCoreApplication exists.
/// Should be called from Probe::initialize().
void installObjectHooks();

/// @brief Uninstall Qt object lifecycle hooks.
///
/// Restores previous hooks (or nullptr if there were none).
/// Should be called from Probe::shutdown().
void uninstallObjectHooks();

}  // namespace qtmcp
