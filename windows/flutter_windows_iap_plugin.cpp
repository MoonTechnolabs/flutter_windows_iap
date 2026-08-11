#include "flutter_windows_iap_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

#pragma once
#include <winrt/Windows.Services.Store.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <shobjidl.h>

#include <flutter/event_sink.h>
#include <flutter/event_channel.h>
#include <flutter/event_stream_handler.h>
#include <flutter/event_stream_handler_functions.h>
#include <iostream>  // Required for std::cout
#include <winrt/Windows.ApplicationModel.h>

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Data.Json.h>
#include <string>


#include <winrt/Windows.System.h>
#include <winrt/Windows.ApplicationModel.Store.h>

#include <fstream>
#include <shlobj.h>
#include <winrt/Windows.Storage.h>
#include <chrono>
#include <iomanip>



using namespace winrt;
using namespace Windows::Services::Store;
using namespace Windows::Foundation::Collections;
namespace foundation = Windows::Foundation;


using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Headers;
using namespace Windows::Data::Json;


using namespace winrt::Windows::ApplicationModel::Store;

namespace flutter_windows_iap {

    void WriteToLocalAppData(std::wstring text);

    std::wstring s2ws(const std::string& s);

    std::wstring CurrentTimestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
        localtime_s(&localTime, &time);
        std::wstringstream ss;
        ss << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    namespace {
        constexpr wchar_t kDefaultResource[] =
            L"https://onestore.microsoft.com";
        constexpr wchar_t kDefaultCollectionsResource[] =
            L"https://onestore.microsoft.com/b2b/keys/create/collections";
        constexpr wchar_t kEmpty[] = L"";

        struct OneStoreOAuthConfig {
            winrt::hstring client_id;
            winrt::hstring client_secret;
            winrt::hstring tenant_id;
            winrt::hstring resource;
            winrt::hstring collections_resource;
        };

        std::wstring MaskSecret(const std::wstring& secret) {
            if (secret.empty()) {
                return L"<empty>";
            }
            if (secret.size() <= 4) {
                return L"****";
            }
            return L"****" + secret.substr(secret.size() - 4);
        }

        std::wstring ReadStringFromArgs(
            const flutter::EncodableMap& args,
            const char* key,
            const wchar_t* fieldLabel) {
            const auto encKey = flutter::EncodableValue(key);
            const auto it = args.find(encKey);
            if (it == args.end()) {
                WriteToLocalAppData(
                    (L"[config] " + std::wstring(fieldLabel) +
                     L" not provided").c_str());
                return kEmpty;
            }

            try {
                const auto value = std::get<std::string>(it->second);
                if (value.empty()) {
                    WriteToLocalAppData(
                        (L"[config] " + std::wstring(fieldLabel) +
                         L" is empty").c_str());
                    return kEmpty;
                }

                WriteToLocalAppData(
                    (L"[config] " + std::wstring(fieldLabel) +
                     L" provided from Flutter").c_str());
                return s2ws(value);
            } catch (...) {
                WriteToLocalAppData(
                    (L"[config] " + std::wstring(fieldLabel) +
                     L" has invalid type").c_str());
                return kEmpty;
            }
        }

        std::wstring ReadStringWithFallback(
            const flutter::EncodableMap& args,
            const char* key,
            const wchar_t* fallback,
            const wchar_t* fieldLabel) {
            const auto encKey = flutter::EncodableValue(key);
            const auto it = args.find(encKey);
            if (it == args.end()) {
                WriteToLocalAppData(
                    (L"[config] " + std::wstring(fieldLabel) +
                     L" not provided; using default fallback").c_str());
                return fallback;
            }

            try {
                const auto value = std::get<std::string>(it->second);
                if (value.empty()) {
                    WriteToLocalAppData(
                        (L"[config] " + std::wstring(fieldLabel) +
                         L" is empty; using default fallback").c_str());
                    return fallback;
                }

                WriteToLocalAppData(
                    (L"[config] " + std::wstring(fieldLabel) +
                     L" provided from Flutter").c_str());
                return s2ws(value);
            } catch (...) {
                WriteToLocalAppData(
                    (L"[config] " + std::wstring(fieldLabel) +
                     L" has invalid type; using default fallback").c_str());
                return fallback;
            }
        }

        OneStoreOAuthConfig ReadOneStoreOAuthConfig(
            const flutter::EncodableMap& args) {
            OneStoreOAuthConfig config;
            config.client_id = ReadStringFromArgs(args, "clientId", L"clientId");
            config.client_secret =
                ReadStringFromArgs(args, "clientSecret", L"clientSecret");
            config.tenant_id = ReadStringFromArgs(args, "tenantId", L"tenantId");
            config.resource = ReadStringWithFallback(
                args, "resource", kDefaultResource, L"resource");
            config.collections_resource = ReadStringWithFallback(
                args, "collectionsResource", kDefaultCollectionsResource,
                L"collectionsResource");
            return config;
        }

        bool ValidateOAuthConfig(
            const OneStoreOAuthConfig& config,
            std::string* errorMessage) {
            if (config.client_id.empty()) {
                *errorMessage = "clientId is required but was not provided.";
                return false;
            }
            if (config.client_secret.empty()) {
                *errorMessage = "clientSecret is required but was not provided.";
                return false;
            }
            if (config.tenant_id.empty()) {
                *errorMessage = "tenantId is required but was not provided.";
                return false;
            }
            return true;
        }

        void LogOAuthConfigSummary(const OneStoreOAuthConfig& config) {
            WriteToLocalAppData(
                (L"[config] clientId=" + std::wstring(config.client_id.c_str()) +
                 L", tenantId=" + std::wstring(config.tenant_id.c_str()) +
                 L", resource=" + std::wstring(config.resource.c_str()) +
                 L", collectionsResource=" +
                 std::wstring(config.collections_resource.c_str()) +
                 L", clientSecret=" +
                 MaskSecret(std::wstring(config.client_secret.c_str()))).c_str());
        }
    }


    void WriteToLocalAppData(std::wstring text) {
        const std::wstring timestamped =
            L"[" + CurrentTimestamp() + L"] " + text;
        PWSTR path = nullptr;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path);
        if (SUCCEEDED(hr)) {
            std::wstring folderPath = path;
            CoTaskMemFree(path);

            folderPath += L"\\flutter_windows_iap_example";
            CreateDirectoryW(folderPath.c_str(), NULL);

            std::wstring filePath = folderPath + L"\\iap_log.txt";
            std::wcout << timestamped << std::endl;

            std::wofstream file(filePath, std::ios::app);
            if (file.is_open()) {
                file << timestamped << std::endl;
                file.close();
            } else {
                std::wcerr << L"Failed to open log file: " << filePath << std::endl;
            }
        } else {
            std::wcerr << L"Failed to get LocalAppData path." << std::endl;
        }
    }

    //////////////////////////////////////////////////////////////////////// BEGIN OF MY CODE //////////////////////////////////////////////////////////////
	flutter::PluginRegistrarWindows* _registrar;

	HWND GetRootWindow(flutter::FlutterView* view) {
		return ::GetAncestor(view->GetNativeWindow(), GA_ROOT);
	}

	StoreContext getStore() {
		StoreContext store = StoreContext::GetDefault();
		auto initWindow = store.try_as<IInitializeWithWindow>();
		if (initWindow != nullptr) {
			initWindow->Initialize(GetRootWindow(_registrar->GetView()));
		}
		return store;
	}

	std::wstring s2ws(const std::string& s)
	{
		int len;
		int slength = (int)s.length() + 1;
		len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
		wchar_t* buf = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
		std::wstring r(buf);
		delete[] buf;
		return r;
	}

	std::string debugString(std::vector<std::string> vt) {

		std::stringstream ss;
		ss << "( ";
		for (auto t : vt) {
			ss << t << ", ";
		}
		ss << " )\n";
		return ss.str();
	}

	std::string getExtendedErrorString(winrt::hresult error) {
		const HRESULT IAP_E_UNEXPECTED = 0x803f6107L;
		std::string message;
		if (error.value == IAP_E_UNEXPECTED) {
			message = "This Product has not been properly configured.";
		}
		else {
			message = "ExtendedError: " + std::to_string(error.value);
		}
		return message;
	}

    foundation::IAsyncAction makePurchase(hstring storeId, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> resultCallback)
    {
        WriteToLocalAppData(L"[1] makePurchase called");

        StorePurchaseResult purchaseResult = co_await getStore().RequestPurchaseAsync(storeId);
        WriteToLocalAppData(L"[2] Requested purchase from Store");

        std::wstringstream statusLog;
        statusLog << L"[3] Purchase Status: " << static_cast<int>(purchaseResult.Status());
        WriteToLocalAppData(statusLog.str().c_str());

        std::wstringstream errorLog;
        errorLog << L"[4] Extended Error: " << purchaseResult.ExtendedError().value;
        WriteToLocalAppData(errorLog.str().c_str());

        if (purchaseResult.Status() != StorePurchaseStatus::Succeeded) {
            WriteToLocalAppData(L"[5] Purchase failed status branch entered");
        }

        if (purchaseResult.Status() == StorePurchaseStatus::Succeeded) {
            WriteToLocalAppData(L"[6] Purchase succeeded block");
        } else {
            WriteToLocalAppData(L"[7] Purchase failed block");
        }

        if (purchaseResult.ExtendedError().value != S_OK) {
            WriteToLocalAppData(L"[8] Purchase has extended error, sending error to Flutter");

            resultCallback->Error(std::to_string(purchaseResult.ExtendedError().value),
                                  getExtendedErrorString(purchaseResult.ExtendedError().value));
            co_return;
        }

        int32_t returnCode;
        WriteToLocalAppData(L"[9] Determining return code");

        switch (purchaseResult.Status()) {
            case StorePurchaseStatus::AlreadyPurchased:
                returnCode = 1;
                WriteToLocalAppData(L"[10] Status: AlreadyPurchased");
                break;

            case StorePurchaseStatus::Succeeded:
                returnCode = 0;
                WriteToLocalAppData(L"[11] Status: Succeeded");
                break;

            case StorePurchaseStatus::NotPurchased:
                returnCode = 2;
                WriteToLocalAppData(L"[12] Status: NotPurchased");
                break;

            case StorePurchaseStatus::NetworkError:
                returnCode = 3;
                WriteToLocalAppData(L"[13] Status: NetworkError");
                break;

            case StorePurchaseStatus::ServerError:
                returnCode = 4;
                WriteToLocalAppData(L"[14] Status: ServerError");
                break;

            default:
                WriteToLocalAppData(L"[15] Status: Unknown/default case");
                auto status = reinterpret_cast<int32_t*>(purchaseResult.Status());
                resultCallback->Error(std::to_string(*status), "Product was not purchased due to an unknown error.");
                co_return;
                break;
        }

        WriteToLocalAppData(L"[16] Returning result to Flutter");
        resultCallback->Success(flutter::EncodableValue(returnCode));
    }


	std::string productsToString(std::vector<StoreProduct> products) {
		std::stringstream ss;
		ss << "[";
		for (int i = 0; i < products.size(); i++) {
			auto product = products.at(i);
			ss << "{";
			ss << "\"title\":\"" << to_string(product.Title()) << "\",";
			ss << "\"description\":\"" << to_string(product.Description()) << "\",";
			ss << "\"price\":\"" << to_string(product.Price().FormattedPrice()) << "\",";
			ss << "\"inCollection\":" << (product.IsInUserCollection() ? "true" : "false") << ",";
			ss << "\"productKind\":\"" << to_string(product.ProductKind()) << "\",";
			ss << "\"storeId\":\"" << to_string(product.StoreId()) << "\"";
			ss << "}";
			if (i != products.size() - 1) {
				ss << ",";
			}
		}
		ss << "]";

		return ss.str();
	}

    foundation::IAsyncAction getProducts(hstring storeId, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> resultCallback) {

        WriteToLocalAppData(L"[1] getProducts called");

        std::wstring wStoreId(storeId.begin(), storeId.end());
        WriteToLocalAppData((L"[2] Converted storeId to wstring: " + wStoreId).c_str());

        auto idVector = single_threaded_vector<hstring>();
        WriteToLocalAppData(L"[3] Created single_threaded_vector");

        idVector.Append(hstring(wStoreId));
        WriteToLocalAppData(L"[4] Appended storeId to idVector");

        // Subscription add-ons use ProductKind "Durable" per Microsoft docs:
        // https://learn.microsoft.com/en-us/uwp/api/windows.services.store.storeproduct.productkind
        auto result = co_await getStore().GetStoreProductsAsync(
            { L"Consumable", L"Durable", L"UnmanagedConsumable" },
            idVector.GetView());
        WriteToLocalAppData(L"[5] Called GetStoreProductsAsync");

        if (result.ExtendedError().value != S_OK) {
            WriteToLocalAppData(L"[6] GetStoreProductsAsync returned error");

            resultCallback->Error(std::to_string(result.ExtendedError().value), getExtendedErrorString(result.ExtendedError()));
        }
        else if (result.Products().Size() == 0) {
            WriteToLocalAppData(L"[7] No products found");

            resultCallback->Success(flutter::EncodableValue("[]"));
        }
        else {
            WriteToLocalAppData(L"[8] Products found, processing...");

            std::vector<StoreProduct> products;

            for (IKeyValuePair<hstring, StoreProduct> addOn : result.Products())
            {
                WriteToLocalAppData(L"[9] Iterating over add-on product");

                StoreProduct product = addOn.Value();
                WriteToLocalAppData(L"[10] Retrieved product from add-on");

                std::wstring title = product.Title().c_str();
                std::wstring price = product.Price().FormattedPrice().c_str();
                std::wstring description = product.Description().c_str();
                WriteToLocalAppData((L"[11] Product title: " + title).c_str());
                WriteToLocalAppData((L"[12] Product price: " + price).c_str());
                WriteToLocalAppData((L"[13] Product description: " + description).c_str());

                bool isSubscription = price.find(L"per month") != std::wstring::npos ||
                                      price.find(L"every") != std::wstring::npos;

                if (isSubscription)
                {
                    std::wcout << L"✅ This is a subscription: " << title << std::endl;
                    WriteToLocalAppData(L"[14] Detected as subscription");
                }
                else
                {
                    std::wcout << L"ℹ️ This is a standard durable product: " << title << std::endl;
                    WriteToLocalAppData(L"[15] Detected as standard product");
                }

                products.push_back(product);
                WriteToLocalAppData(L"[16] Added product to list");
            }

            std::string productsString = productsToString(products);
            WriteToLocalAppData(L"[17] Converted product list to string");

            resultCallback->Success(flutter::EncodableValue(productsString));
            WriteToLocalAppData(L"[18] Returned product list to Flutter");
        }
    }


	std::string getStoreLicenseString(StoreLicense license) {
		std::stringstream ss;
		ss << "{";
		ss << "\"isActive\":" << (license.IsActive() ? "true" : "false") << ",";
		ss << "\"skuStoreId\":\"" << to_string(license.SkuStoreId()) << "\",";
		ss << "\"inAppOfferToken\":\"" << to_string(license.InAppOfferToken()) << "\",";
		ss << "\"expirationDate\":" << license.ExpirationDate().time_since_epoch().count() << "";
		ss << "}";

		return ss.str();
	}

    bool IsPackaged() {
        try {
            auto package = winrt::Windows::ApplicationModel::Package::Current();
            auto id = package.Id();
            (void)id.Name();  // Accessing something to trigger exception if not packaged
            return true;
        } catch (...) {
            return false;
        }
    }




	foundation::IAsyncAction getAddonLicenses(std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> resultCallback) {

		auto result = co_await getStore().GetAppLicenseAsync();
		auto addonLicenses = result.AddOnLicenses();

		std::map<flutter::EncodableValue, flutter::EncodableValue> mapLicenses;

		for (IKeyValuePair<hstring, StoreLicense> addonLicense : addonLicenses)
		{
			mapLicenses[flutter::EncodableValue(to_string(addonLicense.Key()))] = flutter::EncodableValue(getStoreLicenseString(addonLicense.Value()));
		}

		resultCallback->Success(flutter::EncodableValue(mapLicenses));
	}

	/// <summary>
	///  need to test in real app on store
	/// </summary>
	foundation::IAsyncAction checkPurchase(std::string storeId, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> resultCallback) {
        WriteToLocalAppData(L"[1] Check Purchase Called =============================>");

        auto result = co_await getStore().GetAppLicenseAsync();
        WriteToLocalAppData(L"[2] After GetAppLicenseAsync");

        if (result.IsActive()) {
            WriteToLocalAppData(L"[3] Licence is active");

            auto addonLicenses = result.AddOnLicenses();
            WriteToLocalAppData(L"[4] Retrieved AddOnLicenses");

            UINT32 licenseCount = addonLicenses.Size();
            std::wstringstream ss;
            ss << L"[5] Total Add-on Licenses: " << licenseCount;
            WriteToLocalAppData(ss.str());

            std::wcout << L"Total Add-on Licenses: " << addonLicenses.Size() << std::endl;
            WriteToLocalAppData(L"[6] Printed license count to console");

            for (IKeyValuePair<hstring, StoreLicense> addonLicense : addonLicenses)
            {
                WriteToLocalAppData(L"[7] Iterating AddonLicense");
                StoreLicense license = addonLicense.Value();
                WriteToLocalAppData(L"[8] Got StoreLicense from AddonLicense");

                if (storeId.compare("") == 0) {
                    WriteToLocalAppData(L"[9] StoreID is empty");
                    std::wstring wideStoreId(storeId.begin(), storeId.end());
                    std::wcout << L"Store ID ::: Empty " << wideStoreId << std::endl;
                    WriteToLocalAppData(L"[10] Printed Empty Store ID");

                    if (license.IsActive()) {
                        WriteToLocalAppData(L"[11] License is active for empty StoreID");
                        resultCallback->Success(flutter::EncodableValue(true));
                        co_return;
                    }
                }
                else {
                    WriteToLocalAppData(L"[12] StoreID is not empty");

                    auto key = to_string(addonLicense.Key());
                    WriteToLocalAppData(L"[13] Converted addonLicense key");

                    std::wstring wideStoreId(storeId.begin(), storeId.end());
                    std::wcout << L"Store ID ::: " << wideStoreId << std::endl;
                    WriteToLocalAppData(L"[14] Printed Store ID");

                    std::cout << "Key ::: " << key << std::endl;
                    WriteToLocalAppData(L"[15] Printed addonLicense key");

                    if (key.compare(storeId) == 0) {
                        WriteToLocalAppData(L"[16] Key matched StoreID, returning license.IsActive()");
                        resultCallback->Success(flutter::EncodableValue(license.IsActive()));
                        co_return;
                    }
                }
            }

            WriteToLocalAppData(L"[17] No matching active add-on license found, returning false");
            resultCallback->Success(flutter::EncodableValue(false));
        }
        else {
            WriteToLocalAppData(L"[18] License is NOT active, returning false");
            resultCallback->Success(flutter::EncodableValue(false));
        }

    }


    winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetTokenFromAzureOAuthAsync(
            winrt::hstring client_id,
            winrt::hstring client_secret,
            winrt::hstring tenant_id,
            winrt::hstring resource,
            const wchar_t* tokenPurpose
    ) {
        co_await winrt::resume_background();

        WriteToLocalAppData(
            (L"[oauth] Requesting token for " + std::wstring(tokenPurpose) +
             L" (resource=" + std::wstring(resource.c_str()) + L")").c_str());

        winrt::Windows::Web::Http::HttpFormUrlEncodedContent content({
            { L"grant_type", L"client_credentials" },
            { L"client_id", client_id },
            { L"client_secret", client_secret },
            { L"resource", resource }
        });

        HttpClient httpClient;
        content.Headers().ContentType(
            winrt::Windows::Web::Http::Headers::HttpMediaTypeHeaderValue(
                L"application/x-www-form-urlencoded")
        );

        std::wstring url =
            L"https://login.microsoftonline.com/" + std::wstring(tenant_id) +
            L"/oauth2/token";
        WriteToLocalAppData((L"[oauth] Token URL: " + url).c_str());

        winrt::Windows::Foundation::Uri uri(url);

        try {
            HttpResponseMessage response = co_await httpClient.PostAsync(uri, content);
            const auto statusCode = static_cast<int>(response.StatusCode());
            hstring responseString = co_await response.Content().ReadAsStringAsync();

            WriteToLocalAppData(
                (L"[oauth] Response status for " + std::wstring(tokenPurpose) +
                 L": " + std::to_wstring(statusCode)).c_str());

            if (statusCode < 200 || statusCode >= 300) {
                WriteToLocalAppData(
                    (L"[oauth] Token request failed for " +
                     std::wstring(tokenPurpose) + L": " +
                     std::wstring(responseString.c_str())).c_str());
                co_return L"";
            }

            JsonObject json = JsonObject::Parse(responseString);
            if (json.HasKey(L"access_token")) {
                const auto token = json.GetNamedString(L"access_token");
                WriteToLocalAppData(
                    (L"[oauth] Access token acquired for " +
                     std::wstring(tokenPurpose) + L" (length=" +
                     std::to_wstring(token.size()) + L")").c_str());
                co_return token;
            }

            if (json.HasKey(L"error")) {
                const auto error = json.GetNamedString(L"error");
                const auto description = json.HasKey(L"error_description")
                    ? json.GetNamedString(L"error_description")
                    : L"";
                WriteToLocalAppData(
                    (L"[oauth] OAuth error for " + std::wstring(tokenPurpose) +
                     L": " + std::wstring(error.c_str()) + L" - " +
                     std::wstring(description.c_str())).c_str());
            } else {
                WriteToLocalAppData(
                    (L"[oauth] No access_token in response for " +
                     std::wstring(tokenPurpose) + L": " +
                     std::wstring(responseString.c_str())).c_str());
            }
            co_return L"";
        } catch (const winrt::hresult_error& ex) {
            WriteToLocalAppData(
                (L"[oauth] WinRT exception for " + std::wstring(tokenPurpose) +
                 L": 0x" + std::to_wstring(ex.code().value) + L" - " +
                 std::wstring(ex.message().c_str())).c_str());
            co_return L"";
        } catch (const std::exception& ex) {
            const std::string narrowMsg = ex.what();
            const std::wstring wideMsg(narrowMsg.begin(), narrowMsg.end());
            WriteToLocalAppData(
                (L"[oauth] std::exception for " + std::wstring(tokenPurpose) +
                 L": " + wideMsg).c_str());
            co_return L"";
        } catch (...) {
            WriteToLocalAppData(
                (L"[oauth] Unknown exception for " +
                 std::wstring(tokenPurpose)).c_str());
            co_return L"";
        }
    }


    foundation::IAsyncAction getPurchaseId(
            hstring userId,
            OneStoreOAuthConfig oauthConfig,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> resultCallback) {

        WriteToLocalAppData(L"[getPurchaseId] Started");
        LogOAuthConfigSummary(oauthConfig);

        std::string validationError;
        if (!ValidateOAuthConfig(oauthConfig, &validationError)) {
            WriteToLocalAppData(
                (L"[getPurchaseId] Config validation failed: " +
                 s2ws(validationError)).c_str());
            resultCallback->Error("invalid_oauth_config", validationError);
            co_return;
        }

        try
        {
            WriteToLocalAppData(L"[getPurchaseId] Requesting One Store B2B token");
            winrt::hstring token = co_await GetTokenFromAzureOAuthAsync(
                oauthConfig.client_id,
                oauthConfig.client_secret,
                oauthConfig.tenant_id,
                oauthConfig.resource,
                L"OneStore B2B");

            if (token.empty()) {
                WriteToLocalAppData(L"[getPurchaseId] B2B token is empty; aborting");
                resultCallback->Error(
                    "oauth_token_failed",
                    "Failed to acquire One Store B2B OAuth token.");
                co_return;
            }

            WriteToLocalAppData(
                (L"[getPurchaseId] userId=" + std::wstring(userId.c_str())).c_str());

            winrt::Windows::Services::Store::StoreContext store =
                winrt::Windows::Services::Store::StoreContext::GetDefault();

            WriteToLocalAppData(L"[getPurchaseId] Calling GetCustomerPurchaseIdAsync");
            auto customerPurchaseId =
                co_await store.GetCustomerPurchaseIdAsync(token.c_str(), userId);

            WriteToLocalAppData(L"[getPurchaseId] Requesting collections token");
            winrt::hstring serviceTicket = co_await GetTokenFromAzureOAuthAsync(
                oauthConfig.client_id,
                oauthConfig.client_secret,
                oauthConfig.tenant_id,
                oauthConfig.collections_resource,
                L"collections");

            if (serviceTicket.empty()) {
                WriteToLocalAppData(
                    L"[getPurchaseId] Collections token is empty; aborting");
                resultCallback->Error(
                    "oauth_collections_token_failed",
                    "Failed to acquire One Store collections OAuth token.");
                co_return;
            }

            WriteToLocalAppData(L"[getPurchaseId] Calling GetCustomerCollectionsIdAsync");
            auto CustomerCollectionsId =
                co_await store.GetCustomerCollectionsIdAsync(serviceTicket, userId);

            winrt::hstring purchaseId = winrt::hstring(customerPurchaseId.c_str());
            winrt::hstring CollectionId = winrt::hstring(CustomerCollectionsId.c_str());

            WriteToLocalAppData(
                (L"[getPurchaseId] purchaseId=" +
                 std::wstring(purchaseId.c_str())).c_str());
            WriteToLocalAppData(
                (L"[getPurchaseId] collectionId=" +
                 std::wstring(CollectionId.c_str())).c_str());

            std::string purchaseIdStr = winrt::to_string(purchaseId);
            std::string collectionIdStr = winrt::to_string(CollectionId);
            std::string azureTokenStr = winrt::to_string(token);

            flutter::EncodableMap resultMap = {
                {flutter::EncodableValue("purchaseId"),
                 flutter::EncodableValue(purchaseIdStr)},
                {flutter::EncodableValue("collectionId"),
                 flutter::EncodableValue(collectionIdStr)},
                {flutter::EncodableValue("azureToken"),
                 flutter::EncodableValue(azureTokenStr)}
            };

            WriteToLocalAppData(L"[getPurchaseId] Completed successfully");
            resultCallback->Success(flutter::EncodableValue(resultMap));
        }
        catch (const winrt::hresult_error& ex)
        {
            WriteToLocalAppData(
                (L"[getPurchaseId] WinRT exception: 0x" +
                 std::to_wstring(ex.code().value) + L" - " +
                 std::wstring(ex.message().c_str())).c_str());
            resultCallback->Error(
                std::to_string(ex.code().value),
                winrt::to_string(ex.message()));
        }
        catch (const std::exception& ex)
        {
            const std::string narrowMsg = ex.what();
            const std::wstring wideMsg(narrowMsg.begin(), narrowMsg.end());
            WriteToLocalAppData(
                (L"[getPurchaseId] std::exception: " + wideMsg).c_str());
            resultCallback->Error("get_purchase_id_failed", narrowMsg);
        }
        catch (...)
        {
            WriteToLocalAppData(L"[getPurchaseId] Unknown exception");
            resultCallback->Error(
                "get_purchase_id_failed",
                "Unknown error while retrieving purchase ID.");
        }

        co_return;
    }

    //////////////////////////////////////////////////////////////////////// END OF MY CODE //////////////////////////////////////////////////////////////

// static
	void FlutterWindowsIapPlugin::RegisterWithRegistrar(
		flutter::PluginRegistrarWindows* registrar) {
		_registrar = registrar;

		auto channel =
			std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
				registrar->messenger(), "com.moontechnolabs.flutter_windows_iap",
				&flutter::StandardMethodCodec::GetInstance());

		auto plugin = std::make_unique<FlutterWindowsIapPlugin>();

		channel->SetMethodCallHandler(
			[plugin_pointer = plugin.get()](const auto& call, auto result) {
			plugin_pointer->HandleMethodCall(call, std::move(result));
		});

		registrar->AddPlugin(std::move(plugin));
	}

	FlutterWindowsIapPlugin::FlutterWindowsIapPlugin() {}

	FlutterWindowsIapPlugin::~FlutterWindowsIapPlugin() {}

	void FlutterWindowsIapPlugin::HandleMethodCall(
		const flutter::MethodCall<flutter::EncodableValue>& method_call,
		std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
		if (method_call.method_name().compare("makePurchase") == 0) {
			auto args = std::get<flutter::EncodableMap>(*method_call.arguments());
			auto storeId = std::get<std::string>(args[flutter::EncodableValue("storeId")]);
			makePurchase(to_hstring(storeId), std::move(result));
		}
		else if (method_call.method_name().compare("getProducts") == 0) {
            auto args = std::get<flutter::EncodableMap>(*method_call.arguments());
            auto storeId = std::get<std::string>(args[flutter::EncodableValue("storeId")]);
			getProducts(to_hstring(storeId), std::move(result));
		}
		else if (method_call.method_name().compare("checkPurchase") == 0) {
			auto args = std::get<flutter::EncodableMap>(*method_call.arguments());
			auto storeId = std::get<std::string>(args[flutter::EncodableValue("storeId")]);
			checkPurchase(storeId, std::move(result));
		}
		else if (method_call.method_name().compare("getAddonLicenses") == 0) {
			getAddonLicenses(std::move(result));
		}
        else if (method_call.method_name().compare("getPurchaseId") == 0) {
            auto args = std::get<flutter::EncodableMap>(*method_call.arguments());
            auto userId = std::get<std::string>(args[flutter::EncodableValue("userId")]);
            const OneStoreOAuthConfig oauthConfig = ReadOneStoreOAuthConfig(args);
            getPurchaseId(to_hstring(userId), oauthConfig, std::move(result));
		}
		else {
			result->NotImplemented();
		}
	}

}  // namespace flutter_windows_iap
