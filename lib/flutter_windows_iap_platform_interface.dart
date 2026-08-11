import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'models/one_store_config.dart';
import 'models/product.dart';
import 'models/store_license.dart';
import 'store_purchase_status.dart';
import 'flutter_windows_iap_method_channel.dart';

abstract class FlutterWindowsIapPlatform extends PlatformInterface {
  /// Constructs a FlutterWindowsIapPlatform.
  FlutterWindowsIapPlatform() : super(token: _token);

  static final Object _token = Object();

  static FlutterWindowsIapPlatform _instance =
      MethodChannelFlutterWindowsIap();

  /// The default instance of [FlutterWindowsIapPlatform] to use.
  ///
  /// Defaults to [MethodChannelFlutterWindowsIap].
  static FlutterWindowsIapPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [FlutterWindowsIapPlatform]
  /// when they register themselves.
  static set instance(FlutterWindowsIapPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<StorePurchaseStatus?> makePurchase(String storeId) {
    throw UnimplementedError('makePurchase() has not been implemented.');
  }

  Future<List<Product>> getProducts(String storeId) {
    throw UnimplementedError('getProducts() has not been implemented.');
  }

  Future<bool> checkPurchase({required String storeId}) {
    throw UnimplementedError('checkPurchase() has not been implemented.');
  }

  Future<Map<String, StoreLicense>> getAddonLicenses() {
    throw UnimplementedError('getAddonLicenses() has not been implemented.');
  }

  Future<Map<String, String>> getPurchaseId(
    String userId, {
    OneStoreConfig? config,
  }) {
    throw UnimplementedError('getPurchaseId() has not been implemented.');
  }
}
