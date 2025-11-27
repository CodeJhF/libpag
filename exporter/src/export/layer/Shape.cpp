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

#include "Shape.h"
#include "export/stream/StreamProperty.h"
#include "export/stream/StreamValue.h"

namespace exporter {

static const std::unordered_map<std::string, pag::ShapeType> ShapeTypeMap = {
    {"ADBE Vector Group", pag::ShapeType::ShapeGroup},
    {"ADBE Vector Shape - Rect", pag::ShapeType::Rectangle},
    {"ADBE Vector Shape - Ellipse", pag::ShapeType::Ellipse},
    {"ADBE Vector Shape - Star", pag::ShapeType::PolyStar},
    {"ADBE Vector Shape - Group", pag::ShapeType::ShapePath},
    {"ADBE Vector Graphic - Fill", pag::ShapeType::Fill},
    {"ADBE Vector Graphic - Stroke", pag::ShapeType::Stroke},
    {"ADBE Vector Graphic - G-Fill", pag::ShapeType::GradientFill},
    {"ADBE Vector Graphic - G-Stroke", pag::ShapeType::GradientStroke},
    {"ADBE Vector Filter - Merge", pag::ShapeType::MergePaths},
    {"ADBE Vector Filter - Trim", pag::ShapeType::TrimPaths},
    {"ADBE Vector Filter - Repeater", pag::ShapeType::Repeater},
    {"ADBE Vector Filter - RC", pag::ShapeType::RoundCorners}};

static pag::ShapeType GetShapeType(const AEGP_StreamRefH& streamH) {
  std::string matchName = AEHelper::GetStreamMatchName(streamH);
  auto result = ShapeTypeMap.find(matchName);
  if (result == ShapeTypeMap.end()) {
    return pag::ShapeType::Unknown;
  }
  return result->second;
}

static pag::ShapeElement* GetShape(const AEGP_StreamRefH& streamH, int& gradientIndex,
                                   const std::shared_ptr<PAGExportSession>& session);

static pag::ShapeElement* GetShapeGroup(const AEGP_StreamRefH& streamH,
                                        const std::shared_ptr<PAGExportSession>& session) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();
  auto element = new pag::ShapeGroupElement();
  float frameRate = session->frameRate;
  auto map = CreatePropertyMap(frameRate);

  element->blendMode =
      GetValue(streamH, "ADBE Vector Blend Mode", AEStreamParser::ShapeBlendModeParser);

  AEGP_StreamRefH transformStreamH = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByMatchname(
      PluginID, streamH, "ADBE Vector Transform Group", &transformStreamH);
  auto transform = new pag::ShapeTransform();
  transform->anchorPoint =
      GetProperty(transformStreamH, "ADBE Vector Anchor", AEStreamParser::PointParser, map);
  transform->position =
      GetProperty(transformStreamH, "ADBE Vector Position", AEStreamParser::PointParser, map);
  transform->scale =
      GetProperty(transformStreamH, "ADBE Vector Scale", AEStreamParser::ScaleParser, map);
  transform->skew = GetProperty(transformStreamH, "ADBE Vector Skew", AEStreamParser::FloatParser, map);
  transform->skewAxis =
      GetProperty(transformStreamH, "ADBE Vector Skew Axis", AEStreamParser::FloatParser, map);
  transform->rotation =
      GetProperty(transformStreamH, "ADBE Vector Rotation", AEStreamParser::FloatParser, map);
  transform->opacity = GetProperty(transformStreamH, "ADBE Vector Group Opacity",
                                   AEStreamParser::Opacity0_100Parser, map);
  element->transform = transform;
  Suites->StreamSuite4()->AEGP_DisposeStream(transformStreamH);

  int gradientIndex = 0;
  AEGP_StreamRefH contents = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByMatchname(PluginID, streamH,
                                                                 "ADBE Vectors Group", &contents);
  A_long numElements = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(contents, &numElements);
  for (A_long index = 0; index < numElements; index++) {
    AEGP_StreamRefH childStreamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, contents, index,
                                                               &childStreamH);
    auto shape = GetShape(childStreamH, gradientIndex, session);
    if (shape != nullptr) {
      element->elements.push_back(shape);
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(childStreamH);
  }
  Suites->StreamSuite4()->AEGP_DisposeStream(contents);
  return element;
}

static pag::ShapeElement* GetRectangle(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::RectangleElement();
  auto map = CreatePropertyMap(frameRate);

  element->reversed =
      GetValue(streamH, "ADBE Vector Shape Direction", AEStreamParser::ShapeDirectionParser);
  element->size = GetProperty(streamH, "ADBE Vector Rect Size", AEStreamParser::PointParser, map, 2);
  element->position =
      GetProperty(streamH, "ADBE Vector Rect Position", AEStreamParser::PointParser, map);
  element->roundness =
      GetProperty(streamH, "ADBE Vector Rect Roundness", AEStreamParser::FloatParser, map);
  return element;
}

static pag::ShapeElement* GetEllipse(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::EllipseElement();
  auto map = CreatePropertyMap(frameRate);

  element->reversed =
      GetValue(streamH, "ADBE Vector Shape Direction", AEStreamParser::ShapeDirectionParser);
  element->size =
      GetProperty(streamH, "ADBE Vector Ellipse Size", AEStreamParser::PointParser, map, 2);
  element->position =
      GetProperty(streamH, "ADBE Vector Ellipse Position", AEStreamParser::PointParser, map);
  return element;
}

static pag::ShapeElement* GetPolyStar(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::PolyStarElement();
  auto map = CreatePropertyMap(frameRate);

  element->reversed =
      GetValue(streamH, "ADBE Vector Shape Direction", AEStreamParser::ShapeDirectionParser);
  element->polyType =
      GetValue(streamH, "ADBE Vector Star Type", AEStreamParser::PolyStarTypeParser);
  element->points = GetProperty(streamH, "ADBE Vector Star Points", AEStreamParser::FloatParser, map);
  element->position =
      GetProperty(streamH, "ADBE Vector Star Position", AEStreamParser::PointParser, map);
  element->rotation =
      GetProperty(streamH, "ADBE Vector Star Rotation", AEStreamParser::FloatParser, map);
  element->innerRadius =
      GetProperty(streamH, "ADBE Vector Star Inner Radius", AEStreamParser::FloatParser, map);
  element->outerRadius =
      GetProperty(streamH, "ADBE Vector Star Outer Radius", AEStreamParser::FloatParser, map);
  element->innerRoundness =
      GetProperty(streamH, "ADBE Vector Star Inner Roundess", AEStreamParser::PercentParser, map);
  element->outerRoundness =
      GetProperty(streamH, "ADBE Vector Star Outer Roundess", AEStreamParser::PercentParser, map);
  return element;
}

static pag::ShapeElement* GetShapePath(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::ShapePathElement();
  auto map = CreatePropertyMap(frameRate);

  auto reversed =
      GetValue(streamH, "ADBE Vector Shape Direction", AEStreamParser::ShapeDirectionParser);
  element->shapePath = GetProperty(streamH, "ADBE Vector Shape", AEStreamParser::PathParser, map);
  if (reversed) {
    if (element->shapePath->animatable()) {
      auto property = static_cast<pag::AnimatableProperty<pag::PathHandle>*>(element->shapePath);
      for (auto& keyframe : property->keyframes) {
        keyframe->startValue->reverse();
        keyframe->endValue->reverse();
      }
    } else {
      element->shapePath->value->reverse();
    }
  }
  return element;
}

static pag::ShapeElement* GetFill(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::FillElement();
  auto map = CreatePropertyMap(frameRate);

  element->blendMode =
      GetValue(streamH, "ADBE Vector Blend Mode", AEStreamParser::ShapeBlendModeParser);
  element->composite =
      GetValue(streamH, "ADBE Vector Composite Order", AEStreamParser::CompositeOrderParser);
  element->fillRule = GetValue(streamH, "ADBE Vector Fill Rule", AEStreamParser::FillRuleParser);
  element->color = GetProperty(streamH, "ADBE Vector Fill Color", AEStreamParser::ColorParser, map);
  element->opacity =
      GetProperty(streamH, "ADBE Vector Fill Opacity", AEStreamParser::Opacity0_100Parser, map);
  return element;
}

static void GetDashes(const AEGP_StreamRefH& streamH, pag::ShapeElement* element) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  AEGP_StreamRefH dashStreamH = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByMatchname(
      PluginID, streamH, "ADBE Vector Stroke Dashes", &dashStreamH);
  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(dashStreamH, &numStreams);
  if (numStreams > 0 && AEHelper::IsStreamActive(dashStreamH)) {
    pag::StrokeElement* strokeElement = nullptr;
    pag::GradientStrokeElement* gradientStrokeElement = nullptr;
    if (element->type() == pag::ShapeType::Stroke) {
      strokeElement = static_cast<pag::StrokeElement*>(element);
    } else {
      gradientStrokeElement = static_cast<pag::GradientStrokeElement*>(element);
    }
    auto& dashes = strokeElement != nullptr ? strokeElement->dashes : gradientStrokeElement->dashes;
    auto& dashOffset =
        strokeElement != nullptr ? strokeElement->dashOffset : gradientStrokeElement->dashOffset;
    for (A_long index = 0; index < numStreams; index++) {
      AEGP_StreamRefH childStreamH = nullptr;
      Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, dashStreamH, index,
                                                                 &childStreamH);
      if (!AEHelper::IsStreamHidden(childStreamH)) {
        auto dash = GetProperty(childStreamH, index, AEStreamParser::FloatParser);
        dashes.push_back(dash);
      }
      Suites->StreamSuite4()->AEGP_DisposeStream(childStreamH);
    }
    if (!dashes.empty()) {
      dashOffset = GetProperty(dashStreamH, numStreams - 1, AEStreamParser::FloatParser);
    }
  }
  Suites->StreamSuite4()->AEGP_DisposeStream(dashStreamH);
}

static pag::ShapeElement* GetStroke(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::StrokeElement();
  auto map = CreatePropertyMap(frameRate);

  element->blendMode =
      GetValue(streamH, "ADBE Vector Blend Mode", AEStreamParser::ShapeBlendModeParser);
  element->composite =
      GetValue(streamH, "ADBE Vector Composite Order", AEStreamParser::CompositeOrderParser);
  element->color = GetProperty(streamH, "ADBE Vector Stroke Color", AEStreamParser::ColorParser, map);
  element->opacity =
      GetProperty(streamH, "ADBE Vector Stroke Opacity", AEStreamParser::Opacity0_100Parser, map);
  element->strokeWidth =
      GetProperty(streamH, "ADBE Vector Stroke Width", AEStreamParser::FloatParser, map);
  element->lineCap =
      GetValue(streamH, "ADBE Vector Stroke Line Cap", AEStreamParser::LineCapParser);
  element->lineJoin =
      GetValue(streamH, "ADBE Vector Stroke Line Join", AEStreamParser::LineJoinParser);
  element->miterLimit =
      GetProperty(streamH, "ADBE Vector Stroke Miter Limit", AEStreamParser::FloatParser, map);
  GetDashes(streamH, element);
  return element;
}

static pag::ShapeElement* GetGradientFill(const AEGP_StreamRefH& streamH, int& gradientIndex,
                                          float frameRate) {
  auto element = new pag::GradientFillElement();
  auto map = CreatePropertyMap(frameRate);
  map["index"] = gradientIndex;

  element->blendMode =
      GetValue(streamH, "ADBE Vector Blend Mode", AEStreamParser::ShapeBlendModeParser);
  element->composite =
      GetValue(streamH, "ADBE Vector Composite Order", AEStreamParser::CompositeOrderParser);
  element->fillRule = GetValue(streamH, "ADBE Vector Fill Rule", AEStreamParser::FillRuleParser);
  element->fillType =
      GetValue(streamH, "ADBE Vector Grad Type", AEStreamParser::GradientFillTypeParser);
  element->startPoint =
      GetProperty(streamH, "ADBE Vector Grad Start Pt", AEStreamParser::PointParser, map);
  element->endPoint = GetProperty(streamH, "ADBE Vector Grad End Pt", AEStreamParser::PointParser, map);
  element->colors =
      GetProperty(streamH, "ADBE Vector Grad Colors", AEStreamParser::GradientColorParser, map);
  element->opacity =
      GetProperty(streamH, "ADBE Vector Fill Opacity", AEStreamParser::Opacity0_100Parser, map);
  gradientIndex++;
  return element;
}

static pag::ShapeElement* GetGradientStroke(const AEGP_StreamRefH& streamH, int& gradientIndex,
                                            float frameRate) {
  auto element = new pag::GradientStrokeElement();
  auto map = CreatePropertyMap(frameRate);

  element->blendMode =
      GetValue(streamH, "ADBE Vector Blend Mode", AEStreamParser::ShapeBlendModeParser);
  element->composite =
      GetValue(streamH, "ADBE Vector Composite Order", AEStreamParser::CompositeOrderParser);
  element->fillType =
      GetValue(streamH, "ADBE Vector Grad Type", AEStreamParser::GradientFillTypeParser);
  element->startPoint =
      GetProperty(streamH, "ADBE Vector Grad Start Pt", AEStreamParser::PointParser, map);
  element->endPoint = GetProperty(streamH, "ADBE Vector Grad End Pt", AEStreamParser::PointParser, map);
  element->colors =
      GetProperty(streamH, "ADBE Vector Grad Colors", AEStreamParser::GradientColorParser, map);
  element->opacity =
      GetProperty(streamH, "ADBE Vector Stroke Opacity", AEStreamParser::Opacity0_100Parser, map);
  element->strokeWidth =
      GetProperty(streamH, "ADBE Vector Stroke Width", AEStreamParser::FloatParser, map);
  element->lineCap =
      GetValue(streamH, "ADBE Vector Stroke Line Cap", AEStreamParser::LineCapParser);
  element->lineJoin =
      GetValue(streamH, "ADBE Vector Stroke Line Join", AEStreamParser::LineJoinParser);
  element->miterLimit =
      GetProperty(streamH, "ADBE Vector Stroke Miter Limit", AEStreamParser::FloatParser, map);
  GetDashes(streamH, element);
  gradientIndex++;
  return element;
}

static pag::ShapeElement* GetMergePaths(const AEGP_StreamRefH& streamH) {
  auto element = new pag::MergePathsElement();
  element->mode = GetValue(streamH, "ADBE Vector Merge Type", AEStreamParser::MergePathsModeParser);
  return element;
}

static pag::ShapeElement* GetTrimPaths(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::TrimPathsElement();
  auto map = CreatePropertyMap(frameRate);

  element->start = GetProperty(streamH, "ADBE Vector Trim Start", AEStreamParser::PercentParser, map);
  element->end = GetProperty(streamH, "ADBE Vector Trim End", AEStreamParser::PercentParser, map);
  element->offset = GetProperty(streamH, "ADBE Vector Trim Offset", AEStreamParser::FloatParser, map);
  element->trimType =
      GetValue(streamH, "ADBE Vector Trim Type", AEStreamParser::TrimPathsTypeParser);
  return element;
}

static pag::RepeaterTransform* GetRepeaterTransform(const AEGP_StreamRefH& streamH, float frameRate) {
  auto transform = new pag::RepeaterTransform();
  auto map = CreatePropertyMap(frameRate);

  transform->anchorPoint =
      GetProperty(streamH, "ADBE Vector Repeater Anchor", AEStreamParser::PointParser, map);
  transform->position =
      GetProperty(streamH, "ADBE Vector Repeater Position", AEStreamParser::PointParser, map);
  transform->scale =
      GetProperty(streamH, "ADBE Vector Repeater Scale", AEStreamParser::ScaleParser, map, 2);
  transform->rotation =
      GetProperty(streamH, "ADBE Vector Repeater Rotation", AEStreamParser::FloatParser, map);
  transform->startOpacity =
      GetProperty(streamH, "ADBE Vector Repeater Opacity 1", AEStreamParser::Opacity0_100Parser, map);
  transform->endOpacity =
      GetProperty(streamH, "ADBE Vector Repeater Opacity 2", AEStreamParser::Opacity0_100Parser, map);
  return transform;
}

static pag::ShapeElement* GetRepeater(const AEGP_StreamRefH& streamH, float frameRate) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();
  auto element = new pag::RepeaterElement();
  auto map = CreatePropertyMap(frameRate);

  element->copies =
      GetProperty(streamH, "ADBE Vector Repeater Copies", AEStreamParser::FloatParser, map);
  element->offset =
      GetProperty(streamH, "ADBE Vector Repeater Offset", AEStreamParser::FloatParser, map);
  element->composite =
      GetValue(streamH, "ADBE Vector Repeater Order", AEStreamParser::RepeaterOrderParser);
  AEGP_StreamRefH transform = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByMatchname(
      PluginID, streamH, "ADBE Vector Repeater Transform", &transform);
  element->transform = GetRepeaterTransform(transform, frameRate);
  Suites->StreamSuite4()->AEGP_DisposeStream(transform);
  return element;
}

static pag::ShapeElement* GetRoundCorners(const AEGP_StreamRefH& streamH, float frameRate) {
  auto element = new pag::RoundCornersElement();
  auto map = CreatePropertyMap(frameRate);

  element->radius =
      GetProperty(streamH, "ADBE Vector RoundCorner Radius", AEStreamParser::FloatParser, map);
  return element;
}

static pag::ShapeElement* GetShape(const AEGP_StreamRefH& streamH, int& gradientIndex,
                                   const std::shared_ptr<PAGExportSession>& session) {
  if (!AEHelper::IsStreamActive(streamH)) {
    return nullptr;
  }

  pag::ShapeElement* element = nullptr;
  auto type = GetShapeType(streamH);
  float frameRate = session->frameRate;
  switch (type) {
    case pag::ShapeType::ShapeGroup:
      element = GetShapeGroup(streamH, session);
      break;
    case pag::ShapeType::Rectangle:
      element = GetRectangle(streamH, frameRate);
      break;
    case pag::ShapeType::Ellipse:
      element = GetEllipse(streamH, frameRate);
      break;
    case pag::ShapeType::PolyStar:
      element = GetPolyStar(streamH, frameRate);
      break;
    case pag::ShapeType::ShapePath:
      element = GetShapePath(streamH, frameRate);
      break;
    case pag::ShapeType::Fill:
      element = GetFill(streamH, frameRate);
      break;
    case pag::ShapeType::Stroke:
      element = GetStroke(streamH, frameRate);
      break;
    case pag::ShapeType::GradientFill:
      element = GetGradientFill(streamH, gradientIndex, frameRate);
      break;
    case pag::ShapeType::GradientStroke:
      element = GetGradientStroke(streamH, gradientIndex, frameRate);
      break;
    case pag::ShapeType::MergePaths:
      element = GetMergePaths(streamH);
      break;
    case pag::ShapeType::TrimPaths:
      element = GetTrimPaths(streamH, frameRate);
      break;
    case pag::ShapeType::Repeater:
      element = GetRepeater(streamH, frameRate);
      break;
    case pag::ShapeType::RoundCorners:
      element = GetRoundCorners(streamH, frameRate);
      break;
    default:
      break;
  }
  return element;
}

std::vector<pag::ShapeElement*> GetShapes(const AEGP_LayerH& layerH, const std::shared_ptr<PAGExportSession>& session) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();
  std::vector<pag::ShapeElement*> contents = {};

  int gradientIndex = 0;
  AEGP_StreamRefH layerStreamH = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefForLayer(PluginID, layerH, &layerStreamH);
  AEGP_StreamRefH rootStreamH = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByMatchname(
      PluginID, layerStreamH, "ADBE Root Vectors Group", &rootStreamH);
  Suites->StreamSuite4()->AEGP_DisposeStream(layerStreamH);
  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(rootStreamH, &numStreams);
  for (A_long index = 0; index < numStreams; index++) {
    AEGP_StreamRefH streamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, rootStreamH, index,
                                                               &streamH);
    auto element = GetShape(streamH, gradientIndex, session);
    if (element != nullptr) {
      contents.push_back(element);
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(streamH);
  }
  Suites->StreamSuite4()->AEGP_DisposeStream(rootStreamH);
  return contents;
}

}  // namespace exporter