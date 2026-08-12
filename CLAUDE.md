# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ESP32 firmware that lives in the **factory** partition and does nothing but update the application in the `app` partition (normally Pixelix) over WiFi via a web UI / REST API. Instead of Espressif's two-OTA-partition layout, the flash holds one small factory app (this project) plus one large app partition, so the application gets more space.

Boot flow: app partition is active → the app requests an OTA update and sets the factory partition as next boot target → device reboots into PixelixUpdater → user uploads firmware/filesystem binaries → `/activateAppPartition` sets `app` (ota_0) as boot partition and restarts.

Two **independent** PlatformIO projects live here:

- Repo root — the PixelixUpdater factory firmware.
- [example/SimpleApp/](example/SimpleApp/) — a minimal app-partition demo standing in for Pixelix. Open this folder as its own VSCode/PlatformIO workspace; its `platformio.ini` references the shared [partition/](partition/) tables and [script/](script/) via `../../`.

## Commands

All from the respective project root (PlatformIO CLI; `pio` is a synonym for `platformio`).

```bash
platformio run                                    # build the default_envs env from [platformio]
platformio run --environment esp32doit-devkit-v1-factory
platformio run --target upload --environment <env>
platformio device monitor                         # 115200 baud, esp32_exception_decoder filter
platformio check --environment <env> --fail-on-defect=medium --fail-on-defect=high   # clang-tidy
platformio run --target clean --environment <env>
```

To switch the default target board, move the comment marker in the `default_envs` block at the top of `platformio.ini` — that block is the list of supported boards.

Formatting is owned by `clang-format` (v18.1.3, [.clang-format](.clang-format)) and applies to `src/` and `lib/`:

```bash
clang-format --dry-run -Werror <file>   # must be silent before declaring done
```

Docs: run Doxygen from [doc/doxygen/](doc/doxygen/) (`Doxyfile` pulls in `../../src`, `../../lib`, `../../include`); warnings land in `doxygen_warnings.txt`. Output `doc/doxygen/html` is gitignored.

There are **no unit tests** — `test/` holds only the PlatformIO placeholder README. Verification is build + `platformio check` + on-target testing with SimpleApp.

## Flashing model (the part that surprises people)

- **Root project** builds *into* the factory partition: `board_build.app_partition_name = factory`, `board_upload.offset_address = 0x10000`, and a custom `upload_command` that calls `esptool write_flash 0x10000 <bin>` directly — deliberately **not** touching bootloader, partition table, NVS or data. PixelixUpdater cannot be updated via OTA; it is always flashed over serial.
- **SimpleApp** builds into the `app` partition (`0xE0000`) and, on `upload` only, [script/add_factory_to_extra_images.py](script/add_factory_to_extra_images.py) appends the matching prebuilt `factory_binaries/<env>-factory.bin` to `FLASH_EXTRA_IMAGES` at the `factory` offset read from the partition CSV — so one Upload writes both images. Pixelix has the same `factory_binaries/` mechanism.
- [script/rename.py](script/rename.py) sets `PROGNAME` to the env name, so artifacts are `.pio/build/<env>/<env>.bin` — that name is what CI uploads and what the release ZIP (and therefore `factory_binaries/`) contains.
- [script/merge_factory.py](script/merge_factory.py) and [script/flash_factory.py](script/flash_factory.py) are alternative approaches and are currently **not** referenced by any `platformio.ini`.

Partition tables: [partition/4MB.csv](partition/4MB.csv), [8MB.csv](partition/8MB.csv), [16MB.csv](partition/16MB.csv). `factory` is fixed at `0x10000`/`0xD0000` and `app` at `0xE0000` in all three; only `app`/`spiffs` sizes differ. On 16 MB the layout stops below `0x1000000` on purpose — crossing that border caused an infinite boot loop.

## Web UI is compiled into the firmware

The factory app has no filesystem of its own to serve from. [script/embed.py](script/embed.py) runs as a pre-build step, gzips everything in [embed/](embed/) (html/css/js/svg; images stay raw), and generates `src/generated/*.{h,cpp}` — one module per file plus an `EmbeddedFiles` index whose `EmbeddedFiles_setup(WebServer&)` registers a GET route per file with the right MIME type and `Content-Encoding: gzip`. `src/generated/` is gitignored; never edit it, edit `embed/` instead. Regeneration is mtime-based per file, so a stale generated file usually means touching the source in `embed/` or deleting `src/generated/`.

`example/SimpleApp/data/` contains **copies** of the same `utils.js`, `dialog.js`, Bootstrap and stylesheet files, served from LittleFS there. They are byte-identical today and have to be kept in sync by hand.

## Firmware architecture

- [src/main.cpp](src/main.cpp) — `setup()` logs the target/version/flash info, reads the hostname from Settings (with `-<chipId>` suffix derived from the eFuse MAC), starts WiFi, then `MyWebServer::begin()`. `loop()` drives a flat state machine (`STATE_INIT` → STA setup/connecting/connected, falling back to `STATE_AP_SETUP`/`AP_UP` when no SSID is stored or the connect times out). AP mode also runs a wildcard `DNSServer` with `DNSReplyCode::NoError` so mobile clients open the captive portal; the AP IP is deliberately `192.169.4.1` (public range) for the same reason.
- [src/MyWebServer.cpp](src/MyWebServer.cpp) — namespace-style module over a file-static `WebServer` on port 80. Every handler starts with `requireAuthentication()` (HTTP basic auth, credentials from Settings) and answers through `sendSuccessResponse` / `sendSuccessPayloadResponse` / `sendErrorResponse`, which produce the `{"data":{...}}` / `{"error":{"code","message"}}` envelope documented in [README.md](README.md). When adding an endpoint: register it in `MyWebServer::begin()` **and**, if it reads a custom request header, add that header to the `collectHeaders()` array — `WebServer` discards any header not listed there.
- [src/BootPartition.cpp](src/BootPartition.cpp) — `setApp0()` (via `esp_ota_set_boot_partition`) and `isFsMountable()`. `/activateAppPartition` only checks mountability when the filesystem was actually just updated (`gIsFsUpdated`): the LittleFS implementation differs between Arduino 2.x and Tasmota Arduino 3.x, so an image written by the app may be unmountable by the factory app. SimpleApp mirrors this file with a `setFactory()` counterpart.
- [src/MiniTerminal.cpp](src/MiniTerminal.cpp) — serial CLI driven by a static `m_cmdTable`; add a command by extending that table. Commands are listed in the README.
- [lib/Settings/](lib/Settings/) — `Settings` singleton over `Preferences` (NVS) exposing `KeyValueString` entries (WiFi STA/AP SSID + passphrase, web login user/password, hostname). Always the same pattern: `open(true)` → read `getValue()`, or fall back to `getDefault()` if opening failed → `close()`.

## Conventions

C/C++ in `src/`, `lib/` and `test/` follows the house embedded C++14 / MISRA-oriented style — Yoda conditions, exactly one `return` per function, mandatory Doxygen (`@file`/`@brief`/`@author`, `@addtogroup` in headers, `/**< ... */` on members), fixed 80-column section banners in a fixed order, fixed-width types with `U` suffixes. The `embedded-cpp14-misra` skill carries the full rules and file templates; invoke it before writing or reviewing firmware code. Doxygen groups in use are declared in [doc/doxygen/mainpage.dox](doc/doxygen/mainpage.dox) (`GENERATED`, `SETTINGS`, `UTILITIES`). Python build scripts use the same `####` section-banner layout and are pylint-clean.

The firmware version is a build flag: `-D VERSION=\"x.y.z\"` in [platformio.ini](platformio.ini).

## Adding a board

1. New `[env:<board>-factory]` in [platformio.ini](platformio.ini) plus a `default_envs` comment line.
2. Add the env name to **all three** CI matrices: `build` and `check` in [.github/workflows/build.yml](.github/workflows/build.yml), and `build` in [.github/workflows/release.yml](.github/workflows/release.yml).
3. Mirror it as `[env:<board>-app]` in [example/SimpleApp/platformio.ini](example/SimpleApp/platformio.ini) with `custom_factory_binary = factory_binaries/<board>-factory.bin`, and drop that binary into `example/SimpleApp/factory_binaries/`.

Releases are cut manually: run the `release` workflow with a tag name; it builds every env and attaches `PixelixUpdater-<tag>.zip` containing all `<env>.bin` files.
