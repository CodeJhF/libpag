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

#include "ImageBytes.h"
#include "export/stream/StreamProperty.h"
#include "utils/ImageData.h"
#include "utils/PAGExportSession.h"

namespace exporter {

static void GetVideoLayerRenderImageSize(const AEGP_LayerH& layerH, A_u_long& srcStride,
                                         A_long& width, A_long& height) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  AEGP_LayerRenderOptionsH renderOptions = nullptr;
  Suites->LayerRenderOptionsSuite2()->AEGP_NewFromLayer(PluginID, layerH, &renderOptions);
  AEHelper::GetLayerRenderFrameSize(renderOptions, srcStride, width, height);
  Suites->LayerRenderOptionsSuite2()->AEGP_Dispose(renderOptions);
}

static void GetVideoLayerRenderImage(uint8* rgbaBytes, A_u_long srcStride, A_u_long dstStride,
                                     A_long width, A_long height, const AEGP_LayerH& layerH) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  AEGP_LayerRenderOptionsH renderOptions = nullptr;
  Suites->LayerRenderOptionsSuite2()->AEGP_NewFromLayer(PluginID, layerH, &renderOptions);
  AEHelper::GetLayerRenderFrame(rgbaBytes, srcStride, dstStride, width, height, renderOptions);
  Suites->LayerRenderOptionsSuite2()->AEGP_Dispose(renderOptions);
}

static void GetImageLayerRenderImageSize(const AEGP_LayerH& layerH, A_u_long& srcStride,
                                         A_long& width, A_long& height) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  AEGP_ItemH itemH = AEHelper::GetLayerItemH(layerH);
  AEGP_RenderOptionsH renderOptions = nullptr;
  Suites->RenderOptionsSuite3()->AEGP_NewFromItem(PluginID, itemH, &renderOptions);
  AEHelper::GetRenderFrameSize(renderOptions, srcStride, width, height);
  Suites->RenderOptionsSuite3()->AEGP_Dispose(renderOptions);
}

static void GetImageLayerRenderImage(uint8* rgbaBytes, A_u_long srcStride, A_u_long dstStride,
                                     A_long width, A_long height, const AEGP_LayerH& layerH) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  AEGP_ItemH itemH = AEHelper::GetLayerItemH(layerH);
  AEGP_RenderOptionsH renderOptions = nullptr;
  Suites->RenderOptionsSuite3()->AEGP_NewFromItem(PluginID, itemH, &renderOptions);
  AEHelper::GetRenderFrame(rgbaBytes, srcStride, dstStride, width, height, renderOptions);
  Suites->RenderOptionsSuite3()->AEGP_Dispose(renderOptions);
}

void GetImageBytesActaully(const std::shared_ptr<PAGExportSession>& session,
                           pag::ImageBytes* imageBytes, const AEGP_LayerH& layerH, bool isVideo,
                           float factor) {
  A_long width = 0;
  A_long height = 0;
  A_u_long srcStride = 0;
  A_u_long dstStride = 0;
  uint8* rgbaBytes = nullptr;
  if (isVideo) {
    GetVideoLayerRenderImageSize(layerH, srcStride, width, height);
    dstStride = width * 4;
    rgbaBytes = new uint8[dstStride * height];
    GetVideoLayerRenderImage(rgbaBytes, srcStride, dstStride, width, height, layerH);
  } else {
    GetImageLayerRenderImageSize(layerH, srcStride, width, height);
    dstStride = width * 4;
    rgbaBytes = new uint8[dstStride * height];
    GetImageLayerRenderImage(rgbaBytes, srcStride, dstStride, width, height, layerH);
  }
  if (width < 0 || height < 0 || rgbaBytes == nullptr) {
    return;
  }

  ImageRect rect = {0, 0, width, height};
  if (session->configParam.isTagCodeEnable(pag::TagCode::ImageBytesV3)) {
    ClipTransparentEdge(rect, rgbaBytes, width, height, dstStride);
  }

  uint8_t* data = nullptr;
  uint8_t* scaledRGBA = nullptr;
  int theStride = 0;
  auto scaledWidth = static_cast<int>(ceil(rect.width * factor));
  auto scaledHeight = static_cast<int>(ceil(rect.height * factor));
  if (scaledWidth == rect.width && scaledHeight == rect.height) {
    data = rgbaBytes + rect.yPos * dstStride + rect.xPos * 4;
    theStride = dstStride;
  } else {
    theStride = scaledWidth * 4;
    scaledRGBA = new uint8_t[theStride * scaledHeight + theStride * 2];
    ScaleCoreGraphics(scaledRGBA, theStride, rgbaBytes + rect.yPos * dstStride + rect.xPos * 4,
                      dstStride, scaledWidth, scaledHeight, rect.width, rect.height);
    data = scaledRGBA;
  }

  auto* byteData = EncodeImageData(data, scaledWidth, scaledHeight, theStride,
                                   session->configParam.imageQuality);
  if (byteData != nullptr) {
    imageBytes->width = width;
    imageBytes->height = height;
    imageBytes->anchorX = -rect.xPos;
    imageBytes->anchorY = -rect.yPos;
    imageBytes->scaleFactor = factor;
    imageBytes->fileBytes = byteData;
  }

  delete[] scaledRGBA;
  delete[] rgbaBytes;
}

}  // namespace exporter
