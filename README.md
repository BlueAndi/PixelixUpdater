# PixelixUpdater <!-- omit in toc -->

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](http://choosealicense.com/licenses/mit/)
[![Repo Status](https://www.repostatus.org/badges/latest/wip.svg)](https://www.repostatus.org/#wip)
[![Release](https://img.shields.io/github/release/BlueAndi/PixelixUpdater.svg)](https://github.com/BlueAndi/PixelixUpdater/releases)
[![Build Status](https://github.com/BlueAndi/PixelixUpdater/actions/workflows/main.yml/badge.svg)](https://github.com/BlueAndi/PixelixUpdater/actions/workflows/main.yml)

The Pixelix updater application is flashed to a factory partition and provides the update possibility for Pixelix.

## Table of Content <!-- omit in toc -->

- [Motivation](#motivation)
- [How It Works](#how-it-works)
- [Used Libraries](#used-libraries)
- [Issues, Ideas And Bugs](#issues-ideas-and-bugs)
- [License](#license)
- [Contribution](#contribution)

## Motivation

Pixelix got over time more and more features. Even more as fit on a 4 MB flash development board. Initially, some features were disabled to save space. But meanwhile with Arduino 3.x the amount of required flash space with OTA support wasn't enough anymore.

The idea raised up to use just one application partition and a small factory partition. This way the application partition size increases and gives the possibility to continue with Arduino 3.x and to support more features again.

## How It Works

Pixelix is flashed to the app partition. Und normal circumstances this partition is active. However, if an OTA update needs to be done the user can trigger a switch to the factory partition via the webinterface. The device will then reboot and start in the factory partition. After a few seconds the website will reload and change to the webinterface of the PixelixUpdater. The user can now upload their binaries. If the upload finishes successfully the device will reboot and start again in the app partition.

## Used Libraries

| Library | Description | License |
| - | - | - |
| [Arduino](https://github.com/platformio/platform-espressif32) | ESP32 Arduino framework v2.x.x | Apache-2.0 |
| [PlatformIO](https://platformio.org) | PlatformIO is a cross-platform, cross-architecture, multiple framework, professional tool for embedded systems engineers and for software developers who write applications for embedded products. | Apache-2.0 |
| [Bootstrap](https://getbootstrap.com/) | CSS Framework | MIT |
| [POPPER JS](https://popper.js.org/) | POPPER JS | MIT |
| [jQuery](https://jquery.com/) | Javascript librariy for DOM handling | MIT |

## Issues, Ideas And Bugs

If you have further ideas or you found some bugs, great! Create a [issue](https://github.com/BlueAndi/PixelixUpdater/issues) or if you are able and willing to fix it by yourself, clone the repository and create a pull request.

## License

The whole source code is published under the [MIT license](http://choosealicense.com/licenses/mit/).
Consider the different licenses of the used third party libraries too!

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in the work by you, shall be licensed as above, without any
additional terms or conditions.
