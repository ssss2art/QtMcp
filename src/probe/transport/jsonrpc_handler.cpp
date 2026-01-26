// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "transport/jsonrpc_handler.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace qtmcp {

using json = nlohmann::json;

JsonRpcHandler::JsonRpcHandler(QObject* parent) : QObject(parent) { RegisterBuiltinMethods(); }

QString JsonRpcHandler::HandleMessage(const QString& message) {
  // Parse JSON
  json request;
  try {
    request = json::parse(message.toStdString());
  } catch (const json::parse_error& e) {
    spdlog::error("JSON parse error: {}", e.what());
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

  spdlog::debug("Handling method: {} with params: {}", method.toStdString(),
                params_str.toStdString());

  // Find and invoke method handler
  auto it = methods_.find(method);
  if (it == methods_.end()) {
    if (is_notification) {
      // Notifications don't get error responses
      spdlog::warn("Unknown notification method: {}", method.toStdString());
      return "";
    }
    return CreateErrorResponse(id_str, JsonRpcError::kMethodNotFound,
                               QString("Method not found: %1").arg(method));
  }

  try {
    QString result = it->second(params_str);
    if (is_notification) {
      return "";  // No response for notifications
    }
    return CreateSuccessResponse(id_str, result);
  } catch (const std::exception& e) {
    spdlog::error("Method {} threw exception: {}", method.toStdString(), e.what());
    if (is_notification) {
      return "";
    }
    return CreateErrorResponse(id_str, JsonRpcError::kInternalError,
                               QString("Internal error: %1").arg(e.what()));
  }
}

void JsonRpcHandler::RegisterMethod(const QString& method, MethodHandler handler) {
  methods_[method] = std::move(handler);
  spdlog::debug("Registered method: {}", method.toStdString());
}

void JsonRpcHandler::UnregisterMethod(const QString& method) {
  methods_.erase(method);
  spdlog::debug("Unregistered method: {}", method.toStdString());
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
    json result;
    result["version"] = "0.1.0";
    result["protocol"] = "jsonrpc-2.0";
    result["name"] = "QtMCP";
    return QString::fromStdString(result.dump());
  });

  // getModes - return available API modes
  RegisterMethod("getModes", [](const QString& /*params*/) -> QString {
    json result = json::array({"native", "computer_use", "chrome"});
    return QString::fromStdString(result.dump());
  });

  // echo - echo back params (for testing)
  RegisterMethod("echo", [](const QString& params) -> QString { return params; });
}

}  // namespace qtmcp
