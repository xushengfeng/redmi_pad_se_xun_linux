# Building and running the xun port

This is the practical side: what to download, how to build the kernel and the
boot image, how to install it, and how the Alpine system on the tablet is
laid out. Everything lives in `build/`; the repository keeps the sources of
the port, not the trees and images they are built from.

## What the tablet runs

| Part | |
| --- | --- |
| Kernel | mainline 7.2.1 + selected `stable-sm6225` commits + xun DTS and drivers |
| Boot image | Android boot header **v4** with the tapas shim, kernel and DTB in the legacy payload fields |
| Slot | `boot_b`, slot B active, slot A keeps Android; `qbootctl -m` at boot keeps ABL from falling back to recovery |
| Rootfs | Alpine 3.24 aarch64 on the internal eMMC, ext4, ~105 GB |
| Session | GDM autologin of `xun` into GNOME 50 on Wayland (mutter, freedreno) |

## Host requirements

An x86_64 Linux host with:

- `clang`/`ld.lld` plus the usual kernel build tools (`make`, `bc`, `flex`,
  `bison`, `pahole` optional — the config builds without debug info),
- `curl`, `git`, `m4`, `python3`, `cpio`, `gzip`, `openssl`,
- `alsatplg` from `alsa-utils` (only to rebuild the audio topology),
- `android-tools` on the host that flashes, or a second device running it,
- `picocom` or `screen` for the serial console on `/dev/ttyACM0` (optional),
- Docker for the full rootfs image. The image builder runs an aarch64 Alpine
  container (a qemu-aarch64 binfmt handler is registered automatically), so
  the aarch64 package scripts run instead of failing with `Exec format error`,
- `e2fsprogs`, `android-tools` (`img2simg`) and about 40 GB of disk for the
  kernel tree, the rootfs and the images.

The port is built on Arch Linux with clang/LLD 23.1, GNU make 4.4, m4 1.4,
Python 3.14 and the `fastboot`/`adb` from `android-tools`.

## 1. Fetch what is not in the repository

```sh
cd build
./fetch-sources.sh
```

`./fetch-vendor.sh` additionally downloads the Xiaomi vendor kernel sources
that the power and fuel-gauge drivers were written from. They are not needed
to build; the files and revisions are listed in
[`DOWNLOADS.md`](DOWNLOADS.md), together with everything else that is kept
outside git.

| What | Where from | Revision / file |
| --- | --- | --- |
| Linux 7.2.1 tarball | `https://mirrors.tuna.tsinghua.edu.cn/kernel/v7.x/linux-7.2.1.tar.xz` | 7.2.1 |
| SM6225 support patches | `https://gitlab.postmarketos.org/sm6225-mainline/kernels/linux-v.7.2.1/-/commit/<sha>.patch` | `650f459d`, `8b6545f7`, `f5a976a8`, `e127de2a` (the commits for 7.2.9, `ae725a2c` and `b8463e4b`, do not apply) |
| tapas bring-up tree (initramfs init, v4 shim, base kernel config) | `https://github.com/Sype0/pmos-tapas.git` | `5dee84cc5d7939a1042cf9daf2a21e67ab5c3579` |
| `boot-tapas-safe.img` (the source of the v4 shim that `build-xun.sh` extracts) | `https://github.com/Sype0/pmos-tapas/releases/download/latest/boot-tapas-safe.img` | `latest` |
| Panel driver generator (used once for `kernel-panel/*.c`) | `https://github.com/msm8916-mainline/linux-mdss-dsi-panel-driver-generator.git` | `28c4df25628f6241ce925af15637a443ea2b3bb5` |
| AudioReach topology macros | `https://github.com/linux-msm/audioreach-topology.git` | `d6bb205aca5f4b8fa6b2f44cf8b5f6a081cbf037` |
| FT8203 GENI SPI reference | `https://github.com/RigoLigoRLC/unixnu-nativefocaltech.git` | `master` |
| `busybox-static`, `apk-tools-static`, Alpine minirootfs | `https://mirrors.tuna.tsinghua.edu.cn/alpine/latest-stable/` | `latest-stable` (3.24) |

Everything lands in `build/src`, `build/tapas-bringup`, `build/tapas-images`,
`build/tools`, `build/rootfs-work` and `build/downloads`; all of it is out of
git.

## 2. Firmware

The firmware comes from the tablet itself (stock partitions) and from the
postmarketOS firmware package for the sibling SM6225 device; it is not
redistributed here. Put it in `build/firmware`:

| Files | From |
| --- | --- |
| `qcom/sm6225/xiaomi/xun/adsp.*` | stock `modem_a`: `mount -t vfat -o ro /dev/disk/by-partlabel/modem_a /mnt` then copy `image/adsp.*` |
| `qcom/sm6225/xiaomi/xun/modem.*` | the same stock image: the modem DSP firmware (needed for WLAN) |
| `qcom/sm6225/xiaomi/xun/wlanmdsp.mbn`, `bdwlan*`, `bdwlanu*`, `bdwlans*`, `bdwlant*`, `bd84*` | the same stock image: WLAN firmware and board data served by the modem |
| `ath10k/WCN3990/hw1.0/*` | Alpine `linux-firmware-qca`; the image builder decompresses the `.zst` files because the kernel has no ZSTD firmware decompressor |
| `qcom/a610_zap.*`, `qcom/a630_sqe.fw` | `firmware-xiaomi-spes` (`https://gitlab.postmarketos.org/sm6225-mainline/firmwares/firmware-xiaomi-spes`), `qcom/sm6225/XIAOMI/SPES/` |
| `focaltech_ts_fw_boe_ft8203.bin`, `focaltech_ts_fw_huaxing_ft8203.bin` | stock `vendor` partition touch firmware (m84 panel) |
| `qcom/sm4250/SM6225-Xun-tplg.bin` | built by `build/mktplg.sh` (needs `m4` and `alsatplg`) |
| `qca/crbtfw13.tlv`, `qca/crnv13.bin` | stock `bluetooth_a` partition: copy `image/cmbtfw13.tlv` and `image/cmnv13.bin` under the `cr` names mainline asks for (see [`BLUETOOTH.md`](BLUETOOTH.md)) |

`config/xun.config` embeds `a610_zap.*`, `a630_sqe.fw`, both touch firmwares
and the two QCA files into the kernel; the ADSP firmware and the topology are
added to the initramfs by `build-xun.sh` because those devices probe before
the rootfs is mounted.

```sh
./mktplg.sh          # writes firmware/qcom/sm4250/SM6225-Xun-tplg.bin
```

## 3. Build the boot image

```sh
./build-xun.sh       # -> out/boot-xun.img
```

It applies the patches to `src/linux-7.2.1` (once, cached in `work/.patched`),
installs the xun DTS, panel drivers, FT8203 touch driver and the SH366101 fuel
gauge, merges `config/xun.config` into the kernel config, builds `Image` and
`sm6225-xiaomi-xun.dtb`, and packs the v4 boot image with the tapas shim.

The sources of the port are small and tracked:

| Path | What |
| --- | --- |
| `build/kernel-dts/sm6225-xiaomi-xun.dts` | the board: display, touch, eMMC/SD, USB, WCN3990, modem, audio, fuel gauge, bootargs |
| `build/kernel-panel/panel-xiaomi-m84-*.c` | the three M84 panels, generated from the stock DSI nodes |
| `build/kernel-touch/ft8203-spi.c` | FocalTech FT8203 over GENI SPI |
| `build/kernel-battery/sh366101.c` | SinoWealth SH366101 fuel gauge (SBS over I2C) |
| `build/kernel-audio/fs1815.c` | Foursemi FS1815 speaker amplifiers (I2C, board preset) |
| `build/patches/*.patch` | power-off through the PMIC (`qcom-pon`, SCM release of `PS_HOLD`), the BQ2589X part number and the QCA Bluetooth in-band sleep workaround |
| `build/config/xun.config` | kernel fragment on top of the tapas base config |
| `build/topology/SM6225-Xun.m4` | AudioReach topology of the sound card |
| `build/ucm2/` | ALSA UCM profile (`conf.d/sm4250`, `Qualcomm/sm6225/xun`) |
| `build/device/` | files installed verbatim on the tablet: `/etc/local.d`, GDM autologin, dconf defaults (power key, idle lock), dnsmasq, time sync, the session wrappers |

## 4. Install

```sh
./deploy-boot.sh out/boot-xun.img   # writes to boot_b, verifies the md5
ssh -i ssh/xun_key root@172.16.42.1 reboot
```

The tablet answers on `172.16.42.1` over the USB ECM link (serial console on
`ttyGS0` of the same cable). A boot takes 60–70 s, GNOME included. `fastboot
boot out/boot-xun.img` also works and writes nothing; `fastboot flash boot_b`
is the alternative to the SSH path. [`DEBUG.md`](DEBUG.md) covers the USB
link, the serial console and what to do when the tablet does not show up.

The files in `build/device` and `build/ucm2` are copied to the corresponding
paths on the tablet (`/etc/...`, `/usr/bin/...`, `/usr/share/alsa/ucm2/...`);
`/sbin/init` comes from `rootfs-init.sh` when the image is built.
`build/deploy-rootfs.sh` does the copy from the tablet, creates the `xun`
account if it is missing and recompiles the system dconf database; the
account, session manager, lock screen and power key are described in
[`SESSION.md`](SESSION.md).

## 5. Full rootfs image

`build/make-install-image.sh` builds an Alpine 3.24 + GNOME rootfs from the
same sources as the bring-up system, wraps it in an ext4 image labelled
`tapas_root` and writes a sparse copy for fastboot. The package list lives in
`build/install-packages.txt`; it includes the GNOME core apps (Files, Text
Editor, Calculator, Calendar, Clocks, Weather, Maps, Software, System Monitor,
Disk Usage, Music, Showtime, Loupe, Papers, Snapshot, Contacts, File Roller,
Epiphany), Firefox, the Chinese input method and fonts, the audio stack and
the firmware packages.

```sh
./build/make-install-image.sh
# out/xun-install.img (raw, 10G) and out/xun-install.img.sparse
```

The rootfs is installed with `apk --root` inside an aarch64 container, so
package triggers compile the GLib schemas, icon caches, font caches, MIME
database and dconf database. The device files from `build/device`, the UCM
profile and `build/firmware` are copied in, then `rootfs-init.sh` becomes
`/sbin/init`. The image is smaller than the `userdata` partition; on the first
boot `/sbin/init` runs `resize2fs` so the filesystem grows to the partition.

The image can be flashed without Android or adb. Enter fastboot with
**power + volume-down**, or from the running Linux system write the standard
`bootonce-bootloader` command to the `misc` partition and reboot:

```sh
ssh -i build/ssh/xun_key root@172.16.42.1 \
  "printf 'bootonce-bootloader\0' | dd of=/dev/disk/by-partlabel/misc bs=1 conv=notrunc; sync; reboot"
```

Then, with the tablet in fastboot:

```sh
fastboot flash boot_b build/out/boot-xun.img
fastboot flash userdata build/out/xun-install.img.sparse
fastboot reboot
```

`boot_b` is the slot the shim boots; slot A still holds Android. The sparse
image is the 10G filesystem with the zero blocks omitted; use
`fastboot -S 256M flash userdata build/out/xun-install.img` instead if a
fastboot version prefers to create the sparse chunks itself.

## 6. Rootfs layout

`make-rootfs.sh` builds the minimal Alpine image used during the first
bring-up. The install image from `make-install-image.sh` is a full Alpine
3.24 aarch64 system with `gnome`, `gnome-apps-core`, `firefox`, `ibus-libpinyin`,
`font-noto-cjk`, `networkmanager`, `wpa_supplicant`, `bluez`, `chrony`,
`upower`, `polkit`, `elogind`, `gdm`, `tqftpserv`, `rmtfs`, `alsa-utils`
and `mesa`/`mesa-vulkan-freedreno` for the Adreno 610.

`build/device/etc/local.d` is what wires that up at boot: `45-audio` applies
the UCM speaker profile, `50-gnome` starts dbus, elogind and polkit, creates
`/run/systemd/seats` and starts GDM, `60-modem` starts `tqftpserv`/`rmtfs`, the
modem and ath10k, `70-time` syncs the clock, `10-boot-success` marks the slot.

## 7. Where the data came from

The board knowledge is in [`HARDWARE.md`](HARDWARE.md),
[`DATA_COLLECTION.md`](DATA_COLLECTION.md) and
[`reference/`](reference/); the stock device tree, the partition map and the
audio/display/touch vendor trees are listed in
[`RESOURCES.md`](RESOURCES.md). The short version:

- stock device tree: decompiled from a full partition dump
  (`gm-stuffs/redmi_xun_dump`),
- display panels and touch wiring:
  `MiCode/vendor_qcom_proprietary_display-devicetree`, branch `xun-t-oss`,
- power path: `MiCode/Xiaomi_Kernel_OpenSource`, branch `xun-t-oss`
  (`bq2589x`, `sh366101`, `nopmi_chg`),
- audio: `MiCode/vendor_qcom_proprietary_audio-kernel` and the tapas tree.

## Status

Working: display (1200×1920, correct orientation), multi-touch, GPU/freedreno,
Wi-Fi (WCN3990, NetworkManager), Bluetooth (QCA over UART), eMMC, SD, USB
(gadget + host), GNOME on Wayland, Chinese locale/input, time sync, battery
gauge (SH366101), speaker audio (WCD937x, four FS1815 amplifiers, UCM routing),
charging (BQ2589X through the bq25890 driver, upower line power).

Not ported yet: suspend/resume, sensors, cameras, USB Power Delivery.
