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

#include "ExportCompositionModel.h"
#include <QStandardPaths>
#include <QUrl>
#include <unordered_set>
#include "ExportConfigWindow.h"

namespace exporter {

ExportCompositionModel::ExportCompositionModel(QObject* parent) : QAbstractListModel(parent) {
}

void ExportCompositionModel::setAEResources(
    const std::vector<std::shared_ptr<AEResource>>& resources) {
  this->resources = resources;
  compositions.clear();
  for (const auto& resource : resources) {
    auto composition = std::make_shared<ExportCompositionData>();
    composition->resource = resource;
    composition->isFolder = resource->type == AEResourceType::Folder;
    composition->savePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    compositions.push_back(composition);
  }
  updateCompositionLevel();
  updateAllSelectedNum();
  beginResetModel();
  endResetModel();
}

bool ExportCompositionModel::getAllSelected() const {
  return selectedNum == allSelectedNum;
}

bool ExportCompositionModel::getCanExport() const {
  return selectedNum > 0;
}

bool ExportCompositionModel::getExportAudio() const {
  return exportAudio;
}

void ExportCompositionModel::setIsSelected(int index, bool isSelected) {
  if (index < 0 || static_cast<size_t>(index) >= compositions.size()) {
    return;
  }
  if (compositions[index]->isFolder || compositions[index]->resource->isExport == isSelected) {
    return;
  }
  selectedNum += isSelected ? 1 : -1;
  compositions[index]->resource->isExport = isSelected;
  QModelIndex modelIndex = this->index(index);
  Q_EMIT dataChanged(modelIndex, modelIndex,
                     {static_cast<int>(ExportCompositionModelRoles::IsSelectedRole)});
  Q_EMIT allSelectedChanged(getAllSelected());
  Q_EMIT canExportChanged(getCanExport());
}

void ExportCompositionModel::setIsUnfold(int index, bool isUnfold) {
  if (index < 0 || static_cast<size_t>(index) >= compositions.size()) {
    return;
  }
  const std::shared_ptr<ExportCompositionData>& composition = compositions[index];
  composition->isUnfold = isUnfold;
  std::unordered_set<A_long> idSet = {};
  idSet.emplace(composition->resource->ID);
  if (isUnfold) {
    std::vector<std::shared_ptr<ExportCompositionData>> tmpCompositions(
        compositions.begin() + index + 1, compositions.end());
    compositions.resize(index + 1);
    for (const auto& resource : resources) {
      if (idSet.find(resource->file.parent->ID) == idSet.end()) {
        continue;
      }
      auto newComposition = std::make_shared<ExportCompositionData>();
      newComposition->resource = resource;
      newComposition->isFolder = resource->type == AEResourceType::Folder;
      newComposition->savePath =
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
      compositions.push_back(newComposition);
      idSet.emplace(resource->ID);
    }
    compositions.insert(compositions.end(), tmpCompositions.begin(), tmpCompositions.end());
  } else {
    auto iter = compositions.begin();
    while (iter != compositions.end()) {
      if (idSet.find((*iter)->resource->file.parent->ID) == idSet.end()) {
        ++iter;
        continue;
      }
      idSet.emplace((*iter)->resource->ID);
      iter = compositions.erase(iter);
    }
  }
  updateCompositionLevel();
  updateAllSelectedNum();
  beginResetModel();
  endResetModel();
}

void ExportCompositionModel::setSavePath(int index, const QString& savePath) {
  if (index < 0 || static_cast<size_t>(index) >= compositions.size()) {
    return;
  }
  QString path = savePath;
  if (path.startsWith("file://")) {
    path = QUrl(path).toLocalFile();
  }
  compositions[index]->savePath = path;
  QModelIndex modelIndex = this->index(index);
  Q_EMIT dataChanged(modelIndex, modelIndex,
                     {static_cast<int>(ExportCompositionModelRoles::SavePathRole)});
}

void ExportCompositionModel::setAllSelected(bool allSelected) {
  for (size_t index = 0; index < compositions.size(); index++) {
    setIsSelected(static_cast<int>(index), allSelected);
  }
}

void ExportCompositionModel::setSerachText(const QString& searchText) {
  compositions.clear();
  for (const auto& resource : resources) {
    if (resource->name.find(searchText.toStdString()) == std::string::npos) {
      continue;
    }
    auto composition = std::make_shared<ExportCompositionData>();
    composition->resource = resource;
    composition->isFolder = resource->type == AEResourceType::Folder;
    composition->savePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    compositions.push_back(composition);
  }
  updateCompositionLevel();
  updateAllSelectedNum();
  beginResetModel();
  endResetModel();
}

void ExportCompositionModel::setExportAudio(bool exportAudio) {
  this->exportAudio = exportAudio;
  Q_EMIT exportAudioChanged(exportAudio);
}

void ExportCompositionModel::exportSelectedCompositions() {
  // todo: add export code here
}

void ExportCompositionModel::previewComposition(int row) {
  Q_UNUSED(row);
  // todo: add preview code here
}

int ExportCompositionModel::rowCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return static_cast<int>(compositions.size());
}

int ExportCompositionModel::columnCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return 1;
}

QVariant ExportCompositionModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return {};
  }

  const std::shared_ptr<ExportCompositionData>& data = compositions[index.row()];
  switch (role) {
    case static_cast<int>(ExportCompositionModelRoles::NameRole): {
      return data->resource->name.c_str();
    }
    case static_cast<int>(ExportCompositionModelRoles::SavePathRole): {
      return data->savePath;
    }
    case static_cast<int>(ExportCompositionModelRoles::IsSelectedRole): {
      return data->resource->isExport;
    }
    case static_cast<int>(ExportCompositionModelRoles::IsFolderRole): {
      return data->isFolder;
    }
    case static_cast<int>(ExportCompositionModelRoles::IsUnfoldRole): {
      return data->isUnfold;
    }
    case static_cast<int>(ExportCompositionModelRoles::LevelRole): {
      return data->level;
    }
    default: {
      return {};
    }
  }
}

void ExportCompositionModel::updateCompositionLevel() {
  for (const auto& composition : compositions) {
    int level = 0;
    auto* parent = composition->resource->file.parent;
    while (parent != nullptr && parent->ID != 0) {
      ++level;
      parent = parent->file.parent;
    }
    composition->level = level;
  }
}

void ExportCompositionModel::updateAllSelectedNum() {
  selectedNum = 0;
  allSelectedNum = 0;
  for (const auto& composition : compositions) {
    if (!composition->isFolder) {
      if (composition->resource->isExport) {
        selectedNum++;
      }
      allSelectedNum++;
    }
  }
  Q_EMIT allSelectedChanged(getAllSelected());
}

QHash<int, QByteArray> ExportCompositionModel::roleNames() const {
  static QHash<int, QByteArray> roles = {
      {static_cast<int>(ExportCompositionModelRoles::NameRole), "name"},
      {static_cast<int>(ExportCompositionModelRoles::SavePathRole), "savePath"},
      {static_cast<int>(ExportCompositionModelRoles::IsSelectedRole), "isSelected"},
      {static_cast<int>(ExportCompositionModelRoles::IsFolderRole), "isFolder"},
      {static_cast<int>(ExportCompositionModelRoles::IsUnfoldRole), "isUnfold"},
      {static_cast<int>(ExportCompositionModelRoles::LevelRole), "level"},
  };
  return roles;
}

}  // namespace exporter
