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

#include <QAbstractListModel>
#include "utils/AEResource.h"

namespace exporter {

class ExportCompositionData {
 public:
  bool isFolder = false;
  bool isUnfold = true;
  int level = 0;
  QString savePath = "";
  std::shared_ptr<AEResource> resource = nullptr;
};

class ExportCompositionModel : public QAbstractListModel {
  Q_OBJECT
 public:
  enum class ExportCompositionModelRoles {
    NameRole = Qt::UserRole + 1,
    SavePathRole,
    IsSelectedRole,
    IsFolderRole,
    IsUnfoldRole,
    LevelRole
  };

  explicit ExportCompositionModel(QObject* parent = nullptr);

  Q_PROPERTY(bool allSelected READ getAllSelected NOTIFY allSelectedChanged)

  void setAEResource(const std::shared_ptr<AEResource>& root);
  void updateData(const std::shared_ptr<AEResource>& node);
  Q_INVOKABLE bool getAllSelected() const;
  Q_INVOKABLE void setIsSelected(int index, bool isSelected);
  Q_INVOKABLE void setIsUnfold(int index, bool isUnfold);
  Q_INVOKABLE void setSavePath(int index, const QString& savePath);
  Q_INVOKABLE void setAllSelected(bool allSelected);
  Q_INVOKABLE void setSerachText(const QString& searchText);

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role) const override;

  Q_SIGNAL void allSelectedChanged(bool allSelected);

 protected:
  void updateCompositionLevel();
  void updateAllSelectedNum();
  QHash<int, QByteArray> roleNames() const override;

 private:
  int selectedNum = 0;
  int allSelectedNum = 0;
  std::shared_ptr<AEResource> root = nullptr;
  std::vector<std::shared_ptr<ExportCompositionData>> compositions = {};
};

}  // namespace exporter
