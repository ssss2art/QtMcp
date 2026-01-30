// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "transport/jsonrpc_handler.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#ifdef QTMCP_HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

#ifdef QTMCP_HAS_SPDLOG
#include <spdlog/spdlog.h>
#define LOG_DEBUG(msg) spdlog::debug(msg)
#define LOG_WARN(msg) spdlog::warn(msg)
#define LOG_ERROR(msg) spdlog::error(msg)
#else
#define LOG_DEBUG(msg) qDebug() << msg
#define LOG_WARN(msg) qWarning() << msg
#define LOG_ERROR(msg) qCritical() << msg
#endif

namespace qtmcp {

#ifdef QTMCP_HAS_NLOHMANN_JSON
using json = nlohmann::json;
#endif

JsonRpcHandler::JsonRpcHandler(QObject* parent) : QObject(parent) { RegisterBuiltinMethods(); }

QString JsonRpcHandler::HandleMessage(const QString& message) {
#ifdef QTMCP_HAS_NLOHMANN_JSON
  // Use nlohmann_json for parsing
  json request;
  try {
    request = json::parse(message.toStdString());
  } catch (const json::parse_error& e) {
    qCritical() << "JSON parse error:" << e.what();
    return CreateErrorResponse("null", JsonRpcError::kParseError, "Parse error");
  }

  // Validate JSON-RPC 2.0 structure
  if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
    return CreateErrorResponse("null", JsonRpcError::kInvalidRequest,
                               "Invalid Request: missing or invalid jsonrpc version");
  }

  if (!request.contains("method") || !request["method"].is_string()) {
    return CreateErrorResponse("null", JsonRpcError::kInvalidRequest,
                               "Invalid Request: missing or invalid method");
  }

  QString method = QString::fromStdString(request["method"].get<std::string>());
  QString id_str = "null";
  bool is_notification = !request.contains("id");

  if (!is_notification) {
    if (request["id"].is_string()) {
      id_str = QString("\"%1\"").arg(QString::fromStdString(request["id"].get<std::string>()));
    } else if (request["id"].is_number()) {
      id_str = QString::number(request["id"].get<int>());
    } else if (request["id"].is_null()) {
      id_str = "null";
    }
  }

  // Get params (default to empty object)
  QString params_str = "{}";
  if (request.contains("params")) {
    params_str = QString::fromStdString(request["params"].dump());
  }
#else
  // Use QJsonDocument for parsing
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCritical() << "JSON parse error:" << parseError.errorString();
    return CreateErrorResponse("null", JsonRpcError::kParseError, "Parse error");
  }

  QJsonObject request = doc.object();

  // Validate JSON-RPC 2.0 structure
  if (!request.contains("jsonrpc") || request["jsonrpc"].toString() != "2.0") {
    return CreateErrorResponse("null", JsonRpcError::kInvalidRequest,
                               "Invalid Request: missing or invalid jsonrpc version");
  }

  if (!request.contains("method") || !request["method"].isString()) {
    return CreateErrorResponse("null", JsonRpcError::kInvalidRequest,
                               "Invalid Request: missing or invalid method");
  }

  QString method = request["method"].toString();
  QString id_str = "null";
  bool is_notification = !request.contains("id");

  if (!is_notification) {
    QJsonValue idValue = request["id"];
    if (idValue.isString()) {
      id_str = QString("\"%1\"").arg(idValue.toString());
    } else if (idValue.isDouble()) {
      id_str = QString::number(static_cast<int>(idValue.toDouble()));
    } else if (idValue.isNull()) {
      id_str = "null";
    }
  }

  // Get params (default to empty object)
  QString params_str = "{}";
  if (request.contains("params")) {
    QJsonValue paramsValue = request["params"];
    if (paramsValue.isObject()) {
      params_str = QString::fromUtf8(QJsonDocument(paramsValue.toObject()).toJson(QJsonDocument::Compact));
    } else if (paramsValue.isArray()) {
      params_str = QString::fromUtf8(QJsonDocument(paramsValue.toArray()).toJson(QJsonDocument::Compact));
    }
  }
#endif

  qDebug() << "Handling method:" << method << "with params:" << params_str;

  // Handle notifications by emitting signal and returning empty
  if (is_notification) {
#ifdef QTMCP_HAS_NLOHMANN_JSON
    QJsonValue paramsValue;
    if (request.contains("params")) {
      // Convert nlohmann::json to QJsonValue
      QString paramsJson = QString::fromStdString(request["params"].dump());
      paramsValue = QJsonDocument::fromJson(paramsJson.toUtf8()).object();
    }
#else
    QJsonValue paramsValue = request.value("params");
#endif
    emit NotificationReceived(method, paramsValue);
    return QString();  // No response for notifications
  }

  // Find and invoke method handler
  auto it = methods_.find(method);
  if (it == methods_.end()) {
    return CreateErrorResponse(id_str, JsonRpcError::kMethodNotFound,
                               QString("Method not found: %1").arg(method));
  }

  try {
    QString result = it->second(params_str);
    return CreateSuccessResponse(id_str, result);
  } catch (const std::exception& e) {
    qCritical() << "Method" << method << "threw exception:" << e.what();
    return CreateErrorResponse(id_str, JsonRpcError::kInternalError,
                               QString("Internal error: %1").arg(e.what()));
  }
}

void JsonRpcHandler::RegisterMethod(const QString& method, MethodHandler handler) {
  methods_[method] = std::move(handler);
  qDebug() << "Registered method:" << method;
}

void JsonRpcHandler::UnregisterMethod(const QString& method) {
  methods_.erase(method);
  qDebug() << "Unregistered method:" << method;
}

QString JsonRpcHandler::CreateSuccessResponse(const QString& id, const QString& result) {
  return QString(R"({"jsonrpc":"2.0","id":%1,"result":%2})").arg(id, result);
}

QString JsonRpcHandler::CreateErrorResponse(const QString& id, int code, const QString& message) {
  // Escape message for JSON
  QString escaped_message = message;
  escaped_message.replace("\\", "\\\\");
  escaped_message.replace("\"", "\\\"");
  escaped_message.replace("\n", "\\n");
  escaped_message.replace("\r", "\\r");
  escaped_message.replace("\t", "\\t");

  return QString(R"({"jsonrpc":"2.0","id":%1,"error":{"code":%2,"message":"%3"}})")
      .arg(id)
      .arg(code)
      .arg(escaped_message);
}

void JsonRpcHandler::RegisterBuiltinMethods() {
  // ping - basic connectivity test
  RegisterMethod("ping", [](const QString& /*params*/) -> QString { return R"("pong")"; });

  // getVersion - return QtMCP version info
  RegisterMethod("getVersion", [](const QString& /*params*/) -> QString {
    QJsonObject result;
    result["version"] = "0.1.0";
    result["protocol"] = "jsonrpc-2.0";
    result["name"] = "QtMCP";
    return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
  });

  // getModes - return available API modes
  RegisterMethod("getModes", [](const QString& /*params*/) -> QString {
    QJsonArray modes;
    modes.append("native");
    modes.append("computer_use");
    modes.append("chrome");
    return QString::fromUtf8(QJsonDocument(modes).toJson(QJsonDocument::Compact));
  });

  // echo - echo back params (for testing)
  RegisterMethod("echo", [](const QString& params) -> QString { return params; });

  // qtmcp.echo - namespaced echo for integration testing (per RESEARCH.md spec)
  RegisterMethod("qtmcp.echo", [](const QString& params) -> QString { return params; });
}

}  // namespace qtmcp
