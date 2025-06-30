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

#include "StringUtils.h"
#include <codecvt>
#include <iostream>
#include <locale>

namespace exporter {

std::string AEMemoryToString(const AEGP_MemHandle& handle) {
  const auto& suites = AEHelper::GetSuites();
  char16_t* str = nullptr;
  suites->MemorySuite1()->AEGP_LockMemHandle(handle, reinterpret_cast<void**>(&str));
  std::u16string u16str(str);

  std::string u8str = U16strToU8str(u16str);

  suites->MemorySuite1()->AEGP_UnlockMemHandle(handle);
  return u8str;
}

std::string U16strToU8str(const std::u16string& str) {
  std::string u8str;
  const char16_t* p = str.c_str();
  while (*p) {
    char32_t codePoint = *p++;
    if (codePoint >= 0xD800 && codePoint <= 0xDBFF && *p) {
      char32_t lowSurrogate = *p++;
      codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
    }

    if (codePoint <= 0x7F) {
      u8str.push_back(static_cast<char>(codePoint));
    } else if (codePoint <= 0x7FF) {
      u8str.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
      u8str.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint <= 0xFFFF) {
      u8str.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
      u8str.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      u8str.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
      u8str.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
      u8str.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
      u8str.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      u8str.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
  }
  return u8str;
}

std::u16string U8strToU16str(const std::string& str) {
  std::u16string u16str;

  for (size_t i = 0; i < str.size();) {
    uint8_t c = str[i];

    if (c < 0x80) {
      u16str += static_cast<char16_t>(c);
      i += 1;
    } else if ((c & 0xE0) == 0xC0) {
      if (i + 1 >= str.size()) break;
      char16_t uc = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F);
      u16str += uc;
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {
      if (i + 2 >= str.size()) break;
      char16_t uc = ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
      u16str += uc;
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {
      i += 4;
    }
  }

  return u16str;
}

void ConvertARGBToRGBA(const uint8_t* argb, int width, int height, int srcStride, uint8* rgba,
                       int dstStride) {
  for (int y = 0; y < height; y++) {
    auto src = argb + srcStride * y;
    auto dst = rgba + dstStride * y;
    for (int x = 0; x < width; x++) {
      dst[0] = src[1];
      dst[1] = src[2];
      dst[2] = src[3];
      dst[3] = src[0];
      src += 4;
      dst += 4;
    }
  }
}

}  // namespace exporter
