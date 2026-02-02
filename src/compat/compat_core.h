// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QMetaType>
#include <QtGlobal>

namespace qtmcp {
namespace compat {

/// Returns the QMetaType ID for the given type name.
/// Qt6: QMetaType::fromName(name).id()
/// Qt5: QMetaType::type(name)
inline int metaTypeIdFromName(const char* name) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return QMetaType::fromName(name).id();
#else
  return QMetaType::type(name);
#endif
}

}  // namespace compat
}  // namespace qtmcp
