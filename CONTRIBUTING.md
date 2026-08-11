# Contributing

Thanks for your interest in `flutter_windows_iap`.

## Development setup

1. Clone this repository.
2. From the package root:

```bash
flutter pub get
cd example
flutter pub get
```

3. Run the example on Windows:

```bash
flutter run -d windows
```

## Before opening a pull request

- Run `flutter analyze` from the package root.
- Run `flutter test`.
- Update `CHANGELOG.md` if your change affects published behavior.
- Do not commit secrets (Store IDs, Azure `ClientSecret`, etc.). Use placeholders in samples.

## Publishing notes (maintainers)

1. Bump `version` in `pubspec.yaml`.
2. Add a matching section in `CHANGELOG.md`.
3. Dry-run:

```bash
dart pub publish --dry-run
```

4. Publish:

```bash
dart pub publish
```
