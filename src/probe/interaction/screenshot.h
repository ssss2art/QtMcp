// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>
#include <QByteArray>
#include <QRect>

namespace qtmcp {

/// @brief Screenshot capture utility (UI-03).
///
/// Provides methods to capture widgets, windows, and regions as PNG images
/// encoded in base64. This is useful for visual debugging, UI verification,
/// and communicating screen state to AI assistants.
///
/// Usage:
/// @code
///   // Capture a widget
///   QByteArray png = Screenshot::captureWidget(button);
///
///   // Capture entire window with frame
///   QByteArray png = Screenshot::captureWindow(mainWindow);
///
///   // Capture a region
///   QByteArray png = Screenshot::captureRegion(widget, QRect(0, 0, 100, 100));
/// @endcode
class Screenshot {
public:
    /// @brief Capture a widget and its children.
    /// @param widget Widget to capture
    /// @return Base64-encoded PNG image
    /// @throws std::invalid_argument if widget is null
    static QByteArray captureWidget(QWidget* widget);

    /// @brief Capture an entire window including frame/decorations.
    /// @param window Window to capture (should be top-level)
    /// @return Base64-encoded PNG image
    /// @throws std::invalid_argument if window is null
    /// @throws std::runtime_error if screen cannot be determined
    static QByteArray captureWindow(QWidget* window);

    /// @brief Capture a rectangular region of a widget.
    /// @param widget Widget to capture from
    /// @param region Rectangle in widget coordinates
    /// @return Base64-encoded PNG image
    /// @throws std::invalid_argument if widget is null
    static QByteArray captureRegion(QWidget* widget, const QRect& region);
};

}  // namespace qtmcp
