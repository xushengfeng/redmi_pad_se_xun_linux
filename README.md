# Linux on the Redmi Pad SE (`xun`)

Native mainline Linux support for the Xiaomi Redmi Pad SE 11" tablet
(codename **`xun`**, Qualcomm **SM6225** / Snapdragon 680).

**Status: usable.** Mainline Linux boots on the tablet from slot B into Alpine
with GNOME on Wayland, and the kernel, the device tree and the drivers for the
board live in this repository under `build/`. Bluetooth, audio output and the
charger driver are still open.

## What works

- Display: M84 panel, 1200×1920, correct orientation, 90 Hz mode list.
- Touch: FocalTech FT8203 over GENI SPI, multi-touch.
- GPU: Adreno 610 with freedreno, GNOME/mutter on Wayland.
- Wi-Fi: WCN3990 with the modem DSP serving the firmware, NetworkManager.
- Storage and I/O: eMMC rootfs, SD card, USB (gadget and host).
- Desk: greetd autologin into GNOME 50, Chinese locale and input method,
  Firefox, GNOME Console.
- Clock: PMIC RTC, chrony, HTTP fallback.
- Battery: SH366101 fuel gauge through power_supply/upower.

## What is missing

- Bluetooth: the QCA firmware downloads, but the HCI controller does not
  register on the UART.
- Audio: the ADSP runs and the sound card registers; the playback routing and
  the two FS1815 speaker amplifiers are not done.
- Charger: the BQ2589X is left as the bootloader configured it; no driver.
- Afterwards: stable Wi-Fi MAC, suspend/resume, sensors, cameras.

## Documents

| Document | Contents |
| --- | --- |
| [`docs/BUILD.md`](docs/BUILD.md) | Build it: download links and revisions, firmware sources, kernel and boot image, install, on-device layout |
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
