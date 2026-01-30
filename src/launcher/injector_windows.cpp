// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

// Windows implementation of probe injection using CreateRemoteThread.
// This file is only compiled on Windows (see CMakeLists.txt).

#include "injector.h"

#ifdef Q_OS_WIN

#include <QProcess>
#include <QFileInfo>

#include <Windows.h>

#include <cstdio>

namespace qtmcp {

qint64 launchWithProbe(const LaunchOptions& options) {
    // Placeholder implementation for Task 1 verification
    // Full implementation will be added in Task 2

    if (!options.quiet) {
        fprintf(stderr, "[injector] Windows injection not yet implemented\n");
        fprintf(stderr, "[injector] Would inject: %s\n", qPrintable(options.probePath));
        fprintf(stderr, "[injector] Into target: %s\n", qPrintable(options.targetExecutable));
    }

    // For now, return -1 to indicate not implemented
    return -1;
}

}  // namespace qtmcp

#endif  // Q_OS_WIN
