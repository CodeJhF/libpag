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

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <memory>
#include "ui/config/ExportCompositionModel.h"

namespace exporter {

class ExportConfigWindow : public QObject {
  Q_OBJECT
 public:
  explicit ExportConfigWindow(QObject* parent = nullptr);

  static void setupQt();

  static QApplication* app;

  void show();

 private:
  QQuickWindow* window = nullptr;
  std::unique_ptr<QQmlApplicationEngine> engine = nullptr;
  std::unique_ptr<ExportCompositionModel> compositionModel = nullptr;
};

}  // namespace exporter
