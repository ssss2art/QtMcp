// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

// QtMCP Launcher
// Launches a Qt application with the QtMCP probe library injected.
// Works on both Windows (DLL injection) and Linux (LD_PRELOAD).

#include "injector.h"

#include <cstdio>
#include <cstdlib>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace {

/// @brief Find the probe library adjacent to the launcher executable.
/// @return Absolute path to the probe library, or empty string if not found.
QString findProbePath() {
  QDir exeDir(QCoreApplication::applicationDirPath());

#ifdef Q_OS_WIN
  // Look for qtmcp-probe.dll in same directory or parent lib directory
  QStringList searchPaths = {exeDir.filePath("qtmcp-probe.dll"),
                             exeDir.filePath("../lib/qtmcp-probe.dll"),
                             exeDir.filePath("lib/qtmcp-probe.dll")};
#else
  // Linux: look for libqtmcp-probe.so
  QStringList searchPaths = {exeDir.filePath("libqtmcp-probe.so"),
                             exeDir.filePath("../lib/libqtmcp-probe.so"),
                             exeDir.filePath("lib/libqtmcp-probe.so")};
#endif

  for (const QString& path : searchPaths) {
    if (QFileInfo::exists(path)) {
      return QFileInfo(path).absoluteFilePath();
    }
  }

  return QString();
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("qtmcp-launch"));
  app.setApplicationVersion(QStringLiteral("0.1.0"));

  // Set up command line parser
  QCommandLineParser parser;
  parser.setApplicationDescription(
      QStringLiteral("Launch Qt applications with QtMCP probe injected"));
  parser.addHelpOption();
  parser.addVersionOption();

  // Define options per CONTEXT.md decisions
  QCommandLineOption portOption(QStringList() << QStringLiteral("p") << QStringLiteral("port"),
                                QStringLiteral("WebSocket port for probe (default: 9222)"),
                                QStringLiteral("port"), QStringLiteral("9222"));
  parser.addOption(portOption);

  QCommandLineOption detachOption(QStringList() << QStringLiteral("d") << QStringLiteral("detach"),
                                  QStringLiteral("Run in background, don't wait for target"));
  parser.addOption(detachOption);

  QCommandLineOption quietOption(QStringList() << QStringLiteral("q") << QStringLiteral("quiet"),
                                 QStringLiteral("Suppress startup messages"));
  parser.addOption(quietOption);

  QCommandLineOption probeOption(
      QStringLiteral("probe"),
      QStringLiteral("Path to probe library (auto-detected if not specified)"),
      QStringLiteral("path"));
  parser.addOption(probeOption);

  // Positional arguments: target executable and its arguments
  parser.addPositionalArgument(QStringLiteral("target"),
                               QStringLiteral("Target executable to launch"));
  parser.addPositionalArgument(QStringLiteral("args"),
                               QStringLiteral("Arguments to pass to target"),
                               QStringLiteral("[args...]"));

  // Parse arguments
  parser.process(app);

  // Get positional arguments
  QStringList positionalArgs = parser.positionalArguments();
  if (positionalArgs.isEmpty()) {
    fprintf(stderr, "Error: No target executable specified\n\n");
    parser.showHelp(1);  // Exits with code 1
  }

  // Build LaunchOptions
  qtmcp::LaunchOptions options;
  options.targetExecutable = positionalArgs.takeFirst();
  options.targetArgs = positionalArgs;  // Remaining args go to target
  options.detach = parser.isSet(detachOption);
  options.quiet = parser.isSet(quietOption);

  // Parse and validate port
  bool portOk = false;
  int portValue = parser.value(portOption).toInt(&portOk);
  if (!portOk || portValue <= 0 || portValue > 65535) {
    fprintf(stderr, "Error: Invalid port value '%s' (must be 1-65535)\n",
            qPrintable(parser.value(portOption)));
    return 1;
  }
  options.port = static_cast<quint16>(portValue);

  // Resolve target executable path
  QFileInfo targetInfo(options.targetExecutable);
  if (!targetInfo.exists()) {
    // Try to find in PATH
    QString pathEnv = QString::fromLocal8Bit(qgetenv("PATH"));
#ifdef Q_OS_WIN
    QStringList pathDirs = pathEnv.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    QStringList extensions = {QStringLiteral(".exe"), QStringLiteral(".com"), QStringLiteral("")};
#else
    QStringList pathDirs = pathEnv.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    QStringList extensions = {QStringLiteral("")};
#endif
    bool found = false;
    for (const QString& dir : pathDirs) {
      for (const QString& ext : extensions) {
        QString candidate = QDir(dir).filePath(options.targetExecutable + ext);
        if (QFileInfo::exists(candidate)) {
          options.targetExecutable = QFileInfo(candidate).absoluteFilePath();
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
    if (!found) {
      fprintf(stderr, "Error: Target executable not found: %s\n",
              qPrintable(options.targetExecutable));
      return 1;
    }
  } else {
    options.targetExecutable = targetInfo.absoluteFilePath();
  }

  // Resolve probe path
  if (parser.isSet(probeOption)) {
    options.probePath = parser.value(probeOption);
    if (!QFileInfo::exists(options.probePath)) {
      fprintf(stderr, "Error: Probe library not found: %s\n", qPrintable(options.probePath));
      return 1;
    }
    options.probePath = QFileInfo(options.probePath).absoluteFilePath();
  } else {
    options.probePath = findProbePath();
    if (options.probePath.isEmpty()) {
      fprintf(stderr, "Error: Could not find QtMCP probe library\n");
      fprintf(stderr, "Use --probe option to specify the path\n");
      return 1;
    }
  }

  // Print startup message unless quiet
  if (!options.quiet) {
    fprintf(stderr, "[qtmcp-launch] Target: %s\n", qPrintable(options.targetExecutable));
    fprintf(stderr, "[qtmcp-launch] Probe: %s\n", qPrintable(options.probePath));
    fprintf(stderr, "[qtmcp-launch] Port: %u, Detach: %s\n", static_cast<unsigned>(options.port),
            options.detach ? "yes" : "no");
  }

  // Launch with probe injection
  qint64 pid = qtmcp::launchWithProbe(options);
  if (pid < 0) {
    fprintf(stderr, "Error: Failed to launch target with probe\n");
    return 1;
  }

  // Success
  if (!options.quiet) {
    fprintf(stderr, "[qtmcp-launch] Started process with PID %lld\n", static_cast<long long>(pid));
  }

  return 0;
}
