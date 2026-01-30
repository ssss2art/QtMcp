// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "screenshot.h"
#include <QPixmap>
#include <QBuffer>
#include <QScreen>
#include <QGuiApplication>
#include <stdexcept>

namespace qtmcp {

QByteArray Screenshot::captureWidget(QWidget* widget) {
    if (!widget) {
        throw std::invalid_argument("captureWidget: widget cannot be null");
    }

    // QWidget::grab() captures the widget and all children
    QPixmap pixmap = widget->grab();

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");

    return bytes.toBase64();
}

QByteArray Screenshot::captureWindow(QWidget* window) {
    if (!window) {
        throw std::invalid_argument("captureWindow: window cannot be null");
    }

    // Use screen grab for window including frame
    QScreen* screen = window->screen();
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (!screen) {
        throw std::runtime_error("captureWindow: cannot determine screen for screenshot");
    }

    QPixmap pixmap = screen->grabWindow(window->winId());

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");

    return bytes.toBase64();
}

QByteArray Screenshot::captureRegion(QWidget* widget, const QRect& region) {
    if (!widget) {
        throw std::invalid_argument("captureRegion: widget cannot be null");
    }

    QPixmap pixmap = widget->grab(region);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");

    return bytes.toBase64();
}

}  // namespace qtmcp
