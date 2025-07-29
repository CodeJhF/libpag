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

ExportWindow::ExportWindow(QApplication* app, QObject* parent) : QObject(parent), app(app) {
  engine = std::make_unique<QQmlApplicationEngine>(app);
  init();
}

void ExportWindow::show() {
  if (itemH == nullptr) {
    return;
  }
  if (outputPath.empty()) {
    outputPath = getOutputPath();
  }
  if (outputPath.empty()) {
    return;
  }

  auto* pagExport = new PAGExport(itemH, outputPath, true);

  QQmlContext* context = engine->rootContext();
  context->setContextProperty("exportWindow", this);
  context->setContextProperty("progressModel", &pagExport->session->progressModel);

  engine->load(QUrl(QStringLiteral("qrc:/qml/ExportCompositionProgress.qml")));
  window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
  window->setPersistentGraphics(true);
  window->setPersistentSceneGraph(true);
  QQuickWindow::setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);
  window->show();

  bool result = PAGExport::ExportFile(pagExport);
  if (result) {
    pagExport->session->progressModel.setExportStatus(ProgressModel::ExportStatus::Success);
  } else {
    pagExport->session->progressModel.setExportStatus(ProgressModel::ExportStatus::Error);
  }
  context->setContextProperty("progressModel", nullptr);
  delete pagExport;
  FileHelper::OpenPAGFile(outputPath);
}

void ExportWindow::setOutputPath(const std::string& outputPath) {
  this->outputPath = outputPath;
}

bool ExportWindow::isWaitToDestory() const {
  return waitToDestory;
}

void ExportWindow::onWindowClosing() {
  if (window != nullptr) {
    window->hide();
  }
  waitToDestory = true;
}

std::string ExportWindow::getOutputPath() {
  if (itemH == nullptr) {
    return "";
  }
  std::string itemName = AEHelper::GetItemName(itemH) + ".pag";

  QDir dir(AEHelper::GetProjectPath());
  QString defaultPath = dir.filePath(itemName.data());
  QString selectPath = QFileDialog::getSaveFileName(
      QApplication::topLevelWidgets().value(0), QObject::tr("Select Storage Path"), defaultPath);
  return selectPath.toStdString();
}

void ExportWindow::init() {
  if (QThread::currentThread() != app->thread()) {
    qCritical() << "Must call init() in main thread";
    return;
  }

  itemH = AEHelper::GetActiveCompositionItem();
}

}  // namespace exporter
