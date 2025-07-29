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

#include "StreamProperty.h"
#include "utils/Helper.h"

namespace exporter {

static float GetCurveLength(const AEGP_StreamValue2& lastValue, const AEGP_StreamValue2& lastOut,
                            const AEGP_StreamValue2& currentIn,
                            const AEGP_StreamValue2& currentValue) {
  auto startX = static_cast<float>(lastValue.val.three_d.x);
  auto startY = static_cast<float>(lastValue.val.three_d.y);
  auto startZ = static_cast<float>(lastValue.val.three_d.z);
  auto controlX1 = static_cast<float>(startX + lastOut.val.three_d.x);
  auto controlY1 = static_cast<float>(startY + lastOut.val.three_d.y);
  auto controlZ1 = static_cast<float>(startY + lastOut.val.three_d.z);
  auto endX = static_cast<float>(currentValue.val.three_d.x);
  auto endY = static_cast<float>(currentValue.val.three_d.y);
  auto endZ = static_cast<float>(currentValue.val.three_d.z);
  auto controlX2 = static_cast<float>(endX + currentIn.val.three_d.x);
  auto controlY2 = static_cast<float>(endY + currentIn.val.three_d.y);
  auto controlZ2 = static_cast<float>(endY + currentIn.val.three_d.z);
  return Helper::GetCubicLength(startX, startY, startZ, controlX1, controlY1, controlZ1, controlX2,
                                controlY2, controlZ2, endX, endY, endZ);
}

std::vector<float> GetAverageSpeed(AEGP_StreamRefH streamH, int index, int dimensionality) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  std::vector<float> averageSpeedList;
  AEGP_StreamType streamType;
  Suites->StreamSuite4()->AEGP_GetStreamType(streamH, &streamType);
  if (streamType == AEGP_StreamType_NO_DATA) {
    averageSpeedList.push_back(1);
    return averageSpeedList;
  }

  AEGP_StreamValue2 lastValue = {};
  Suites->KeyframeSuite4()->AEGP_GetNewKeyframeValue(PluginID, streamH, index - 1, &lastValue);
  AEGP_StreamValue2 value = {};
  Suites->KeyframeSuite4()->AEGP_GetNewKeyframeValue(PluginID, streamH, index, &value);

  A_Time lastTime = {};
  Suites->KeyframeSuite4()->AEGP_GetKeyframeTime(streamH, index - 1, AEGP_LTimeMode_CompTime,
                                                 &lastTime);
  A_Time time = {};
  Suites->KeyframeSuite4()->AEGP_GetKeyframeTime(streamH, index, AEGP_LTimeMode_CompTime, &time);
  auto duration = static_cast<float>(time.value - lastTime.value) / time.scale;

  switch (streamType) {
    case AEGP_StreamType_ThreeD_SPATIAL:
    case AEGP_StreamType_TwoD_SPATIAL: {
      AEGP_StreamValue2 spatialTemp = {};
      AEGP_StreamValue2 spatialIn = {};
      AEGP_StreamValue2 spatialOut = {};
      Suites->KeyframeSuite4()->AEGP_GetNewKeyframeSpatialTangents(PluginID, streamH, index - 1,
                                                                   &spatialTemp, &spatialOut);
      Suites->KeyframeSuite4()->AEGP_GetNewKeyframeSpatialTangents(PluginID, streamH, index,
                                                                   &spatialIn, &spatialTemp);
      auto curveLength = GetCurveLength(lastValue, spatialOut, spatialIn, value);
      Suites->StreamSuite4()->AEGP_DisposeStreamValue(&spatialTemp);
      Suites->StreamSuite4()->AEGP_DisposeStreamValue(&spatialIn);
      Suites->StreamSuite4()->AEGP_DisposeStreamValue(&spatialOut);
      averageSpeedList.push_back(curveLength / duration);
    } break;
    case AEGP_StreamType_ThreeD:
      averageSpeedList.push_back(static_cast<float>(value.val.three_d.x - lastValue.val.three_d.x) /
                                 duration);
      averageSpeedList.push_back(static_cast<float>(value.val.three_d.y - lastValue.val.three_d.y) /
                                 duration);
      averageSpeedList.push_back(static_cast<float>(value.val.three_d.z - lastValue.val.three_d.z) /
                                 duration);
      break;
    case AEGP_StreamType_TwoD:
      averageSpeedList.push_back(static_cast<float>(value.val.two_d.x - lastValue.val.two_d.x) /
                                 duration);
      averageSpeedList.push_back(static_cast<float>(value.val.two_d.y - lastValue.val.two_d.y) /
                                 duration);
      break;
    case AEGP_StreamType_OneD:
      averageSpeedList.push_back(static_cast<float>(value.val.one_d - lastValue.val.one_d) /
                                 duration);
      break;
    case AEGP_StreamType_COLOR:
      averageSpeedList.push_back(
          static_cast<float>(value.val.color.redF - lastValue.val.color.redF) * 255 / duration);
      averageSpeedList.push_back(
          static_cast<float>(value.val.color.greenF - lastValue.val.color.greenF) * 255 / duration);
      averageSpeedList.push_back(
          static_cast<float>(value.val.color.blueF - lastValue.val.color.blueF) * 255 / duration);
      break;
    default:
      averageSpeedList.push_back(1);
      break;
  }

  Suites->StreamSuite4()->AEGP_DisposeStreamValue(&value);
  Suites->StreamSuite4()->AEGP_DisposeStreamValue(&lastValue);

  if (dimensionality == 1 && averageSpeedList.size() > 1) {
    float averageSpeed = 0;
    for (const auto& speed : averageSpeedList) {
      averageSpeed += speed * speed;
    }
    averageSpeedList.clear();
    averageSpeedList.push_back(sqrtf(averageSpeed));
  }
  return averageSpeedList;
}

void CheckStreamExpression(AEGP_StreamRefH stream) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  A_Boolean canVary = false;
  Suites->StreamSuite4()->AEGP_CanVaryOverTime(stream, &canVary);
  if (canVary) {
    A_Boolean flag = false;
    Suites->StreamSuite4()->AEGP_GetExpressionState(PluginID, stream, &flag);
    if (flag) {
      PAGExportSession::RecordWarning(AlertInfoType::Expression);
    }
  }
}

}  // namespace exporter
