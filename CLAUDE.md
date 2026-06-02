# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ElegantOTA is an Arduino library for Over-The-Air (OTA) firmware updates with a web-based UI. It targets ESP8266, ESP32, RP2040, and RP2350 microcontrollers. This fork (by MiguelAguirre1202/Gatria SAS) extends the original with a modem configuration portal and custom industrial UI.

## Build System

This project uses **PlatformIO**. The `platformio.ini` sets `lib_dir = .` (the repo root acts as the library) and `src_dir = examples/Demo` (or `examples/AsyncDemo`).

```bash
# Build
pio build -e esp32      # ESP32-S3
pio build -e esp8266    # ESP8266
pio build -e picow      # Raspberry Pi Pico W
pio build -e pico2w     # Raspberry Pi Pico 2W

# Build and upload
pio run -e esp32 -t upload

# Monitor serial output
pio device monitor
```

To switch between sync and async webserver examples, edit `src_dir` in `platformio.ini`.

## Key Source Files

| File | Purpose |
|------|---------|
| `src/ServidorWeb.h` | Public API — class declaration, platform macros, mode enums |
| `src/ServidorWeb.cpp` | Core implementation — HTTP route handlers, OTA update logic |
| `src/elop.h` | Declares the embedded HTML byte arrays (`ELEGANT_HTML`, `CONFIG_MODEM_HTML`) |
| `src/elop.cpp` | Defines the actual compressed HTML content as `uint8_t` arrays |

## Embedded HTML Resources

The web UIs are stored as **gzip-compressed byte arrays** in `src/elop.cpp` and declared in `src/elop.h`. They are served with `Content-Encoding: gzip`.

- `ELEGANT_HTML[84912]` → served at `GET /update` (OTA update portal)
- `CONFIG_MODEM_HTML[56394]` → served at `GET /modemConfiguration` (modem config page)

**When updating an HTML file:** After editing `.html` files in `src/`, you must:
1. Gzip-compress the HTML file
2. Convert it to a C byte array
3. Replace the corresponding array in `src/elop.cpp`
4. Update the array size constant in `src/elop.h` to match the new byte count

The source HTML files are:
- `src/FocusLiteModem_WebUI_and_OTA.html` — generates `ELEGANT_HTML`
- `src/ConfiguracionModem_WebUI_OTA.html` — generates `CONFIG_MODEM_HTML`

## Architecture

**Dual webserver support** is controlled by `#define ELEGANTOTA_USE_ASYNC_WEBSERVER`:
- `0` (default): synchronous `WebServer` / `ESP8266WebServer`
- `1`: `ESPAsyncWebServer` — required for `AsyncDemo` example

**Platform abstraction** uses preprocessor guards (`#if defined(TARGET_RP2040)`, `ESP8266`, `ESP32`) throughout `ServidorWeb.cpp` for filesystem selection (LittleFS vs SPIFFS) and OTA partition types.

**OTA flow:**
1. `GET /ota/start?mode=firmware|fs` — initializes `Update` with MD5 hash
2. `POST /ota/upload` — receives chunked binary, calls `Update.write()`
3. On completion: fires `onEnd` callback, optionally auto-reboots (via `loop()`)

**Callback API:**
```cpp
ElegantOTA.begin(&server);           // registers all routes
ElegantOTA.onStart([](){});          // called before OTA begins
ElegantOTA.onProgress([](size_t cur, size_t total){});
ElegantOTA.onEnd([](bool success){}); // called after OTA completes
ElegantOTA.loop();                   // must be called in loop() for reboot handling
```

## Web UI Technology

The HTML interfaces use **Tailwind CSS** (minified, embedded inline) with Gatria SAS branding (`#2666af`). The UIs communicate with the device via `fetch()` API calls to the firmware endpoints.
