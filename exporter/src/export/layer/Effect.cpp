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

#include "Effect.h"
#include <unordered_map>
#include "export/PAGExport.h"
#include "export/stream/StreamProperty.h"
#include "utils/AEHelper.h"
#include "utils/PAGExportSession.h"
#include "utils/PAGExportSessionManager.h"

namespace exporter {
enum class AEEffectType {
  Unknown,
  ImageFillRule,
  ImageFillRuleV2,
  TextBackground,
  MotionTile,
  LevelsIndividual,
  CornerPin,
  Bulge,
  FastBlur,
  GaussBlur,
  Glow,
  DisplacementMap,
  RadialBlur,
  Mosaic,
  BrightnessContrast,
  HueSaturation,
};

static const std::unordered_map<std::string, AEEffectType> effectTypeMap = {
    {"ADBE Image Fill Rule", AEEffectType::ImageFillRule},
    {"ADBE Image Fill Rule2", AEEffectType::ImageFillRuleV2},
    {"ADBE Text Background", AEEffectType::TextBackground},
    {"ADBE Tile", AEEffectType::MotionTile},
    {"ADBE Pro Levels2", AEEffectType::LevelsIndividual},
    {"ADBE Corner Pin", AEEffectType::CornerPin},
    {"ADBE Bulge", AEEffectType::Bulge},
    {"ADBE Fast Blur", AEEffectType::FastBlur},
    {"ADBE Gaussian Blur 2", AEEffectType::GaussBlur},
    {"ADBE Glo2", AEEffectType::Glow},
    {"ADBE Displacement Map", AEEffectType::DisplacementMap},
    {"ADBE Radial Blur", AEEffectType::RadialBlur},
    {"ADBE Mosaic", AEEffectType::Mosaic},
    {"ADBE Brightness & Contrast 2", AEEffectType::BrightnessContrast},
    {"ADBE HUE SATURATION", AEEffectType::HueSaturation},
};

static pag::Effect* GetHueSaturationEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::HueSaturationEffect();

  auto channelControl =
      GetValue(streamH, "ADBE HUE SATURATION-0002", AEStreamParser::ChannelControlTypeParser);
  auto hue = GetValue(streamH, "ADBE HUE SATURATION-0004", AEStreamParser::FloatParser);
  auto saturation = GetValue(streamH, "ADBE HUE SATURATION-0005", AEStreamParser::FloatParser);
  auto lightness = GetValue(streamH, "ADBE HUE SATURATION-0006", AEStreamParser::FloatParser);
  effect->channelControl = channelControl;
  effect->hue[static_cast<int>(effect->channelControl)] = hue;
  effect->saturation[static_cast<int>(effect->channelControl)] = saturation;
  effect->lightness[static_cast<int>(effect->channelControl)] = lightness;
  effect->colorize = GetValue(streamH, "ADBE HUE SATURATION-0007", AEStreamParser::BooleanParser);
  effect->colorizeHue =
      GetProperty(streamH, "ADBE HUE SATURATION-0008", AEStreamParser::FloatParser);
  effect->colorizeSaturation =
      GetProperty(streamH, "ADBE HUE SATURATION-0009", AEStreamParser::FloatParser);
  effect->colorizeLightness =
      GetProperty(streamH, "ADBE HUE SATURATION-0010", AEStreamParser::FloatParser);

  return effect;
}

static pag::Effect* GetBrightnessContrastEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::BrightnessContrastEffect();

  effect->brightness =
      GetProperty(streamH, "ADBE Brightness & Contrast 2-0001", AEStreamParser::FloatParser);
  effect->contrast =
      GetProperty(streamH, "ADBE Brightness & Contrast 2-0002", AEStreamParser::FloatParser);
  effect->useOldVersion =
      GetProperty(streamH, "ADBE Brightness & Contrast 2-0003", AEStreamParser::BooleanParser);
  return effect;
}

static pag::Effect* GetMosaicEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::MosaicEffect();

  effect->horizontalBlocks = GetProperty(streamH, "ADBE Mosaic-0001", AEStreamParser::Uint16Parser);
  effect->verticalBlocks = GetProperty(streamH, "ADBE Mosaic-0002", AEStreamParser::Uint16Parser);
  effect->sharpColors = GetProperty(streamH, "ADBE Mosaic-0003", AEStreamParser::BooleanParser);
  return effect;
}

static pag::Effect* GetRadialBlurEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::RadialBlurEffect();

  effect->amount = GetProperty(streamH, "ADBE Radial Blur-0001", AEStreamParser::FloatParser);
  effect->center = GetProperty(streamH, "ADBE Radial Blur-0002", AEStreamParser::PointParser);
  effect->mode =
      GetProperty(streamH, "ADBE Radial Blur-0003", AEStreamParser::RadialBlurModeParser);
  effect->antialias =
      GetProperty(streamH, "ADBE Radial Blur-0004", AEStreamParser::RadialBlurAntialiasParser);
  return effect;
}

static pag::Effect* GetDisplacementMapEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::DisplacementMapEffect();

  effect->displacementMapLayer = new pag::Layer();
  effect->displacementMapLayer->id =
      GetProperty(streamH, "ADBE Displacement Map-0001", AEStreamParser::LayerIDParser)->value;
  if (effect->displacementMapLayer->id <= 0) {
    delete effect->displacementMapLayer;
    effect->displacementMapLayer = nullptr;
  }
  effect->useForHorizontalDisplacement = GetProperty(streamH, "ADBE Displacement Map-0002",
                                                     AEStreamParser::DisplacementMapSourceParser);
  effect->maxHorizontalDisplacement =
      GetProperty(streamH, "ADBE Displacement Map-0003", AEStreamParser::FloatParser);
  effect->useForVerticalDisplacement = GetProperty(streamH, "ADBE Displacement Map-0004",
                                                   AEStreamParser::DisplacementMapSourceParser);
  effect->maxVerticalDisplacement =
      GetProperty(streamH, "ADBE Displacement Map-0005", AEStreamParser::FloatParser);
  effect->displacementMapBehavior = GetProperty(streamH, "ADBE Displacement Map-0006",
                                                AEStreamParser::DisplacementMapBehaviorParser);
  effect->edgeBehavior =
      GetProperty(streamH, "ADBE Displacement Map-0007", AEStreamParser::BooleanParser);
  effect->expandOutput =
      GetProperty(streamH, "ADBE Displacement Map-0008", AEStreamParser::BooleanParser);
  return effect;
}

static pag::Effect* GetGlowEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::GlowEffect();

  effect->glowThreshold = GetProperty(streamH, "ADBE Glo2-0002", AEStreamParser::PercentD255Parser);
  effect->glowRadius = GetProperty(streamH, "ADBE Glo2-0003", AEStreamParser::FloatParser);
  effect->glowIntensity = GetProperty(streamH, "ADBE Glo2-0004", AEStreamParser::FloatParser);
  return effect;
}

static pag::Effect* GetGaussBlurEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::FastBlurEffect();

  effect->blurriness =
      GetProperty(streamH, "ADBE Gaussian Blur 2-0001", AEStreamParser::FloatParser);
  effect->blurDimensions = GetProperty(streamH, "ADBE Gaussian Blur 2-0002",
                                       AEStreamParser::BlurDimensionsDirectionParser);
  effect->repeatEdgePixels =
      GetProperty(streamH, "ADBE Gaussian Blur 2-0003", AEStreamParser::BooleanParser);
  return effect;
}

static pag::Effect* GetFastBlurEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::FastBlurEffect();

  effect->blurriness = GetProperty(streamH, "ADBE Fast Blur-0001", AEStreamParser::FloatParser);
  effect->blurDimensions =
      GetProperty(streamH, "ADBE Fast Blur-0002", AEStreamParser::BlurDimensionsDirectionParser);
  effect->repeatEdgePixels =
      GetProperty(streamH, "ADBE Fast Blur-0003", AEStreamParser::BooleanParser);

  return effect;
}

static pag::Effect* GetBulgeEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::BulgeEffect();

  effect->horizontalRadius = GetProperty(streamH, "ADBE Bulge-0001", AEStreamParser::FloatParser);
  effect->verticalRadius = GetProperty(streamH, "ADBE Bulge-0002", AEStreamParser::FloatParser);
  effect->bulgeCenter = GetProperty(streamH, "ADBE Bulge-0003", AEStreamParser::PointParser);
  effect->bulgeHeight = GetProperty(streamH, "ADBE Bulge-0004", AEStreamParser::FloatParser);
  effect->taperRadius = GetProperty(streamH, "ADBE Bulge-0005", AEStreamParser::FloatParser);
  effect->pinning = GetProperty(streamH, "ADBE Bulge-0007", AEStreamParser::BooleanParser);
  return effect;
}

static pag::Effect* GetCornerPinEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::CornerPinEffect();

  effect->upperLeft = GetProperty(streamH, "ADBE Corner Pin-0001", AEStreamParser::PointParser);
  effect->upperRight = GetProperty(streamH, "ADBE Corner Pin-0002", AEStreamParser::PointParser);
  effect->lowerLeft = GetProperty(streamH, "ADBE Corner Pin-0003", AEStreamParser::PointParser);
  effect->lowerRight = GetProperty(streamH, "ADBE Corner Pin-0004", AEStreamParser::PointParser);
  return effect;
}

static pag::Effect* GetLevelsIndividualEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::LevelsIndividualEffect();

  effect->inputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0004", AEStreamParser::FloatM255Parser);
  effect->inputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0005", AEStreamParser::FloatM255Parser);
  effect->gamma = GetProperty(streamH, "ADBE Pro Levels2-0006", AEStreamParser::FloatParser);
  effect->outputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0007", AEStreamParser::FloatM255Parser);
  effect->outputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0008", AEStreamParser::FloatM255Parser);

  effect->redInputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0011", AEStreamParser::FloatM255Parser);
  effect->redInputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0012", AEStreamParser::FloatM255Parser);
  effect->redGamma = GetProperty(streamH, "ADBE Pro Levels2-0013", AEStreamParser::FloatParser);
  effect->redOutputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0014", AEStreamParser::FloatM255Parser);
  effect->redOutputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0015", AEStreamParser::FloatM255Parser);

  effect->greenInputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0018", AEStreamParser::FloatM255Parser);
  effect->greenInputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0019", AEStreamParser::FloatM255Parser);
  effect->greenGamma = GetProperty(streamH, "ADBE Pro Levels2-0020", AEStreamParser::FloatParser);
  effect->greenOutputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0021", AEStreamParser::FloatM255Parser);
  effect->greenOutputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0022", AEStreamParser::FloatM255Parser);

  effect->blueInputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0025", AEStreamParser::FloatM255Parser);
  effect->blueInputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0026", AEStreamParser::FloatM255Parser);
  effect->blueGamma = GetProperty(streamH, "ADBE Pro Levels2-0027", AEStreamParser::FloatParser);
  effect->blueOutputBlack =
      GetProperty(streamH, "ADBE Pro Levels2-0028", AEStreamParser::FloatM255Parser);
  effect->blueOutputWhite =
      GetProperty(streamH, "ADBE Pro Levels2-0029", AEStreamParser::FloatM255Parser);

  return effect;
}

static pag::Effect* GetMotionTileEffect(const AEGP_StreamRefH& streamH) {
  auto effect = new pag::MotionTileEffect();
  effect->tileCenter = GetProperty(streamH, "ADBE Tile-0001", AEStreamParser::PointParser);
  effect->tileWidth = GetProperty(streamH, "ADBE Tile-0002", AEStreamParser::FloatParser);
  effect->tileHeight = GetProperty(streamH, "ADBE Tile-0003", AEStreamParser::FloatParser);
  effect->outputWidth = GetProperty(streamH, "ADBE Tile-0004", AEStreamParser::FloatParser);
  effect->outputHeight = GetProperty(streamH, "ADBE Tile-0005", AEStreamParser::FloatParser);
  effect->mirrorEdges = GetProperty(streamH, "ADBE Tile-0006", AEStreamParser::BooleanParser);
  effect->phase = GetProperty(streamH, "ADBE Tile-0007", AEStreamParser::FloatParser);
  effect->horizontalPhaseShift =
      GetProperty(streamH, "ADBE Tile-0008", AEStreamParser::BooleanParser);
  return effect;
}

static void InitEffect(const AEGP_StreamRefH& streamH, pag::Effect* effect) {
  if (effect == nullptr) {
    return;
  }

  const auto Suites = AEHelper::GetSuites();
  const auto PluginID = AEHelper::GetPluginID();

  A_long numParams = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(streamH, &numParams);
  AEGP_StreamRefH builtInParams = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, streamH, numParams - 1,
                                                             &builtInParams);
  static char matchName[100];
  Suites->DynamicStreamSuite4()->AEGP_GetMatchName(builtInParams, matchName);
  if (strcmp(matchName, "ADBE Effect Built In Params") != 0) {
    Suites->StreamSuite4()->AEGP_DisposeStream(builtInParams);
    return;
  }
  AEGP_StreamRefH masks = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, builtInParams, 0, &masks);
  A_long numMasks = 0;
  Suites->DynamicStreamSuite4()->AEGP_GetNumStreamsInGroup(masks, &numMasks);
  for (int i = 0; i < numMasks; i++) {
    AEGP_StreamRefH maskReference = nullptr;
    Suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefByIndex(PluginID, masks, i, &maskReference);
    auto maskID = GetValue(maskReference, AEStreamParser::MaskIDParser);
    auto mask = new pag::MaskData();
    mask->id = maskID;
    Suites->StreamSuite4()->AEGP_DisposeStream(maskReference);
    effect->maskReferences.push_back(mask);
  }
  Suites->StreamSuite4()->AEGP_DisposeStream(masks);
  effect->effectOpacity = GetProperty(builtInParams, 1, AEStreamParser::Opacity0_100Parser);
  Suites->StreamSuite4()->AEGP_DisposeStream(builtInParams);
  if (!effect->effectOpacity->animatable() && effect->effectOpacity->value == pag::Opaque) {
    delete effect->effectOpacity;
    effect->effectOpacity = nullptr;
  }
}

static pag::Effect* GetEffectByType(const AEGP_StreamRefH& streamH, AEEffectType type) {
  pag::Effect* effect = nullptr;
  switch (type) {
    case AEEffectType::MotionTile:
      effect = GetMotionTileEffect(streamH);
      break;
    case AEEffectType::LevelsIndividual:
      effect = GetLevelsIndividualEffect(streamH);
      break;
    case AEEffectType::CornerPin:
      effect = GetCornerPinEffect(streamH);
      break;
    case AEEffectType::Bulge:
      effect = GetBulgeEffect(streamH);
      break;
    case AEEffectType::FastBlur:
      effect = GetFastBlurEffect(streamH);
      break;
    case AEEffectType::GaussBlur:
      effect = GetGaussBlurEffect(streamH);
      break;
    case AEEffectType::Glow:
      effect = GetGlowEffect(streamH);
      break;
    case AEEffectType::DisplacementMap:
      effect = GetDisplacementMapEffect(streamH);
      break;
    case AEEffectType::RadialBlur:
      effect = GetRadialBlurEffect(streamH);
      break;
    case AEEffectType::Mosaic:
      effect = GetMosaicEffect(streamH);
      break;
    case AEEffectType::BrightnessContrast:
      effect = GetBrightnessContrastEffect(streamH);
      break;
    case AEEffectType::HueSaturation:
      effect = GetHueSaturationEffect(streamH);
      break;
    case AEEffectType::Unknown:
      PAGExportSessionManager::GetInstance()->recordWarning(AlertInfoType::UnsupportedEffects,
                                                            AEHelper::GetStreamMatchName(streamH));
      break;
    default:
      break;
  }
  InitEffect(streamH, effect);
  return effect;
}

static AEEffectType GetEffectType(const AEGP_StreamRefH& streamH) {
  std::string matchName = AEHelper::GetStreamMatchName(streamH);
  auto result = effectTypeMap.find(matchName);
  if (result == effectTypeMap.end()) {
    return AEEffectType::Unknown;
  }
  return result->second;
}

static pag::Effect* GetEffect(const AEGP_EffectRefH& effectH) {
  const auto PluginID = AEHelper::GetPluginID();
  const auto Suites = AEHelper::GetSuites();

  A_long numParams = 0;
  ;
  Suites->StreamSuite4()->AEGP_GetEffectNumParamStreams(effectH, &numParams);
  if (numParams < 2) {
    return nullptr;
  }
  AEGP_EffectFlags effectFlags;
  Suites->EffectSuite4()->AEGP_GetEffectFlags(effectH, &effectFlags);
  if ((effectFlags & AEGP_EffectFlags_ACTIVE) == 0) {
    return nullptr;
  }
  AEGP_StreamRefH firstStreamH = nullptr;
  Suites->StreamSuite4()->AEGP_GetNewEffectStreamByIndex(PluginID, effectH, 1, &firstStreamH);
  AEGP_StreamRefH effectStreamH = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewParentStreamRef(PluginID, firstStreamH, &effectStreamH);
  Suites->StreamSuite4()->AEGP_DisposeStream(firstStreamH);
  auto type = GetEffectType(effectStreamH);
  pag::Effect* effect = GetEffectByType(effectStreamH, type);
  Suites->StreamSuite4()->AEGP_DisposeStream(effectStreamH);
  return effect;
}

std::vector<pag::Effect*> GetEffects(const AEGP_LayerH& layerH) {
  const auto PluginID = AEHelper::GetPluginID();
  const auto Suites = AEHelper::GetSuites();
  std::vector<pag::Effect*> effects = {};

  int numEffects = AEHelper::GetLayerEffectNum(layerH);
  for (int i = 0; i < numEffects; i++) {
    AEGP_EffectRefH effectH = nullptr;
    Suites->EffectSuite4()->AEGP_GetLayerEffectByIndex(PluginID, layerH, i, &effectH);
    auto effect = GetEffect(effectH);
    if (effect != nullptr) {
      effects.push_back(effect);
    }
    Suites->EffectSuite4()->AEGP_DisposeEffect(effectH);
  }
  return effects;
}

void GetTextBackground(const AEGP_StreamRefH& streamH,
                       pag::Property<pag::TextDocumentHandle>* textDocument) {
  auto backgroundColor =
      GetValue(streamH, "ADBE Text Background-0001", AEStreamParser::ColorParser);
  auto backgroundAlpha =
      GetValue(streamH, "ADBE Text Background-0002", AEStreamParser::Opacity0_100Parser);
  if (textDocument->animatable()) {
    for (auto& keyframe :
         reinterpret_cast<pag::AnimatableProperty<pag::TextDocumentHandle>*>(textDocument)
             ->keyframes) {
      keyframe->startValue->backgroundColor = backgroundColor;
      keyframe->startValue->backgroundAlpha = backgroundAlpha;
      keyframe->endValue->backgroundColor = backgroundColor;
      keyframe->endValue->backgroundAlpha = backgroundAlpha;
    }
  } else {
    auto document = textDocument->getValueAt(0);
    document->backgroundColor = backgroundColor;
    document->backgroundAlpha = backgroundAlpha;
  }
}

pag::ImageFillRule* GetImageFillRuleV2(const AEGP_StreamRefH& streamH, float frameRate,
                                       uint16_t tagLevel) {
  if (tagLevel < static_cast<uint16_t>(pag::TagCode::ImageFillRule)) {
    PAGExportSessionManager::GetInstance()->recordWarning(AlertInfoType::TagLevelImageFillRule);
    return nullptr;
  }

  auto imageFillRule = new pag::ImageFillRule();
  imageFillRule->scaleMode =
      GetValue(streamH, "ADBE Image Fill Rule2-0001", AEStreamParser::ScaleModeParser);
  QVariantMap map = {};
  map["frameRate"] = frameRate;
  imageFillRule->timeRemap =
      GetProperty(streamH, "ADBE Image Fill Rule2-0002", AEStreamParser::TimeParser, map);
  if (tagLevel < static_cast<uint16_t>(pag::TagCode::ImageFillRuleV2)) {
    if (imageFillRule->timeRemap->animatable()) {
      auto timeRemap = imageFillRule->timeRemap;
      for (auto& keyFrame :
           static_cast<pag::AnimatableProperty<pag::Frame>*>(timeRemap)->keyframes) {
        keyFrame->interpolationType = pag::KeyframeInterpolationType::Linear;
      }
    }
    PAGExportSessionManager::GetInstance()->recordWarning(AlertInfoType::TagLevelImageFillRuleV2);
  }

  return imageFillRule;
}

pag::ImageFillRule* GetImageFillRule(const AEGP_StreamRefH& streamH, float frameRate,
                                     uint16_t tagLevel) {
  if (tagLevel < static_cast<uint16_t>(pag::TagCode::ImageFillRule)) {
    PAGExportSessionManager::GetInstance()->recordWarning(AlertInfoType::TagLevelImageFillRule);
    return nullptr;
  }

  auto imageFillRule = new pag::ImageFillRule();
  imageFillRule->scaleMode =
      GetValue(streamH, "ADBE Image Fill Rule1-0001", AEStreamParser::ScaleModeParser);
  QVariantMap map = {};
  map["frameRate"] = frameRate;
  imageFillRule->timeRemap =
      GetProperty(streamH, "ADBE Image Fill Rule1-0002", AEStreamParser::TimeParser, map);
  if (imageFillRule->timeRemap->animatable()) {
    auto timeRemap = imageFillRule->timeRemap;
    for (auto& keyFrame : static_cast<pag::AnimatableProperty<pag::Frame>*>(timeRemap)->keyframes) {
      keyFrame->interpolationType = pag::KeyframeInterpolationType::Linear;
    }
  }

  return imageFillRule;
}

void GetAttachment(const AEGP_EffectRefH& effectH, float frameRate, pag::Layer* layer,
                   uint16_t tagLevel) {
  const auto PluginID = AEHelper::GetPluginID();
  const auto Suites = AEHelper::GetSuites();

  A_long numParams = 0;
  Suites->StreamSuite4()->AEGP_GetEffectNumParamStreams(effectH, &numParams);
  if (numParams < 2) {
    return;
  }

  AEGP_EffectFlags effectFlags;
  Suites->EffectSuite4()->AEGP_GetEffectFlags(effectH, &effectFlags);
  if ((effectFlags & AEGP_EffectFlags_ACTIVE) == 0) {
    return;
  }

  AEGP_StreamRefH firstStreamH = nullptr;
  Suites->StreamSuite4()->AEGP_GetNewEffectStreamByIndex(PluginID, effectH, 1, &firstStreamH);
  AEGP_StreamRefH effectStreamH = nullptr;
  Suites->DynamicStreamSuite4()->AEGP_GetNewParentStreamRef(PluginID, firstStreamH, &effectStreamH);
  Suites->StreamSuite4()->AEGP_DisposeStream(firstStreamH);
  auto type = GetEffectType(effectStreamH);
  switch (type) {
    case AEEffectType::TextBackground:
      if (layer->type() == pag::LayerType::Text) {
        GetTextBackground(effectStreamH, static_cast<pag::TextLayer*>(layer)->sourceText);
      } else {
        PAGExportSessionManager::GetInstance()->recordWarning(
            AlertInfoType::TextBackgroundOnlyTextLayer);
      }
      break;
    case AEEffectType::ImageFillRule:
    case AEEffectType::ImageFillRuleV2:
      if (layer->type() == pag::LayerType::Image) {
        if (static_cast<pag::ImageLayer*>(layer)->imageFillRule == nullptr) {
          auto func = type == AEEffectType::ImageFillRule ? GetImageFillRule : GetImageFillRuleV2;
          static_cast<pag::ImageLayer*>(layer)->imageFillRule =
              func(effectStreamH, frameRate, tagLevel);
        } else {
          PAGExportSessionManager::GetInstance()->recordWarning(
              AlertInfoType::ImageFillRuleOnlyOne);
        }
      } else {
        PAGExportSessionManager::GetInstance()->recordWarning(
            AlertInfoType::ImageFillRuleOnlyImageLayer);
      }
      break;
    default:
      break;
  }
  Suites->StreamSuite4()->AEGP_DisposeStream(effectStreamH);
}

void GetAttachments(const AEGP_LayerH& layerH, float frameRate, pag::Layer* layer,
                    uint16_t tagLevel) {
  const auto PluginID = AEHelper::GetPluginID();
  const auto Suites = AEHelper::GetSuites();

  int numEffects = AEHelper::GetLayerEffectNum(layerH);
  for (int i = 0; i < numEffects; i++) {
    AEGP_EffectRefH effectH = nullptr;
    Suites->EffectSuite4()->AEGP_GetLayerEffectByIndex(PluginID, layerH, i, &effectH);
    GetAttachment(effectH, frameRate, layer, tagLevel);
    Suites->EffectSuite4()->AEGP_DisposeEffect(effectH);
  }
}

}  // namespace exporter
