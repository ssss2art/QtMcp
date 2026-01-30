// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "input_simulator.h"
#include <QTest>
#include <QKeySequence>
#include <QApplication>
#include <stdexcept>

namespace qtmcp {

void InputSimulator::mouseClick(QWidget* widget, MouseButton button,
                                const QPoint& pos, Qt::KeyboardModifiers modifiers) {
    if (!widget) {
        throw std::invalid_argument("mouseClick: widget cannot be null");
    }

    // Use widget center if no position specified
    QPoint clickPos = pos.isNull() ? widget->rect().center() : pos;

    // Ensure widget is visible and ready for input
    widget->activateWindow();
    widget->raise();
    QApplication::processEvents();

    QTest::mouseClick(widget, toQtButton(button), modifiers, clickPos);
}

void InputSimulator::mouseDoubleClick(QWidget* widget, MouseButton button,
                                      const QPoint& pos, Qt::KeyboardModifiers modifiers) {
    if (!widget) {
        throw std::invalid_argument("mouseDoubleClick: widget cannot be null");
    }

    // Use widget center if no position specified
    QPoint clickPos = pos.isNull() ? widget->rect().center() : pos;

    // Ensure widget is visible and ready for input
    widget->activateWindow();
    widget->raise();
    QApplication::processEvents();

    QTest::mouseDClick(widget, toQtButton(button), modifiers, clickPos);
}

void InputSimulator::sendText(QWidget* widget, const QString& text) {
    if (!widget) {
        throw std::invalid_argument("sendText: widget cannot be null");
    }

    // Ensure widget has focus for keyboard input
    widget->setFocus();
    QApplication::processEvents();

    // QTest::keyClicks sends each character as a key event
    QTest::keyClicks(widget, text);
}

void InputSimulator::sendKeySequence(QWidget* widget, const QString& sequence) {
    if (!widget) {
        throw std::invalid_argument("sendKeySequence: widget cannot be null");
    }

    // Parse sequence string like "Ctrl+Shift+A" or "Ctrl+S"
    QKeySequence keySeq(sequence, QKeySequence::PortableText);

    if (keySeq.isEmpty()) {
        throw std::invalid_argument("sendKeySequence: invalid key sequence '" +
                                    sequence.toStdString() + "'");
    }

    widget->setFocus();
    QApplication::processEvents();

    // Extract key and modifiers from first key combination
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    int key = keySeq[0].key();
    Qt::KeyboardModifiers mods = keySeq[0].keyboardModifiers();
#else
    int key = keySeq[0] & ~Qt::KeyboardModifierMask;
    Qt::KeyboardModifiers mods = Qt::KeyboardModifiers(keySeq[0] & Qt::KeyboardModifierMask);
#endif

    QTest::keyClick(widget, static_cast<Qt::Key>(key), mods);
}

void InputSimulator::sendKey(QWidget* widget, Qt::Key key,
                            Qt::KeyboardModifiers modifiers) {
    if (!widget) {
        throw std::invalid_argument("sendKey: widget cannot be null");
    }

    widget->setFocus();
    QApplication::processEvents();

    QTest::keyClick(widget, key, modifiers);
}

Qt::MouseButton InputSimulator::toQtButton(MouseButton button) {
    switch (button) {
        case MouseButton::Left:   return Qt::LeftButton;
        case MouseButton::Right:  return Qt::RightButton;
        case MouseButton::Middle: return Qt::MiddleButton;
    }
    return Qt::LeftButton;  // Default fallback
}

}  // namespace qtmcp
