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

#pragma once

#include <AEGP_SuiteHandler.h>
#include <AE_GeneralPlug.h>
#include <string>
#include <vector>

namespace exporter {

enum class AEResourceType { Unknown, Folder, Composition, Image };

class AEResource {
 public:
  static std::shared_ptr<AEResource> BuildResourceTree();
  static std::shared_ptr<AEResource> GetResourceByID(const std::shared_ptr<AEResource>& node,
                                                     A_long id);
  static void RemoveEmptyFolder(const std::shared_ptr<AEResource>& node);

  bool isExport = false;
  AEResourceType type = AEResourceType::Unknown;
  A_long id = -1;
  std::string name = "";
  AEGP_ItemH itemHandle = nullptr;
  AEResource* parent = nullptr;
  std::vector<std::shared_ptr<AEResource>> children = {};
};

bool HasCompositionResource();

AEResourceType GetAEItemResourceType(const AEGP_SuiteHandler& suites, const AEGP_ItemH& item);

}  // namespace exporter
