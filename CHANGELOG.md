# Changelog - PandaTouch StreamDeck (Fork)

All improvements and changes made in this enhanced version.

## [v1.5.1] - 2026-01-31
### Bug Fixes
- **Image Restore**: Fixed corruption issue when restoring custom images from backup thanks to a more robust Base64 decoder.

## [v1.5.0] - 2026-01-31
### Bug Fixes
- **Definitive Hybrid Fix**: Successfully matched the original BTT firmware's 80MHz QIO/OPI performance.
- **Boot Stability**: Using DIO mode for the bootloader header to ensure 100% reliable starts on all hardware units (fixes `ets_loader.c 78`).
- **Screen Performance**: Fixed interference and artifacts by correctly mapping the Octal PSRAM at 80MHz with the `qio_opi` memory type.
- **Size Unification**: Locked 16MB flash size across all build and release stages.

## [v1.4.0] - 2026-01-30
### New Features
- **Multi-language Support (EN/ES)**: Full localization system for both the web dashboard and the on-device display. Toggle between English and Spanish seamlessly.
- **Unified Backup & Restore**: The backup system now includes all custom images from LittleFS as Base64 encoded strings within a single JSON file. One-click solution to restore your entire setup, including graphics.
- **Improved Web Dashboard**: Added descriptive labels for "Basic Combo" and localized all interface elements, including icon names and tooltips.

### Improvements & Fixes
- **UI Clarification**: Renamed "Combo Básico" and added helpful hints in the web UI for better user guidance.
- **Standardized UI**: Improved consistency of button sizes and labels across different languages on the device screen.
- **Localized OTA Updates**: Firmware update progress messages are now shown in the user's selected language.

## [v1.3.3] - 2026-01-29
### Critical Fixes
- **Fix WiFi Crash**: Fixed crash during WiFi initialization (TCP/IP stack) that caused reboots on fresh installations.
- **Fix Watchdog Bootloop**: Added a delay in the main loop to prevent watchdog timer resets.

## [v1.3.2] - 2026-01-29
### Improvements and Fixes
- **Factory Binary**: Automatic generation of `factory.bin` in releases for initial installations without bootloops.
- **Partition Table**: New `partitions_custom.csv` optimized for OTA with 3MB per app slot.
- **Documentation**: Updated README with clear instructions on when to use `factory.bin` vs `firmware.bin`.
- **CI/CD**: GitHub Actions now automatically builds and publishes both binaries.

## [v1.3.1] - 2026-01-29
### New Features
- **Enhanced OTA Update**: New persistent update interface on the device that eliminates flickering, shows a real progress bar, and prevents text overlapping.
- **Advanced Shortcuts (Combos)**: Full support for `CTRL+SHIFT+ALT+Key` combinations.
- **Visual Shortcut Builder**: New panel in the web dashboard to create combos by checking boxes, without manual text entry.
- **Media Controls**: Added `next`, `prev`, and `stop` commands.
- **Visual Icons**: Icon selectors on web and device now show the graphic symbol next to the name.

### Improvements and Fixes
- **Fix Ghost Shift**: Fixed an issue where the HID library would automatically send `Shift` when uppercase characters were detected.
- **Security**: System files `win_btns.bin` and `mac_btns.bin` are now hidden and protected against accidental deletion.
- **PlayPause Icon**: New combined ▶⏸ icon.
- **CI/CD**: Automatic GitHub release notes generation from this Changelog file.

## [v1.2.0] - 2026-01-27
### Added
- **Web OTA Update**: New interface in the control panel to upload `.bin` files and update firmware wirelessly.
- **macOS Support**: Automatic mapping of the Cmd key and Spotlight launcher (`Cmd+Space`).
- **Turbo Typing**: Reduced typing delay to 5ms for an instant "paste" effect.
- **I2C Robustness**: Validated touch coordinates to avoid ghost clicks and reduced I2C speed to 100kHz.
- **MIT License**: Added official license to the repository.

### Changed
- **Storage System**: Migration from NVS to binary files in LittleFS (`/win_btns.bin`, `/mac_btns.bin`) for unlimited profiles.
- **Configuration Interface**: Complete redesign of the button editing screen to prevent overlapping.
- **Interface Colors**: All screens now respect the global background color defined by the user.

### Fixed
- **NVS Out of Space Error**: Resolved via migration to LittleFS.
- **Ghost Clicks**: Solved with coordinate filters in the touch driver.
- **Black Screen**: Added default colors (dark grey) to avoid confusion on first boot.

---
*This fork focuses on stability and cross-platform ease of use.*
