# Data collection notes

## What was used

1. **Stock partition dump** — [`gm-stuffs/redmi_xun_dump`](https://github.com/gm-stuffs/redmi_xun_dump)
   (branch `missi-user-15-AQ3A.240829.003-OS2.0.1.0.VMUMIXM`).
   It contains `boot.img`, `init_boot.img`, `vendor_boot.img`, `dtbo.img`,
   the extracted `dtbo/` files, `aosp-device-tree/prebuilts/dtb.img`, and the
   vendor/ODM/system partitions.
2. **Base DTB** — `aosp-device-tree/prebuilts/dtb.img`, extracted from the stock
   `vendor_boot` image. It is the bare SoC tree (`Khaje SoC`, board ID 0).
3. **xun DTBO overlay** — entries 46/47 of the dump's extracted `dtbo/` set,
   decompiled as `KHAJE IDP nopmi xun`.
4. **Merged tree** — `fdtoverlay` applied the xun overlay to the base DTB; the
   result is `docs/reference/xun-merged.dts` (15k+ lines).
5. **Cross-checks** — public SM6225 mainline DTS, the official xun display DT
   (`MiCode/vendor_qcom_proprietary_display-devicetree`, branch `xun-t-oss`),
   the vendor module list (`vendor_dlkm/lib/modules`), vendor firmware/config
   files, and the `firmware-xiaomi-spes` package.

Two different DTBO images circulate:

- The dump's `dtbo/` directory is a pre-extracted set of 47 overlays and
  contains the real xun overlay (entries 46/47).
- The dump's root `dtbo.img` is a generic 32-entry Qualcomm QRD/IDP image and
  does **not** contain the xun overlay. The LineageOS/other prebuilt
  `dtbo.img` files are generic in the same way.

Use the extracted `dtbo/` set (or a stock xun `dtbo.img` from the device) for
the xun overlay, not the generic root image.

The community `Xiaomi-Redmi-Pad-SE-Resources` device/kernel repositories also
use prebuilt `dtbs/`/`dtbo.img`; their kernel tree has no `bengal`/`khaje`/`xun`
DTS. Treat those as boot-image references, not as board DTS source.

Its `android_device_xiaomi_xun/proprietary-firmware.txt` lists the stock
firmware-partition images (`abl.img`, `bluetooth.img`, `dsp.img`, `modem.img`,
`tz.img`, `xbl.img`, ...) extracted from `OS2.0.206.0.VMUMIXM`; use that list
when obtaining the missing `firmware` partition.

## Reproduce

The helper scripts in `scripts/` do the mechanical parts. Manually:

```sh
git clone --depth 1 --filter=blob:none --no-checkout \
    https://github.com/gm-stuffs/redmi_xun_dump.git xun-dump
cd xun-dump

# Materialize the small DT files only (git fetches the needed blobs).
git checkout HEAD -- dtbo aosp-device-tree/prebuilts

# Base SoC DTB.
git show HEAD:aosp-device-tree/prebuilts/dtb.img > base.dtb

# The extracted dtbo/ set already contains the xun overlay. Decompile all
# entries and locate it by model string.
mkdir -p out-dtbo
for f in dtbo/*.dtb; do
    dtc -I dtb -O dts -o "out-dtbo/$(basename "${f%.dtb}").dts" "$f"
done
grep -l 'KHAJE IDP nopmi xun' out-dtbo/*.dts

# Merge and decompile (46 is the xun entry in this set).
fdtoverlay -i base.dtb -o xun-merged.dtb dtbo/46_*.dtb
dtc -I dtb -O dts -o xun-merged.dts xun-merged.dtb
```


`fdtoverlay` needs `libfdt` on `LD_LIBRARY_PATH` if it was built from source;
install the `dtc` package to avoid that.

## DTBO format note

The stock `dtbo.img` header starts with magic `d7 b7 ab 1e`, stores fields
big-endian, and the 32-byte entry table starts at `header_size` (`0x20`).
Each entry is:

```
u32 dt_size; u32 dt_offset; u32 id; u32 rev; u32 custom[4];
```

All values are big-endian. In the dump's extracted `dtbo/` set, the xun
entries are 46 and 47 (zero-based). Both have
`qcom,board-id = <0x20022 0>`; entry 46 has
`qcom,msm-id = <0x206 0x10000>`.

`scripts/parse_dtbo.py` can unpack any dtbo image, including a stock xun
`dtbo.img` if one is obtained from the device; it prints the real entry table
offset it used, which helps when an image uses a non-standard header.


## What the stock tree tells us

The merged tree contains the complete board wiring:

- PM6125 regulators and PMIC GPIOs.
- Three M84 panel nodes with full init/off sequences, DSC settings and timings.
- SPI touch node (FT8203/NT36532) with GPIOs and panel phandles.
- BQ2589X charger, SH366101 fuel gauge, SC2150 PD, WUSB3801 switch, NOPMI charger.
- WCD937x codec, FS1815 amplifiers, SIA81xx placeholders, WSA881x disabled.
- WCN3990 Wi-Fi/BT, UFS, SD, USB, cameras, hall key, KTZ8866 backlight.
- Reserved-memory layout and splash region.
- The `memory` node is a placeholder; ABL fills it at boot.

## Gaps in the dump

The dump does **not** include the `firmware` partition. Consequences:

- No WCN3990 board data (`bdwlan*.bin`) or `firmware-5.bin`.
- No ADSP/CDSP/MPSS/IPA images in the dump; only `vendor/firmware` (45 files)
  contains GPU/video/touch/amp firmware and the WLAN config.
- `wlanmdsp.mbn` is absent (only `wlanmdsp.otaupdate`).

`firmware-xiaomi-spes` can supply SM6225-family ADSP/CDSP/MPSS/IPA and
`wlanmdsp.mbn`, but xun-specific WCN3990 calibration should come from the
device. Prefer dumping the `firmware` partition before wiping.

## Live-device checklist

With the stock Android still bootable and root/adb available, collect:

```sh
# Device identity and bootloader
getprop | sort > props.txt
fastboot getvar all > fastboot-vars.txt 2>&1
cat /proc/cmdline
cat /proc/version
cat /proc/config.gz > config.gz

# Actual merged device tree, including ABL memory fixups
cat /sys/firmware/fdt > fdt.dtb
dtc -I dtb -O dts -o live.dts fdt.dtb

# Panel / touch identity
dmesg | grep -iE 'panel|dsi|touch|fts|novatek|nvt|goodix|synaptics'
ls -l /sys/class/drm /sys/class/input
cat /proc/bus/input/devices
getprop | grep -iE 'panel|display|touch|tp|hwc'

# Sensors and audio
dumpsys sensorservice
dumpsys media.audio_flinger
cat /proc/asound/cards
cat /proc/asound/pcm

# Partitions and firmware
ls -l /dev/block/by-name
cat /proc/partitions
ls -l /vendor/firmware /vendor/firmware_mnt 2>/dev/null
# Root: dump the firmware partition (name depends on the slot)
dd if=/dev/block/by-name/firmware of=/sdcard/firmware.img
```

Also save a photograph or text dump of the bootloader screen (RAM size, panel
variant, regulatory info) and the exact model/SKU.

## Data still needed

| Item | Why it matters |
| --- | --- |
| Panel/touch variant of the target unit | Selects the right panel driver/DTB and touch chip |
| `firmware` partition | Wi-Fi/BT, ADSP audio, modem (unused) and IPA |
| Live `fdt.dtb` | Confirms ABL memory layout, reserved regions and the selected overlay |
| `dmesg`/input/sensor lists | Confirms the touch IC, sensor models and regulator usage |
| `fastboot getvar all` | Slot state, anti-rollback, unlock status, partition sizes |
| Thermal/battery logs | Safe charge limits and thermal zones |
