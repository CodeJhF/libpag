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
#include <QFont>
#include <QQmlContext>
#include <QThread>
#include "utils/AEResource.h"

namespace exporter {

QApplication* ExportConfigWindow::app = nullptr;

ExportConfigWindow::ExportConfigWindow(QObject* parent) : QObject(parent) {
  setupQt();
  engine = std::make_unique<QQmlApplicationEngine>(app);
}

void ExportConfigWindow::setupQt() {
  if (app != nullptr) {
    return;
  }
  QApplication::setAttribute(Qt::AA_PluginApplication, true);

  QSurfaceFormat defaultFormat = QSurfaceFormat();
  defaultFormat.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
  defaultFormat.setVersion(3, 2);
  defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(defaultFormat);
  std::vector<std::string> fallbackList;
#ifdef WIN32
  QFont defaultFonts("Microsoft Yahei");
  defaultFonts.setStyleHint(QFont::SansSerif);
  QApplication::setFont(defaultFonts);
  fallbackList = {"Microsoft YaHei"};
#else
  QFont defaultFonts("Helvetica Neue,PingFang SC");
  QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
  defaultFonts.setStyleHint(QFont::SansSerif);
  QApplication::setFont(defaultFonts);
  fallbackList = {"PingFang SC", "Apple Color Emoji"};
#endif

  int argc = 0;
  auto* app = new QApplication(argc, nullptr);
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QString appName("PAG-Exporter");
  app->setObjectName(appName);
  ExportConfigWindow::app = app;
  printf("SetupQT objectName: %s\n", appName.toStdString().c_str());
}

void ExportConfigWindow::show() {
  if (QThread::currentThread() != qApp->thread()) {
    qCritical() << "Must call show() in main thread";
    return;
  }

  compositionModel = std::make_unique<ExportCompositionModel>();

  QQmlContext* context = engine->rootContext();
  context->setContextProperty("compositionModel", compositionModel.get());

  engine->load(QUrl(QStringLiteral("qrc:/qml/ExportConfigWindow.qml")));

  window = static_cast<QQuickWindow*>(engine->rootObjects().at(0));
  window->setPersistentGraphics(true);
  window->setPersistentSceneGraph(true);
  window->setPersistentSceneGraph(true);
  window->setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);
  window->show();

  auto root = AEResource::BuildResourceTree();
  compositionModel->setAEResource(root);

  app->exec();
}

}  // namespace exporter
