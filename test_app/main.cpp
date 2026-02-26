// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include <QApplication>
#include <QStringList>

#include "childwindow.h"
#include "mainwindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("QtMCP Test App");
  app.setApplicationVersion("0.1.0");
  app.setOrganizationName("QtMCP");

  // --child mode: show a simple child window (used to test child-process injection)
  if (app.arguments().contains(QStringLiteral("--child"))) {
    ChildWindow child;
    child.show();
    return app.exec();
  }

  MainWindow window;
  window.show();

  return app.exec();
}
