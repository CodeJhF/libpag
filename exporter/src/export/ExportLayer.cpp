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

#include "ExportLayer.h"
#include "ExportComposition.h"
#include "Marker.h"
#include "layer/CameraOption.h"
#include "layer/Effect.h"
#include "layer/ImageBytes.h"
#include "layer/LayerStyle.h"
#include "layer/Mask.h"
#include "layer/Shape.h"
#include "layer/TextProperty.h"
#include "layer/Transform2D.h"
#include "layer/Transform3D.h"
#include "stream/StreamProperty.h"
#include "utils/AETypeTransform.h"
#include "utils/ScopedHelper.h"

namespace exporter {

ExportLayerType GetLayerType(const AEGP_LayerH& layerH) {
  AEGP_LayerFlags layerFlags = AEHelper::GetLayerFlags(layerH);
  if (layerFlags &
      (AEGP_LayerFlag_NULL_LAYER | AEGP_LayerFlag_GUIDE_LAYER | AEGP_LayerFlag_ADJUSTMENT_LAYER)) {
    return ExportLayerType::Null;
  }
  AEGP_ObjectType objectType;
  AEHelper::GetSuites()->LayerSuite6()->AEGP_GetLayerObjectType(layerH, &objectType);
  if (objectType == AEGP_ObjectType_VECTOR) {
    return ExportLayerType::Shape;
  }
  if (objectType == AEGP_ObjectType_TEXT) {
    return ExportLayerType::Text;
  }
  if (objectType == AEGP_ObjectType_CAMERA) {
    return ExportLayerType::Camera;
  }
  if (objectType == AEGP_ObjectType_AV) {
    AEGP_ItemH itemH;
    AEHelper::GetSuites()->LayerSuite6()->AEGP_GetLayerSourceItem(layerH, &itemH);
    AEGP_ItemType itemType;
    AEHelper::GetSuites()->ItemSuite6()->AEGP_GetItemType(itemH, &itemType);
    if (itemType == AEGP_ItemType_COMP) {
      return ExportLayerType::PreCompose;
    }
    if (itemType == AEGP_ItemType_FOOTAGE) {
      AEGP_ItemFlags itemFlags;
      AEHelper::GetSuites()->ItemSuite6()->AEGP_GetItemFlags(itemH, &itemFlags);
      if (itemFlags & AEGP_ItemFlag_STILL) {
        AEGP_FootageH footageH;
        AEHelper::GetSuites()->FootageSuite5()->AEGP_GetMainFootageFromItem(itemH, &footageH);
        AEGP_FootageSignature signature;
        AEHelper::GetSuites()->FootageSuite5()->AEGP_GetFootageSignature(footageH, &signature);
        if (signature == AEGP_FootageSignature_SOLID) {
          return ExportLayerType::Solid;
        }
        if (signature != AEGP_FootageSignature_MISSING && signature != AEGP_FootageSignature_NONE) {
          return ExportLayerType::Image;
        }
      }
      if (itemFlags & AEGP_ItemFlag_HAS_VIDEO) {
        return ExportLayerType::Video;
      }
      if (itemFlags & AEGP_ItemFlag_HAS_AUDIO && !(itemFlags & AEGP_ItemFlag_HAS_VIDEO)) {
        return ExportLayerType::Audio;
      }
      if (itemFlags & AEGP_ItemFlag_MISSING) {
        return ExportLayerType::Unknown;
      }
    }
    return ExportLayerType::Null;
  }
  return ExportLayerType::Unknown;
}

static bool IsLayerBeReferencedByEffect(pag::ID id, const std::vector<pag::Layer*>& layers) {
  for (auto layer : layers) {
    for (auto effect : layer->effects) {
      if (effect->type() == pag::EffectType::DisplacementMap) {
        auto displacementMap = static_cast<pag::DisplacementMapEffect*>(effect);
        if (displacementMap->displacementMapLayer &&
            id == displacementMap->displacementMapLayer->id) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool IsLayerBeReferenced(pag::ID id, const std::vector<pag::Layer*>& layers,
                                bool lastLayerHasTrackMatte) {
  if (IsLayerBeReferencedByEffect(id, layers)) {
    return true;
  }

  auto iter = std::find_if(layers.begin(), layers.end(), [id](pag::Layer* layer) -> bool {
    return layer->parent != nullptr && id == layer->parent->id;
  });
  if (iter != layers.end()) {
    return true;
  }

  return lastLayerHasTrackMatte;
}

static pag::Layer* ExportLayer(const AEGP_LayerH& layerH,
                               const std::shared_ptr<PAGExportSession>& session);

static void ModifyTransform3DForCameraLayer(pag::Layer* layer, AEGP_LayerFlags layerFlags) {
  if (layer->type() != pag::LayerType::Camera) {
    return;
  }
  auto transform3D = layer->transform3D;
  if (transform3D == nullptr) {
    return;
  }

  if (transform3D->scale->animatable()) {
    delete transform3D->scale;
    transform3D->scale = new pag::Property<pag::Point3D>();
  }
  transform3D->scale->value = pag::Point3D::Make(1, 1, 1);

  if (transform3D->opacity->animatable()) {
    delete transform3D->opacity;
    transform3D->opacity = new pag::Property<pag::Opacity>();
  }
  transform3D->opacity->value = pag::Opaque;

  if (!(layerFlags & AEGP_LayerFlag_LOOK_AT_POI)) {
    delete transform3D->anchorPoint;

    auto position = transform3D->position;
    if (!position->animatable()) {
      transform3D->anchorPoint = new pag::Property<pag::Point3D>();
      transform3D->anchorPoint->value = position->value;
    } else {
      std::vector<pag::Keyframe<pag::Point3D>*> anchorPointKeyframes;
      for (auto& srcKeyFrame :
           static_cast<pag::AnimatableProperty<pag::Point3D>*>(position)->keyframes) {
        auto dstKeyframe = new pag::Keyframe<pag::Point3D>();
        dstKeyframe->startValue = srcKeyFrame->startValue;
        dstKeyframe->endValue = srcKeyFrame->endValue;
        dstKeyframe->startTime = srcKeyFrame->startTime;
        dstKeyframe->endTime = srcKeyFrame->endTime;
        dstKeyframe->bezierOut = srcKeyFrame->bezierOut;
        dstKeyframe->bezierIn = srcKeyFrame->bezierIn;
        dstKeyframe->spatialOut = srcKeyFrame->spatialOut;
        dstKeyframe->spatialIn = srcKeyFrame->spatialIn;
        anchorPointKeyframes.push_back(dstKeyframe);
      }
      transform3D->anchorPoint = new pag::AnimatableProperty<pag::Point3D>(anchorPointKeyframes);
    }
  }
}

static void InitLayer(const std::shared_ptr<PAGExportSession>& session, const AEGP_LayerH& layerH,
                      pag::Layer* layer, ExportLayerType layerType) {
  layer->id = AEHelper::GetLayerID(layerH);
  layer->name = AEHelper::GetLayerName(layerH);
  AEGP_LayerH parentLayerH = AEHelper::GetLayerParentLayerH(layerH);
  if (parentLayerH != nullptr) {
    layer->parent = new pag::Layer();
    layer->parent->id = AEHelper::GetLayerID(parentLayerH);
  }
  layer->stretch = AEHelper::GetLayerStretch(layerH);
  layer->startTime = AEHelper::GetLayerStartTime(layerH, session->frameRate);
  layer->duration = AEHelper::GetLayerDuration(layerH, session->frameRate);
  AEGP_LayerFlags layerFlags = AEHelper::GetLayerFlags(layerH);
  layer->autoOrientation = layerFlags & AEGP_LayerFlag_AUTO_ORIENT_ROTATION;

  if ((layerFlags & AEGP_LayerFlag_LAYER_IS_3D) &&
      session->configParam.isTagCodeEnable(pag::TagCode::Transform3D)) {
    layer->transform3D = GetTransform3D(layerH, session->frameRate);
  } else {
    layer->transform = GetTransform2D(layerH, session->frameRate);
  }
  if (layerFlags & AEGP_LayerFlag_TIME_REMAPPING) {
    layer->timeRemap =
        GetProperty(layerH, AEGP_LayerStream_TIME_REMAP, AEStreamParser::FloatParser);
  }
  if (layerType == ExportLayerType::Camera) {
    ModifyTransform3DForCameraLayer(layer, layerFlags);
  } else {
    layer->blendMode = AEHelper::GetLayerBlendMode(layerH);
    layer->trackMatteType = AEHelper::GetLayerTrackMatteType(layerH);
    if (session->configParam.isTagCodeEnable(pag::TagCode::LayerAttributesExtra)) {
      layer->motionBlur = static_cast<bool>(layerFlags & AEGP_LayerFlag_MOTION_BLUR);
    }
    if (layer->trackMatteType != pag::TrackMatteType::None) {
      AEGP_LayerH trackMatteLayerH = AEHelper::GetLayerTrackMatteLayerH(layerH);
      if (trackMatteLayerH == nullptr) {
        layer->trackMatteType = pag::TrackMatteType::None;
      } else {
        layer->trackMatteLayer = ExportLayer(trackMatteLayerH, session);
        layer->trackMatteLayer->isActive = false;
        layer->trackMatteLayer->trackMatteType = pag::TrackMatteType::None;
      }
    }
    layer->masks = GetMasks(layerH);
    layer->effects = GetEffects(layerH);
    layer->layerStyles = GetLayerStyles(layerH);
    GetAttachments(layerH, session->frameRate, layer, session->configParam.exportTagLevel);
  }

  if (layerType == ExportLayerType::Video) {
    auto marker = new pag::Marker();
    marker->comment = "{\"videoTrack\":1}";
    marker->startTime = layer->startTime;
    marker->duration = layer->duration;
    layer->markers.emplace_back(marker);
  }

  if (layerType == ExportLayerType::Audio || layerType == ExportLayerType::Unknown) {
    layer->isActive = false;
  } else {
    layer->isActive = layerFlags & AEGP_LayerFlag_VIDEO_ACTIVE;
  }

  if (session->configParam.isTagCodeEnable(pag::TagCode::MarkerList)) {
    auto markers = Marker::ExportMarkers(session, layerH);
    layer->markers.insert(layer->markers.end(), markers.begin(), markers.end());
    Marker::ParseMarkers(layer);
  }
}

static pag::SolidLayer* CreateSolidLayer(const AEGP_LayerH& layerH) {
  const auto& Suites = AEHelper::GetSuites();
  auto layer = new pag::SolidLayer();

  AEGP_ItemH itemH = AEHelper::GetLayerItemH(layerH);
  AEGP_ColorVal solidColor = {};
  Suites->FootageSuite5()->AEGP_GetSolidFootageColor(itemH, FALSE, &solidColor);
  Suites->ItemSuite6()->AEGP_GetItemDimensions(itemH, &layer->width, &layer->height);
  layer->solidColor = AEHelper::AEColorToColor(solidColor);
  return layer;
}

static pag::TextLayer* CreateTextLayer(const AEGP_LayerH& layerH,
                                       const std::shared_ptr<PAGExportSession>& session) {
  auto layer = new pag::TextLayer();
  GetTextProperties(session, layerH, layer);
  return layer;
}

static pag::ShapeLayer* CreateShapeLayer(const AEGP_LayerH& layerH) {
  auto layer = new pag::ShapeLayer();
  layer->contents = GetShapes(layerH);
  return layer;
}

static pag::ImageLayer* CreateImageLayer(const AEGP_LayerH& layerH,
                                         const std::shared_ptr<PAGExportSession>& session) {
  auto layer = new pag::ImageLayer();
  AEGP_ItemH itemH = AEHelper::GetLayerItemH(layerH);
  pag::ID imageID = AEHelper::GetItemID(itemH);
  auto res = std::find_if(
      session->imageBytesList.begin(), session->imageBytesList.end(),
      [imageID](const pag::ImageBytes* image) -> bool { return image->id == imageID; });
  if (res == session->imageBytesList.end()) {
    auto imageBytes = new pag::ImageBytes();
    imageBytes->id = imageID;
    layer->imageBytes = imageBytes;
    session->imageBytesList.emplace_back(imageBytes);
    session->imageLayerHList.emplace_back(false, layerH);
  } else {
    layer->imageBytes = *res;
  }

  return layer;
}

static pag::ImageLayer* CreateVideoLayer(const AEGP_LayerH& layerH,
                                         const std::shared_ptr<PAGExportSession>& session) {
  auto layer = new pag::ImageLayer();
  AEGP_ItemH itemH = AEHelper::GetLayerItemH(layerH);
  pag::ID imageID = AEHelper::GetItemID(itemH);
  auto res = std::find_if(
      session->imageBytesList.begin(), session->imageBytesList.end(),
      [imageID](const pag::ImageBytes* image) -> bool { return image->id == imageID; });
  if (res == session->imageBytesList.end()) {
    auto imageBytes = new pag::ImageBytes();
    imageBytes->id = imageID;
    layer->imageBytes = imageBytes;
    session->imageBytesList.emplace_back(imageBytes);
    session->imageLayerHList.emplace_back(true, layerH);
  } else {
    layer->imageBytes = *res;
  }

  return layer;
}

static pag::PreComposeLayer* CreatePreComposeLayer(
    const AEGP_LayerH& layerH, const std::shared_ptr<PAGExportSession>& session) {
  const auto& Suites = AEHelper::GetSuites();
  auto layer = new pag::PreComposeLayer();

  AEGP_ItemH itemH = AEHelper::GetLayerItemH(layerH);
  layer->composition = new pag::Composition();
  layer->composition->id = AEHelper::GetItemID(itemH);

  A_Time layerOffset = {};
  Suites->LayerSuite6()->AEGP_GetLayerOffset(layerH, &layerOffset);
  layer->compositionStartTime = AEHelper::AETimeToTime(layerOffset, session->frameRate);

  for (const auto& composition : session->compositions) {
    if (composition->id == layer->composition->id) {
      return layer;
    }
  }

  ExportComposition(session, itemH);

  return layer;
}

static pag::CameraLayer* CreateCameraLayer(const AEGP_LayerH& layerH) {
  auto layer = new pag::CameraLayer();
  layer->cameraOption = GetCameraOption(layerH);
  return layer;
}

static pag::Layer* ExportLayer(const AEGP_LayerH& layerH,
                               const std::shared_ptr<PAGExportSession>& session) {
  ExportLayerType layerType = GetLayerType(layerH);
  pag::Layer* layer = nullptr;
  ScopedAssign<pag::ID> layerID(session->layerID, AEHelper::GetLayerID(layerH));
  switch (layerType) {
    case ExportLayerType::Solid:
      layer = CreateSolidLayer(layerH);
      break;
    case ExportLayerType::Text:
      layer = CreateTextLayer(layerH, session);
      break;
    case ExportLayerType::Shape:
      layer = CreateShapeLayer(layerH);
      break;
    case ExportLayerType::Image:
      layer = CreateImageLayer(layerH, session);
      break;
    case ExportLayerType::Video:
      layer = CreateVideoLayer(layerH, session);
      break;
    case ExportLayerType::PreCompose:
      layer = CreatePreComposeLayer(layerH, session);
      break;
    case ExportLayerType::Camera:
      layer = CreateCameraLayer(layerH);
      break;
    case ExportLayerType::Null:
      layer = new pag::NullLayer();
      break;
    default:
      layer = new pag::Layer();
      break;
  }
  InitLayer(session, layerH, layer, layerType);

  return layer;
}

std::vector<pag::Layer*> ExportLayers(const std::shared_ptr<PAGExportSession>& session,
                                      const AEGP_CompH& compH) {
  bool hasSoloLayer = false;
  std::vector<bool> soloFlags = {};
  std::vector<pag::Layer*> layers = {};

  A_long numLayers = 0;
  if (AEHelper::GetSuites()->LayerSuite6()->AEGP_GetCompNumLayers(compH, &numLayers) !=
      A_Err_NONE) {
    session->pushWarning(AlertInfoType::ExportAEError);
    return layers;
  }

  for (int index = 0; index < numLayers; index++) {
    AEGP_LayerH layerH = nullptr;
    if (AEHelper::GetSuites()->LayerSuite6()->AEGP_GetCompLayerByIndex(compH, index, &layerH) !=
        A_Err_NONE) {
      session->pushWarning(AlertInfoType::ExportAEError);
      continue;
    }

    uint32_t layerID = AEHelper::GetLayerID(layerH);
    session->layerHMap[layerID] = layerH;
    ExportLayerType layerType = GetLayerType(layerH);
    if (layerType == ExportLayerType::Audio && session->audioMarkers != nullptr &&
        session->configParam.isTagCodeEnable(pag::TagCode::MarkerList)) {
      auto markers = Marker::ExportMarkers(session, layerH);
      session->audioMarkers->insert(session->audioMarkers->end(), markers.begin(), markers.end());
    }
    ScopedAssign<int> layerIndex(session->layerIndex, index);
    auto layer = ExportLayer(layerH, session);
    if (layer->trackMatteLayer != nullptr) {
      soloFlags.push_back(false);
      layers.push_back(layer->trackMatteLayer);
    }
    layers.push_back(layer);

    AEGP_LayerFlags layerFlags = AEHelper::GetLayerFlags(layerH);
    if (layerType == ExportLayerType::Audio || layerType == ExportLayerType::Unknown) {
      layer->isActive = false;
    } else {
      layer->isActive = layerFlags & AEGP_LayerFlag_VIDEO_ACTIVE;
    }
    bool soloFlag = layer->isActive && (layerFlags & AEGP_LayerFlag_SOLO) &&
                    (layer->type() != pag::LayerType::Camera);
    if (soloFlag) {
      hasSoloLayer = true;
    }
    soloFlags.push_back(soloFlag);
  }

  {
    std::unordered_set<pag::ID> sets = {};
    auto layerIter = layers.begin();
    auto soloFlagIter = soloFlags.begin();
    while (layerIter != layers.end() && soloFlagIter != soloFlags.end()) {
      pag::Layer* layer = *layerIter;
      if (sets.find(layer->id) != sets.end()) {
        layerIter = layers.erase(layerIter);
        soloFlagIter = soloFlags.erase(soloFlagIter);
      } else {
        sets.insert(layer->id);
        ++layerIter;
        ++soloFlagIter;
      }
    }
  }

  if (session->stopExport) {
    return layers;
  }

  pag::Codec::InstallReferences(layers);
  AEGP_ItemH itemH = AEHelper::GetCompItemH(compH);
  A_Time duration = {};
  AEHelper::GetSuites()->ItemSuite6()->AEGP_GetItemDuration(itemH, &duration);
  pag::Frame totalDuration = AEHelper::AETimeToTime(duration, session->frameRate);

  bool nextLayerHasTrackMatte = false;
  numLayers = static_cast<A_long>(layers.size());
  for (int index = numLayers - 1; index >= 0; index--) {
    pag::Layer* layer = layers[index];
    if (layer->startTime >= totalDuration) {
      layer->isActive = false;
    }
    if (hasSoloLayer && !soloFlags[index]) {
      layer->isActive = false;
    }

    bool isErrorType =
        layer->type() == pag::LayerType::Null || layer->type() == pag::LayerType::Unknown;
    bool isBeReferenced = IsLayerBeReferenced(layer->id, layers, nextLayerHasTrackMatte);
    if (isErrorType && (isBeReferenced || layer->isActive)) {
      ScopedAssign<pag::ID> layerID(session->layerID, layer->id);
      AEGP_LayerH layerH = session->layerHMap[layer->id];
      auto layerFlags = AEHelper::GetLayerFlags(layerH);
      if (layerFlags & AEGP_LayerFlag_ADJUSTMENT_LAYER) {
        session->pushWarning(AlertInfoType::AdjustmentLayer);
      }
      layer->isActive = false;
    }

    if (layer->duration == 0) {
      layer->duration = 1;
      layer->isActive = false;
    }

    auto opacity2D = layer->transform == nullptr ? nullptr : layer->transform->opacity;
    auto opacity3D = layer->transform3D == nullptr ? nullptr : layer->transform3D->opacity;
    if ((opacity2D == nullptr || (!opacity2D->animatable() && opacity2D->value == 0)) &&
        (opacity3D == nullptr || (!opacity3D->animatable() && opacity3D->value == 0))) {
      layer->isActive = false;
    }

    if (!layer->isActive && !isBeReferenced) {
      session->layerHMap.erase(layer->id);
      layers.erase(layers.begin() + index);
      delete layer;
    } else {
      nextLayerHasTrackMatte = layer->trackMatteType != pag::TrackMatteType::None;
    }
  }

  return layers;
}

}  // namespace exporter
