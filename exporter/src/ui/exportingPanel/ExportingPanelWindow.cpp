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

#include "ExportingPanelWindow.h"
#include <QApplication>
#include <QQmlContext>
#include <QThread>
#include <QTimer>
#include <map>
#include "CompositionsModel.h"
#include "export/ExportComposition.h"
#include "platform/PlatformHelper.h"
#include "src/export/ExportLayer.h"
#include "utils/AEHelper.h"
#include "utils/AEResource.h"
#include "utils/StringHelper.h"
#include "utils/FileHelper.h"

namespace exporter {

ExportingPanelWindow::ExportingPanelWindow(QApplication* app, QObject* parent)
    : BaseWindow(app, parent) {
  init();
}

void ExportingPanelWindow::init() {
  if (QThread::currentThread() != app->thread()) {
    qCritical() << "Must call init() in main thread";
    return;
  }

  compositionsModel = std::make_unique<CompositionsModel>(engine.get());

  QQmlContext* context = engine->rootContext();
  QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
  context->setContextProperty("exportingPanelWindow", this);
  QQmlEngine::setObjectOwnership(compositionsModel.get(), QQmlEngine::CppOwnership);
  context->setContextProperty("compositionsModel", compositionsModel.get());

  engine->load(QUrl(QStringLiteral("qrc:/qml/ExportingPanel.qml")));

  window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
  window->setPersistentGraphics(true);
  window->setPersistentSceneGraph(true);
  QQuickWindow::setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);

  resources = AEResource::getAEResourceList();
  compositionsModel->setQmlEngine(engine.get());
  compositionsModel->setAEResources(resources);
}

ExportCompositionInfoModel* ExportingPanelWindow::getCompositionInfoModel(int row) {
  if (window != nullptr) {
    const auto& resource = resources[row];
    A_long id = resource->ID;
    auto iter = compositionInfoModelMap.find(id);
    if (iter != compositionInfoModelMap.end()) {
      QQmlEngine::setObjectOwnership(iter->second.get(), QQmlEngine::CppOwnership);
      return iter->second.get();
    }
  }

  return nullptr;
}

QString ExportingPanelWindow::getBackgroundColor(int row) const {
  const auto& resource = resources[row];
  A_long id = resource->ID;
  if (sessionMap.find(id) == sessionMap.end()) {
    return "transparent";
  }
  const auto& session = sessionMap.at(id);
  if (session->compositions.empty()) {
    return "transparent";
  }
  auto& mainComposition = session->compositions[session->compositions.size() - 1];
  return StringHelper::ColorToQString(mainComposition->backgroundColor);
}

ExportFrameImageProvider* ExportingPanelWindow::getImageProvider(A_long ID) {
  if (frameImageProviderMap.find(ID) == frameImageProviderMap.end()) {
    return nullptr;
  }
  return frameImageProviderMap[ID];
}

ExportTextLayerModel* ExportingPanelWindow::getTextLayerModel(int row) {
  const auto& resource = resources[row];
  A_long id = resource->ID;
  auto iter = textLayerModelMap.find(id);
  if (iter != textLayerModelMap.end()) {
    QQmlEngine::setObjectOwnership(iter->second.get(), QQmlEngine::CppOwnership);
    return iter->second.get();
  }

  return nullptr;
}

ExportImageLayerModel* ExportingPanelWindow::getImageLayerModel(int row) {
  const auto& resource = resources[row];
  A_long id = resource->ID;
  auto iter = imageLayerModelMap.find(id);
  if (iter != imageLayerModelMap.end()) {
    QQmlEngine::setObjectOwnership(iter->second.get(), QQmlEngine::CppOwnership);
    return iter->second.get();
  }

  return nullptr;
}

ExportTimeStretchModel* ExportingPanelWindow::getTimeStretchModel(int row) {
  const auto& resource = resources[row];
  A_long id = resource->ID;
  auto iter = timeStretchModelMap.find(id);
  if (iter != timeStretchModelMap.end()) {
    QQmlEngine::setObjectOwnership(iter->second.get(), QQmlEngine::CppOwnership);
    return iter->second.get();
  }

  return nullptr;
}

void ExportingPanelWindow::updateCompositionSetting(int row) {
  if (window == nullptr) {
    return;
  }
  const auto& resource = resources[row];
  if (sessionMap.find(resource->ID) == sessionMap.end()) {
    std::string tempPagPath = FileHelper::JoinPaths(GetTempFolderPath(), "tmp.pag");
    auto session = std::make_shared<PAGExportSession>(resource->itemH, tempPagPath);
    session->setCurrent();
    session->exportAudio = false;
    session->enableRunScript = false;
    ExportComposition(session, resource->itemH);
    sessionMap[resource->ID] = session;
    session->unsetCurrent();
  }
  viewLayers(resource);
  auto frameImageProvider = new ExportFrameImageProvider();
  frameImageProvider->setAEResource(resource);
  auto compositionInfoModel = std::make_unique<ExportCompositionInfoModel>(frameImageProvider);
  compositionInfoModel->setAEResource(resource);
  auto textLayerModel = std::make_unique<ExportTextLayerModel>();
  textLayerModel->setAEResource(resource);
  auto imageLayerModel = std::make_unique<ExportImageLayerModel>();
  imageLayerModel->setAEResource(resource);
  auto timeStretchModel = std::make_unique<ExportTimeStretchModel>();
  timeStretchModel->setAEResource(resource);
  textLayerModelMap[resource->ID] = std::move(textLayerModel);
  imageLayerModelMap[resource->ID] = std::move(imageLayerModel);
  timeStretchModelMap[resource->ID] = std::move(timeStretchModel);
  compositionInfoModelMap[resource->ID] = std::move(compositionInfoModel);
  if (frameImageProviderMap.find(resource->ID) != frameImageProviderMap.end()) {
    engine->removeImageProvider(frameImageProviderMap[resource->ID]->getName());
  }
  frameImageProviderMap[resource->ID] = frameImageProvider;
  engine->addImageProvider(frameImageProvider->getName(), frameImageProvider);
  connect(compositionInfoModelMap[resource->ID].get(),
          &ExportCompositionInfoModel::compositionExportAsBmpChanged,
          imageLayerModelMap[resource->ID].get(),
          &ExportImageLayerModel::onCompositionExportAsBmpChanged);
  connect(compositionInfoModelMap[resource->ID].get(),
          &ExportCompositionInfoModel::compositionExportAsBmpChanged,
          textLayerModelMap[resource->ID].get(),
          &ExportTextLayerModel::onCompositionExportAsBmpChanged);
}

Q_INVOKABLE bool ExportingPanelWindow::isAEWindowActive() {
  return IsAEWindowActive();
}

void ExportingPanelWindow::viewLayers(const std::shared_ptr<AEResource>& resource) {
  if (!resource->composition.children.empty() || !resource->composition.textLayers.empty() ||
      !resource->composition.imageLayers.empty()) {
    return;
  }
  if (resource->type != AEResourceType::Composition) {
    return;
  }
  AEGP_CompH compH = AEHelper::GetItemCompH(resource->itemH);
  A_long layerCount = 0;
  AEHelper::GetSuites()->LayerSuite6()->AEGP_GetCompNumLayers(compH, &layerCount);
  for (A_long index = 0; index < layerCount; index++) {
    AEGP_LayerH layerH = nullptr;
    AEHelper::GetSuites()->LayerSuite6()->AEGP_GetCompLayerByIndex(compH, index, &layerH);
    if (layerH == nullptr) {
      continue;
    }

    AEResource::Layer layer;
    layer.layerID = AEHelper::GetLayerID(layerH);
    layer.name = AEHelper::GetLayerName(layerH);
    layer.layerH = layerH;
    AEGP_ItemH layerItemH = nullptr;
    ExportLayerType layerType = GetLayerType(layerH);
    if (layerType == ExportLayerType::Text) {
      resource->composition.textLayers.push_back(layer);
    } else if (layerType == ExportLayerType::Image || layerType == ExportLayerType::Video) {
      layerItemH = AEHelper::GetLayerItemH(layerH);
      auto iter = std::find_if(
          resource->composition.imageLayers.begin(), resource->composition.imageLayers.end(),
          [&](const auto& item) { return AEHelper::GetLayerItemH(item.layerH) == layerItemH; });
      if (iter == resource->composition.imageLayers.end()) {
        resource->composition.imageLayers.push_back(layer);
      }
    } else if (layerType == ExportLayerType::PreCompose) {
      layerItemH = AEHelper::GetLayerItemH(layerH);
      auto iter = std::find_if(resources.begin(), resources.end(),
                               [&](const auto& item) { return item->itemH == layerItemH; });
      if (iter != resources.end()) {
        resource->composition.children.push_back(*iter);
      }
    }
  }

  for (const auto& child : resource->composition.children) {
    viewLayers(child);
  }
}

}  // namespace exporter
