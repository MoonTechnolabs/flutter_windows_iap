# flutter_windows_iap — Example app

A minimal **Windows** Flutter app that demonstrates every public API in [`flutter_windows_iap`](../README.md). Use it to learn the purchase flow before integrating the plugin into your own app.

---

## What this example does

The example is a single screen with five buttons. Each button calls one plugin method and shows the result in a dialog (and prints to the debug console).

| Button | Plugin call | What you learn |
| ------ | ----------- | -------------- |
| **checkPurchase** | `checkPurchase(storeId: …)` | Whether the signed-in Store user currently owns the configured add-on |
| **makePurchase** | `makePurchase(storeId)` | How to start the Microsoft Store purchase UI and read `StorePurchaseStatus` |
| **getProducts** | `getProducts(storeId)` | How to load title, price, kind, and `inCollection` for an add-on |
| **getAddonLicenses** | `getAddonLicenses()` | How to read the full map of add-on licenses (active state, expiration) |
| **getPurchaseId** | `getPurchaseId(userId, config: …)` | How to obtain `purchaseId`, `collectionId`, and OAuth token for server validation |

There is no custom paywall UI — the goal is to show **raw API usage** and expected inputs/outputs.

---

## Prerequisites

Before the Store APIs return real data, you need:

1. **Windows 10/11** with the Microsoft Store available.
2. **Flutter Windows** toolchain (`flutter doctor` shows Windows desktop enabled).
3. An app registered in **Partner Center** with at least one **add-on** (Monetization → Add-ons).
4. A **Store ID** for that add-on (replace the `StoreId` placeholder in `main.dart`).
5. An **MSIX-packaged build** whose package identity matches Partner Center (see [MSIX config](#msix-packaging) below).
6. A **sandbox tester** Microsoft account (Partner Center → Testers) for purchase testing.

Optional — for **getPurchaseId** only:

- Azure AD app registration with `clientId`, `clientSecret`, and `tenantId` for One Store commerce APIs.

---

## Project layout

```
example/
├── lib/
│   └── main.dart          # All demo UI and plugin calls
├── pubspec.yaml           # Depends on ../ (local plugin) + msix config
├── windows/               # Standard Flutter Windows runner
└── README.md              # This file
```

The plugin instance is created once:

```dart
final _iapPlugin = FlutterWindowsIap();
```

The add-on Store ID is a constant at the top of `main.dart` — replace it with your own Partner Center Store ID before testing.

---

## Run locally

```bash
cd example
flutter pub get
flutter run -d windows
```

This launches the debug runner. **Store purchase APIs may not work fully** until you install an MSIX build tied to your Partner Center identity (see next section).

---

## MSIX packaging

`pubspec.yaml` includes an `msix_config` block (display name, publisher, identity name, etc.). To produce an installable package for Store or sideload testing:

```bash
cd example
flutter pub get
dart run msix:create
```

Install the generated `.msix` on your machine, sign in to the Store with a sandbox account, then exercise the buttons.

Adjust `msix_config` in `pubspec.yaml` so `identity_name` and `publisher` match your Partner Center app — otherwise licenses and purchases will not bind to your listing.

---

## Walkthrough — each button

### 1. `checkPurchase`

**Purpose:** Quick entitlement check without opening the Store UI.

**Flow in the example:**

1. Calls `checkPurchase(storeId: storeId)` with the hard-coded Store ID.
2. Shows `true` or `false` in a dialog.

**When `true`:** The user has an **active** license for that add-on (and the app license is active).

**When `false`:** User never bought it, license expired, wrong Store ID, wrong package identity, or app license inactive.

**Tip:** Call this on startup to restore premium state after reinstall.

---

### 2. `makePurchase`

**Purpose:** Trigger the system purchase dialog.

**Flow in the example:**

1. Calls `makePurchase(storeId)`.
2. On success, shows the returned `StorePurchaseStatus` (`succeeded`, `alreadyPurchased`, `notPurchased`, etc.).
3. On Store extended error, catches `PlatformException` and shows `e.message`.

**What to expect:**

| Result | Meaning |
| ------ | ------- |
| `succeeded` | Purchase completed |
| `alreadyPurchased` | User already owns the add-on |
| `notPurchased` | User cancelled or did not complete payment |
| `networkError` / `serverError` | Retry later |

After `succeeded`, call `checkPurchase` again to confirm entitlement in your app logic.

---

### 3. `getProducts`

**Purpose:** Load catalog metadata before showing a price or buy button.

**Flow in the example:**

1. Calls `getProducts(storeId)`.
2. Formats each `Product` (title, description, price, kind, storeId, inCollection) into a multi-line string.
3. Shows the string in a dialog; empty list shows “No products found”.

**Fields to use in your UI:**

- `title`, `price` — display on paywall
- `inCollection` — hide buy button if already owned
- `productKind` — `Consumable`, `Durable`, or `UnmanagedConsumable`

**Errors:** Misconfigured Store IDs often throw `PlatformException` (e.g. product not set up in Partner Center).

---

### 4. `getAddonLicenses`

**Purpose:** Inspect **all** add-on licenses, not just one Store ID.

**Flow in the example:**

1. Calls `getAddonLicenses()` with no arguments.
2. Prints and displays the full `Map<String, StoreLicense>`.

**Use in your app when you need:**

- Expiration dates → `license.getExpirationDate()`
- Trial / offer info → `inAppOfferToken`
- A dashboard of everything the user owns

Keys in the map are add-on Store IDs.

---

### 5. `getPurchaseId`

**Purpose:** Get IDs and a token for **backend** validation via One Store APIs.

**Flow in the example:**

1. Passes a placeholder `userId` (`'UserId'`).
2. Passes `OneStoreConfig` with placeholders: `ClientId`, `ClientSecret`, `TenantId`.
3. Shows the returned map: `purchaseId`, `collectionId`, `azureToken`.

**Before testing:**

- Replace `'UserId'` with a stable anonymous ID your backend recognizes.
- Replace `ClientId`, `ClientSecret`, and `TenantId` with credentials from **your** Azure AD app registration.
- Never commit real secrets to source control — use environment variables or a secrets file in real projects.

**Typical production pattern:** Your server holds the client secret; the app calls your API, not Azure directly.

See the main [README — getPurchaseId](../README.md#5-purchase--collection-ids-getpurchaseid) for field descriptions and error codes.

---

## Debugging

Native plugin code writes timestamped logs to:

```
%LocalAppData%\flutter_windows_iap_example\iap_log.txt
```

Open this file when:

- `getProducts` returns empty or throws
- `makePurchase` fails with an extended error
- `getPurchaseId` OAuth steps fail

Each log line is prefixed with `[1]`, `[oauth]`, `[getPurchaseId]`, etc., matching the native implementation.

---

## Suggested learning order

1. **getProducts** — confirm your Store ID and MSIX identity are correct.
2. **checkPurchase** — baseline entitlement before buying.
3. **makePurchase** — complete a sandbox purchase.
4. **checkPurchase** again — verify it returns `true`.
5. **getAddonLicenses** — inspect expiration and SKU details.
6. **getPurchaseId** — only after Azure AD is configured (server validation path).

---
