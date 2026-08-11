#include "include/flutter_windows_iap/flutter_windows_iap_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "flutter_windows_iap_plugin.h"

void FlutterWindowsIapPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  flutter_windows_iap::FlutterWindowsIapPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
