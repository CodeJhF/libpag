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

#pragma once

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include "CompositionsModel.h"
#include "settings/ExportCompositionInfoModel.h"
#include "settings/ExportFrameImageProvider.h"
#include "settings/ExportImageLayerModel.h"
#include "settings/ExportTextLayerModel.h"
#include "settings/ExportTimeStretchModel.h"
#include "utils/AEResource.h"
#include "utils/PAGExportSession.h"

namespace exporter {

class ExportingPanelWindow : public QObject {
  Q_OBJECT
 public:
  explicit ExportingPanelWindow(QApplication* app, QObject* parent = nullptr);

  void show();
  void viewLayers(const std::shared_ptr<AEResource>& resource,
                  const std::unordered_map<pag::ID, AEGP_LayerH>& layerHMap);
  bool isWaitToDestory() const;
  Q_SLOT void onWindowClosing();

  Q_INVOKABLE exporter::ExportTextLayerModel* getTextLayerModel(int row);
  Q_INVOKABLE exporter::ExportImageLayerModel* getImageLayerModel(int row);
  Q_INVOKABLE exporter::ExportTimeStretchModel* getTimeStretchModel(int row);
  Q_INVOKABLE exporter::ExportCompositionInfoModel* getCompositionInfoModel(int row);
  exporter::ExportFrameImageProvider* getImageProvider(A_long ID);
  Q_INVOKABLE void updateCompositionSetting(int row);
  Q_INVOKABLE bool isAEWindowActive();

 private:
  void init();

  bool waitToDestory = false;
  QApplication* app = nullptr;
  QQuickWindow* window = nullptr;
  std::unique_ptr<QQmlApplicationEngine> engine = nullptr;
  std::unique_ptr<CompositionsModel> compositionsModel = nullptr;
  std::vector<std::shared_ptr<AEResource>> resources = {};
  std::map<A_long, std::shared_ptr<PAGExportSession>> sessionMap = {};
  std::map<A_long, std::unique_ptr<ExportTextLayerModel>> textLayerModelMap = {};
  std::map<A_long, std::unique_ptr<ExportImageLayerModel>> imageLayerModelMap = {};
  std::map<A_long, std::unique_ptr<ExportTimeStretchModel>> timeStretchModelMap = {};
  std::map<A_long, std::unique_ptr<ExportCompositionInfoModel>> compositionInfoModelMap = {};
  std::map<A_long, ExportFrameImageProvider*> frameImageProviderMap = {};
};

}  // namespace exporter
