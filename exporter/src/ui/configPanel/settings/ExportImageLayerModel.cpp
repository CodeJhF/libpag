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

#include "ExportImageLayerModel.h"
#include "utils/AEHelper.h"

namespace exporter {

static pag::PAGScaleMode QStringToScaleMode(const QString& str) {
  QString lowerStr = str.toLower();
  if (lowerStr == "none") {
    return pag::PAGScaleMode::None;
  } else if (lowerStr == "stretch") {
    return pag::PAGScaleMode::Stretch;
  } else if (lowerStr == "letterBox") {
    return pag::PAGScaleMode::LetterBox;
  } else if (lowerStr == "zoom") {
    return pag::PAGScaleMode::Zoom;
  }
  return pag::PAGScaleMode::LetterBox;
}

static QString ScaleModeToQString(pag::PAGScaleMode mode) {
  switch (mode) {
    case pag::PAGScaleMode::None:
      return "None";
    case pag::PAGScaleMode::Stretch:
      return "Stretch";
    case pag::PAGScaleMode::LetterBox:
      return "LetterBox";
    case pag::PAGScaleMode::Zoom:
      return "Zoom";
    default:
      return "LetterBox";
  }
}

ExportImageLayerModel::ExportImageLayerModel(QObject* parent) : QAbstractListModel(parent) {
}

void ExportImageLayerModel::setAEResource(const std::shared_ptr<AEResource>& resource) {
  this->resource = resource;
  refreshData(resource);
  editableItemNum = 0;
  for (const auto& item : items) {
    if (this->resource->composition.imagesLayerFlagMap[item.layerID].isEditable) {
      editableItemNum++;
    }
  }
}

void ExportImageLayerModel::refreshData(const std::shared_ptr<AEResource>& resource) {
  if (resource->isExportAsBmp) {
    return;
  }
  for (const auto& layer : resource->composition.imageLayers) {
    auto iter = std::find_if(items.begin(), items.end(), [&](const AEResource::Layer& item) {
      return AEHelper::GetLayerItemH(layer.layerH) == AEHelper::GetLayerItemH(item.layerH);
    });
    if (iter != items.end()) {
      continue;
    }
    AEResource::Layer item = layer;
    items.push_back(item);
    if (this->resource->composition.imagesLayerFlagMap.find(layer.layerID) ==
        this->resource->composition.imagesLayerFlagMap.end()) {
      this->resource->composition.imagesLayerFlagMap[layer.layerID] = {
          true, pag::PAGScaleMode::LetterBox};
      // TODO: Read from marker
    }
  }

  for (const auto& child : resource->composition.children) {
    refreshData(child);
  }
}

void ExportImageLayerModel::setIsEditable(int row, bool isEditable) {
  if (row < 0 || static_cast<size_t>(row) >= items.size()) {
    return;
  }
  if (this->resource->composition.imagesLayerFlagMap[items[row].layerID].isEditable == isEditable) {
    return;
  }
  // TODO: Write to marker
  editableItemNum += isEditable ? 1 : -1;
  this->resource->composition.imagesLayerFlagMap[items[row].layerID].isEditable = isEditable;
  QModelIndex index = this->index(row, 0);
  Q_EMIT dataChanged(index, index, {static_cast<int>(ExportImageLayerModelRoles::IsEditableRole)});
  Q_EMIT allEditableChanged(getAllEditable());
}

void ExportImageLayerModel::setAllEditable(bool allEditable) {
  for (size_t index = 0; index < items.size(); index++) {
    setIsEditable(static_cast<int>(index), allEditable);
  }
}

void ExportImageLayerModel::setScaleMode(int row, QString scaleMode) {
  if (row < 0 || static_cast<size_t>(row) >= items.size()) {
    return;
  }
  pag::PAGScaleMode mode = QStringToScaleMode(scaleMode);
  if (this->resource->composition.imagesLayerFlagMap[items[row].layerID].scaleMode == mode) {
    return;
  }
  // TODO: Write to marker
  this->resource->composition.imagesLayerFlagMap[items[row].layerID].scaleMode = mode;
  QModelIndex index = this->index(row, 0);
  Q_EMIT dataChanged(index, index, {static_cast<int>(ExportImageLayerModelRoles::FillModeRole)});
}

bool ExportImageLayerModel::getAllEditable() const {
  return editableItemNum == items.size();
}

QStringList ExportImageLayerModel::getScaleModes() {
  static QStringList scaleModes = {"None", "Stretch", "LetterBox", "Zoom"};
  return scaleModes;
}

int ExportImageLayerModel::rowCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return static_cast<int>(items.size());
}

QVariant ExportImageLayerModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return {};
  }

  const auto& item = items[index.row()];
  switch (role) {
    case static_cast<int>(ExportImageLayerModelRoles::NumberRole): {
      return index.row();
    }
    case static_cast<int>(ExportImageLayerModelRoles::NameRole): {
      return item.name.data();
    }
    case static_cast<int>(ExportImageLayerModelRoles::IsEditableRole): {
      return this->resource->composition.imagesLayerFlagMap[item.layerID].isEditable;
    }
    case static_cast<int>(ExportImageLayerModelRoles::FillModeRole): {
      return ScaleModeToQString(
          this->resource->composition.imagesLayerFlagMap[item.layerID].scaleMode);
    }
    default:
      return {};
  }
}

void ExportImageLayerModel::onCompositionExportAsBmpChanged() {
  items.clear();
  editableItemNum = 0;
  refreshData(resource);
  for (const auto& item : items) {
    if (this->resource->composition.imagesLayerFlagMap[item.layerID].isEditable) {
      editableItemNum++;
    }
  }
  beginResetModel();
  endResetModel();
}

QHash<int, QByteArray> ExportImageLayerModel::roleNames() const {
  static QHash<int, QByteArray> roles = {
      {static_cast<int>(ExportImageLayerModelRoles::NumberRole), "number"},
      {static_cast<int>(ExportImageLayerModelRoles::NameRole), "name"},
      {static_cast<int>(ExportImageLayerModelRoles::IsEditableRole), "isEditable"},
      {static_cast<int>(ExportImageLayerModelRoles::FillModeRole), "fillMode"},
  };
  return roles;
}

}  // namespace exporter
