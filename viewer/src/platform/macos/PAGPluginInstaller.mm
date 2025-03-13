#import "PAGPluginInstaller.h"
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <inttypes.h>
#import "utils/Translate.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#define AE_LIB_PATH "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/"
#define AE_CEP_PATH "/Library/Application Support/Adobe/CEP/extensions/"

#define VERSION_SEPARATE(version)                                                     \
  ((version >> 48) & 0xFFFF), ((version >> 32) & 0xFFFF), ((version >> 16) & 0xFFFF), \
      ((version >> 0) & 0xFFFF)

// TODO Implement this functions

auto PAGPluginInstaller::copyFileByCmd(char* originPath, char* targetPath) -> int {
  return 0;
}

auto PAGPluginInstaller::InstallPlugin(std::string pluginName) -> int {
  return 0;
}

auto PAGPluginInstaller::InstallPlugins(bool bForceInstall) -> int {
  return 0;
}

auto PAGPluginInstaller::UninstallPlugins() -> int {
  return 0;
}

auto PAGPluginInstaller::HasUpdate() -> bool {
  return true;
}

#pragma clang diagnostic pop
