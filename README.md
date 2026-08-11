# flutter_windows_iap

A Flutter plugin that exposes **Microsoft Store in-app purchase APIs for Windows** — no bundled UI.  
Fetch add-on products, initiate purchases, verify licenses, and resolve One Store purchase/collection IDs — then build your own paywall and entitlement logic.

### Main highlight — native Microsoft Store integration

Built on **Windows.Services.Store** (`StoreContext`) so your Flutter Windows app can use the same add-on catalog, purchase flow, and license checks as a native UWP/WinUI app — **Consumable**, **Durable**, and **UnmanagedConsumable** add-ons, including subscriptions configured as durable products in Partner Center.

| **Platform** | Windows (Microsoft Store / MSIX packaged app) |
| ------------ | --------------------------------------------- |
| **Dart**     | `>=3.7.0` `<4.0.0`                            |
| **Flutter**  | `>=3.16.0`                                    |
| **Version**  | 0.0.1                                         |
| **Channel**  | `com.moontechnolabs.flutter_windows_iap` |
| **License**  | [MIT](https://github.com/MoonTechnolabs/flutter_windows_iap/blob/main/LICENSE) |

---

## Features

| Feature | Description |
| ------- | ----------- |
| **Product catalog** | Load add-on metadata (title, description, price, kind) from the Microsoft Store by Store ID |
| **Purchase flow** | Trigger the system purchase UI via `RequestPurchaseAsync` |
| **License checks** | Verify whether the user owns a specific add-on or any active add-on |
| **Add-on licenses** | Read full license map (`isActive`, expiration, SKU, offer token) |
| **One Store IDs** | Resolve `purchaseId`, `collectionId`, and OAuth token for server-side validation |
| **No bundled UI** | Data APIs only — you own the widgets and paywall |

---

## Table of contents

- [Install](#install)
- [Configure Microsoft Store](#configure-microsoft-store)
- [Quick start](#quick-start)
- [Usage](#usage)
- [API reference](#api-reference)
- [Architecture](#architecture)
- [Example app](#example-app)
- [Limitations & notes](#limitations--notes)
- [License](#license)

---

## Install

```yaml
dependencies:
  flutter_windows_iap: ^0.0.1
```

```bash
flutter pub add flutter_windows_iap
```

```dart
import 'package:flutter_windows_iap/flutter_windows_iap.dart';
```

---

## Configure Microsoft Store

This plugin talks to the **Microsoft Store** at runtime. Your app must be **MSIX-packaged** and associated with a Store listing. Unpackaged debug builds will not receive valid Store responses.

### 1. Partner Center — create add-ons

1. Open [Partner Center](https://partner.microsoft.com/dashboard) → your app → **Monetization** → **Add-ons**.
2. Create one or more add-ons (Consumable, Durable, or Unmanaged Consumable).
3. Copy each add-on’s **Store ID**. You pass this string to every API that accepts a `storeId`.

| Add-on type in Partner Center | `productKind` you may see | Typical use |
| ----------------------------- | ------------------------- | ----------- |
| Consumable | `Consumable` | Coins, credits, one-time use |
| Durable | `Durable` | Permanent unlock, non-expiring subscription SKU |
| Unmanaged consumable | `UnmanagedConsumable` | Consumables you track on your own server |

> Subscription add-ons are often configured with **ProductKind `Durable`** in the Store API. The plugin queries `Consumable`, `Durable`, and `UnmanagedConsumable` when loading products.

### 2. Package identity

Your MSIX package identity (name, publisher, version) must match the app registered in Partner Center. Use tools such as [`msix`](https://pub.dev/packages/msix) or Visual Studio packaging to produce a build you can sideload or submit to the Store.

### 3. Azure AD / One Store (optional — for `getPurchaseId`)

Server-side purchase validation requires OAuth credentials from Azure AD:

| Field | Required | Why |
| ----- | -------- | --- |
| `clientId` | Yes | Azure AD application (client) ID |
| `clientSecret` | Yes | Client secret for the app registration |
| `tenantId` | Yes | Azure AD tenant that owns the app |
| `resource` | No | OAuth resource for One Store B2B token (default: `https://onestore.microsoft.com`) |
| `collectionsResource` | No | OAuth resource for collections token (default: `https://onestore.microsoft.com/b2b/keys/create/collections`) |

Register your app in Azure Portal, grant it access to the Microsoft Store commerce APIs, and pass credentials via [`OneStoreConfig`](#onestoreconfig).

### 4. Testing

- **Store sandbox**: Use a Microsoft account enrolled in Partner Center’s sandbox testers.
- **Sideloaded MSIX**: Install a signed package whose identity matches Partner Center before calling purchase APIs.
- **Logs**: Native code writes debug logs to `%LocalAppData%\flutter_windows_iap_example\iap_log.txt` when running the example app.

---

## Quick start

```dart
import 'package:flutter/services.dart';
import 'package:flutter_windows_iap/flutter_windows_iap.dart';

Future<void> purchaseAddon() async {
  final iap = FlutterWindowsIap();

  // Store ID from Partner Center → Monetization → Add-ons
  const storeId = 'StoreId';

  // 1. Load product info (title, price, whether user already owns it)
  final products = await iap.getProducts(storeId);
  if (products.isEmpty) return;

  // 2. Start purchase (opens Microsoft Store UI)
  final status = await iap.makePurchase(storeId);

  // 3. Verify entitlement
  final owns = await iap.checkPurchase(storeId: storeId);
}
```

---

## Usage

Create one instance of [`FlutterWindowsIap`](#flutterwindowsiap) and reuse it for the app lifetime.

```dart
final iap = FlutterWindowsIap();
```

---

### 1. Get products (`getProducts`)

Loads Store metadata for a single add-on Store ID.

```dart
final List<Product> products = await iap.getProducts('StoreId');
```

| Parameter | Type | Required | What to pass | Why |
| --------- | ---- | -------- | ------------ | --- |
| `storeId` | `String` | Yes | Add-on **Store ID** from Partner Center | Identifies the SKU in `GetStoreProductsAsync` |

**Returns:** `Future<List<Product>>` — usually one item when the ID is valid; empty list when nothing is found.

**Throws:** `PlatformException` when the Store returns an extended error (misconfigured product, network failure, app not associated with Store, etc.).

| Field | Type | Description |
| ----- | ---- | ----------- |
| `title` | `String?` | Localized product title from the Store |
| `description` | `String?` | Localized description |
| `price` | `String?` | Formatted price string (e.g. `$4.99`, `$9.99 per month`) |
| `inCollection` | `bool?` | `true` if the signed-in user already owns this add-on |
| `productKind` | `String?` | Store kind: `Consumable`, `Durable`, or `UnmanagedConsumable` |
| `storeId` | `String?` | Echo of the Store ID |

**Example — display in UI:**

```dart
for (final p in products) {
print('${p.title} — ${p.price} (${p.productKind})');
if (p.inCollection == true) print('Already owned');
}
```

---

### 2. Make purchase (`makePurchase`)

Opens the Microsoft Store purchase dialog for the given add-on.

```dart
final StorePurchaseStatus? status = await iap.makePurchase('StoreId');
```

| Parameter | Type | Required | What to pass | Why |
| --------- | ---- | -------- | ------------ | --- |
| `storeId` | `String` | Yes | Same Store ID used in `getProducts` | Passed to `RequestPurchaseAsync` |

**Returns:** `Future<StorePurchaseStatus?>` — see enum below. Returns `null` for an unrecognized status code.

**Throws:** `PlatformException` when `ExtendedError` is not `S_OK` (e.g. product not configured — HRESULT `0x803f6107`).

| Status | Meaning | Typical next step |
| ------ | ------- | ----------------- |
| `StorePurchaseStatus.succeeded` | User completed purchase | Unlock content; call `checkPurchase` |
| `StorePurchaseStatus.alreadyPurchased` | User already owns it | Treat as entitled |
| `StorePurchaseStatus.notPurchased` | User cancelled or did not buy | Keep paywall |
| `StorePurchaseStatus.networkError` | Network issue | Retry later |
| `StorePurchaseStatus.serverError` | Store server error | Retry later |

```dart
switch (await iap.makePurchase(storeId)) {
case StorePurchaseStatus.succeeded:
case StorePurchaseStatus.alreadyPurchased:
// grant access
break;
case StorePurchaseStatus.notPurchased:
// user declined
break;
case StorePurchaseStatus.networkError:
case StorePurchaseStatus.serverError:
// show error, retry
break;
default:
break;
}
```

---

### 3. Check purchase (`checkPurchase`)

Checks whether the current user has an **active** add-on license. Uses `GetAppLicenseAsync` and inspects add-on licenses.

```dart
final bool owns = await iap.checkPurchase(storeId: 'StoreId');
```

| Parameter | Type | Required | What to pass | Why |
| --------- | ---- | -------- | ------------ | --- |
| `storeId` | `String` | No (default `''`) | Specific add-on Store ID, or empty string | Controls which license is checked — see rules below |

**Returns:** `Future<bool>`

| `storeId` value | Returns `true` when… | Returns `false` when… |
| --------------- | -------------------- | --------------------- |
| **Non-empty** | That add-on’s license exists and `IsActive == true` | ID not found, inactive, or app license inactive |
| **Empty (`''`)** | **Any** add-on license has `IsActive == true` | No active add-ons, or app license inactive |

> If the **app license** itself is not active (`AppLicense.IsActive == false`), this method always returns `false`.

Use this on app start, after purchase, and when restoring entitlements:

```dart
// Premium feature for one SKU
if (await iap.checkPurchase(storeId: premiumStoreId)) {
showPremiumContent();
}

// Any paid add-on (e.g. “has anything unlocked?”)
if (await iap.checkPurchase()) {
showPaidTier();
}
```

---

### 4. Add-on licenses (`getAddonLicenses`)

Returns the full license map for all add-ons the user has ever acquired.

```dart
final Map<String, StoreLicense> licenses = await iap.getAddonLicenses();
```

| Parameter | None |

**Returns:** `Future<Map<String, StoreLicense>>` — keys are add-on Store IDs; values are license objects.

| `StoreLicense` field | Type | Description |
| -------------------- | ---- | ----------- |
| `isActive` | `bool?` | Whether the license is currently valid |
| `skuStoreId` | `String?` | SKU Store ID |
| `inAppOfferToken` | `String?` | Offer token for promotions / trials |
| `expirationDate` | `num?` | Raw Windows `DateTime` tick count |

**Helper:**

```dart
final expiry = license.getExpirationDate(); // DateTime? — converted from Store ticks
```

Use when you need expiration dates, offer tokens, or a full entitlement snapshot:

```dart
for (final entry in licenses.entries) {
print('${entry.key}: active=${entry.value.isActive}, '
'expires=${entry.value.getExpirationDate()}');
}
```

---

### 5. Purchase & collection IDs (`getPurchaseId`)

Resolves Microsoft Store customer identifiers for **server-side** purchase and collections validation (One Store commerce API).

```dart
final Map<String, String> ids = await iap.getPurchaseId(
'UserId',
config: const OneStoreConfig(
clientId: 'ClientId',
clientSecret: 'ClientSecret',
tenantId: 'TenantId',
),
);
```

| Parameter | Type | Required | What to pass | Why |
| --------- | ---- | -------- | ------------ | --- |
| `userId` | `String` | Yes | A **stable, anonymous ID** you assign per user (not PII) | Passed to `GetCustomerPurchaseIdAsync` / `GetCustomerCollectionsIdAsync` |
| `config` | `OneStoreConfig?` | Yes (credentials required) | Azure AD OAuth settings | Acquires tokens for Store API calls |

**Returns:** `Future<Map<String, String>>`

| Key | Description |
| --- | ----------- |
| `purchaseId` | Customer purchase ID for purchase validation APIs |
| `collectionId` | Customer collections ID for entitlement queries |
| `azureToken` | One Store B2B OAuth access token (use promptly; short-lived) |

If native code returns no map, Dart fills `'NA'` for each key. On failure, expect `PlatformException` with codes such as `invalid_oauth_config`, `oauth_token_failed`, or `oauth_collections_token_failed`.

**Security:** Never ship `clientSecret` in client code for production. Prefer a backend that holds secrets and returns tokens or validated entitlements to the app.

```dart
// Send to your backend — do not log secrets or tokens in release builds
await http.post(
Uri.parse('https://your-api.com/validate-purchase'),
body: {
'userId': userId,
'purchaseId': ids['purchaseId']!,
'collectionId': ids['collectionId']!,
},
);
```

---

## API reference

### `FlutterWindowsIap`

| Method | Returns | Description |
| ------ | ------- | ----------- |
| `getProducts(storeId)` | `Future<List<Product>>` | Load add-on metadata from Store catalog |
| `makePurchase(storeId)` | `Future<StorePurchaseStatus?>` | Start Store purchase UI |
| `checkPurchase({storeId})` | `Future<bool>` | Check active add-on license |
| `getAddonLicenses()` | `Future<Map<String, StoreLicense>>` | All add-on licenses for current user |
| `getPurchaseId(userId, {config})` | `Future<Map<String, String>>` | Purchase ID, collection ID, OAuth token |




## Limitations & notes

| Topic | Detail |
| ----- | ------ |
| **Platform** | Windows only. Other platforms are not supported by this package. |
| **Packaging** | Store APIs require an MSIX-packaged app associated with Partner Center. Plain `flutter run` without Store association may fail or return empty data. |
| **Store ID** | Must match an add-on configured and published (or in flight) in Partner Center for your app identity. |
| **User account** | Purchases and licenses are tied to the Microsoft account signed into the Store on the device. |
| **Consumables** | Microsoft does not auto-consume consumables; your app or backend must track consumption if needed. |
| **Subscriptions** | Renewal and grace periods follow Store rules; use `StoreLicense.isActive` and `getExpirationDate()` for time-bound access. |
| **OAuth secrets** | `getPurchaseId` needs Azure credentials — keep secrets on a server in production. |
| **Errors** | `getProducts` and `makePurchase` surface Store `ExtendedError` as `PlatformException`. Always wrap in `try/catch` in UI code. |
| **macOS stub** | `getProducts` / `checkPurchase` contain macOS stubs that throw or return `false` — ignore unless you fork the plugin. |

---

## License

MIT © MOON TECHNOLABS PVT LTD — see [LICENSE](LICENSE).
