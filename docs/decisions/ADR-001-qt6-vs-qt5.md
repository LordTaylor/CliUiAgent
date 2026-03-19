# ADR-001: Qt6 zamiast Qt5

**Status:** Zaakceptowane

## Kontekst

Projekt wymaga nowoczesnego API audio (QAudioSource/QAudioSink) oraz dobrego wsparcia dla SVG i HiDPI.

## Decyzja

Używamy Qt6.

## Powód

- Qt6 Multimedia ma ujednolicone API audio (zastąpiło QAudioInput/QAudioOutput z Qt5)
- Lepsze wsparcie dla M1/M2 (arm64) przez Qt6
- Qt5 osiągnie EOL, Qt6 jest aktywnie rozwijane
- `qt_add_resources` w CMake jest czystsze niż stare `QT5_ADD_RESOURCES`
