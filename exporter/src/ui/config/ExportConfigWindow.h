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

#pragma once

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include "ui/config/ExportCompositionModel.h"
#include "ui/config/settings/ExportCompositionInfoModel.h"
#include "ui/config/settings/ExportFrameImageProvider.h"
#include "ui/config/settings/ExportImageLayerModel.h"
#include "ui/config/settings/ExportTextLayerModel.h"
#include "ui/config/settings/ExportTimeStretchModel.h"
#include "utils/AEResource.h"

namespace exporter {

class ExportConfigWindow : public QObject {
  Q_OBJECT
 public:
  explicit ExportConfigWindow(QApplication* app, QObject* parent = nullptr);

  void show();
  void viewLayers(const std::shared_ptr<AEResource>& resource);
  bool isWaitToDestory() const;
  Q_SLOT void onWindowClosing();

  Q_INVOKABLE exporter::ExportTextLayerModel* getTextLayerModel(int row);
  Q_INVOKABLE exporter::ExportImageLayerModel* getImageLayerModel(int row);
  Q_INVOKABLE exporter::ExportTimeStretchModel* getTimeStretchModel(int row);
  Q_INVOKABLE exporter::ExportCompositionInfoModel* getCompositionInfoModel(int row);
  exporter::ExportFrameImageProvider* getImageProvider(A_long ID);
  Q_INVOKABLE void updateCompositionSetting(int row);

  Q_SIGNAL void destoryLater();

 private:
  void init();

  bool waitToDestory = false;
  QApplication* app = nullptr;
  QQuickWindow* window = nullptr;
  std::unique_ptr<QQmlApplicationEngine> engine = nullptr;
  std::unique_ptr<ExportCompositionModel> compositionModel = nullptr;
  std::vector<std::shared_ptr<AEResource>> resources = {};
  std::map<A_long, std::unique_ptr<ExportTextLayerModel>> textLayerModelMap = {};
  std::map<A_long, std::unique_ptr<ExportImageLayerModel>> imageLayerModelMap = {};
  std::map<A_long, std::unique_ptr<ExportTimeStretchModel>> timeStretchModelMap = {};
  std::map<A_long, std::unique_ptr<ExportCompositionInfoModel>> compositionInfoModelMap = {};
  std::map<A_long, ExportFrameImageProvider*> frameImageProviderMap = {};
};

}  // namespace exporter
