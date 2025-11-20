/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2025 Tencent. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "ExportWindow.h"
#include <QFileDialog>
#include <QQmlContext>
#include <QThread>
#include "export/PAGExport.h"
#include "platform//PlatformHelper.h"
#include "ui/ProgressModel.h"
#include "utils/FileHelper.h"

namespace exporter {

ExportWindow::ExportWindow(QApplication* app, const std::string& outputPath, QObject* parent)
    : BaseWindow(app, parent), showAlertInfo(outputPath.empty()), outputPath(outputPath) {
  init();
}

void ExportWindow::show() {
  finished = false;
  if (pagExport == nullptr) {
    if (initAttempted) {
      init();
    }
    if (pagExport == nullptr) {
      finished = true;
      initAttempted = true;
      return;
    }
  }
  BaseWindow::show();

  bool result = pagExport->exportFile();
  if (result) {
    pagExport->session->progressModel.setExportStatus(ProgressModel::ExportStatus::Success);
  } else {
    pagExport->session->progressModel.setExportStatus(ProgressModel::ExportStatus::Error);
  }
  QQmlContext* context = engine->rootContext();
  context->setContextProperty("progressModel", nullptr);
  pagExport.reset();
  if (result) {
    FileHelper::OpenPAGFile(outputPath);
  }
  finished = true;
}

void ExportWindow::onWindowClosing() {
  if (pagExport != nullptr && pagExport->session != nullptr) {
    pagExport->session->stopExport = true;
  }
  finished = true;
  BaseWindow::onWindowClosing();
}

std::string ExportWindow::getOutputPath() {
  if (itemH == nullptr) {
    return "";
  }
  std::string itemName = AEHelper::GetItemName(itemH) + ".pag";

  QDir dir(AEHelper::GetProjectPath());
  QString defaultPath = dir.filePath(itemName.data());

  QWidget* parentWidget = nullptr;
  auto topLevelWidgets = QApplication::topLevelWidgets();
  if (!topLevelWidgets.isEmpty()) {
    parentWidget = topLevelWidgets.value(0);
  }
  QFileDialog dialog(parentWidget, QObject::tr("Select Storage Path"), defaultPath);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix("pag");
  dialog.setOption(QFileDialog::DontUseNativeDialog, false);

  int dialogResult = dialog.exec();

  
  if (dialogResult == QDialog::Accepted) {
    QStringList selectedFiles = dialog.selectedFiles();
    if (!selectedFiles.isEmpty()) {
      QString selected = selectedFiles.first();
      return selected.toStdString();
    }
  }

  return "";
}

void ExportWindow::init() {
  if (QThread::currentThread() != app->thread()) {
    qCritical() << "Must call init() in main thread";
    return;
  }

  itemH = AEHelper::GetActiveCompositionItem();
  if (itemH == nullptr) {
    return;
  }

  if (outputPath.empty()) {
    outputPath = getOutputPath();
    if (outputPath.empty()) {
      return;
    }
  }

  PAGExportConfigParam configParam = {};
  configParam.exportAudio = true;
  configParam.activeItemH = itemH;
  configParam.outputPath = outputPath;
  configParam.showAlertInfo = showAlertInfo;
  pagExport = std::make_unique<PAGExport>(configParam);
  
  initAttempted = true;

  QQmlContext* context = engine->rootContext();
  QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
  context->setContextProperty("exportWindow", this);
  QQmlEngine::setObjectOwnership(&pagExport->session->progressModel, QQmlEngine::CppOwnership);
  context->setContextProperty("progressModel", &pagExport->session->progressModel);

  engine->load(QUrl(QStringLiteral("qrc:/qml/ExportCompositionProgress.qml")));
  window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
  window->setPersistentGraphics(true);
  window->setPersistentSceneGraph(true);
  QQuickWindow::setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);
}

void ExportWindow::wait() {
  while (!finished) {
    QApplication::processEvents();
    iterations++;
  }
}

}  // namespace exporter
