// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

// QtMCP Launcher for Windows
// Launches a Qt application with the QtMCP probe library loaded

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#include <iostream>

void PrintUsage(const char* program_name) {
  std::cout << "QtMCP Launcher - Launch Qt applications with QtMCP probe\n\n";
  std::cout << "Usage: " << program_name << " [options] <application> [app-args...]\n\n";
  std::cout << "Options:\n";
  std::cout << "  --port <port>    WebSocket server port (default: 9999)\n";
  std::cout << "  --mode <mode>    API mode: native, computer_use, chrome, all (default: all)\n";
  std::cout << "  --disabled       Start with probe disabled\n";
  std::cout << "  --help           Show this help message\n";
  std::cout << "\nExamples:\n";
  std::cout << "  " << program_name << " myapp.exe\n";
  std::cout << "  " << program_name << " --port 8888 myapp.exe --app-arg1\n";
}

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);

  // Parse arguments
  QStringList args = app.arguments();
  QString application;
  QStringList app_args;
  int port = 9999;
  QString mode = "all";
  bool disabled = false;

  for (int i = 1; i < args.size(); ++i) {
    const QString& arg = args[i];

    if (arg == "--help" || arg == "-h") {
      PrintUsage(argv[0]);
      return 0;
    }
    if (arg == "--port" && i + 1 < args.size()) {
      port = args[++i].toInt();
      continue;
    }
    if (arg == "--mode" && i + 1 < args.size()) {
      mode = args[++i];
      continue;
    }
    if (arg == "--disabled") {
      disabled = true;
      continue;
    }

    // First non-option argument is the application
    if (application.isEmpty()) {
      application = arg;
    } else {
      // Remaining arguments are passed to the application
      app_args << arg;
    }
  }

  if (application.isEmpty()) {
    std::cerr << "Error: No application specified\n\n";
    PrintUsage(argv[0]);
    return 1;
  }

  // Find the probe library
  QDir exe_dir(QCoreApplication::applicationDirPath());
  QString probe_path;

#ifdef Q_OS_WIN
  // Look for qtmcp.dll in same directory or lib subdirectory
  QStringList search_paths = {exe_dir.filePath("qtmcp.dll"),
                              exe_dir.filePath("../lib/qtmcp.dll")};

  for (const QString& path : search_paths) {
    if (QFileInfo::exists(path)) {
      probe_path = QFileInfo(path).absoluteFilePath();
      break;
    }
  }
#endif

  if (probe_path.isEmpty()) {
    std::cerr << "Error: Could not find QtMCP probe library\n";
    return 1;
  }

  // Set environment variables
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("QTMCP_PORT", QString::number(port));
  env.insert("QTMCP_MODE", mode);
  env.insert("QTMCP_ENABLED", disabled ? "0" : "1");

#ifdef Q_OS_WIN
  // On Windows, we need to use a different approach since we can't use LD_PRELOAD
  // For MVP, we assume the application will load qtmcp.dll explicitly
  // or we use the PATH to make it available
  QString path = env.value("PATH");
  path = QFileInfo(probe_path).absolutePath() + ";" + path;
  env.insert("PATH", path);
#endif

  std::cout << "Starting: " << application.toStdString() << "\n";
  std::cout << "QtMCP probe: " << probe_path.toStdString() << "\n";
  std::cout << "Port: " << port << ", Mode: " << mode.toStdString() << "\n";

  // Launch the application
  QProcess process;
  process.setProcessEnvironment(env);
  process.setProgram(application);
  process.setArguments(app_args);

  // Forward stdin/stdout/stderr
  process.setProcessChannelMode(QProcess::ForwardedChannels);

  process.start();

  if (!process.waitForStarted()) {
    std::cerr << "Error: Failed to start application: "
              << process.errorString().toStdString() << "\n";
    return 1;
  }

  // Wait for process to finish
  process.waitForFinished(-1);

  return process.exitCode();
}
