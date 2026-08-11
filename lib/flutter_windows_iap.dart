import 'dart:io';

import 'package:flutter/services.dart';

import 'models/one_store_config.dart';
import 'models/product.dart';
import 'models/store_license.dart';
import 'store_purchase_status.dart';
import 'flutter_windows_iap_platform_interface.dart';

export 'models/one_store_config.dart';
export 'models/product.dart';
export 'models/store_license.dart';
export 'store_purchase_status.dart';

class FlutterWindowsIap {
  Future<StorePurchaseStatus?> makePurchase(String storeId) {
    return FlutterWindowsIapPlatform.instance.makePurchase(storeId);
  }

  /// throw PlatformException if error
  Future<List<Product>> getProducts(String storeId) {
    if (Platform.isMacOS) {
      return Future.delayed(const Duration(seconds: 2), () {
        throw PlatformException(
            code: '123123123', message: 'Products can not loaded now.');
      });
    }
    return FlutterWindowsIapPlatform.instance.getProducts(storeId);
  }

  /// Check when user has current valid purchase
  ///
  /// - Add-On type: Subscription, Durable
  ///
  /// - Always return false if AppLicense has IsActive status = false.
  ///
  /// - if storeId is Not Empty:
  ///
  /// -- it will return true if Product(storeId) has IsActive status = true.
  ///
  /// -- return false if not.
  ///
  /// - if storeId is Empty:
  ///
  /// -- it will return true if any Add-On have IsActive status = true.
  ///
  /// -- return false if all Add-On have IsActive status = false.
  Future<bool> checkPurchase({String storeId = ''}) {
    if (Platform.isMacOS) {
      return Future.value(false);
    }
    return FlutterWindowsIapPlatform.instance
        .checkPurchase(storeId: storeId);
  }

  /// return the map of StoreLicense
  ///
  /// A map of key and value pairs, where each key is the Store ID of an add-on SKU from the
  /// Microsoft Store catalog and each value is a StoreLicense object that contains license
  /// info for the add-on.
  Future<Map<String, StoreLicense>> getAddonLicenses() {
    return FlutterWindowsIapPlatform.instance.getAddonLicenses();
  }

  /// Returns `purchaseId`, `collectionId`, and `azureToken` (OAuth token for One Store).
  ///
  /// Pass [config] to provide Azure AD / One Store OAuth credentials.
  /// [clientId], [clientSecret], and [tenantId] must be set in [config].
  /// [resource] and [collectionsResource] fall back to defaults when omitted.
  Future<Map<String, String>> getPurchaseId(
    String userId, {
    OneStoreConfig? config,
  }) {
    return FlutterWindowsIapPlatform.instance.getPurchaseId(
      userId,
      config: config,
    );
  }
}
