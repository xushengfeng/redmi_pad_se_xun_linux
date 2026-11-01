# Existing resources

Everything below was checked while preparing this plan. Links are to the
upstream project or the most useful public mirror.

## SM6225 mainline kernel and packaging

| Resource | What it provides | Use in this project |
| --- | --- | --- |
| [`sm6225-mainline` kernel](https://gitlab.postmarketos.org/sm6225-mainline/kernels/linux-v.7.2.1) branches `stable-sm6225`, `stable-sm6225-tapas` | Linux 7.2 fork with SM6225 SoC support: DPU/DSI/DSC, Adreno 610, UFS, USB, SDHCI, GENI I²C/SPI/UART, WCN3990, LPASS/q6apm, CAMSS, remoteprocs; `sm6225.dtsi`, `sm6225-xiaomi-spesn.dts`, `sm6225-xiaomi-tapas.dts` | Primary kernel base |
| [`sm6225-mainline/pmaports`](https://gitlab.postmarketos.org/sm6225-mainline/pmaports) | postmarketOS fork with `device-xiaomi-spes`, `firmware-xiaomi-spes`, and `linux-postmarketos-qcom-sm6225` | Template for `device-xiaomi-xun` and the kernel package |
| [`firmware-xiaomi-spes`](https://gitlab.postmarketos.org/sm6225-mainline/firmwares/firmware-xiaomi-spes) | ADSP/CDSP/MPSS/IPA/Venus/WLAN-MDSP/Adreno zap firmware for SM6225 Xiaomi phones | Candidate firmware source; xun-specific calibration/board data still needed |
| [`sm6225-mainline/u-boot`](https://gitlab.postmarketos.org/sm6225-mainline/u-boot) branch `sm6225` | U-Boot 2026.07 port with SM6225 clocks/pinctrl, `xiaomi_spes_defconfig`, EFI loader, fastboot, UFS/USB, phone environment | Production bootloader path; add an xun DTS/defconfig |
| [`Sype0/pmos-tapas`](https://github.com/Sype0/pmos-tapas) | Mainline bring-up for Redmi Note 12 4G (`tapas`): v4 boot shim, USB-serial initramfs, DTS, ALSA topology scripts, build/CI scripts | Closest boot-bring-up template; reuse the shim and initramfs |
| [`SCOS100/sm6225-mainline-linux`](https://github.com/SCOS100/sm6225-mainline-linux) | Older SM6225 mainline tree (v6.19-rc8) with `sm6225.dtsi`, `sm6225-xiaomi-spes.dts`, Lenovo TB128FU DTS | Historical reference; the 7.2 fork is newer |
| [`rhode-on-mainline/linux`](https://github.com/rhode-on-mainline/linux) and related repos | Mainline port for a Motorola SM6225 device | Second reference for SM6225 board bring-up |

## Stock device data and vendor sources

| Resource | What it provides | Use in this project |
| --- | --- | --- |
| [`gm-stuffs/redmi_xun_dump`](https://github.com/gm-stuffs/redmi_xun_dump) | Full stock partition dump: `boot.img`, `init_boot.img`, `vendor_boot.img`, `dtbo.img`, `vendor`, `odm`, `system*`, `mi_ext`, `modem.img`, plus the extracted xun DTBO and `aosp-device-tree` | Source of the xun device tree, boot headers, vendor configs, firmware and module list |
| [`MiCode/Xiaomi_Kernel_OpenSource`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) branch `xun-t-oss` | Xiaomi kernel source for xun (GKI/msm-kernel): xun config fragments, `bq2589x` charger, `sh366101` fuel gauge, nopmi power-supply code, module lists | Vendor driver source for the power path |
| [`MiCode/vendor_qcom_proprietary_display-devicetree`](https://github.com/MiCode/vendor_qcom_proprietary_display-devicetree) branch `xun-t-oss` | Official display DT for xun: `xun-sde-display-idp-nopmi-overlay.dts`, `khaje-sde-display-*`, `dsi_panel_m84_35_03_0b_fhdp_video.dtsi`, `dsi_panel_m84_36_02_0a_fhdp_video.dtsi`, `touchscreen_m84.dtsi` | Cleaner panel/touch source than the decompiled stock DT |
| [`MiCode/vendor_qcom_opensource_camera-kernel`](https://github.com/MiCode/vendor_qcom_opensource_camera-kernel) branch `xun-t-oss` | Camera kernel modules for xun | Camera bring-up reference |
| [`MiCode/vendor_qcom_opensource_wlan`](https://github.com/MiCode/vendor_qcom_opensource_wlan) branch `xun-t-oss` | WLAN driver source for xun | Reference; mainline `ath10k` is preferred |
| [`Xiaomi-Redmi-Pad-SE-Resources/android_kernel_xiaomi_sm6225-modules`](https://github.com/Xiaomi-Redmi-Pad-SE-Resources/android_kernel_xiaomi_sm6225-modules) | Vendor module source: `qcom/opensource/touch-drivers` (focaltech, `nt36xxx` SPI), `audio-kernel` (WCD937x, FS1815, SIPA), `display-drivers` (KTZ8866), camera/WLAN modules | Driver source for all vendor-specific peripherals |
| [`Xiaomi-Redmi-Pad-SE-Resources/android_kernel_xiaomi_sm6225`](https://github.com/Xiaomi-Redmi-Pad-SE-Resources/android_kernel_xiaomi_sm6225) | Xiaomi/Linaro 5.15 kernel tree used for xun builds | Secondary vendor reference |
| [`Xiaomi-Redmi-Pad-SE-Resources/android_vendor_xiaomi_xun`](https://github.com/Xiaomi-Redmi-Pad-SE-Resources/android_vendor_xiaomi_xun) | Vendor/ODM blobs, display XML, touch firmware, sensor configs, audio policies | Firmware/config extraction |
| [`bangprovn/android_device_xiaomi_xun`](https://github.com/bangprovn/android_device_xiaomi_xun) | Android device tree: BoardConfig (partition sizes, header v4), init scripts, recovery config, properties | Partition/boot facts |
| [`Project-Silicium/Mu-Silicium`](https://github.com/Project-Silicium/Mu-Silicium) | Decompiled stock DTBs for many Qualcomm devices, including `tapas.dts` | Cross-check for the tapas board; xun overlay is in the dump instead |
| `xun` DTBO overlay (this repository, `docs/reference/`) | Decompiled xun DTBO and merged base+overlay DTS | Direct source for the xun DTS |

## Drivers and driver templates

| Resource | What it provides | Use in this project |
| --- | --- | --- |
| [`msm8916-mainline/linux-mdss-dsi-panel-driver-generator`](https://github.com/msm8916-mainline/linux-mdss-dsi-panel-driver-generator) | Generates a mainline DRM panel driver from a downstream `qcom,mdss_dsi_*` DT node | Generate M84 panel driver skeletons |
| [`Marker284/samsung-gts7xllite-display-touch`](https://github.com/Marker284/samsung-gts7xllite-display-touch) | Mainline FT8203 SPI touchscreen + panel driver for a Samsung tablet | FT8203 SPI reference |
| [`RigoLigoRLC/unixnu-nativefocaltechtouch`](https://github.com/RigoLigoRLC/unixnu-nativefocaltechtouch) | FT8203 over Qualcomm GENI SPI | GENI SPI transfer/IRQ reference |
| [`UniXNU/ShengNovatekTouch`](https://github.com/UniXNU/ShengNovatekTouch) | Novatek NT36532E touch driver | NT36532 SPI reference |
| Vendor `nt36xxx` source (in the modules repo) | Full Novatek SPI driver, firmware update, MP test | Port to mainline input subsystem |
| Vendor `focaltech_touch` source (in the modules repo) | Focaltech core, flash, ESD, gesture; I²C glue; core supports `BUS_TYPE_SPI` | Port SPI glue to mainline |
| `fm350`/`linux-mdss` panel drivers in the 7.2 tree | `panel-xiaomi-m7-38-0c-0a.c` (tapas OLED, DSC) and `panel-xiaomi-k7-...` (spes) | Style/DSC reference for M84 |
| Mainline `bq25890_charger.c` + vendor `bq2589x_charger.c` | BQ2589X register quirks | Charger driver adaptation |
| Vendor `sh366101_fg.c` | SH366101 fuel gauge | Port to `power_supply` |
| Vendor `fs1815` and `sipa` codecs | Speaker amplifier drivers | Audio bring-up |
| Vendor `ktz8866_leds_i2c.c` | KTZ8866 backlight | Backlight fallback |
| `drivers/usb/typec/wusb3801.c` in the 7.2 tree | WUSB3801 Type-C driver | USB-C role switch |

## Firmware and tooling

| Resource | What it provides | Notes |
| --- | --- | --- |
| `linux-firmware-ath10k`, `linux-firmware-qca` (Alpine/pmOS) | WCN3990 firmware and generic board files | Board calibration may still need xun data |
| `firmware-qcom-adreno-a630` (pmOS) | Adreno zap firmware | The stock xun `vendor/firmware` already has `a610_zap.*` |
| AOSP `mkbootimg` / `unpack_bootimg` | v4 boot/vendor_boot headers and image packing | Needed for the shim image and analysis |
| `dtc`/`fdtoverlay` | DTB decompile/merge | Used to produce the reference DTS |
| `pmbootstrap` | postmarketOS image build/install | Distribution integration |
| `qbootctl`, `android-tools`, `make-dynpart-mappings` | A/B slot control and dynamic partitions | Used by the pmOS `spes` package |

## Xiaomi official open-source scope

Xiaomi publishes **GPL/compliance source**, not hardware design. There are no
schematics, PCB files, BOMs, mechanical CAD, or full board bring-up documents
in the MiCode GitHub organisation.

What MiCode does publish, device-dependent:

- kernel source (`Xiaomi_Kernel_OpenSource`) for many phones/tablets;
- kernel device-tree branches (`kernel_devicetree`) for selected devices;
- selected vendor kernel modules: audio, camera, display, WLAN, data, touch
  (some repositories are only placeholders);
- display/camera devicetree repositories, sometimes with a per-device branch;
- bootloader source for a few old devices and some MIUI apps.

For `xun` specifically (checked 2026-10):

| Repository | Branch | Contents |
| --- | --- | --- |
| `MiCode/Xiaomi_Kernel_OpenSource` | `xun-t-oss` | Kernel source, xun config fragments, power-supply vendor code |
| `MiCode/vendor_qcom_proprietary_display-devicetree` | `xun-t-oss` | Official panel/touch display DT, including `dsi_panel_m84_35_03_0b`, `dsi_panel_m84_36_02_0a`, `touchscreen_m84.dtsi`, `xun-sde-display-idp-nopmi-overlay.dts` |
| `MiCode/vendor_qcom_opensource_camera-kernel` | `xun-t-oss` | Camera kernel modules |
| `MiCode/vendor_qcom_opensource_wlan` | `xun-t-oss` | WLAN driver source |
| `MiCode/kernel_devicetree` | no `xun` branch | Device-tree branch index only; xun board DTS is not published there |
| `MiCode/vendor_xiaomi_proprietary_touch-driver` | `main` | README placeholder only |
| `MiCode/device_xiaomi` | `main` | Empty placeholder |

So the accurate description is: **not hardware open source, and not only the
kernel** — Xiaomi publishes the GPL-required kernel plus a selected set of
vendor kernel modules and devicetrees. The full xun board wiring still comes
from the stock DTBO/dump, which is why the extracted reference tree in this
repository matters.

## Xiaomi-Redmi-Pad-SE-Resources (community Android org)

This organisation is an **Android/LineageOS 23.x resource collection** for
`xun` (CAF SM6225), not a Linux port. It is still very useful as a source of
vendor code, blobs, configs and boot facts.

| Repository | Contents | Relevance to mainline |
| --- | --- | --- |
| `android_device_xiaomi_xun` | LineageOS device tree, BoardConfig (header v4, A/B, prebuilt DTB/DTBO paths), init scripts, `proprietary-firmware.txt` | Boot/partition facts; not a Linux DTS |
| `android_device_xiaomi_xun-kernel` | Prebuilt kernel, `dtbs/`, `dtbo.img`, kernel headers | DTB/DTBO are generic QTI images, not xun DTS source |
| `android_kernel_xiaomi_sm6225` | Full Android 5.15 kernel tree, `xun_GKI.config`, `bengal` configs, vendor code | Reference for vendor driver behavior; no `bengal`/`khaje`/`xun` DTS (167 qcom DTS files, none relevant) |
| `android_kernel_xiaomi_sm6225-modules` | Vendor module source: touch (`focaltech_ts_spi`, `nt36xxx`), audio (WCD937x, FS1815, SIPA), display (KTZ8866), camera, WLAN | **Primary vendor driver source** for the missing peripherals |
| `android_vendor_xiaomi_xun` | Vendor/ODM blobs, display XML, touch firmware, sensor configs, audio policies | Firmware/config extraction |
| `device_xiaomi_xun-old` | Older Android device tree | Historical config reference |
| `android_device_xiaomi_xun-recovery` | TWRP/recovery tree with prebuilt kernel | Boot image TWRP reference |
| `local_manifests` | LineageOS 23.1/23.2 CAF SM6225 build manifest | Confirms the Android ROM context and repo set |
| `hardware_qcom_display`, `android_hardware_qcom_audio-ar`, `android_vendor_qcom_opensource_agm`/`arpal-lx`, `dataipa` | Android HALs/libraries | Not usable by a native Linux userspace |
| `android_device_xiaomi_sm6225-devicetrees` | Empty repository | No DTS source |
| `bug_tracker` | ROM bug tracker README | Android ROM only |

`android_device_xiaomi_xun/proprietary-firmware.txt` is useful because it
lists the stock firmware-partition images (`abl.img`, `bluetooth.img`,
`dsp.img`, `modem.img`, `tz.img`, `xbl.img`, ...) extracted from
`OS2.0.206.0.VMUMIXM`. That is the list to look for when obtaining the missing
`firmware` partition.

## Gaps not covered by existing resources

- No public xun Linux port or `device-xiaomi-xun` pmOS package was found.
- No full xun board DTS in the official `kernel_devicetree`; the official
  display DT covers panels/touch, not the whole board.
- No public xun `firmware` partition dump or WCN3990 `bdwlan` for this exact
  board.
- The SM6225 mainline display/audio work is young; the tapas README still says
  "builds, not yet booted" for that particular bring-up tree. The 7.2 fork is
  newer, but xun-specific display/audio have not been tested by anyone public.
- The U-Boot SM6225 port has only a `spes` defconfig; xun needs its own DTS and
  validation.
- No mainline `SH366101`, `SC2150`, `FS1815`, or `KTZ8866` drivers.
- No confirmed panel/touch runtime detection; per-variant DTBs are the safe
  initial approach.

## What can be reused from an official ROM/firmware package

Yes, an official package is useful, but not as a source of drop-in kernel
drivers for mainline. Separate the contents by type:

| Item | Reusable for mainline? | Notes |
| --- | --- | --- |
| Firmware blobs in `firmware`, `vendor/firmware`, `odm/firmware` (`*.mbn`, `*.bin`, `*.fsm`, `bdwlan`, `firmware-5.bin`, touch FW) | **Yes** | Loaded by mainline drivers after the driver exists. Keep the vendor/license terms. |
| Device tree / DTBO / panel init sequences / touch config | **Yes** | Already extracted from the stock dump. |
| Android kernel modules (`vendor_dlkm`, `vendor_boot` ramdisk, `system_dlkm`) | **No, not directly** | Built for Android GKI 5.15 (the dump's `system_dlkm` is `5.15.148`), with GKI/vendor symbol versions, CRCs, `vermagic`, struct layouts and Android-only APIs. They cannot be inserted into a mainline 7.x kernel. |
| Vendor kernel source (MiCode, `sm6225-modules`, Xiaomi trees) | **Yes** | The right source for porting drivers. Prefer this over disassembling `.ko`. |
| Android userspace HALs/libraries (`sensors.ssc.so`, audio/camera HALs) | **No** | Not kernel drivers and not usable by a native Linux userspace. |
| Calibration/thermal/battery config | **Yes** | Often in DT, XML/JSON or vendor config files; can be translated into mainline properties. |

The official `.ko` files are still useful as:

- an inventory of which driver/chip is actually used (`focaltech_ts_spi.ko`,
  `nt36532_spi.ko`, `bq2589x_charger.ko`, `sh366101_fg.ko`, `fs1815_dlkm.ko`,
  `ktz8866_leds.ko`, ...);
- a source of strings, register names and probe parameters;
- a cross-check that a vendor source tree matches the shipped firmware.

If the goal is to load the stock modules, the only realistic route is to keep
the vendor 5.15 kernel (Halium/Droidian-style), not a mainline kernel. That can
help with quick hardware validation but does not give a native mainline distro.

### Extracting an official package

A fastboot ROM is usually a `.tgz` containing a `.tar` with `images/`; a
recovery ROM contains `payload.bin`. Useful tools:

- `unpack_bootimg` for `boot.img`/`init_boot.img`/`vendor_boot.img`;
- `lpunpack` for `super.img` (dynamic partitions);
- `simg2img` for sparse images;
- `erofs-utils`/`erofs-fuse` or `mount` for `vendor`/`system` images;
- `payload_dumper` for OTA `payload.bin`;
- `extract-vmlinux` and the stock `bootRE/boot_kallsyms.txt` (in the public
  dump) to inspect built-in drivers and symbols.

The missing `firmware` partition is the main reason to obtain a full official
fastboot ROM (or dump it from the device), not the `.ko` files.
