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

#include "TextProperty.h"
#include "base/keyframes/SpatialPointKeyframe.h"
#include "export/stream/StreamProperty.h"
#include "rendering/graphics/Text.h"

namespace exporter {

enum class TextPropertyType { Unknown, Document, PathOptions, MoreOptions, Animators };

enum class TextAnimatorsType { Unknown, Animator };

enum class TextAnimatorType { Unknown, Selectors, AnimatorProperties };

enum class TextSelectorType { Unknown, RangeSelector, WigglySelector, ExpressibleSelector };

enum class TextAnimatorPropertiesType {
  Unknown,
  TrackingType,
  TrackingAmount,
  FillColor,
  StrokeColor,
  Position,
  Scale,
  Rotation,
  Opacity
};

static const std::unordered_map<std::string, TextPropertyType> TextPropertyTypeMap = {
    {"ADBE Text Document", TextPropertyType::Document},
    {"ADBE Text Path Options", TextPropertyType::PathOptions},
    {"ADBE Text More Options", TextPropertyType::MoreOptions},
    {"ADBE Text Animators", TextPropertyType::Animators},
};

static const std::unordered_map<std::string, TextAnimatorsType> TextAnimatorsTypeMap = {
    {"ADBE Text Animator", TextAnimatorsType::Animator},
};

static const std::unordered_map<std::string, TextAnimatorType> TextAnimatorTypeMap = {
    {"ADBE Text Selectors", TextAnimatorType::Selectors},
    {"ADBE Text Animator Properties", TextAnimatorType::AnimatorProperties},
};

static const std::unordered_map<std::string, TextSelectorType> TextSelectorTypeMap = {
    {"ADBE Text Selector", TextSelectorType::RangeSelector},
    {"ADBE Text Wiggly Selector", TextSelectorType::WigglySelector},
    {"ADBE Text Expressible Selector", TextSelectorType::ExpressibleSelector}};

static const std::unordered_map<std::string, TextAnimatorPropertiesType>
    TextAnimatorPropertiesTypeMap = {
        {"ADBE Text Track Type", TextAnimatorPropertiesType::TrackingType},
        {"ADBE Text Tracking Amount", TextAnimatorPropertiesType::TrackingAmount},
        {"ADBE Text Position 3D", TextAnimatorPropertiesType::Position},
        {"ADBE Text Scale 3D", TextAnimatorPropertiesType::Scale},
        {"ADBE Text Rotation", TextAnimatorPropertiesType::Rotation},
        {"ADBE Text Opacity", TextAnimatorPropertiesType::Opacity},
        {"ADBE Text Fill Color", TextAnimatorPropertiesType::FillColor},
        {"ADBE Text Stroke Color", TextAnimatorPropertiesType::StrokeColor}};

static TextPropertyType GetTextPropertyType(const AEGP_StreamRefH& streamH) {
  std::string matchName = AEHelper::GetStreamMatchName(streamH);
  auto result = TextPropertyTypeMap.find(matchName);
  if (result == TextPropertyTypeMap.end()) {
    return TextPropertyType::Unknown;
  }
  return result->second;
}

static TextAnimatorsType GetTextAnimatorsType(const AEGP_StreamRefH& streamH) {
  std::string matchName = AEHelper::GetStreamMatchName(streamH);
  auto result = TextAnimatorsTypeMap.find(matchName);
  if (result == TextAnimatorsTypeMap.end()) {
    return TextAnimatorsType::Unknown;
  }
  return result->second;
}

static TextAnimatorType GetTextAnimatorType(const AEGP_StreamRefH& streamH) {
  std::string matchName = AEHelper::GetStreamMatchName(streamH);
  auto result = TextAnimatorTypeMap.find(matchName);
  if (result == TextAnimatorTypeMap.end()) {
    return TextAnimatorType::Unknown;
  }
  return result->second;
}

static TextSelectorType GetTextSelectorType(const AEGP_StreamRefH& streamH) {
  std::string matchName = AEHelper::GetStreamMatchName(streamH);
  auto result = TextSelectorTypeMap.find(matchName);
  if (result == TextSelectorTypeMap.end()) {
    return TextSelectorType::Unknown;
  }
  return result->second;
}

static TextAnimatorPropertiesType GetTextAnimatorPropertiesType(const AEGP_StreamRefH& streamH) {
  std::string matchName = AEHelper::GetStreamMatchName(streamH);
  auto result = TextAnimatorPropertiesTypeMap.find(matchName);
  if (result == TextAnimatorPropertiesTypeMap.end()) {
    return TextAnimatorPropertiesType::Unknown;
  }
  return result->second;
}

static pag::TextRangeSelector* GetTextRangeSelector(const AEGP_StreamRefH& streamH) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();
  auto selector = new pag::TextRangeSelector();

  selector->start = GetProperty(streamH, "ADBE Text Percent Start", AEStreamParser::PercentParser);
  selector->end = GetProperty(streamH, "ADBE Text Percent End", AEStreamParser::PercentParser);
  selector->offset =
      GetProperty(streamH, "ADBE Text Percent Offset", AEStreamParser::PercentParser);

  AEGP_StreamRefH advancedStream;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByMatchname(
      PluginID, streamH, "ADBE Text Range Advanced", &advancedStream);
  selector->units = GetValue(advancedStream, "ADBE Text Range Units",
                             AEStreamParser::TextRangeSelectorUnitsParser);
  selector->basedOn =
      GetValue(advancedStream, "ADBE Text Range Type2", AEStreamParser::TextSelectorBasedOnParser);
  selector->mode = GetProperty(advancedStream, "ADBE Text Selector Mode",
                               AEStreamParser::TextSelectorModeParser);
  selector->amount =
      GetProperty(advancedStream, "ADBE Text Selector Max Amount", AEStreamParser::PercentParser);
  selector->shape = GetValue(advancedStream, "ADBE Text Range Shape",
                             AEStreamParser::TextRangeSelectorShapeParser);
  selector->smoothness =
      GetProperty(advancedStream, "ADBE Text Selector Smoothness", AEStreamParser::PercentParser);
  selector->easeHigh =
      GetProperty(advancedStream, "ADBE Text Levels Max Ease", AEStreamParser::PercentParser);
  selector->easeLow =
      GetProperty(advancedStream, "ADBE Text Levels Min Ease", AEStreamParser::PercentParser);
  selector->randomizeOrder =
      GetValue(advancedStream, "ADBE Text Randomize Order", AEStreamParser::BooleanParser);
  selector->randomSeed =
      GetProperty(advancedStream, "ADBE Text Random Seed", AEStreamParser::Uint16Parser);

  return selector;
}

static pag::TextWigglySelector* GetTextWigglySelector(const AEGP_StreamRefH& streamH) {
  auto selector = new pag::TextWigglySelector();

  selector->mode =
      GetProperty(streamH, "ADBE Text Selector Mode", AEStreamParser::TextSelectorModeParser);
  selector->maxAmount =
      GetProperty(streamH, "ADBE Text Wiggly Max Amount", AEStreamParser::PercentParser);
  selector->minAmount =
      GetProperty(streamH, "ADBE Text Wiggly Min Amount", AEStreamParser::PercentParser);
  selector->basedOn =
      GetValue(streamH, "ADBE Text Range Type2", AEStreamParser::TextSelectorBasedOnParser);
  selector->wigglesPerSecond =
      GetProperty(streamH, "ADBE Text Temporal Freq", AEStreamParser::FloatParser);
  selector->correlation =
      GetProperty(streamH, "ADBE Text Character Correlation", AEStreamParser::PercentParser);
  selector->temporalPhase =
      GetProperty(streamH, "ADBE Text Temporal Phase", AEStreamParser::FloatParser);
  selector->spatialPhase =
      GetProperty(streamH, "ADBE Text Spatial Phase", AEStreamParser::FloatParser);
  selector->lockDimensions =
      GetProperty(streamH, "ADBE Text Wiggly Lock Dim", AEStreamParser::BooleanParser);
  selector->randomSeed =
      GetProperty(streamH, "ADBE Text Wiggly Random Seed", AEStreamParser::Uint16Parser);

  return selector;
}

static void CheckTextDirection(pag::Property<pag::TextDocumentHandle>* textDocument,
                               uint16_t tagLevel) {
  if (textDocument == nullptr) {
    return;
  }
  bool hasVerticalText = false;
  if (textDocument->animatable()) {
    for (const auto& keyFrame :
         reinterpret_cast<pag::AnimatableProperty<pag::TextDocumentHandle>*>(textDocument)
             ->keyframes) {
      if (keyFrame->startValue->direction == pag::TextDirection::Vertical ||
          keyFrame->endValue->direction == pag::TextDirection::Vertical) {
        hasVerticalText = true;
        break;
      }
    }
    if (!hasVerticalText || tagLevel < static_cast<uint16_t>(pag::TagCode::TextSourceV3)) {
      for (auto keyframe :
           reinterpret_cast<pag::AnimatableProperty<pag::TextDocumentHandle>*>(textDocument)
               ->keyframes) {
        keyframe->startValue->direction = pag::TextDirection::Default;
        keyframe->endValue->direction = pag::TextDirection::Default;
      }
    }
  } else {
    auto document = textDocument->getValueAt(0);
    if (document->direction == pag::TextDirection::Vertical) {
      hasVerticalText = true;
    }
    if (!hasVerticalText || tagLevel < static_cast<uint16_t>(pag::TagCode::TextSourceV3)) {
      document->direction = pag::TextDirection::Default;
    }
  }

  if (hasVerticalText && tagLevel < static_cast<uint16_t>(pag::TagCode::TextSourceV3)) {
    PAGExportSession::RecordWarning(AlertInfoType::TagLevelVerticalText);
  }
}

static std::vector<pag::TextSelector*> GetTextSelectors(const AEGP_StreamRefH& streamH) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();
  std::vector<pag::TextSelector*> vec = {};

  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(streamH, &numStreams);
  for (A_long index = 0; index < numStreams; index++) {
    AEGP_StreamRefH childStreamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, streamH, index,
                                                               &childStreamH);
    if (!AEHelper::IsStreamHidden(childStreamH) && AEHelper::IsStreamActive(childStreamH)) {
      auto type = GetTextSelectorType(childStreamH);
      pag::TextSelector* selector = nullptr;
      switch (type) {
        case TextSelectorType::RangeSelector:
          selector = GetTextRangeSelector(childStreamH);
          break;
        case TextSelectorType::WigglySelector:
          selector = GetTextWigglySelector(childStreamH);
          break;
        case TextSelectorType::ExpressibleSelector:
          break;
        default:
          break;
      }
      if (selector != nullptr) {
        vec.push_back(selector);
      }
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(childStreamH);
  }

  return vec;
}

static pag::TextAnimatorColorProperties* GetTextAnimatorColorProperties(
    const AEGP_StreamRefH& streamH) {
  auto properties = new pag::TextAnimatorColorProperties();
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(streamH, &numStreams);
  for (A_long index = 0; index < numStreams; index++) {
    AEGP_StreamRefH childStreamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, streamH, index,
                                                               &childStreamH);
    if (!AEHelper::IsStreamHidden(childStreamH) && AEHelper::IsStreamActive(childStreamH)) {
      auto type = GetTextAnimatorPropertiesType(childStreamH);
      switch (type) {
        case TextAnimatorPropertiesType::FillColor:
          properties->fillColor = GetProperty(childStreamH, AEStreamParser::ColorParser);
          break;
        case TextAnimatorPropertiesType::StrokeColor:
          properties->strokeColor = GetProperty(childStreamH, AEStreamParser::ColorParser);
          break;
        default:
          break;
      }
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(childStreamH);
  }
  if (!properties->verify()) {
    delete properties;
    return nullptr;
  }
  return properties;
}

static pag::Property<float>* GetTextAnimatorTrackingAmount(const AEGP_StreamRefH& streamH) {
  auto trackingAmount = GetProperty(streamH, AEStreamParser::FloatParser);
  if (!trackingAmount->animatable() && trackingAmount->value == 0.0f) {
    delete trackingAmount;
    return nullptr;
  }
  return trackingAmount;
}

static pag::Property<pag::Point>* GetTextAnimatorPosition(const AEGP_StreamRefH& streamH) {
  auto position = GetProperty(streamH, AEStreamParser::PointParser);
  if (!position->animatable() && position->value.x == 0.0f && position->value.y == 0.0f) {
    delete position;
    return nullptr;
  }

  if (position->animatable()) {
    auto& keyframes = static_cast<pag::AnimatableProperty<pag::Point>*>(position)->keyframes;
    for (auto& keyframe : keyframes) {
      auto newKeyframe = new pag::SpatialPointKeyframe();
      *static_cast<pag::Keyframe<pag::Point>*>(newKeyframe) = *keyframe;
      newKeyframe->initialize();
      delete keyframe;
      keyframe = newKeyframe;
    }
  }

  return position;
}

static pag::Property<pag::Point>* GetTextAnimatorScale(const AEGP_StreamRefH& streamH) {
  auto scale = GetProperty(streamH, AEStreamParser::ScaleParser);
  if (!scale->animatable() && scale->value.x == 1.0f && scale->value.x == 1.0f) {
    delete scale;
    return nullptr;
  }
  return scale;
}

static pag::Property<float>* GetTextAnimatorRotation(const AEGP_StreamRefH& streamH) {
  auto rotation = GetProperty(streamH, AEStreamParser::FloatParser);
  if (!rotation->animatable() && rotation->value == 0.0f) {
    delete rotation;
    return nullptr;
  }
  return rotation;
}

static pag::Property<pag::Opacity>* GetTextAnimatorOpacity(const AEGP_StreamRefH& streamH) {
  auto opacity = GetProperty(streamH, AEStreamParser::Opacity0_100Parser);
  if (!opacity->animatable() && opacity->value == pag::Opaque) {
    delete opacity;
    return nullptr;
  }
  return opacity;
}

static pag::TextAnimatorTypographyProperties* GetTextAnimatorTypographyProperties(
    const AEGP_StreamRefH& streamH) {
  auto properties = new pag::TextAnimatorTypographyProperties();
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(streamH, &numStreams);
  for (A_long index = 0; index < numStreams; index++) {
    AEGP_StreamRefH childStreamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, streamH, index,
                                                               &childStreamH);
    auto type = GetTextAnimatorPropertiesType(childStreamH);
    switch (type) {
      case TextAnimatorPropertiesType::TrackingType:
        properties->trackingType =
            GetProperty(childStreamH, AEStreamParser::TextAnimatorTrackingTypeParser);
        break;
      case TextAnimatorPropertiesType::TrackingAmount:
        properties->trackingAmount = GetTextAnimatorTrackingAmount(childStreamH);
        break;
      case TextAnimatorPropertiesType::Position:
        properties->position = GetTextAnimatorPosition(childStreamH);
        break;
      case TextAnimatorPropertiesType::Scale:
        properties->scale = GetTextAnimatorScale(childStreamH);
        break;
      case TextAnimatorPropertiesType::Rotation:
        properties->rotation = GetTextAnimatorRotation(childStreamH);
        break;
      case TextAnimatorPropertiesType::Opacity:
        properties->opacity = GetTextAnimatorOpacity(childStreamH);
        break;
      default:
        break;
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(childStreamH);
  }

  if (properties->trackingAmount == nullptr && properties->trackingType != nullptr) {
    delete properties->trackingType;
    properties->trackingType = nullptr;
  }

  if (!properties->verify()) {
    delete properties;
    return nullptr;
  }
  return properties;
}

static pag::TextAnimator* GetTextAnimator(const AEGP_StreamRefH& streamH) {
  auto animator = new pag::TextAnimator();
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(streamH, &numStreams);
  for (long index = 0; index < numStreams; index++) {
    AEGP_StreamRefH childStreamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, streamH, index,
                                                               &childStreamH);
    if (!AEHelper::IsStreamHidden(childStreamH) && AEHelper::IsStreamActive(childStreamH)) {
      auto type = GetTextAnimatorType(childStreamH);
      if (type == TextAnimatorType::Selectors) {
        auto selectors = GetTextSelectors(childStreamH);
        animator->selectors.insert(selectors.begin(), selectors.end(), animator->selectors.end());
      } else if (type == TextAnimatorType::AnimatorProperties) {
        animator->colorProperties = GetTextAnimatorColorProperties(childStreamH);
        animator->typographyProperties = GetTextAnimatorTypographyProperties(childStreamH);
      }
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(childStreamH);
  }
  if (!animator->verify()) {
    delete animator;
    return nullptr;
  }
  return animator;
}

static std::vector<pag::TextAnimator*> GetTextAnimators(const AEGP_StreamRefH& streamH) {
  const auto& Suites = AEHelper::GetSuites();
  std::vector<pag::TextAnimator*> vec = {};

  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(streamH, &numStreams);
  for (A_long index = 0; index < numStreams; index++) {
    AEGP_StreamRefH childStreamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(AEHelper::GetPluginID(), streamH,
                                                               index, &childStreamH);
    if (!AEHelper::IsStreamHidden(childStreamH) && AEHelper::IsStreamActive(childStreamH)) {
      auto type = GetTextAnimatorsType(childStreamH);
      if (type == TextAnimatorsType::Animator) {
        auto animator = GetTextAnimator(childStreamH);
        if (animator != nullptr) {
          vec.push_back(animator);
        }
      }
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(childStreamH);
  }

  return vec;
}

static pag::TextPathOptions* GetTextPathOptions(const AEGP_StreamRefH& streamH) {
  auto pathOptions = new pag::TextPathOptions();
  pag::ID maskID = GetValue(streamH, "ADBE Text Path", AEStreamParser::MaskIDParser);

  pathOptions->path = new pag::MaskData();
  pathOptions->path->id = maskID;
  if (pathOptions->path->id <= 0) {
    delete pathOptions;
    return nullptr;
  }
  pathOptions->reversedPath =
      GetProperty(streamH, "ADBE Text Reverse Path", AEStreamParser::BooleanParser);
  pathOptions->perpendicularToPath =
      GetProperty(streamH, "ADBE Text Perpendicular To Path", AEStreamParser::BooleanParser);
  pathOptions->forceAlignment =
      GetProperty(streamH, "ADBE Text Force Align Path", AEStreamParser::BooleanParser);
  pathOptions->firstMargin =
      GetProperty(streamH, "ADBE Text First Margin", AEStreamParser::FloatParser);
  pathOptions->lastMargin =
      GetProperty(streamH, "ADBE Text Last Margin", AEStreamParser::FloatParser);
  return pathOptions;
}

void GetTextProperties(const std::shared_ptr<PAGExportSession>& session, const AEGP_LayerH& layerH,
                       pag::TextLayer* layer) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  AEGP_StreamRefH layerStreamH = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefForLayer(PluginID, layerH, &layerStreamH);
  AEGP_StreamRefH rootStream = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByMatchname(
      PluginID, layerStreamH, "ADBE Text Properties", &rootStream);
  Suites->StreamSuite4()->AEGP_DisposeStream(layerStreamH);
  if (AEHelper::IsStreamHidden(rootStream) || !AEHelper::IsStreamActive(rootStream)) {
    Suites->StreamSuite4()->AEGP_DisposeStream(rootStream);
    return;
  }

  A_long numStreams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(rootStream, &numStreams);
  for (A_long index = 0; index < numStreams; index++) {
    AEGP_StreamRefH streamH = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, rootStream, index,
                                                               &streamH);
    if (!AEHelper::IsStreamHidden(rootStream) || AEHelper::IsStreamActive(rootStream)) {
      auto type = GetTextPropertyType(streamH);
      switch (type) {
        case TextPropertyType::Document: {
          QVariantMap map = {};
          map["runJavaScript"] = session->enableRunScript;
          map["compID"] = session->compID;
          map["layerIndex"] = session->layerIndex;
          map["keyFrame"] = 0;
          if (session->configParam.exportFontFile) {
            map["outPath"] = QString(session->outputPath.data());
          }
          layer->sourceText = GetProperty(streamH, AEStreamParser::TextDocumentParser, map);
          CheckTextDirection(layer->sourceText, session->configParam.exportTagLevel);
          break;
        }
        case TextPropertyType::PathOptions: {
          layer->pathOption = GetTextPathOptions(streamH);
          break;
        }
        case TextPropertyType::Animators: {
          auto vec = GetTextAnimators(streamH);
          layer->animators.insert(layer->animators.end(), vec.begin(), vec.end());
          break;
        }
        default: {
          break;
        }
      }
    }
    Suites->StreamSuite4()->AEGP_DisposeStream(streamH);
  }

  Suites->StreamSuite4()->AEGP_DisposeStream(rootStream);
}

}  // namespace exporter
