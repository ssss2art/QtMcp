// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QStringList>

namespace qtmcp {

/// @brief Utility class for QMetaObject introspection with JSON output.
///
/// Provides static methods to inspect QObjects via Qt's meta-object system:
///   - objectInfo(): Get detailed object information (OBJ-04)
///   - listProperties(): List all properties with values (OBJ-05)
///   - listMethods(): List all invokable methods (OBJ-08)
///   - listSignals(): List all signals (OBJ-10)
///
/// All methods return JSON-friendly output suitable for MCP protocol.
class MetaInspector {
public:
    /// @brief Get detailed object info (OBJ-04).
    ///
    /// Returns a JSON object containing:
    ///   - className: The object's class name
    ///   - objectName: The object's objectName property
    ///   - superClasses: Array of ancestor class names
    ///   - visible: (if QWidget) whether the widget is visible
    ///   - enabled: (if QWidget) whether the widget is enabled
    ///
    /// @param obj The object to inspect.
    /// @return JSON object with object information.
    static QJsonObject objectInfo(QObject* obj);

    /// @brief List all properties (OBJ-05).
    ///
    /// Returns an array of property objects, each containing:
    ///   - name: Property name
    ///   - type: Property type name
    ///   - readable: Whether the property is readable
    ///   - writable: Whether the property is writable
    ///   - value: Current property value (via variantToJson)
    ///
    /// Properties include both the object's own properties and inherited ones.
    ///
    /// @param obj The object to inspect.
    /// @return JSON array of property information.
    static QJsonArray listProperties(QObject* obj);

    /// @brief List all invokable methods (OBJ-08).
    ///
    /// Returns an array of method objects for slots and Q_INVOKABLE methods:
    ///   - name: Method name (without parameters)
    ///   - signature: Full method signature
    ///   - returnType: Return type name (empty for void)
    ///   - parameterTypes: Array of parameter type names
    ///   - parameterNames: Array of parameter names (if available)
    ///   - access: "public", "protected", or "private"
    ///
    /// @param obj The object to inspect.
    /// @return JSON array of method information.
    static QJsonArray listMethods(QObject* obj);

    /// @brief List all signals (OBJ-10).
    ///
    /// Returns an array of signal objects, each containing:
    ///   - name: Signal name (without parameters)
    ///   - signature: Full signal signature
    ///   - parameterTypes: Array of parameter type names
    ///   - parameterNames: Array of parameter names (if available)
    ///
    /// @param obj The object to inspect.
    /// @return JSON array of signal information.
    static QJsonArray listSignals(QObject* obj);

    /// @brief Get inheritance chain.
    ///
    /// Returns a list of class names from the object's class up to QObject.
    /// Example: ["QPushButton", "QAbstractButton", "QWidget", "QObject"]
    ///
    /// @param obj The object to inspect.
    /// @return List of class names in inheritance order.
    static QStringList inheritanceChain(QObject* obj);

private:
    // Helper to convert QMetaMethod::Access to string
    static QString accessSpecifierToString(int access);

    // Helper to extract parameter type names from QMetaMethod
    static QJsonArray extractParameterTypes(const QMetaMethod& method);

    // Helper to extract parameter names from QMetaMethod
    static QJsonArray extractParameterNames(const QMetaMethod& method);
};

}  // namespace qtmcp
