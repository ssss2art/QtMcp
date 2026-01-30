// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

// Linux implementation of probe injection using LD_PRELOAD.
// This file is only compiled on Linux (see CMakeLists.txt).

#include "injector.h"

#ifndef Q_OS_WIN

#include <QProcess>
#include <QProcessEnvironment>
#include <QFileInfo>

#include <cstdio>

namespace qtmcp {

qint64 launchWithProbe(const LaunchOptions& options) {
    // Placeholder implementation for Task 1 verification
    // Full implementation will be added in Task 2

    if (!options.quiet) {
        fprintf(stderr, "[injector] Linux injection not yet implemented\n");
        fprintf(stderr, "[injector] Would set LD_PRELOAD: %s\n", qPrintable(options.probePath));
        fprintf(stderr, "[injector] For target: %s\n", qPrintable(options.targetExecutable));
    }

    // For now, return -1 to indicate not implemented
    return -1;
}

}  // namespace qtmcp

#endif  // !Q_OS_WIN
