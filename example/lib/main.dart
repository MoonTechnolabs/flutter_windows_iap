// ignore_for_file: use_build_context_synchronously

import 'package:andesgroup_common/common.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_windows_iap/flutter_windows_iap.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final _iapPlugin = FlutterWindowsIap();

  // Replace with your add-on Store ID from Partner Center (Products > Add-ons).
  static const String storeId = 'StoreId';

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Builder(
        builder: (context) {
          return Scaffold(
            appBar: AppBar(title: const Text('Plugin example app')),
            body: Center(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  ElevatedButton(
                    onPressed: () async {
                      final result = await _iapPlugin.checkPurchase(
                        storeId: storeId,
                      );
                      showAlertDialog(
                        context,
                        content: 'checkPurchase: $result',
                      );
                    },
                    child: const Text('checkPurchase'),
                  ),
                  const Gap(16),
                  ElevatedButton(
                    onPressed: () async {
                      try {
                        final result = await _iapPlugin.makePurchase(storeId);
                        showAlertDialog(
                          context,
                          content: 'makePurchase: $result',
                        );
                      } on PlatformException catch (e) {
                        showAlertDialog(
                          context,
                          content: 'makePurchase error: ${e.message}',
                        );
                      }
                    },
                    child: const Text('makePurchase'),
                  ),
                  const Gap(16),
                  ElevatedButton(
                    onPressed: () async {
                      try {
                        final products = await _iapPlugin.getProducts(storeId);
                        final info =
                            products.isEmpty
                                ? 'No products found'
                                : products
                                    .map(
                                      (p) =>
                                          'Title: ${p.title}\n'
                                          'Description: ${p.description}\n'
                                          'Price: ${p.price}\n'
                                          'Kind: ${p.productKind}\n'
                                          'StoreId: ${p.storeId}\n'
                                          'InCollection: ${p.inCollection}',
                                    )
                                    .join('\n\n');
                        showAlertDialog(context, content: info);
                      } on PlatformException catch (e) {
                        showAlertDialog(
                          context,
                          content: 'getProducts error: ${e.message}',
                        );
                      }
                    },
                    child: const Text('getProducts'),
                  ),
                  const Gap(16),
                  ElevatedButton(
                    onPressed: () async {
                      final result = await _iapPlugin.getAddonLicenses();
                      showAlertDialog(context, content: 'licenses: $result');
                    },
                    child: const Text('getAddonLicenses'),
                  ),
                  const Gap(16),
                  ElevatedButton(
                    onPressed: () async {
                      try {
                        final result = await _iapPlugin.getPurchaseId(
                          'UserId',
                          // resource / collectionsResource use defaults when omitted.
                          config: const OneStoreConfig(
                            clientId: 'ClientId',
                            clientSecret: 'ClientSecret',
                            tenantId: 'TenantId',
                          ),
                        );
                        showAlertDialog(
                          context,
                          content: 'purchase info: $result',
                        );
                      } on PlatformException catch (e) {
                        showAlertDialog(
                          context,
                          content: 'getPurchaseId error: ${e.message}',
                        );
                      }
                    },
                    child: const Text('getPurchaseId'),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }
}
