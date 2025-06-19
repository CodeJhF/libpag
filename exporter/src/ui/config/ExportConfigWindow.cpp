/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2025 THL A29 Limited, a Tencent company. All rights reserved.
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

#include "ExportConfigWindow.h"
#include <QApplication>
#include <QQmlContext>
#include <QThread>
#include "utils/AEResource.h"

namespace exporter {

ExportConfigWindow::ExportConfigWindow(QObject* parent) : QObject(parent) {
  int argc = 0;
  app = std::make_unique<QApplication>(argc, nullptr);
  app->setObjectName("PAG-Exporter");
  engine = std::make_unique<QQmlApplicationEngine>(app.get());
  init();
}

void ExportConfigWindow::init() {
  if (QThread::currentThread() != qApp->thread()) {
    qCritical() << "Must call init() in main thread";
    return;
  }

  compositionModel = std::make_unique<ExportCompositionModel>();

  QQmlContext* context = engine->rootContext();
  context->setContextProperty("compositionModel", compositionModel.get());

  engine->load(QUrl(QStringLiteral("qrc:/qml/ExportConfigWindow.qml")));

  window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
  window->setPersistentGraphics(true);
  window->setPersistentSceneGraph(true);
  window->setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);

  auto root = AEResource::BuildResourceTree();
  compositionModel->setAEResource(root);
}

void ExportConfigWindow::show() {
  if (window == nullptr) {
    return;
  }

  window->show();
  app->exec();
}

}  // namespace exporter
