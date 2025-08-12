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

#include "WindowManager.h"
#include <QApplication>
#include <QFile>
#include <QQuickStyle>
#include <QTranslator>
#include <QtGui/QFont>
#include <QtQuick/QQuickWindow>
#include "config/ConfigFile.h"
#include "config/ConfigModel.h"
#include "platform/PlatformHelper.h"
#include "utils/AEHelper.h"
#include "utils/FileHelper.h"
#include "utils/StringHelper.h"

namespace exporter {

WindowManager& WindowManager::GetInstance() {
  static WindowManager instance;
  return instance;
}

WindowManager::WindowManager() {
  AEHelper::RunScriptPreWarm();
  initializeQtEnvironment();
  translator = std::make_unique<QTranslator>();
}

void WindowManager::showExportPanelWindow() {
  init();
  if (exportingPanelWindow == nullptr) {
    exportingPanelWindow = std::make_unique<ExportingPanelWindow>(app.get());
  }
  exportingPanelWindow->show();
  app->exec();
}

void WindowManager::showPAGConfigWindow() {
  init();
  if (configWindow == nullptr) {
    configWindow = std::make_unique<ConfigModel>(app.get());
  }
  configWindow->show();
  app->exec();
}

void WindowManager::showExportPreviewWindow() {
  init();
  if (previewWindow == nullptr) {
    std::string outputPath = FileHelper::JoinPaths(GetTempFolderPath(), ".previewTmp.pag");
    previewWindow = std::make_unique<ExportWindow>(app.get(), outputPath);
  }
  previewWindow->show();
  app->exec();
}

void WindowManager::showExportWindow() {
  init();
  if (exportWindow == nullptr) {
    exportWindow = std::make_unique<ExportWindow>(app.get());
  }
  exportWindow->show();
  app->exec();
}

void WindowManager::initializeQtEnvironment() {
  QApplication::setAttribute(Qt::AA_PluginApplication, true);
  QSurfaceFormat defaultFormat = QSurfaceFormat();
  defaultFormat.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
  defaultFormat.setVersion(3, 2);
  defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(defaultFormat);

#ifdef WIN32
  QFont defaultFonts("Microsoft Yahei");
  defaultFonts.setStyleHint(QFont::SansSerif);
  QApplication::setFont(defaultFonts);
#else
  QFont defaultFonts("Helvetica Neue,PingFang SC");
  QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
  defaultFonts.setStyleHint(QFont::SansSerif);
  QApplication::setFont(defaultFonts);
#endif
  app = std::make_unique<QApplication>(argc, argv);
  app->setObjectName("PAG-Exporter");
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickStyle::setStyle("Universal");
}

bool WindowManager::showWarnings(std::vector<std::string>& /*infos*/) {

  return true;
}

bool WindowManager::showErrors(std::vector<std::string>& /*infos*/) {

  return true;
}

void WindowManager::init() {
  ConfigParam config;
  ReadConfigFile(&config);
  bool result = translator->load(":/translation/Chinese.qm");
  if (result) {
    if (config.language == Language::Chinese) {
      app->installTranslator(translator.get());
    } else {
      app->removeTranslator(translator.get());
    }
  }

  if (configWindow != nullptr && configWindow->isWaitToDestory()) {
    configWindow.reset();
  }

  if (previewWindow != nullptr && previewWindow->isWaitToDestory()) {
    previewWindow.reset();
  }

  if (exportWindow != nullptr && exportWindow->isWaitToDestory()) {
    exportWindow.reset();
  }

  if (exportingPanelWindow != nullptr && exportingPanelWindow->isWaitToDestory()) {
    exportingPanelWindow.reset();
  }
}

}  // namespace exporter
