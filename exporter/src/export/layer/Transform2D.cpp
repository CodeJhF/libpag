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

#include "Transform2D.h"
#include "export/stream/StreamProperty.h"

namespace exporter {

pag::Transform2D* GetTransform2D(const AEGP_LayerH& layerH, float frameRate) {
  const A_long PluginID = AEHelper::GetPluginID();
  const auto Suites = AEHelper::GetSuites();
  auto transform = new pag::Transform2D();

  auto map = CreatePropertyMap(frameRate);

  AEGP_StreamRefH streamH = nullptr;
  Suites->StreamSuite4()->AEGP_GetNewLayerStream(PluginID, layerH, AEGP_LayerStream_POSITION,
                                                 &streamH);
  A_Boolean dimensionSeparated = 0;
  Suites->DynamicStreamSuite4()->AEGP_AreDimensionsSeparated(streamH, &dimensionSeparated);
  if (dimensionSeparated != 0) {
    AEGP_StreamRefH xPosition;
    Suites->DynamicStreamSuite4()->AEGP_GetSeparationFollower(streamH, 0, &xPosition);
    transform->xPosition = GetProperty(xPosition, AEStreamParser::FloatParser, map);
    Suites->StreamSuite4()->AEGP_DisposeStream(xPosition);
    AEGP_StreamRefH yPosition;
    Suites->DynamicStreamSuite4()->AEGP_GetSeparationFollower(streamH, 1, &yPosition);
    transform->yPosition = GetProperty(yPosition, AEStreamParser::FloatParser, map);
    Suites->StreamSuite4()->AEGP_DisposeStream(yPosition);
  } else {
    transform->position = GetProperty(streamH, AEStreamParser::PointParser, map);
  }
  Suites->StreamSuite4()->AEGP_DisposeStream(streamH);
  transform->anchorPoint =
      GetProperty(layerH, AEGP_LayerStream_ANCHORPOINT, AEStreamParser::PointParser, map);
  transform->scale =
      GetProperty(layerH, AEGP_LayerStream_SCALE, AEStreamParser::ScaleParser, map, 2);
  transform->rotation =
      GetProperty(layerH, AEGP_LayerStream_ROTATION, AEStreamParser::FloatParser, map);
  transform->opacity =
      GetProperty(layerH, AEGP_LayerStream_OPACITY, AEStreamParser::Opacity0_100Parser, map);
  return transform;
}

}  // namespace exporter
