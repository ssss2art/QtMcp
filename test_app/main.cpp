// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include <QApplication>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("QtMCP Test App");
  app.setApplicationVersion("0.1.0");
  app.setOrganizationName("QtMCP");

  MainWindow window;
  window.show();

  return app.exec();
}
