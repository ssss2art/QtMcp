// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include "mainwindow.h"

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui_(new Ui::MainWindow) {
  ui_->setupUi(this);

  // Connect signals
  connect(ui_->submitButton, &QPushButton::clicked, this, &MainWindow::OnSubmitClicked);
  connect(ui_->clearButton, &QPushButton::clicked, this, &MainWindow::OnClearClicked);
  connect(ui_->slider, &QSlider::valueChanged, this, &MainWindow::OnSliderChanged);

  // Initialize status bar
  statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() { delete ui_; }

void MainWindow::OnSubmitClicked() {
  QString name = ui_->nameEdit->text();
  QString email = ui_->emailEdit->text();
  QString message = ui_->messageEdit->toPlainText();

  QString result = QString("Name: %1\nEmail: %2\nMessage: %3").arg(name, email, message);
  ui_->resultText->setPlainText(result);

  statusBar()->showMessage("Form submitted", 3000);
}

void MainWindow::OnClearClicked() {
  ui_->nameEdit->clear();
  ui_->emailEdit->clear();
  ui_->messageEdit->clear();
  ui_->resultText->clear();
  ui_->slider->setValue(50);
  ui_->checkBox->setChecked(false);
  ui_->comboBox->setCurrentIndex(0);

  statusBar()->showMessage("Form cleared", 3000);
}

void MainWindow::OnSliderChanged(int value) {
  ui_->sliderValueLabel->setText(QString::number(value));
}
