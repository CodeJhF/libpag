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

namespace exporter {

ExportCompositionModel::ExportCompositionModel(QObject* parent) : QAbstractListModel(parent){};

void ExportCompositionModel::setAEResource(const std::shared_ptr<AEResource>& root) {
  this->root = root;
  compositions.clear();
  updateData(root);
  updateCompositionLevel();
  updateAllSelectedNum();
  beginResetModel();
  endResetModel();
}

void ExportCompositionModel::updateData(const std::shared_ptr<AEResource>& node) {
  if (node->id != 0) {
    auto composition = std::make_shared<ExportCompositionData>();
    composition->resource = node;
    composition->isFolder = node->type == AEResourceType::Folder;
    composition->savePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    compositions.push_back(composition);
  }

  for (const auto& child : node->children) {
    updateData(child);
  }
}

bool ExportCompositionModel::getAllSelected() const {
  return selectedNum == allSelectedNum;
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
}

void ExportCompositionModel::setIsUnfold(int index, bool isUnfold) {
  if (index < 0 || static_cast<size_t>(index) >= compositions.size()) {
    return;
  }
  const std::shared_ptr<ExportCompositionData>& composition = compositions[index];
  composition->isUnfold = isUnfold;
  if (isUnfold) {
    std::vector<std::shared_ptr<ExportCompositionData>> newCompositions(
        compositions.begin() + index + 1, compositions.end());
    compositions.resize(index + 1);
    for (const auto& child : composition->resource->children) {
      updateData(child);
    }
    compositions.insert(compositions.end(), newCompositions.begin(), newCompositions.end());
  } else {
    auto parent = composition->resource->parent;
    auto iter1 = std::find_if(
        parent->children.begin(), parent->children.end(),
        [&](const std::shared_ptr<AEResource>& node) { return composition->resource == node; });
    auto iter2 = iter1 + 1;
    if (iter2 == parent->children.end()) {
      compositions.resize(index + 1);
    } else {
      auto brotherIter = std::find_if(compositions.begin(), compositions.end(),
                                      [&](const std::shared_ptr<ExportCompositionData>& node) {
                                        return (*iter2) == node->resource;
                                      });
      auto iter = compositions.begin() + index;
      compositions.erase(iter + 1, brotherIter);
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
  for (size_t i = 0; i < compositions.size(); i++) {
    setIsSelected(static_cast<int>(i), allSelected);
  }
  Q_EMIT allSelectedChanged(allSelected);
}

void ExportCompositionModel::setSerachText(const QString& searchText) {
  compositions.clear();
  updateData(root);
  for (auto iter = compositions.begin(); iter != compositions.end();) {
    if ((*iter)->resource->name.find(searchText.toStdString()) == std::string::npos) {
      iter = compositions.erase(iter);
    } else {
      ++iter;
    }
  }
  updateCompositionLevel();
  updateAllSelectedNum();
  beginResetModel();
  endResetModel();
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
    auto* parent = composition->resource->parent;
    while (parent->id != 0) {
      ++level;
      parent = parent->parent;
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
  allSelectedChanged(getAllSelected());
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
