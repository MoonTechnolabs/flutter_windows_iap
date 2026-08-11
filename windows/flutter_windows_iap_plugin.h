#ifndef FLUTTER_PLUGIN_FLUTTER_WINDOWS_IAP_PLUGIN_H_
#define FLUTTER_PLUGIN_FLUTTER_WINDOWS_IAP_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace flutter_windows_iap {

class FlutterWindowsIapPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FlutterWindowsIapPlugin();

  virtual ~FlutterWindowsIapPlugin();

  // Disallow copy and assign.
  FlutterWindowsIapPlugin(const FlutterWindowsIapPlugin&) = delete;
  FlutterWindowsIapPlugin& operator=(const FlutterWindowsIapPlugin&) = delete;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace flutter_windows_iap

#endif  // FLUTTER_PLUGIN_FLUTTER_WINDOWS_IAP_PLUGIN_H_
