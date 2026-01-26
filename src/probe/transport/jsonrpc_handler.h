// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>

#include <functional>
#include <unordered_map>

namespace qtmcp {

/// @brief JSON-RPC 2.0 message handler.
///
/// This class parses incoming JSON-RPC requests, dispatches them to
/// registered method handlers, and formats the responses.
class JsonRpcHandler : public QObject {
  Q_OBJECT

 public:
  /// @brief Method handler function type.
  /// Takes JSON params, returns JSON result or throws for errors.
  using MethodHandler = std::function<QString(const QString& params)>;

  /// @brief Construct a JSON-RPC handler.
  /// @param parent Parent QObject.
  explicit JsonRpcHandler(QObject* parent = nullptr);

  /// @brief Destructor.
  ~JsonRpcHandler() override = default;

  /// @brief Handle an incoming JSON-RPC message.
  /// @param message The raw JSON message string.
  /// @return The JSON response string, or empty for notifications.
  QString HandleMessage(const QString& message);

  /// @brief Register a method handler.
  /// @param method Method name.
  /// @param handler Handler function.
  void RegisterMethod(const QString& method, MethodHandler handler);

  /// @brief Unregister a method handler.
  /// @param method Method name.
  void UnregisterMethod(const QString& method);

 signals:
  /// @brief Emitted when a notification should be sent to clients.
  /// @param notification The JSON notification string.
  void NotificationReady(const QString& notification);

 private:
  /// @brief Create a JSON-RPC success response.
  /// @param id Request ID.
  /// @param result Result value as JSON string.
  /// @return Formatted JSON-RPC response.
  QString CreateSuccessResponse(const QString& id, const QString& result);

  /// @brief Create a JSON-RPC error response.
  /// @param id Request ID (can be null for parse errors).
  /// @param code Error code.
  /// @param message Error message.
  /// @return Formatted JSON-RPC error response.
  QString CreateErrorResponse(const QString& id, int code, const QString& message);

  /// @brief Register built-in methods (ping, getVersion, etc.).
  void RegisterBuiltinMethods();

  std::unordered_map<QString, MethodHandler> methods_;
};

// JSON-RPC 2.0 error codes
namespace JsonRpcError {
constexpr int kParseError = -32700;
constexpr int kInvalidRequest = -32600;
constexpr int kMethodNotFound = -32601;
constexpr int kInvalidParams = -32602;
constexpr int kInternalError = -32603;
// Server errors: -32000 to -32099
constexpr int kServerError = -32000;
}  // namespace JsonRpcError

}  // namespace qtmcp
