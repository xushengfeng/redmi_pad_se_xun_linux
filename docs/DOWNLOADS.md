# Downloads and local copies

Everything the port needs but does not keep in git lives under `build/`, which
`.gitignore` excludes; the scripts recreate it. Only the small sources of the
port are tracked: the xun DTS, the drivers, the kernel patches, the topology
and UCM files, the on-device files and these documents. Large tarballs,
extracted trees, images and firmware stay out of git so that cloning stays
small; every one of them is listed here with the URL and revision it comes
from.

## Fetch scripts

| Script | What it fetches |
| --- | --- |
| `build/fetch-sources.sh` | kernel tarball and SM6225 patches, the tapas bring-up tree and its boot image, the panel-driver generator, the AudioReach m4 macros, the FT8203 reference, Alpine `busybox-static`/`apk-tools-static`/minirootfs |
| `build/fetch-vendor.sh` | Xiaomi vendor kernel sources used as references while writing the power, fuel-gauge and speaker drivers |

Both keep what is already there, so running them again only downloads what is
missing. They are the reproducible way to rebuild `build/` from a clone.

## What was downloaded

| Artifact | Source | Local path | Size |
| --- | --- | --- | --- |
| Linux 7.2.1 | `https://mirrors.tuna.tsinghua.edu.cn/kernel/v7.x/linux-7.2.1.tar.xz` | `build/src/linux-7.2.1.tar.xz`, extracted to `build/src/linux-7.2.1` | 153 MB / 2.1 GB |
| SM6225 support patches | `https://gitlab.postmarketos.org/sm6225-mainline/kernels/linux-v.7.2.1/-/commit/<sha>.patch`, commits `650f459d`, `8b6545f7`, `f5a976a8`, `e127de2a` | `build/src/patches/*.patch` | 30 KB |
| tapas bring-up tree | `https://github.com/Sype0/pmos-tapas.git` at `5dee84cc5d7939a1042cf9daf2a21e67ab5c3579` | `build/tapas-bringup` | 1.1 MB |
| tapas boot image (v4 shim source) | `https://github.com/Sype0/pmos-tapas/releases/download/latest/boot-tapas-safe.img` | `build/tapas-images/boot-tapas-safe.img` | 51 MB |
| Panel driver generator | `https://github.com/msm8916-mainline/linux-mdss-dsi-panel-driver-generator.git` at `28c4df25628f6241ce925af15637a443ea2b3bb5` | `build/downloads/panel-generator` | 268 KB |
| AudioReach topology macros | `https://github.com/linux-msm/audioreach-topology.git` at `d6bb205aca5f4b8fa6b2f44cf8b5f6a081cbf037` | `build/downloads/audioreach-topology` | 668 KB |
| FT8203 GENI SPI reference | `https://github.com/RigoLigoRLC/unixnu-nativefocaltech.git` (master) | `build/downloads/ft8203-reference` | 152 KB |
| Alpine `busybox-static`, `apk-tools-static`, minirootfs | `https://mirrors.tuna.tsinghua.edu.cn/alpine/latest-stable/` (3.24) | `build/tools/`, `build/rootfs-work/` | 6.7 MB |
| Xiaomi vendor kernel sources | `https://raw.githubusercontent.com/MiCode/Xiaomi_Kernel_OpenSource/<rev>/...` at `0418ef63b947c0cfe144074fae559ea9dfc3638e` (branch `xun-t-oss`) | `build/downloads/xun-vendor` | 412 KB, 13 files |
| Xiaomi vendor audio sources | `https://raw.githubusercontent.com/Xiaomi-Redmi-Pad-SE-Resources/android_kernel_xiaomi_sm6225-modules/<rev>/qcom/opensource/audio-kernel/asoc/codecs/fs1815/...` at `819001a03074d4ba3befd21b996f23cda06e6abb` (branch `lineage-22.2`) | `build/downloads/xun-vendor/audio/fs1815` | 112 KB, 9 files |
| `fs1815.fsm` (amplifier preset) | the tablet's Android `vendor` logical partition (`/vendor/firmware/fs1815.fsm`, EROFS inside `super`) | `build/firmware/fs1815.fsm` | 2.4 KB |
| xun stock partition dump | `https://github.com/gm-stuffs/redmi_xun_dump` (branch `missi-user-15-AQ3A.240829.003-OS2.0.1.0.VMUMIXM`) | `build/live/` (local dump); the decompiled DT is in `docs/reference/` and tracked | 643 MB |
| Firmware | stock tablet partitions and `firmware-xiaomi-spes`; see the table in [`BUILD.md`](BUILD.md#2-firmware) | `build/firmware/` | 38 MB |

The SSH key `build/ssh/xun_key` is generated locally, not downloaded; see
[`DEBUG.md`](DEBUG.md#ssh) for recreating it.

## Vendor reference files

`build/fetch-vendor.sh` fetches these from the Xiaomi GPL drop, at the pin
above. They are documentation-grade references for the drivers in
`build/kernel-*` and the kernel patches:

| File | Used for |
| --- | --- |
| `drivers/power/reset/msm-poweroff.c`, `drivers/firmware/qcom_scm.c`, `drivers/firmware/qcom_scm.h` | the vendor power-off sequence: PON setup, `PS_HOLD` release through SCM |
| `drivers/input/misc/qpnp-power-on.c`, `include/linux/input/qpnp-power-on.h`, `include/dt-bindings/input/qcom,qpnp-power-on.h` | PON register offsets, `PS_HOLD_RST_CTL` values, power-on trigger bits |
| `drivers/power/reset/qcom-pon.c` | PON driver this port's patch is based on |
| `drivers/power/supply/nopmi/sh366101_fg.c`, `sh366101_fg.h`, `sh366101_iio.h` | SH366101 register map and SBS protocol behind `build/kernel-battery/sh366101.c` |
| `drivers/power/supply/nopmi/bq2589x_charger.c`, `.h`, `bq2589x_reg.h` | BQ2589X charger, for the driver that is still missing |
| `drivers/bluetooth/hci_qca.c`, `btqca.c` | QCA Bluetooth setup and firmware download, compared while adding the in-band sleep workaround |
| `qcom/opensource/audio-kernel/asoc/codecs/fs1815/` | FS1815 register access, preset parser and the tables hardcoded in `build/kernel-audio/fs1815.c`; `build/kernel-audio/fsm-preset.py` parses the same preset file |

## Firmware

Firmware is not redistributed here. `docs/BUILD.md` lists where each file
comes from; the files themselves end up in:

| Path | Contents |
| --- | --- |
| `build/firmware/qcom/sm6225/xiaomi/xun/adsp.*` | ADSP firmware, copied from the tablet's stock `modem_a` partition; `build-xun.sh` puts it into the initramfs |
| `build/firmware/qcom/sm6225/xiaomi/xun/modem.*` | modem DSP firmware from the stock `modem_a` image; without it the WLAN service never appears |
| `build/firmware/qcom/sm6225/xiaomi/xun/wlanmdsp.mbn`, `bdwlan*`, `bd84*` | WLAN firmware and board data from the stock `modem_a` image |
| `build/firmware/qcom/a610_zap.*`, `a630_sqe.fw` | Adreno zap/sqe, from `firmware-xiaomi-spes`; embedded into the kernel |
| `build/firmware/focaltech_ts_fw_*_ft8203.bin` | touch firmware from the stock `vendor` partition; embedded into the kernel |
| `build/firmware/qca/crbtfw13.tlv`, `crnv13.bin` | stock `bluetooth_a` (`cmbtfw13.tlv`, `cmnv13.bin` under the `cr` names); embedded into the kernel, see [`BLUETOOTH.md`](BLUETOOTH.md) |
| `build/firmware/qcom/sm4250/SM6225-Xun-tplg.bin` | built locally by `build/mktplg.sh`, put into the initramfs |
| `build/firmware/fs1815.fsm` | amplifier preset from the Android `vendor` partition, only used to derive the tables (documented in [`AUDIO.md`](AUDIO.md)) |

## Rebuilding from a fresh clone

```sh
cd build
./fetch-sources.sh        # kernel, tapas, tools, Alpine
./fetch-vendor.sh         # vendor driver references (not needed to build)
./build-xun.sh            # build/out/boot-xun.img
./deploy-boot.sh out/boot-xun.img
./deploy-rootfs.sh        # on the tablet, installs build/device and build/ucm2
```

The firmware has to be extracted from the tablet (or the postmarketOS package)
once, as described in `BUILD.md`; it is the only input that cannot be fetched
by URL.
