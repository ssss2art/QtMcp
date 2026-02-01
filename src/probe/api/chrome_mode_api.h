// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

#include "transport/jsonrpc_handler.h"  // For QTMCP_EXPORT, JsonRpcHandler

namespace qtmcp {

/// @brief Chrome Mode API surface registering all chr.* namespaced JSON-RPC methods.
///
/// This is the core deliverable of Phase 5 - the accessibility-based API surface
/// that provides Chrome-compatible element interaction via accessibility tree refs.
///
/// Registers 8 methods:
///   - chr.readPage           - Read accessibility tree with numbered refs
///   - chr.click              - Click element by ref
///   - chr.formInput          - Set form input value by ref
///   - chr.getPageText        - Get all visible text from active window
///   - chr.find               - Find elements by natural language query
///   - chr.navigate           - Activate tabs/menu items by ref
///   - chr.tabsContext        - List all top-level windows
///   - chr.readConsoleMessages - Read captured qDebug/qWarning messages
///
/// All methods use ephemeral ref identifiers (rebuilt on each readPage call).
/// All responses wrapped with ResponseEnvelope::wrap().
class QTMCP_EXPORT ChromeModeApi : public QObject {
    Q_OBJECT

public:
    /// @brief Construct and register all chr.* methods on the given handler.
    /// @param handler The JSON-RPC handler to register methods on.
    /// @param parent Parent QObject.
    explicit ChromeModeApi(JsonRpcHandler* handler, QObject* parent = nullptr);

    /// @brief Clear all ephemeral ref mappings (called on client disconnect).
    static void clearRefs();

private:
    void registerReadPageMethod();
    void registerClickMethod();
    void registerFormInputMethod();
    void registerGetPageTextMethod();
    void registerFindMethod();
    void registerNavigateMethod();
    void registerTabsContextMethod();
    void registerReadConsoleMessagesMethod();

    JsonRpcHandler* m_handler;
};

}  // namespace qtmcp
