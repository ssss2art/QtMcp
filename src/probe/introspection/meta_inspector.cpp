// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "meta_inspector.h"
#include "variant_json.h"

#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QWidget>

namespace qtmcp {

QJsonObject MetaInspector::objectInfo(QObject* obj)
{
    if (!obj) {
        return QJsonObject();
    }

    QJsonObject info;
    info[QStringLiteral("className")] = QString::fromLatin1(obj->metaObject()->className());
    info[QStringLiteral("objectName")] = obj->objectName();
    info[QStringLiteral("superClasses")] = QJsonArray::fromStringList(inheritanceChain(obj));

    // Add widget-specific info
    if (auto* widget = qobject_cast<QWidget*>(obj)) {
        info[QStringLiteral("visible")] = widget->isVisible();
        info[QStringLiteral("enabled")] = widget->isEnabled();
        // Note: Geometry will be added via separate function in Plan 05 (UI Interaction)
    }

    return info;
}

QJsonArray MetaInspector::listProperties(QObject* obj)
{
    if (!obj) {
        return QJsonArray();
    }

    QJsonArray result;
    const QMetaObject* meta = obj->metaObject();

    // Iterate through all properties (including inherited from QObject)
    // Using 0 instead of propertyOffset() to include QObject base properties
    // like objectName, which are often useful
    for (int i = 0; i < meta->propertyCount(); ++i) {
        QMetaProperty prop = meta->property(i);

        QJsonObject propInfo;
        propInfo[QStringLiteral("name")] = QString::fromLatin1(prop.name());
        propInfo[QStringLiteral("type")] = QString::fromLatin1(prop.typeName());
        propInfo[QStringLiteral("readable")] = prop.isReadable();
        propInfo[QStringLiteral("writable")] = prop.isWritable();

        // Include current value if readable
        if (prop.isReadable()) {
            QVariant value = prop.read(obj);
            propInfo[QStringLiteral("value")] = variantToJson(value);
        } else {
            propInfo[QStringLiteral("value")] = QJsonValue();
        }

        result.append(propInfo);
    }

    return result;
}

QJsonArray MetaInspector::listMethods(QObject* obj)
{
    if (!obj) {
        return QJsonArray();
    }

    QJsonArray result;
    const QMetaObject* meta = obj->metaObject();

    // Iterate through all methods
    for (int i = 0; i < meta->methodCount(); ++i) {
        QMetaMethod method = meta->method(i);

        // Filter for Slot or Invokable methods only (not signals or constructors)
        if (method.methodType() != QMetaMethod::Slot &&
            method.methodType() != QMetaMethod::Method) {  // Q_INVOKABLE is Method type
            continue;
        }

        QJsonObject methodInfo;
        methodInfo[QStringLiteral("name")] = QString::fromLatin1(method.name());
        methodInfo[QStringLiteral("signature")] = QString::fromLatin1(method.methodSignature());

        // Return type (empty string for void)
        QString returnType = QString::fromLatin1(method.typeName());
        methodInfo[QStringLiteral("returnType")] = returnType;

        // Parameter types and names
        methodInfo[QStringLiteral("parameterTypes")] = extractParameterTypes(method);
        methodInfo[QStringLiteral("parameterNames")] = extractParameterNames(method);

        // Access specifier
        methodInfo[QStringLiteral("access")] = accessSpecifierToString(method.access());

        result.append(methodInfo);
    }

    return result;
}

QJsonArray MetaInspector::listSignals(QObject* obj)
{
    if (!obj) {
        return QJsonArray();
    }

    QJsonArray result;
    const QMetaObject* meta = obj->metaObject();

    // Iterate through all methods looking for signals
    for (int i = 0; i < meta->methodCount(); ++i) {
        QMetaMethod method = meta->method(i);

        // Filter for Signal methods only
        if (method.methodType() != QMetaMethod::Signal) {
            continue;
        }

        QJsonObject signalInfo;
        signalInfo[QStringLiteral("name")] = QString::fromLatin1(method.name());
        signalInfo[QStringLiteral("signature")] = QString::fromLatin1(method.methodSignature());
        signalInfo[QStringLiteral("parameterTypes")] = extractParameterTypes(method);
        signalInfo[QStringLiteral("parameterNames")] = extractParameterNames(method);

        result.append(signalInfo);
    }

    return result;
}

QStringList MetaInspector::inheritanceChain(QObject* obj)
{
    QStringList chain;

    if (!obj) {
        return chain;
    }

    const QMetaObject* meta = obj->metaObject();
    while (meta) {
        chain.append(QString::fromLatin1(meta->className()));
        meta = meta->superClass();
    }

    return chain;
}

QString MetaInspector::accessSpecifierToString(int access)
{
    switch (access) {
    case QMetaMethod::Private:
        return QStringLiteral("private");
    case QMetaMethod::Protected:
        return QStringLiteral("protected");
    case QMetaMethod::Public:
    default:
        return QStringLiteral("public");
    }
}

QJsonArray MetaInspector::extractParameterTypes(const QMetaMethod& method)
{
    QJsonArray types;
    for (const QByteArray& type : method.parameterTypes()) {
        types.append(QString::fromLatin1(type));
    }
    return types;
}

QJsonArray MetaInspector::extractParameterNames(const QMetaMethod& method)
{
    QJsonArray names;
    for (const QByteArray& name : method.parameterNames()) {
        names.append(QString::fromLatin1(name));
    }
    return names;
}

}  // namespace qtmcp
