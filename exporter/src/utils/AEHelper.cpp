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

#include "AEHelper.h"
#include <iostream>
#include "StringHelper.h"
#include "platform/PlatformHelper.h"
#include "src/base/utils/Log.h"
namespace AEHelper {

AEGP_PluginID PluginID = 0L;
std::shared_ptr<AEGP_SuiteHandler> Suites = nullptr;
std::string DocumentsFolderPath = "";
std::string AeVersion = "";

int32_t MAJORVERSION = 23;

void SetSuitesAndPluginID(SPBasicSuite* basicSuite, AEGP_PluginID id) {
  Suites = std::make_shared<AEGP_SuiteHandler>(basicSuite);
  PluginID = id;
}

ExportLayerType GetLayerType(const AEGP_LayerH& layerH) {
  const auto& suites = GetSuites();
  AEGP_LayerFlags layerFlags;
  suites->LayerSuite6()->AEGP_GetLayerFlags(layerH, &layerFlags);
  if (layerFlags &
      (AEGP_LayerFlag_NULL_LAYER | AEGP_LayerFlag_GUIDE_LAYER | AEGP_LayerFlag_ADJUSTMENT_LAYER)) {
    return ExportLayerType::Null;
  }
  AEGP_ObjectType objectType;
  suites->LayerSuite6()->AEGP_GetLayerObjectType(layerH, &objectType);
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
    suites->LayerSuite6()->AEGP_GetLayerSourceItem(layerH, &itemH);
    AEGP_ItemType itemType;
    suites->ItemSuite6()->AEGP_GetItemType(itemH, &itemType);
    if (itemType == AEGP_ItemType_COMP) {
      return ExportLayerType::PreCompose;
    }
    if (itemType == AEGP_ItemType_FOOTAGE) {
      AEGP_ItemFlags itemFlags;
      suites->ItemSuite6()->AEGP_GetItemFlags(itemH, &itemFlags);
      if (itemFlags & AEGP_ItemFlag_STILL) {
        AEGP_FootageH footageH;
        suites->FootageSuite5()->AEGP_GetMainFootageFromItem(itemH, &footageH);
        AEGP_FootageSignature signature;
        suites->FootageSuite5()->AEGP_GetFootageSignature(footageH, &signature);
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

AEGP_ItemH GetLayerItemH(const AEGP_LayerH& layerH) {
  const auto& suites = GetSuites();
  AEGP_ItemH itemH;
  suites->LayerSuite6()->AEGP_GetLayerSourceItem(layerH, &itemH);
  return itemH;
}

A_long GetLayerID(const AEGP_LayerH& layerH) {
  const auto& suites = GetSuites();
  A_long id;
  suites->LayerSuite6()->AEGP_GetLayerID(layerH, &id);
  return id;
}

AEGP_ItemH GetActiveCompositionItem() {
  const auto& suites = GetSuites();
  AEGP_ItemH activeItemH = nullptr;
  suites->ItemSuite6()->AEGP_GetActiveItem(&activeItemH);
  if (activeItemH == nullptr) {
    AEGP_ProjectH projectHandle;
    suites->ProjSuite6()->AEGP_GetProjectByIndex(0, &projectHandle);
    suites->ItemSuite6()->AEGP_GetFirstProjItem(projectHandle, &activeItemH);
  }
  if (activeItemH == nullptr) {
    return nullptr;
  }
  AEGP_ItemType itemType = AEGP_ItemType_NONE;
  suites->ItemSuite6()->AEGP_GetItemType(activeItemH, &itemType);
  if (itemType != AEGP_ItemType_COMP) {
    return nullptr;
  }
  return activeItemH;
}

AEGP_PluginID GetPluginID() {
  return PluginID;
}

std::shared_ptr<AEGP_SuiteHandler> GetSuites() {
  return Suites;
}

A_long GetItemID(const AEGP_ItemH& item) {
  A_long id = -1;
  Suites->ItemSuite6()->AEGP_GetItemID(item, &id);
  return id;
}

A_long GetItemParentID(const AEGP_ItemH& item) {
  A_long id = -1;
  AEGP_ItemH parentItem = nullptr;
  Suites->ItemSuite6()->AEGP_GetItemParentFolder(item, &parentItem);
  if (parentItem != nullptr) {
    id = GetItemID(parentItem);
  }
  return id;
}

AEGP_CompH GetItemCompH(const AEGP_ItemH& item) {
  AEGP_CompH compH = nullptr;
  Suites->CompSuite6()->AEGP_GetCompFromItem(item, &compH);
  return compH;
}

float GetItemFrameRate(const AEGP_ItemH& item) {
  auto compH = GetItemCompH(item);
  A_FpLong frameRate = 0;
  Suites->CompSuite6()->AEGP_GetCompFramerate(compH, &frameRate);
  return static_cast<float>(frameRate);
}

pag::Frame GetItemDuration(const AEGP_ItemH& item) {
  A_Time time = {};
  Suites->ItemSuite6()->AEGP_GetItemDuration(item, &time);
  A_FpLong frameRate = GetItemFrameRate(item);
  return static_cast<pag::Frame>(std::round(time.value * frameRate / time.scale));
}

QImage GetCompositionFrameImage(const AEGP_ItemH& itemH, pag::Frame frame) {
  AEGP_RenderOptionsH renderOptions = nullptr;
  float frameRate = GetItemFrameRate(itemH);
  A_Time getTime = {};
  A_Time currentTime = {};
  currentTime.value = static_cast<A_long>(1000 * frame);
  currentTime.scale = static_cast<A_u_long>(std::lround(1000 * frameRate));
  Suites->RenderOptionsSuite3()->AEGP_NewFromItem(PluginID, itemH, &renderOptions);
  if (renderOptions == nullptr) {
    printf("GetCompositionFrameImage: NewFromItem failed.\n");
    return {};
  }
  Suites->RenderOptionsSuite3()->AEGP_SetWorldType(renderOptions, AEGP_WorldType_8);
  Suites->RenderOptionsSuite3()->AEGP_SetTime(renderOptions, currentTime);
  Suites->RenderOptionsSuite3()->AEGP_GetTime(renderOptions, &getTime);
  if (getTime.value != currentTime.value || getTime.scale != currentTime.scale) {
    printf("GetCompositionFrameImage: GetTime failed.\n");
    return {};
  }

  uint8_t* rgbaBytes = nullptr;
  A_u_long stride = 0;
  A_long width = 0;
  A_long height = 0;
  GetRenderFrame(rgbaBytes, stride, width, height, *Suites, renderOptions);
  if (width > 0 && height > 0 && rgbaBytes != nullptr) {
    QImage image(rgbaBytes, width, height, stride, QImage::Format_RGBA8888);
    delete[] rgbaBytes;
    return image;
  }
  return {};
}

QSize GetItemDimensions(const AEGP_ItemH& itemH) {
  A_long width = 0;
  A_long height = 0;
  Suites->ItemSuite8()->AEGP_GetItemDimensions(itemH, &width, &height);
  return {width, height};
}

void GetRenderFrame(uint8*& rgbaBytes, A_u_long& stride, A_long& width, A_long& height,
                    const AEGP_SuiteHandler& suites, AEGP_RenderOptionsH& renderOptions) {
  suites.RenderOptionsSuite3()->AEGP_SetWorldType(renderOptions, AEGP_WorldType_8);
  suites.RenderOptionsSuite3()->AEGP_SetDownsampleFactor(renderOptions, 1, 1);

  AEGP_FrameReceiptH frameReceipt;
  suites.RenderSuite5()->AEGP_RenderAndCheckoutFrame(renderOptions, nullptr, nullptr,
                                                     &frameReceipt);

  AEGP_WorldH imageWorld;
  suites.RenderSuite5()->AEGP_GetReceiptWorld(frameReceipt, &imageWorld);
  suites.WorldSuite3()->AEGP_GetSize(imageWorld, &width, &height);

  AEGP_WorldType worldType;
  suites.WorldSuite3()->AEGP_GetType(imageWorld, &worldType);

  PF_Pixel* pixels;
  suites.WorldSuite3()->AEGP_GetBaseAddr8(imageWorld, &pixels);

  A_u_long rowBytes;
  suites.WorldSuite3()->AEGP_GetRowBytes(imageWorld, &rowBytes);

  if (width > 0 && height > 0 && rowBytes > 0) {
    if (rgbaBytes == nullptr) {
      stride = rowBytes;
      rgbaBytes = new uint8_t[stride * height + stride * 2];
    }
    StringHelper::ConvertARGBToRGBA(&(pixels->alpha), width, height, rowBytes, rgbaBytes, stride);
  }
  suites.RenderSuite5()->AEGP_CheckinFrame(frameReceipt);
}

void SetItemName(const AEGP_ItemH& item, const std::string& name) {
  std::u16string u16str = StringHelper::Utf8ToUtf16(name);
  Suites->ItemSuite8()->AEGP_SetItemName(item, reinterpret_cast<const A_UTF16Char*>(u16str.data()));
}

std::string RunScript(std::shared_ptr<AEGP_SuiteHandler> suites, AEGP_PluginID pluginID,
                      const std::string& scriptText) {
  AEGP_MemHandle scriptResult;
  AEGP_MemHandle errorResult;
  suites->UtilitySuite6()->AEGP_ExecuteScript(pluginID, scriptText.c_str(), FALSE, &scriptResult,
                                              &errorResult);
  A_char* result = nullptr;
  suites->MemorySuite1()->AEGP_LockMemHandle(scriptResult, reinterpret_cast<void**>(&result));
  std::string resultText = result;
  suites->MemorySuite1()->AEGP_FreeMemHandle(scriptResult);
  A_char* error = nullptr;
  suites->MemorySuite1()->AEGP_LockMemHandle(errorResult, reinterpret_cast<void**>(&error));
  std::string errorText = error;
  suites->MemorySuite1()->AEGP_FreeMemHandle(errorResult);
  return resultText;
}

void RegisterTextDocumentScript() {
  static bool hasInit = false;
  if (!hasInit) {
    const auto& suites = GetSuites();
    auto pluginID = GetPluginID();
    RunScript(suites, pluginID, TextDocumentScript);
    hasInit = true;
  }
}

std::string GetDocumentsFolderPath() {
  if (DocumentsFolderPath.empty()) {
    const auto& suites = GetSuites();
    auto pluginID = GetPluginID();
    DocumentsFolderPath = RunScript(suites, pluginID, "Folder.myDocuments.fsName;");
  }
  return DocumentsFolderPath;
}

std::string GetAeVersion() {
  if (AeVersion.empty()) {
    const auto& suites = GetSuites();
    auto pluginID = GetPluginID();
    AeVersion = RunScript(suites, pluginID, "app.version");
  }
  return AeVersion;
}

void RunScriptPreWarm() {
  static bool hasInit = false;
  if (!hasInit) {
    RegisterTextDocumentScript();
    GetDocumentsFolderPath();
    GetAeVersion();
    hasInit = true;
  }
}

bool CheckAeVersion() {
  int32_t majorVersion = 0;
  if (AeVersion.empty()) {
    return false;
  }
  try {
    size_t dotPos = AeVersion.find('.');
    if (dotPos == std::string::npos) {
      LOGE("Invalid version format!");
      return false;
    }
    std::string versionStr = AeVersion.substr(0, dotPos);
    majorVersion = std::stoi(versionStr);
  } catch (const std::invalid_argument& e) {
    LOGE("Invalid argument: %s", e.what());
    return false;
  } catch (const std::out_of_range& e) {
    LOGE("Out of range: %s", e.what());
    return false;
  }
  if (majorVersion >= MAJORVERSION) {
    return true;
  }
  return false;
}

std::string GetItemName(const AEGP_ItemH& itemH) {
  std::string itemName;
  if (itemH == nullptr) {
    return itemName;
  }
  const auto& suites = GetSuites();
  auto pluginID = GetPluginID();
  AEGP_MemHandle nameMemory = nullptr;
  suites->ItemSuite8()->AEGP_GetItemName(pluginID, itemH, &nameMemory);
  if (!nameMemory) {
    return itemName;
  }
  itemName = StringHelper::AeMemoryHandleToString(nameMemory);
  suites->MemorySuite1()->AEGP_FreeMemHandle(nameMemory);

  itemName = StringHelper::DeleteLastSpace(itemName);
  return itemName;
}

std::string GetLayerName(const AEGP_LayerH& layerH) {
  std::string layerName;
  if (layerH == nullptr) {
    return layerName;
  }
  const auto& suites = GetSuites();
  auto pluginID = GetPluginID();
  AEGP_MemHandle layerNameHandle = nullptr;
  AEGP_MemHandle sourceNameHandle = nullptr;
  suites->LayerSuite6()->AEGP_GetLayerName(pluginID, layerH, &layerNameHandle, &sourceNameHandle);
  if (!layerNameHandle || !sourceNameHandle) {
    return layerName;
  }
  layerName = StringHelper::AeMemoryHandleToString(layerNameHandle);
  if (layerName.empty()) {
    layerName = StringHelper::AeMemoryHandleToString(sourceNameHandle);
  }
  suites->MemorySuite1()->AEGP_FreeMemHandle(layerNameHandle);
  suites->MemorySuite1()->AEGP_FreeMemHandle(sourceNameHandle);

  layerName = StringHelper::DeleteLastSpace(layerName);
  return layerName;
}

AEGP_ItemH GetItemFromComp(const AEGP_CompH& compH) {
  const auto& suites = GetSuites();
  AEGP_ItemH itemH = nullptr;
  if (compH != nullptr) {
    suites->CompSuite6()->AEGP_GetItemFromComp(compH, &itemH);
  }
  return itemH;
}

std::string GetCompName(const AEGP_CompH& compH) {
  if (compH == nullptr) {
    return "";
  }
  auto itemH = GetItemFromComp(compH);
  return GetItemName(itemH);
}

void SelectItem(const AEGP_ItemH& itemH) {
  const auto& suites = GetSuites();
  if (itemH != nullptr) {
    suites->ItemSuite6()->AEGP_SelectItem(itemH, true, true);
    suites->CommandSuite1()->AEGP_DoCommand(
        3061);  // 3061: Open selection, ignoring any modifier keys.
  }
}

void SelectItem(const AEGP_ItemH& itemH, const AEGP_LayerH& layerH) {
  const auto& suites = GetSuites();
  auto pluginID = GetPluginID();
  if (itemH == nullptr) {
    return;
  }
  bool hasLayer = (layerH != nullptr);
  AEGP_CollectionItemV2 collectionItem;
  AEGP_StreamRefH streamH;
  if (hasLayer) {
    suites->DynamicStreamSuite4()->AEGP_GetNewStreamRefForLayer(pluginID, layerH, &streamH);
    collectionItem.type = AEGP_CollectionItemType_LAYER;
    collectionItem.u.layer.layerH = layerH;
    collectionItem.stream_refH = streamH;
  }

  suites->ItemSuite6()->AEGP_SelectItem(itemH, true, true);
  if (!hasLayer) {
    suites->CommandSuite1()->AEGP_DoCommand(
        3061);  // 3061: Open selection, ignoring any modifier keys.
    return;
  }

  auto compH = GetCompFromItem(itemH);
  AEGP_Collection2H collectionH = nullptr;
  suites->CollectionSuite2()->AEGP_NewCollection(pluginID, &collectionH);
  suites->CollectionSuite2()->AEGP_CollectionPushBack(collectionH, &collectionItem);
  suites->CompSuite6()->AEGP_SetSelection(compH, collectionH);
  suites->CommandSuite1()->AEGP_DoCommand(
      3061);  // 3061: Open selection, ignoring any modifier keys.
  suites->CollectionSuite2()->AEGP_DisposeCollection(collectionH);
}

AEGP_CompH GetCompFromItem(const AEGP_ItemH& itemH) {
  const auto& suites = GetSuites();
  AEGP_CompH compH = nullptr;
  if (itemH != nullptr) {
    suites->CompSuite6()->AEGP_GetCompFromItem(itemH, &compH);
  }
  return compH;
}

uint32_t GetItemId(const AEGP_ItemH& itemH) {
  const auto& suites = GetSuites();
  A_long id = 0;
  if (itemH != nullptr) {
    suites->ItemSuite6()->AEGP_GetItemID(itemH, &id);
  }
  return static_cast<uint32_t>(id);
}

uint32_t GetItemIdFromLayer(const AEGP_LayerH& layerH) {
  auto itemH = GetItemFromLayer(layerH);
  auto id = GetItemId(itemH);
  return id;
}

uint32_t GetLayerId(const AEGP_LayerH& layerH) {
  const auto& suites = GetSuites();
  A_long id = 0;
  if (layerH != nullptr) {
    suites->LayerSuite6()->AEGP_GetLayerID(layerH, &id);
  }
  return static_cast<uint32_t>(id);
}

AEGP_ItemH GetItemFromLayer(const AEGP_LayerH& layerH) {
  const auto& suites = GetSuites();
  AEGP_ItemH itemH = nullptr;
  if (layerH != nullptr) {
    suites->LayerSuite6()->AEGP_GetLayerSourceItem(layerH, &itemH);
  }
  return itemH;
}

}  // namespace AEHelper
