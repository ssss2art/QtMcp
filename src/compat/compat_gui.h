// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QKeySequence>
#include <QtGlobal>

namespace qtmcp {
namespace compat {

/// Extracts the key and modifiers from a QKeySequence at the given index.
/// Qt6: Uses QKeyCombination::key() and QKeyCombination::keyboardModifiers().
/// Qt5: Uses bitmask extraction on the integer key code.
inline void extractKeyCombination(const QKeySequence& seq, int index, Qt::Key& key,
                                  Qt::KeyboardModifiers& mods) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  key = seq[index].key();
  mods = seq[index].keyboardModifiers();
#else
  key = static_cast<Qt::Key>(seq[index] & ~Qt::KeyboardModifierMask);
  mods = Qt::KeyboardModifiers(seq[index] & Qt::KeyboardModifierMask);
#endif
}

}  // namespace compat
}  // namespace qtmcp
