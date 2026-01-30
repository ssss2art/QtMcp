// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>
#include <QPoint>
#include <QString>

namespace qtmcp {

/// @brief Mouse and keyboard input simulation using QTest functions.
///
/// Provides reliable input simulation for UI testing and automation.
/// Uses Qt's QTest module internally for cross-platform compatibility.
///
/// All mouse positions are widget-local coordinates. If no position is
/// specified, the widget center is used.
///
/// Usage:
/// @code
///   // Click a button
///   InputSimulator::mouseClick(button);
///
///   // Type text into a line edit
///   InputSimulator::sendText(lineEdit, "Hello World");
///
///   // Send key combination
///   InputSimulator::sendKeySequence(widget, "Ctrl+S");
/// @endcode
class InputSimulator {
public:
    /// Mouse button types
    enum class MouseButton { Left, Right, Middle };

    /// @brief Simulate mouse click (UI-01).
    /// @param widget Target widget
    /// @param button Mouse button to click
    /// @param pos Position relative to widget (default: center)
    /// @param modifiers Keyboard modifiers (Ctrl, Shift, Alt)
    static void mouseClick(QWidget* widget, MouseButton button = MouseButton::Left,
                          const QPoint& pos = QPoint(),
                          Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /// @brief Simulate mouse double-click.
    /// @param widget Target widget
    /// @param button Mouse button to double-click
    /// @param pos Position relative to widget (default: center)
    /// @param modifiers Keyboard modifiers
    static void mouseDoubleClick(QWidget* widget, MouseButton button = MouseButton::Left,
                                 const QPoint& pos = QPoint(),
                                 Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /// @brief Simulate text input (UI-02).
    /// @param widget Target widget (should be focusable)
    /// @param text Text to type
    static void sendText(QWidget* widget, const QString& text);

    /// @brief Simulate key sequence (UI-02).
    /// @param widget Target widget
    /// @param sequence Key sequence string (e.g., "Ctrl+S", "Alt+F4")
    ///        Accepts standard QKeySequence format strings
    static void sendKeySequence(QWidget* widget, const QString& sequence);

    /// @brief Simulate individual key press.
    /// @param widget Target widget
    /// @param key Qt key code
    /// @param modifiers Keyboard modifiers
    static void sendKey(QWidget* widget, Qt::Key key,
                       Qt::KeyboardModifiers modifiers = Qt::NoModifier);

private:
    /// Convert MouseButton enum to Qt::MouseButton
    static Qt::MouseButton toQtButton(MouseButton button);
};

}  // namespace qtmcp
