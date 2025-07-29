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

#include "ExportTimeStretchModel.h"
#include "utils/AEHelper.h"

namespace exporter {

std::map<pag::PAGTimeStretchMode, QString> timeStretchModeMap = {
    {pag::PAGTimeStretchMode::None, "None"},
    {pag::PAGTimeStretchMode::Scale, "Stretch"},
    {pag::PAGTimeStretchMode::Repeat, "Repeat"},
    {pag::PAGTimeStretchMode::RepeatInverted, "Repeat(Inverted)"}};

ExportTimeStretchModel::ExportTimeStretchModel(QObject* parent) : QObject(parent) {
}

QString ExportTimeStretchModel::getTimeStretchMode() const {
  return timeStretchModeMap[resource->stretchMode];
}

int ExportTimeStretchModel::getStretchStartTime() const {
  return static_cast<int>(resource->stretchStartTime);
}

int ExportTimeStretchModel::getStretchDuration() const {
  return static_cast<int>(resource->stretchDuration);
}

int ExportTimeStretchModel::getDuration() const {
  return static_cast<int>(duration);
}

int ExportTimeStretchModel::getFrameRate() const {
  return static_cast<int>(frameRate);
}

QStringList ExportTimeStretchModel::getTimeStretchModes() const {
  QStringList timeStretchModes;
  for (const auto& iter : timeStretchModeMap) {
    timeStretchModes.append(iter.second);
  }
  return timeStretchModes;
}

void ExportTimeStretchModel::setTimeStretchMode(const QString& timeStretchMode) {
  for (const auto& iter : timeStretchModeMap) {
    if (iter.second == timeStretchMode) {
      resource->stretchMode = iter.first;
      break;
    }
  }
  // TODO: Write to marker
  Q_EMIT timeStretchModeChanged(timeStretchMode);
}

void ExportTimeStretchModel::setStretchStartTime(int stretchStartTime) {
  // TODO: Write to marker
  resource->stretchStartTime = static_cast<pag::Frame>(stretchStartTime);
  Q_EMIT stretchStartTimeChanged(stretchStartTime);
}

void ExportTimeStretchModel::setStretchDuration(int stretchDuation) {
  // TODO: Write to marker
  resource->stretchDuration = static_cast<pag::Frame>(stretchDuation);
  Q_EMIT stretchDurationChanged(stretchDuation);
}

void ExportTimeStretchModel::setAEResource(const std::shared_ptr<AEResource>& resource) {
  this->resource = resource;
  duration = AEHelper::GetItemDuration(resource->itemH);
  frameRate = AEHelper::GetItemFrameRate(resource->itemH);
  // TODO: Read from marker
  Q_EMIT durationChanged(static_cast<int>(duration));
  Q_EMIT frameRateChanged(static_cast<int>(frameRate));
}

}  // namespace exporter
