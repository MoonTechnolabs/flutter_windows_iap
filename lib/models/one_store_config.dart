/// Azure AD / One Store OAuth configuration for
/// [FlutterWindowsIap.getPurchaseId].
///
/// [clientId], [clientSecret], and [tenantId] must be provided by the caller.
/// [resource] and [collectionsResource] fall back to built-in defaults when omitted.
class OneStoreConfig {
  const OneStoreConfig({
    this.clientId,
    this.clientSecret,
    this.tenantId,
    this.resource,
    this.collectionsResource,
  });

  static const String defaultResource = 'https://onestore.microsoft.com';
  static const String defaultCollectionsResource =
      'https://onestore.microsoft.com/b2b/keys/create/collections';

  final String? clientId;
  final String? clientSecret;
  final String? tenantId;
  final String? resource;
  final String? collectionsResource;

  /// Applies fallback defaults only for [resource] and [collectionsResource].
  OneStoreConfig resolved() {
    return OneStoreConfig(
      clientId: _trimOrNull(clientId),
      clientSecret: _trimOrNull(clientSecret),
      tenantId: _trimOrNull(tenantId),
      resource: _orDefault(resource, defaultResource),
      collectionsResource:
          _orDefault(collectionsResource, defaultCollectionsResource),
    );
  }

  Map<String, String> toMethodChannelMap() {
    final resolvedConfig = resolved();
    return <String, String>{
      if (resolvedConfig.clientId != null)
        'clientId': resolvedConfig.clientId!,
      if (resolvedConfig.clientSecret != null)
        'clientSecret': resolvedConfig.clientSecret!,
      if (resolvedConfig.tenantId != null) 'tenantId': resolvedConfig.tenantId!,
      'resource': resolvedConfig.resource!,
      'collectionsResource': resolvedConfig.collectionsResource!,
    };
  }

  static String? _trimOrNull(String? value) {
    if (value == null || value.trim().isEmpty) {
      return null;
    }
    return value.trim();
  }

  static String _orDefault(String? value, String fallback) {
    if (value == null || value.trim().isEmpty) {
      return fallback;
    }
    return value.trim();
  }
}
