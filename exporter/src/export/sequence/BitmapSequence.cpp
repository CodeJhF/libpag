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

#include "BitmapSequence.h"
#include <webp/encode.h>
#include "utils/AEHelper.h"
#include "utils/ImageData.h"

namespace exporter {

static bool IsKeyFrame(pag::Frame curFrame, pag::Frame lastKeyFrame, int diffSize, int fullSize,
                       int keyFrameRate) {
  pag::Frame frameDistance = curFrame - lastKeyFrame;
  if (curFrame == 0) {
    return true;
  }
  if (diffSize == 0) {
    return false;
  }
  if (diffSize == fullSize) {
    return true;
  }
  if (diffSize > (fullSize * 90 / 100) && frameDistance > 5) {
    return true;
  }
  if (diffSize > (fullSize * 75 / 100) && keyFrameRate > 20 && frameDistance > (keyFrameRate / 2)) {
    return true;
  }
  if (keyFrameRate > 0 && frameDistance > keyFrameRate) {
    return true;
  }

  return false;
}

void GetBitmapSequence(const std::shared_ptr<PAGExportSession>& session,
                       pag::BitmapComposition* composition, float compositionFactor) {
  const auto& Suites = AEHelper::GetSuites();
  const auto& PluginID = AEHelper::GetPluginID();

  AEGP_ItemH itemH = session->itemHMap[composition->id];
  float factor = compositionFactor;
  float frameRate = std::min(session->configParam.frameRate, composition->frameRate);
  auto duration =
      static_cast<pag::Frame>(ceil(composition->duration * frameRate / composition->frameRate));

  if (session->configParam.bitmapMaxResolution > 0) {
    int shorterSideLength =
        static_cast<int>(std::min(composition->width, composition->height) * compositionFactor);
    if (shorterSideLength > session->configParam.bitmapMaxResolution) {
      factor *= static_cast<float>(session->configParam.bitmapMaxResolution) /
                static_cast<float>(shorterSideLength);
    }
  }

  if (factor > 0.99) {
    factor = 1.0;
  }

  auto sequence = new pag::BitmapSequence();
  composition->sequences.push_back(sequence);
  sequence->composition = composition;
  sequence->frameRate = frameRate;
  sequence->width = static_cast<int>(ceil(composition->width * factor));
  sequence->height = static_cast<int>(ceil(composition->height * factor));

  int width = sequence->width;
  int height = sequence->height;
  int stride = width * 4;
  auto compStride = static_cast<A_u_long>(composition->width * 4);
  bool needToScale =
      (sequence->width != composition->width || sequence->height != composition->height);
  std::vector<uint8_t> preData(stride * height + stride * 2);
  std::vector<uint8_t> curData(stride * height + stride * 2);
  std::vector<uint8_t> rgbaData(compStride * composition->height + compStride * 2);
  uint8_t* curRGBABytes = needToScale ? rgbaData.data() : curData.data();

  ImageRect lastKeyFrameDiffRect = {0, 0, width, height};
  pag::Frame lastKeyFrame = 0;
  AEGP_RenderOptionsH renderOptions = nullptr;
  Suites->RenderOptionsSuite3()->AEGP_NewFromItem(PluginID, itemH, &renderOptions);
  Suites->RenderOptionsSuite3()->AEGP_SetWorldType(renderOptions, AEGP_WorldType_8);
  for (pag::Frame frame = 0; frame < duration && !session->stopExport; frame++) {
    A_long compWidth = 0;
    A_long compHeight = 0;
    A_u_long compBytesLength = 0;

    AEHelper::SetRenderTime(renderOptions, frameRate, frame);
    AEHelper::GetRenderFrameSize(renderOptions, compBytesLength, compWidth, compHeight);
    AEHelper::GetRenderFrame(curRGBABytes, compBytesLength, compStride, compWidth, compHeight,
                             renderOptions);

    auto bitmapFrame = new pag::BitmapFrame();
    sequence->frames.push_back(bitmapFrame);
    if (compWidth == composition->width && compHeight == composition->height) {
      if (needToScale) {
        ScaleCoreGraphics(curData.data(), stride, curRGBABytes, static_cast<int>(compStride), width,
                          height, compWidth, compHeight);
      }

      ImageRect diffRect = {0, 0, width, height};
      GetImageDiffRect(diffRect, curData.data(), preData.data(), width, height, stride);

      ImageRect encodeRect = {0, 0, width, height};
      bool isKeyFrame = IsKeyFrame(frame, lastKeyFrame, diffRect.width * diffRect.height,
                                   width * height, session->configParam.bitmapKeyFrameInterval);
      if (isKeyFrame) {
        lastKeyFrame = frame;
        diffRect.xPos = 0;
        diffRect.yPos = 0;
        diffRect.width = width;
        diffRect.height = height;
        ClipTransparentEdge(diffRect, curData.data(), width, height, stride);
        encodeRect = diffRect;
      } else if (diffRect.width > 0 && diffRect.height > 0) {
        ExpandRectRange(encodeRect, diffRect, lastKeyFrameDiffRect, width, height, 4);
      }

      if (diffRect.width > 0 && diffRect.height > 0) {
        uint8_t* data = curData.data() + encodeRect.yPos * stride + encodeRect.xPos * 4;
        pag::ByteData* bitmapBytes = EncodeImageData(data, encodeRect.width, encodeRect.height,
                                                     stride, session->configParam.imageQuality);
        if (bitmapBytes == nullptr) {
          session->pushWarning(AlertInfoType::WebpEncodeError);
        }

        auto bitmapRect = new pag::BitmapRect();
        bitmapRect->x = encodeRect.xPos;
        bitmapRect->y = encodeRect.yPos;
        bitmapRect->fileBytes = bitmapBytes;
        bitmapFrame->bitmaps.push_back(bitmapRect);
      }
      bitmapFrame->isKeyframe = isKeyFrame;
      lastKeyFrameDiffRect = diffRect;
    } else {
      session->pushWarning(AlertInfoType::ExportRenderError);
    }

    session->progressModel.addProgress();
    std::swap(curData, preData);
  }

  Suites->RenderOptionsSuite3()->AEGP_Dispose(renderOptions);
}

}  // namespace exporter
