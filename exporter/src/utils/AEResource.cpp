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

#include "AEResource.h"
#include "AEHelper.h"

namespace exporter {

static AEResourceType GetResourceType(const std::shared_ptr<AEGP_SuiteHandler>& suites,
                                      AEGP_ItemH& item) {
  AEGP_ItemType itemType = AEGP_ItemType_NONE;
  suites->ItemSuite8()->AEGP_GetItemType(item, &itemType);
  if (itemType == AEGP_ItemType_FOLDER) {
    return AEResourceType::Folder;
  }
  if (itemType == AEGP_ItemType_COMP) {
    return AEResourceType::Composition;
  }
  if (itemType == AEGP_ItemType_FOOTAGE) {
    AEGP_ItemFlags itemFlags;
    suites->ItemSuite6()->AEGP_GetItemFlags(item, &itemFlags);
    if (itemFlags & AEGP_ItemFlag_STILL) {
      AEGP_FootageH footageHandle;
      suites->FootageSuite5()->AEGP_GetMainFootageFromItem(item, &footageHandle);
      AEGP_FootageSignature signature;
      suites->FootageSuite5()->AEGP_GetFootageSignature(footageHandle, &signature);
      if (signature != AEGP_FootageSignature_SOLID && signature != AEGP_FootageSignature_MISSING &&
          signature != AEGP_FootageSignature_NONE) {
        return AEResourceType::Image;
      }
    }
  }
  return AEResourceType::Unknown;
}

bool HasCompositionResource() {
  const auto& suites = AEHelper::GetSuites();
  A_long numProjects = 0;
  suites->ProjSuite6()->AEGP_GetNumProjects(&numProjects);
  for (A_long i = 0; i < numProjects; i++) {
    AEGP_ProjectH projectHandle;
    suites->ProjSuite6()->AEGP_GetProjectByIndex(i, &projectHandle);

    AEGP_ItemH item;
    suites->ItemSuite6()->AEGP_GetFirstProjItem(projectHandle, &item);
    while (item != nullptr) {
      auto type = GetResourceType(suites, item);
      if (type == AEResourceType::Composition) {
        return true;
      }
      AEGP_ItemH nextItem;
      suites->ItemSuite6()->AEGP_GetNextProjItem(projectHandle, item, &nextItem);
      item = nextItem;
    }
  }
  return false;
}

AEResourceType GetAEItemResourceType(const AEGP_SuiteHandler& suites, const AEGP_ItemH& item) {
  AEGP_ItemType itemType = AEGP_ItemType_NONE;
  suites.ItemSuite6()->AEGP_GetItemType(item, &itemType);
  if (itemType == AEGP_ItemType_FOLDER) {
    return AEResourceType::Folder;
  }
  if (itemType == AEGP_ItemType_COMP) {
    return AEResourceType::Composition;
  }
  if (itemType == AEGP_ItemType_FOOTAGE) {
    AEGP_ItemFlags itemFlags;
    suites.ItemSuite6()->AEGP_GetItemFlags(item, &itemFlags);
    if (itemFlags & AEGP_ItemFlag_STILL) {
      AEGP_FootageH footageHandle;
      suites.FootageSuite5()->AEGP_GetMainFootageFromItem(item, &footageHandle);
      AEGP_FootageSignature signature;
      suites.FootageSuite5()->AEGP_GetFootageSignature(footageHandle, &signature);
      if (signature != AEGP_FootageSignature_SOLID && signature != AEGP_FootageSignature_MISSING &&
          signature != AEGP_FootageSignature_NONE) {
        return AEResourceType::Image;
      }
    }
  }
  return AEResourceType::Unknown;
}

std::shared_ptr<AEResource> AEResource::BuildResourceTree() {
  const auto& suites = AEHelper::GetSuites();
  A_long projectsNum = 0;
  suites->ProjSuite6()->AEGP_GetNumProjects(&projectsNum);
  std::shared_ptr<AEResource> root = nullptr;

  for (A_long index = 0; index < projectsNum; index++) {
    AEGP_ProjectH projectHandle = nullptr;
    suites->ProjSuite6()->AEGP_GetProjectByIndex(index, &projectHandle);
    A_char projectName[AEGP_MAX_PROJ_NAME_SIZE];
    suites->ProjSuite6()->AEGP_GetProjectName(projectHandle, projectName);
    AEGP_ItemH itemHandle = nullptr;
    suites->ItemSuite6()->AEGP_GetFirstProjItem(projectHandle, &itemHandle);
    while (itemHandle != nullptr) {
      auto item = std::make_shared<AEResource>();
      auto type = GetAEItemResourceType(*suites, itemHandle);
      if (type != AEResourceType::Unknown) {
        item->type = type;
        item->id = AEHelper::GetItemID(itemHandle);
        item->name = AEHelper::GetItemName(itemHandle);
        item->itemHandle = itemHandle;
        if (item->id == 0) {
          root = item;
        } else {
          auto parentID = AEHelper::GetItemParentID(itemHandle);
          auto parentItem = AEResource::GetResourceByID(root, parentID);
          if (parentItem != nullptr) {
            parentItem->children.push_back(item);
            item->parent = parentItem.get();
          }
        }
      }

      AEGP_ItemH nextItemHandle = nullptr;
      suites->ItemSuite6()->AEGP_GetNextProjItem(projectHandle, itemHandle, &nextItemHandle);
      itemHandle = nextItemHandle;
    }
  }

  RemoveEmptyFolder(root);

  return root;
}

std::shared_ptr<AEResource> AEResource::GetResourceByID(const std::shared_ptr<AEResource>& node,
                                                        A_long id) {
  if (node->id == id) {
    return node;
  }

  for (const auto& child : node->children) {
    auto result = GetResourceByID(child, id);
    if (result != nullptr) {
      return result;
    }
  }

  return nullptr;
}

void AEResource::RemoveEmptyFolder(const std::shared_ptr<AEResource>& node) {
  auto iter = node->children.begin();
  while (iter != node->children.end()) {
    const auto& child = *iter;
    if (child->type == AEResourceType::Folder && child->children.empty()) {
      iter = node->children.erase(iter);
    } else {
      RemoveEmptyFolder(child);
      ++iter;
    }
  }
}

}  // namespace exporter
