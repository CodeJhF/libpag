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

#include "ExportFrameImageProvider.h"
#include "utils/AEHelper.h"

namespace exporter {

ExportFrameImageProvider::ExportFrameImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {
}

void ExportFrameImageProvider::setAEResource(const std::shared_ptr<AEResource>& resource) {
  this->resource = resource;
  updateFrameImage(0);
}

void ExportFrameImageProvider::updateFrameImage(pag::Frame frame) {
  QImage image = AEHelper::GetCompositionFrameImage(resource->itemH, frame);
  if (frameImages.size() >= 5) {
    frameImages.erase(frameImages.begin());
  }
  frameImages[frame] = image;
}

QString ExportFrameImageProvider::getName() {
  return QString("PAGExporterFrameImageProvider_") +
         QString::number(reinterpret_cast<quintptr>(this));
}

QImage ExportFrameImageProvider::requestImage(const QString& id, QSize* size,
                                              const QSize& requestedSize) {
  Q_UNUSED(size);
  Q_UNUSED(requestedSize);
  if (frameImages.find(id.toInt()) != frameImages.end()) {
    return frameImages[id.toLongLong()];
  }
  return {};
}

}  // namespace exporter
