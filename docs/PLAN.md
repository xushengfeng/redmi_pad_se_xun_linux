# Port plan

> The port runs: [`BUILD.md`](BUILD.md) has the build and install steps and
> the README says what works. This plan records the scope and the open items.

## 1. Goal and scope

Bring up native mainline Linux on the Xiaomi Redmi Pad SE 11" (`xun`,
SM6225 / Snapdragon 680), with postmarketOS as the reference distribution.

Minimum viable port:

- boots from the existing Android boot chain without opening the device;
- display, touch, UFS, microSD, USB, buttons, battery/charging, Wi-Fi, BT;
- suspend/resume and thermal behaviour good enough for daily use.

Later work:

- speakers/headphones/microphones, GPU acceleration, cameras, sensors,
  USB-C DisplayPort, installation/updates, dual-boot with Android.

Out of scope initially:

- modem/telephony (the tablet is Wi-Fi-only);
- stylus support (not a feature of this model);
- upstreaming every vendor driver before the device is usable.

## 2. Decisions

| Topic | Decision | Reason |
| --- | --- | --- |
| Kernel base | `sm6225-mainline` Linux 7.2 fork (`stable-sm6225` / `stable-sm6225-tapas`) | Already contains SM6225 display, GPU, UFS, USB, WCN3990, LPASS, remoteproc and the closest board ports |
| DTS base | `sm6225-xiaomi-tapas.dts` + xun stock DT overlay | Tapas is the same SoC/PMIC family; the stock overlay has the exact xun wiring |
| Reference distro | postmarketOS edge | Existing `device-xiaomi-spes` package and SM6225 kernel/firmware packaging to copy |
| Bring-up boot | v4 boot image with a small shim (tapas method) | ABL hands the kernel the stock DTB/vendor ramdisk; the shim supplies our DTB and starts the real kernel |
| Production boot | U-Boot + EFI/extlinux | U-Boot fixes up the memory node, gives a boot menu/serial console and avoids per-RAM-size shim images |
| Android coexistence | Keep one Android slot untouched | Safer recovery and easier comparison/debugging |
| Driver policy | Mainline driver first, port vendor driver only when needed | Keeps the port maintainable and upstreamable |

## 3. Current state

### Available

- SM6225 SoC support in the 7.2 `sm6225-mainline` tree: display (DPU/DSI/DSC),
  Adreno 610/GPUCC, UFS, USB, SDHCI, GENI I2C/SPI/UART, WCN3990 Wi-Fi/BT,
  LPASS/q6apm audio, camera subsystem, ADSP/CDSP/MPSS remoteprocs.
- Working board ports for `spes` (Redmi Note 11) and a bring-up tree for
  `tapas` (Redmi Note 12 4G), including a v4 boot shim and USB-serial initramfs.
- Full stock xun device tree recovered from the partition dump, including the
  xun DTBO overlay (board ID `0x20022`, MSM ID `0x206`).
- Vendor module sources for the pieces that are not upstream: Focaltech and
  Novatek SPI touch, `SH366101` fuel gauge, `BQ2589X` charger, `SC2150` TCPC,
  `FS1815`/`SIA81xx` audio amplifiers, `KTZ8866` backlight.
- Mainline FT8203 SPI touch drivers and a NT36532E driver from other devices.
- U-Boot SM6225 support with a `xiaomi_spes` defconfig, EFI loader, fastboot,
  USB and UFS.

### Missing

- `sm6225-xiaomi-xun` DTS and a U-Boot xun DTS/defconfig.
- M84 panel drivers (three variants), plus a decision on runtime panel
  detection versus per-panel DTBs.
- Mainline FT8203/NT36532 SPI touch probe.
- Mainline `SH366101`, `SC2150`, `FS1815`, `KTZ8866` glue.
- The stock `firmware` partition (WCN3990 board data, ADSP/CDSP/modem/IPA).
- A tested v4 boot path for xun and a pmOS device/firmware package.

## 4. Boot architecture

### 4.1 Stock boot flow

```
XBL / ABL
  ├── boot.img          header v4, kernel only (~47 MB)
  ├── init_boot.img     generic ramdisk (header v4)
  ├── vendor_boot.img   header v4, vendor ramdisk + DTB + vendor cmdline
  └── dtbo.img          overlays; xun overlay board-id 0x20022
        ↓
Android GKI kernel
```

Partition facts from the stock unit:

| Item | Value |
| --- | --- |
| Boot header | v4 |
| `boot` size | 100663296 bytes |
| `init_boot` size | 8388608 bytes |
| `vendor_boot` size | 100663296 bytes |
| `dtbo` size | 8388608 bytes |
| `super` size | 6979321856 bytes |
| Slot layout | A/B, dynamic partitions in `super` |
| Stock vendor cmdline | `video=vfb:640x400,bpp=32,memsize=3072000 bootconfig` |

The DTBO overlay for xun declares `qcom,msm-id = <518 0x10000>` (`0x206`),
`qcom,board-id = <0x20022 0>`, `qcom,pmic-id = <0x2d 0 0 0>`.

### 4.2 Bring-up: v4 boot image with a shim

Tapas proved that `fastboot boot` works with a v4 image on this bootloader
family, but ABL passes the DTB from `vendor_boot` (stock, non-mainline) and
does not fill in the memory node for a foreign DTB.

Approach:

1. Build the real kernel with a built-in initramfs (USB ACM console,
   busybox, ramoops).
2. Put a tiny position-independent shim in front of it. The shim:
   - receives ABL's DTB and ignores it;
   - copies the real kernel to a safe RAM bank;
   - starts the kernel with our compiled DTB and simple-framebuffer.
3. Pack as a v4 boot image. Boot with `fastboot boot`; nothing is written.
4. Keep separate memory layouts for 4/6/8 GB units, or use the U-Boot path
   below once it works.

This is a development vehicle, not the final installation method.

### 4.3 Production: U-Boot + EFI

The SM6225 U-Boot branch already provides:

- `CONFIG_EFI_LOADER`, bootstd, extlinux and EFI boot manager;
- fastboot, USB mass storage, USB serial gadget, UFS and SD;
- display output through the bootloader framebuffer;
- `ARCH_FIXUP_FDT_MEMORY`, which fixes the RAM node for every capacity.

Plan:

1. Add `dts/upstream/src/arm64/qcom/sm6225-xiaomi-xun.dts` and a
   `xiaomi_xun_defconfig` based on the `spes` port.
2. Build U-Boot with `qcom_defconfig qcom-phone.config`.
3. `fastboot boot u-boot.img` during development; later flash it to `boot`
   or keep it on an SD card.
4. Put an EFI system partition or an extlinux `/boot` on UFS/SD, then boot the
   mainline kernel with the xun DTB.
5. Use the U-Boot fastboot/UMS menu for recovery; no Android reflash needed
   to get back.

## 5. Work breakdown

### Phase 0 — Prerequisites, recovery, live data (0.5–1 week)

- [ ] Unlock the bootloader (Xiaomi account/region rules; data wipe).
- [ ] Obtain an EDL loader/test point procedure for `xun` before flashing.
- [ ] Download the official fastboot ROM (or use the stock dump) to extract the
      `firmware` partition, `vendor_boot`/`boot` images and the vendor module
      inventory. Treat stock `.ko` files as reference only; they are Android
      GKI 5.15 modules and cannot be loaded into mainline.
- [ ] Install `adb`, `fastboot`, `dtc`, `mkbootimg`/`unpack_bootimg`,
      `clang`, `lld`, `llvm`, `pmbootstrap`.
- [ ] Back up `boot`, `init_boot`, `vendor_boot`, `dtbo`, `vbmeta`, and the
      current slot's `super` metadata; keep a known-good Android image.
- [ ] Collect from the live device:
      `/sys/firmware/fdt`, `dmesg`, `/proc/config.gz`, panel ID, touch IC
      (`/proc/bus/input/devices`, `dmesg`), sensor list (`dumpsys sensorservice`),
      `fastboot getvar all`, partition table, and the `firmware` partition.
- [ ] Record the exact model, RAM/storage, panel and touch variant.

Exit: a recovery path that does not depend on the port, and a live-device data
set that fills the gaps in `docs/DATA_COLLECTION.md`.

### Phase 1 — Kernel DTS and first boot (1–2 weeks)

- [ ] Create `sm6225-xiaomi-xun.dts` from the tapas DTS and the stock xun
      merged tree.
- [ ] Port the board essentials: PM6125 regulators, UFS, SD, USB, buttons,
      UART, simple-framebuffer, ramoops, reserved-memory, `qcom,msm-id`/
      `board-id`.
- [ ] Build the 7.2 kernel with a minimal xun config fragment.
- [ ] Build the shim + v4 boot image and boot via `fastboot boot`.
- [ ] Get a USB ACM shell and readable kernel log; verify UFS and SD block
      devices.

Exit: kernel reaches userspace in the initramfs, logs over USB serial, UFS and
SD are usable.

### Phase 2 — Display, backlight, touch (2–4 weeks)

- [ ] Generate a panel driver template from the stock `m84_36_02_0a` node with
      `linux-mdss-dsi-panel-driver-generator`; use the official
      `MiCode/vendor_qcom_proprietary_display-devicetree` branch `xun-t-oss`
      as the clean source, and the stock dump for the `m84_42` variant. Adapt
      it to the mainline msm DSC API.
- [ ] Add `m84_35_03_0b` and `m84_42_03_0c` variants (DTB variants first;
      runtime detection later if a panel ID read is reliable).
- [ ] Wire `vddio`, reset GPIO 82, TE GPIO 81, 4-lane DSI, 1200×1920 @ 90 Hz,
      DSC.
- [ ] Implement brightness (DCS first, `KTZ8866` if the panel variant uses it).
- [ ] Port/probe the SPI touch:
      - Focaltech FT8203 (use the existing FT8203 SPI drivers as reference);
      - Novatek NT36532 (vendor `nt36xxx` SPI source is available).
- [ ] Map touch IRQ GPIO 80, reset GPIO 86, and the correct touch supply.

Exit: stable 1200×1920 output at 90 Hz, brightness control, multi-touch, no
garbage after suspend/resume.

### Phase 3 — Storage, USB, power, thermal (1–2 weeks)

- [ ] Root filesystem on UFS; optional SD root.
- [ ] USB gadget (ACM/RNDIS/MTP) and USB host/OTG.
- [ ] Charger `BQ2589X` (I²C 0x6A, IRQ GPIO 84) via the mainline `bq25890`
      driver plus the vendor register quirks.
- [ ] Fuel gauge `SH366101` (I²C 0x55) — port the nopmi vendor driver to
      `power_supply`.
- [ ] NOPMI charger glue (`qcom,nopmi-chg`), battery parameters, JEITA.
- [ ] USB-C `SC2150` (I²C 0x4E, IRQ GPIO 93) and `WUSB3801` (I²C 0x60).
- [ ] Thermal zones and throttling; safe charge current/voltage limits.

Exit: battery percentage, charge/discharge, USB role switch, reboot/poweroff,
no thermal shutdowns in normal use.

### Phase 4 — GPU, Wi-Fi/BT, audio (3–5 weeks)

- [ ] Adreno 610 with `a610_zap` firmware from the stock vendor image.
- [ ] WCN3990 Wi-Fi: `ath10k` + `qcom,wcn3990-wifi`, board data and
      calibration from the stock `firmware` partition.
- [ ] WCN3990 Bluetooth over UART (`hci_qca`).
- [ ] Audio: `WCD937x` codec + ADSP/q6apm + `FS1815` ×2 speaker amplifiers
      (I²C 0x34/0x35, reset GPIO 106); add topology files and UCM.
- [ ] Headphones, speaker, microphones, jack detection.

Exit: GPU-accelerated desktop, Wi-Fi/BT, speakers and headphones work; audio
does not break suspend.

### Phase 5 — Distribution integration (1–2 weeks)

- [ ] `device-xiaomi-xun` in a pmaports fork: deviceinfo, kernel cmdline,
      firmware dependency, U-Boot/boot-deploy, dynamic partitions.
- [ ] `firmware-xiaomi-xun` package built from the stock `firmware`/`vendor`
      partitions (licensed as proprietary, not committed to this repo).
- [ ] Kernel package config fragment and DTS installation.
- [ ] Installer/recovery flow; A/B slot handling; keep Android.
- [ ] CI builds of kernel, U-Boot, boot images and pmOS image.

Exit: clean installation from a host, repeatable image builds, documented
rollback.

### Phase 6 — Polish (3–6 weeks, parallel where possible)

- [ ] Accelerometer/gyro/light/proximity (SSC or direct IIO, depending on
      wiring), hall cover sensor.
- [ ] Cameras: GC08A3 rear (8 MP), GC05A2 front (5 MP), HI556 variant.
- [ ] USB-C DisplayPort alt mode (hardware appears to support it).
- [ ] Suspend/resume, runtime PM, battery life, thermal tuning.
- [ ] Upstream DTS/drivers where practical.

## 6. Deliverables and proposed repository layout

```
.
├── docs/                  # this planning and hardware data
├── kernel/                # DTS + patches against sm6225-mainline
├── u-boot/                # xun DTS + defconfig/patches
├── firmware/              # extraction scripts only; no proprietary blobs
├── boot/                  # v4 shim, initramfs, image packaging
├── pmaports/              # device-xiaomi-xun (+ kernel/firmware packages)
├── ci/                    # reproducible builds
└── scripts/               # data extraction and helper tooling
```

## 7. Risks and mitigations

| Risk | Impact | Mitigation |
| --- | --- | --- |
| Bootloader unlock not possible / account restrictions | Blocks everything | Confirm before buying/erasing; keep a second device or Android slot |
| Wrong panel/touch variant | Black screen/no touch | Build three DTBs; collect panel ID from stock; probe both touch ICs |
| ABL v4 DTB/vendor_boot behaviour | No boot or wrong RAM | Use the tapas shim first; use U-Boot for memory fixup |
| No stock `firmware` partition | No Wi-Fi/BT/ADSP/audio | Dump it from the device or a full fastboot ROM before wiping |
| Vendor drivers are GPL but messy | Porting time | Port only the minimum; prefer mainline subsystems |
| Battery/charging mistakes | Hardware damage | Validate against stock limits; start with low current; keep charging off until proven |
| Mainline SM6225 display/audio still young | Bring-up stalls | Use framebuffer + USB serial first; compare with spes/tapas |
| U-Boot port is new | Debugging hard | Keep the shim path; U-Boot is optional until it works |

## 8. Rough effort

| Scope | Estimate |
| --- | --- |
| Boot to initramfs + UFS/SD/USB | 1–3 person-weeks after unlock |
| Display + touch + brightness | +2–4 person-weeks |
| Charging + fuel gauge + USB-C | +1–2 person-weeks |
| Wi-Fi/BT + GPU | +1–2 person-weeks |
| Audio (speakers/mics) | +2–4 person-weeks |
| pmOS installer/CI | +1–2 person-weeks |
| Sensors/cameras/polish | +3–6 person-weeks |

A usable tablet port (boot, display, touch, storage, USB, charging, Wi-Fi) is
roughly **6–10 person-weeks** of focused work, assuming the device and the
missing firmware are available. Full daily-driver quality is a multi-month
effort.

## 9. Open questions

1. Is the bootloader already unlocked, and is the device available for
   repeated `fastboot boot` tests and a data wipe?
2. Which panel/touch/RAM variant is the target unit?
3. Is a stock `firmware` partition dump (or full fastboot ROM) available?
4. Target distribution: postmarketOS only, or also Arch/Debian/NixOS?
5. Is dual-boot with Android required, or may one slot be reused?
6. Is UART/EDL access available for recovery?
7. Is the goal an upstream-quality port, or a locally usable device first?
