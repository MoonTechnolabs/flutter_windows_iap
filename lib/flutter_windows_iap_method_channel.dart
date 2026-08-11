import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'models/one_store_config.dart';
import 'models/product.dart';
import 'models/store_license.dart';
import 'store_purchase_status.dart';
import 'utils.dart';
import 'flutter_windows_iap_platform_interface.dart';

/// Package / method-channel id.
const String kFlutterWindowsIapChannel =
    'com.moontechnolabs.flutter_windows_iap';

/// A [Map] between whitespace characters and their escape sequences.
const _escapeMap = {
  '\n': '',
  '\r': '',
  '\f': '',
  '\b': '',
  '\t': '',
  '\v': '',
  '\x7F': '', // delete
};

/// A [RegExp] that matches whitespace characters that should be escaped.
var _escapeRegExp = RegExp(
    '[\\x00-\\x07\\x0E-\\x1F${_escapeMap.keys.map(_getHexLiteral).join()}]');

/// Returns [str] with all whitespace characters represented as their escape
/// sequences.
///
/// Backslash characters are escaped as `\\`
String escape(String str) {
  str = str.replaceAll('\\', r'\\');
  return str.replaceAllMapped(_escapeRegExp, (match) {
    var mapped = _escapeMap[match[0]];
    if (mapped != null) return mapped;
    return _getHexLiteral(match[0]!);
  });
}

/// Given single-character string, return the hex-escaped equivalent.
String _getHexLiteral(String input) {
  var rune = input.runes.single;
  return r'\x' + rune.toRadixString(16).toUpperCase().padLeft(2, '0');
}

/// An implementation of [FlutterWindowsIapPlatform] that uses method channels.
class MethodChannelFlutterWindowsIap
    extends FlutterWindowsIapPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel(kFlutterWindowsIapChannel);

  @override
  Future<StorePurchaseStatus?> makePurchase(String storeId) async {
    final result = await methodChannel
        .invokeMethod<int>('makePurchase', {'storeId': storeId});
    if (result == null) {
      return null;
    }
    switch (result) {
      case 0:
        return StorePurchaseStatus.succeeded;
      case 1:
        return StorePurchaseStatus.alreadyPurchased;
      case 2:
        return StorePurchaseStatus.notPurchased;
      case 3:
        return StorePurchaseStatus.networkError;
      case 4:
        return StorePurchaseStatus.serverError;
    }
    return null;
  }

  Stream<List<Product>> productsStream() {
    return const EventChannel(
            '$kFlutterWindowsIapChannel/event_products')
        .receiveBroadcastStream()
        .map((event) {
      if (event is String) {
        return parseListNotNull(
            json: jsonDecode(escape(event)), fromJson: Product.fromJson);
      } else {
        return [];
      }
    });
  }

  @override
  Future<List<Product>> getProducts(String storeId) async {
    final result = await methodChannel
        .invokeMethod<String>('getProducts', {'storeId': storeId});
    if (result == null) {
      return [];
    }
    return parseListNotNull(
        json: jsonDecode(escape(result)), fromJson: Product.fromJson);
  }

  @override
  Future<bool> checkPurchase({required String storeId}) async {
    final result = await methodChannel
        .invokeMethod<bool>('checkPurchase', {'storeId': storeId});
    return result ?? false;
  }

  @override
  Future<Map<String, StoreLicense>> getAddonLicenses() async {
    final result = await methodChannel.invokeMethod<Map>('getAddonLicenses');
    if (result == null) {
      return {};
    }
    return result.map((key, value) =>
        MapEntry(key.toString(), StoreLicense.fromJson(jsonDecode(value))));
  }

  @override
  Future<Map<String, String>> getPurchaseId(
    String userId, {
    OneStoreConfig? config,
  }) async {
    final resolvedConfig = (config ?? const OneStoreConfig()).resolved();
    final Object? raw = await methodChannel.invokeMethod<Object>(
      'getPurchaseId',
      <String, Object>{
        'userId': userId,
        ...resolvedConfig.toMethodChannelMap(),
      },
    );
    if (raw == null) {
      return <String, String>{
        'purchaseId': 'NA',
        'collectionId': 'NA',
        'azureToken': 'NA',
      };
    }
    if (raw is! Map) {
      return <String, String>{
        'purchaseId': 'NA',
        'collectionId': 'NA',
        'azureToken': 'NA',
      };
    }
    final Map<dynamic, dynamic> map = raw;
    final Object? pid = map['purchaseId'];
    final Object? cid = map['collectionId'];
    final Object? at = map['azureToken'];
    return <String, String>{
      'purchaseId': pid is String ? pid : 'NA',
      'collectionId': cid is String ? cid : 'NA',
      'azureToken': at is String ? at : 'NA',
    };
  }
}
