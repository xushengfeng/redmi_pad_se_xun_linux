# Linux on the Redmi Pad SE (`xun`)

Native mainline Linux support for the Xiaomi Redmi Pad SE 11" tablet
(codename **`xun`**, Qualcomm **SM6225** / Snapdragon 680).

**Status: usable.** Mainline Linux boots on the tablet from slot B into Alpine
with GNOME on Wayland, and the kernel, the device tree and the drivers for the
board live in this repository under `build/`. Suspend/resume, the sensors and
the cameras are the main pieces still missing.

## What works

- Display: M84 panel, 1200×1920, correct orientation, 90 Hz mode list.
- Touch: FocalTech FT8203 over GENI SPI, multi-touch.
- GPU: Adreno 610 with freedreno, GNOME/mutter on Wayland.
- Wi-Fi: WCN3990 with the modem DSP serving the firmware, NetworkManager.
- Storage and I/O: eMMC rootfs, SD card, USB (gadget and host).
- Desk: GDM autologin into GNOME 50 (Wayland), Chinese locale and input
  method, Firefox, GNOME Console, lock screen with the account password.
- Power: the power key opens the shutdown dialog, and a shutdown powers the
  PMIC down instead of resetting the SoC.
- Audio: WCD937x behind the AudioReach ADSP, four FS1815 speaker amplifiers,
  UCM routing and PulseAudio playback.
- Bluetooth: QCA combo over UART, firmware download, scanning and pairing.
- Clock: PMIC RTC, chrony, HTTP fallback.
- Battery and charging: SH366101 fuel gauge and BQ2589X charger through
  power_supply/upower, with USB charger type detection.

## What is missing

- Suspend/resume, the sensors (accelerometer, hall) and the cameras.
- USB Power Delivery beyond 5 V, and a per-device Wi-Fi MAC address.

## Documents

| Document | Contents |
| --- | --- |
| [`docs/BUILD.md`](docs/BUILD.md) | Build it: download links and revisions, firmware sources, kernel and boot image, install, on-device layout |
| [`docs/SESSION.md`](docs/SESSION.md) | Account, display manager, lock screen, power key, and how a shutdown reaches the PMIC |
| [`docs/AUDIO.md`](docs/AUDIO.md) | Speaker path: AudioReach routing, the four FS1815 amplifiers, the UCM profile and testing |
| [`docs/BLUETOOTH.md`](docs/BLUETOOTH.md) | QCA Bluetooth over UART: firmware, the board address, the in-band sleep workaround and testing |
| [`docs/CHARGER.md`](docs/CHARGER.md) | BQ2589X charger, battery state on AC, USB type detection and what upower sees |
| [`docs/DEBUG.md`](docs/DEBUG.md) | USB link and serial console, SSH, deploying kernel/rootfs files, fastboot/ADB, collecting state |
| [`docs/DOWNLOADS.md`](docs/DOWNLOADS.md) | Every downloaded artifact with its URL and revision, where it lands, and how to rebuild it |
| [`docs/PLAN.md`](docs/PLAN.md) | Phased port plan, boot strategy, acceptance criteria, risks, estimate |
| [`docs/BRINGUP.md`](docs/BRINGUP.md) | Unlocked-bootloader bring-up runbook: data collection, first v4 boot, minimal DTS, debug checklist |
| [`docs/HARDWARE.md`](docs/HARDWARE.md) | Device inventory from the stock device tree: display, touch, power, audio, cameras, buses, GPIOs, partitions |
| [`docs/RESOURCES.md`](docs/RESOURCES.md) | Existing kernels, ports, drivers, firmware, tooling, and what each one provides |
| [`docs/DATA_COLLECTION.md`](docs/DATA_COLLECTION.md) | How the stock DTB/DTBO was extracted and what to collect from a live device |
| [`docs/reference/`](docs/reference/) | Decompiled stock xun device-tree overlay and merged device tree, plus stock boot information |
| [`build/`](build/) | The port itself: DTS, panel/touch/fuel-gauge drivers, kernel config, topology, UCM, on-device files |
| [`scripts/`](scripts/) | Reproducible DTBO unpacking and overlay-merging helpers |

## Short version

What this is built on:

- The `sm6225-mainline` kernel fork (Linux 7.2) for SM6225: DPU/DSI/DSC, Adreno
  610, eMMC, USB, SD, GENI I²C/SPI/UART, WCN3990, LPASS/q6apm, remoteprocs.
- The tapas bring-up tree for the Redmi Note 12 4G for the v4 boot shim, the
  initramfs and the base kernel config.
- The stock Redmi Pad SE device tree, recovered from a full partition dump
  together with the xun DTBO overlay, for the board wiring.

What was added here:

- An `sm6225-xiaomi-xun` device tree, panel drivers for the three M84 LCD
  variants, an FT8203 SPI touch driver, an SH366101 fuel gauge driver, the
  LPASS/AudioReach audio description, the ALSA UCM profile, and the boot
  integration for the v4 Android boot header.

## Target

- Mainline-first kernel, no Android GPL drivers where an upstream driver exists.
- Reference distribution: postmarketOS edge, but the kernel/DTS work is
  distribution-independent.
- Keep the Android slot bootable while the port is developed.
- Do not require opening the device or using EDL for normal installation.
