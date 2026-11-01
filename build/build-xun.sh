#!/bin/bash
# Build the xun bring-up kernel: mainline v7.2.1 + the SM6225 support
# patches from stable-sm6225, our DTS and the M84 panel drivers.
set -eu
ROOT=$(cd "$(dirname "$0")" && pwd)
SRC=$ROOT/src/linux-7.2.1
WORK=$ROOT/work
OUT=$ROOT/out
KO=$WORK/kernel-out
DTS_DIR=arch/arm64/boot/dts/qcom
PATCHES=$ROOT/src/patches

mkdir -p "$WORK" "$OUT"

[ -d "$SRC" ] || {
	echo "kernel tree $SRC missing: run ./fetch-sources.sh first" >&2
	exit 1
}
[ -d "$ROOT/tapas-bringup" ] || {
	echo "tapas bring-up tree missing: run ./fetch-sources.sh first" >&2
	exit 1
}
[ -f "$ROOT/tapas-images/boot-tapas-safe.img" ] || {
	echo "boot-tapas-safe.img missing: run ./fetch-sources.sh first" >&2
	exit 1
}

# --- patches -------------------------------------------------------------
if [ ! -e "$WORK/.patched" ]; then
	for c in 650f459d 8b6545f7 f5a976a8 e127de2a; do
		p=$PATCHES/$c.patch
		[ -f "$p" ] || { echo "missing patch $c" >&2; exit 1; }
		echo "applying $c"
		patch -d "$SRC" -p1 -s -N <"$p" || {
			echo "patch $c did not apply cleanly" >&2
			exit 1
		}
	done
	touch "$WORK/.patched"
fi

# Powering off has to tell the PMIC PON to shut down; otherwise releasing
# PS_HOLD resets the SoC and a shutdown comes back up as a reboot.
if ! grep -q qcom_pon_power_off_prepare "$SRC/drivers/power/reset/qcom-pon.c"; then
	patch -d "$SRC" -p1 -s -N <"$ROOT/patches/qcom-pon-poweroff.patch" || {
		echo "qcom-pon poweroff patch did not apply cleanly" >&2
		exit 1
	}
fi
# The TCSR PS_HOLD register is write protected; the secure monitor has to
# release it, so ask it to before falling back to the direct write.
if ! grep -q qcom_deassert_ps_hold "$SRC/drivers/power/reset/msm-poweroff.c"; then
	patch -d "$SRC" -p1 -s -N <"$ROOT/patches/msm-poweroff-scm.patch" || {
		echo "msm-poweroff SCM patch did not apply cleanly" >&2
		exit 1
	}
fi
# The QCA Bluetooth controller does not answer the HCI init commands once the
# in-band sleep protocol is enabled, so keep it off.
if ! grep -q 'in-band sleep protocol off' "$SRC/drivers/bluetooth/hci_qca.c"; then
	patch -d "$SRC" -p1 -s -N <"$ROOT/patches/qca-no-ibs.patch" || {
		echo "QCA no-IBS patch did not apply cleanly" >&2
		exit 1
	}
fi

# --- board files ---------------------------------------------------------
cp "$ROOT/kernel-dts/sm6225-xiaomi-xun.dts" "$SRC/$DTS_DIR/"
cp "$ROOT"/kernel-panel/panel-xiaomi-m84-*.c "$SRC/drivers/gpu/drm/panel/"

PANEL=$SRC/drivers/gpu/drm/panel
if ! grep -q 'PANEL_XIAOMI_M84_35_03_0B' "$PANEL/Kconfig"; then
	cat >"$WORK/panel-kconfig" <<'EOF'

config DRM_PANEL_XIAOMI_M84_35_03_0B
	tristate "Xiaomi M84 35 03 0b (1200x1920) panel"
	depends on DRM && DRM_MIPI_DSI
	depends on BACKLIGHT_CLASS_DEVICE
	help
	  Say Y here if you want to enable support for the Xiaomi M84 35 03 0b
	  panel found in the Redmi Pad SE (xun).

config DRM_PANEL_XIAOMI_M84_36_02_0A
	tristate "Xiaomi M84 36 02 0a (1200x1920) panel"
	depends on DRM && DRM_MIPI_DSI
	depends on BACKLIGHT_CLASS_DEVICE
	help
	  Say Y here if you want to enable support for the Xiaomi M84 36 02 0a
	  panel found in the Redmi Pad SE (xun).

config DRM_PANEL_XIAOMI_M84_42_03_0C
	tristate "Xiaomi M84 42 03 0c (1200x1920) panel"
	depends on DRM && DRM_MIPI_DSI
	depends on BACKLIGHT_CLASS_DEVICE
	help
	  Say Y here if you want to enable support for the Xiaomi M84 42 03 0c
	  panel found in the Redmi Pad SE (xun).
EOF
	python3 - "$PANEL/Kconfig" "$WORK/panel-kconfig" <<'PY'
import sys
kconfig, extra = sys.argv[1], open(sys.argv[2]).read()
text = open(kconfig).read()
idx = text.rfind("endmenu")
open(kconfig, "w").write(text[:idx] + extra + text[idx:])
PY
fi
for p in 35-03-0b 36-02-0a 42-03-0c; do
	sym=$(echo "$p" | tr 'a-z-' 'A-Z_')
	grep -q "panel-xiaomi-m84-$p" "$PANEL/Makefile" ||
		echo "obj-\$(CONFIG_DRM_PANEL_XIAOMI_M84_$sym) += panel-xiaomi-m84-$p.o" >>"$PANEL/Makefile"
done

# --- SH366101 fuel gauge ---------------------------------------------------
cp "$ROOT"/kernel-battery/*.c "$SRC/drivers/power/supply/"
PS=$SRC/drivers/power/supply
if ! grep -q 'SH366101' "$PS/Kconfig"; then
	cat >"$WORK/sh366101-kconfig" <<'EOF2'

config SH366101
	tristate "SinoWealth SH366101 battery fuel gauge"
	depends on I2C
	help
	  Say Y here to enable the SH366101 SBS battery fuel gauge used in the
	  Redmi Pad SE.

EOF2
	python3 - "$PS/Kconfig" "$WORK/sh366101-kconfig" <<'PY'
import sys
kconfig, extra = sys.argv[1], open(sys.argv[2]).read()
text = open(kconfig).read()
idx = text.rfind("endif")
open(kconfig, "w").write(text[:idx] + extra + text[idx:])
PY
fi
grep -q 'sh366101' "$PS/Makefile" ||
	echo 'obj-$(CONFIG_SH366101)	+= sh366101.o' >>"$PS/Makefile"

# --- FS1815 speaker amplifiers --------------------------------------------
cp "$ROOT"/kernel-audio/*.c "$SRC/sound/soc/codecs/"
SC=$SRC/sound/soc/codecs
if ! grep -q 'SND_SOC_FS1815' "$SC/Kconfig"; then
	cat >"$WORK/fs1815-kconfig" <<'EOF2'

config SND_SOC_FS1815
	tristate "Foursemi FS1815 speaker amplifier"
	depends on I2C
	help
	  Say Y here to enable the Foursemi FS1815 speaker amplifier of the
	  Redmi Pad SE. The driver initialises the amplifier from the board
	  preset and leaves it running behind the WCD937x AUX output.

EOF2
	python3 - "$SC/Kconfig" "$WORK/fs1815-kconfig" <<'PY'
import sys
kconfig, extra = sys.argv[1], open(sys.argv[2]).read()
text = open(kconfig).read()
idx = text.rfind("endif")
open(kconfig, "w").write(text[:idx] + extra + text[idx:])
PY
fi
grep -q 'snd-soc-fs1815' "$SC/Makefile" ||
	printf 'snd-soc-fs1815-objs := fs1815.o\nobj-$(CONFIG_SND_SOC_FS1815) += snd-soc-fs1815.o\n' >>"$SC/Makefile"

# --- FT8203 SPI touchscreen ----------------------------------------------
cp "$ROOT"/kernel-touch/*.c "$SRC/drivers/input/touchscreen/"
TS=$SRC/drivers/input/touchscreen
if ! grep -q 'TOUCHSCREEN_FT8203' "$TS/Kconfig"; then
	cat >"$WORK/ft8203-kconfig" <<'EOF2'

config TOUCHSCREEN_FT8203
	tristate "FocalTech FT8203 SPI touchscreen"
	depends on SPI
	help
	  Say Y here if you want to enable support for the FocalTech FT8203
	  touchscreen controller connected over SPI (Redmi Pad SE).

EOF2
	python3 - "$TS/Kconfig" "$WORK/ft8203-kconfig" <<'PY'
import sys
kconfig, extra = sys.argv[1], open(sys.argv[2]).read()
text = open(kconfig).read()
idx = text.rfind("endif")
open(kconfig, "w").write(text[:idx] + extra + text[idx:])
PY
fi
grep -q 'ft8203-spi' "$TS/Makefile" ||
	echo 'obj-$(CONFIG_TOUCHSCREEN_FT8203)	+= ft8203-spi.o' >>"$TS/Makefile"

grep -q 'sm6225-xiaomi-xun.dtb' "$SRC/$DTS_DIR/Makefile" ||
	echo 'dtb-$(CONFIG_ARCH_QCOM) += sm6225-xiaomi-xun.dtb' >>"$SRC/$DTS_DIR/Makefile"

# --- initramfs ------------------------------------------------------------
BUSYBOX=$ROOT/tools/bin/busybox.static
cat >"$WORK/initramfs.list" <<EOF
dir /bin 755 0 0
file /bin/busybox $BUSYBOX 755 0 0
file /init $ROOT/tapas-bringup/initramfs/init 755 0 0
dir /dev 755 0 0
nod /dev/console 600 0 0 c 5 1
EOF
# The ADSP probes before the rootfs is mounted, so its firmware has to travel
# in the initramfs (compressed inside the kernel image).
FWROOT=$ROOT/firmware/qcom/sm6225/xiaomi/xun
if [ -d "$FWROOT" ]; then
	cat >>"$WORK/initramfs.list" <<'EOF3'
dir /lib 755 0 0
dir /lib/firmware 755 0 0
dir /lib/firmware/qcom 755 0 0
dir /lib/firmware/qcom/sm6225 755 0 0
dir /lib/firmware/qcom/sm6225/xiaomi 755 0 0
dir /lib/firmware/qcom/sm6225/xiaomi/xun 755 0 0
EOF3
	for f in "$FWROOT"/adsp.*; do
		[ -f "$f" ] || continue
		echo "file /lib/firmware/qcom/sm6225/xiaomi/xun/${f##*/} $f 644 0 0" >>"$WORK/initramfs.list"
	done
	# the AudioReach topology is loaded when the card probes, also early
	if [ -f "$ROOT/firmware/qcom/sm4250/SM6225-Xun-tplg.bin" ]; then
		echo 'dir /lib/firmware/qcom/sm4250 755 0 0' >>"$WORK/initramfs.list"
		echo "file /lib/firmware/qcom/sm4250/SM6225-Xun-tplg.bin $ROOT/firmware/qcom/sm4250/SM6225-Xun-tplg.bin 644 0 0" >>"$WORK/initramfs.list"
	fi
fi

# --- config --------------------------------------------------------------
mkdir -p "$KO"
[ -f "$KO/.config" ] || cp "$ROOT"/tapas-bringup/config/base.config "$KO/.config"
# The fragment is merged on every build so edits to it take effect; the
# fragment names the firmware directory of this checkout.
sed "s|@BUILD_DIR@|$ROOT|" "$ROOT"/config/xun.config >"$WORK/xun-fragment.config"
cat "$ROOT"/tapas-bringup/config/tapas.config "$WORK/xun-fragment.config" >"$WORK/xun.config"
echo "CONFIG_INITRAMFS_SOURCE=\"$WORK/initramfs.list\"" >>"$WORK/xun.config"
"$SRC/scripts/kconfig/merge_config.sh" -m -O "$KO" "$KO/.config" "$WORK/xun.config"
make -C "$SRC" O="$KO" ARCH=arm64 LLVM=1 olddefconfig
grep -v '^#' "$WORK/xun.config" | grep . | while IFS= read -r opt; do
	grep -qxF "$opt" "$KO/.config" || echo "warning: not applied: $opt" >&2
done

# --- build ---------------------------------------------------------------
make -C "$SRC" O="$KO" ARCH=arm64 LLVM=1 -j"$(nproc)" Image qcom/sm6225-xiaomi-xun.dtb
ls -l "$KO/arch/arm64/boot/Image" "$KO/$DTS_DIR/sm6225-xiaomi-xun.dtb"

# --- boot image v4 -------------------------------------------------------
if [ ! -f "$WORK/shim.bin" ]; then
	python3 - "$ROOT/tapas-images/boot-tapas-safe.img" "$WORK/shim.bin" <<'EOF'
import struct, sys
img = open(sys.argv[1], "rb").read()
ksize = struct.unpack_from("<I", img, 8)[0]
payload = img[4096:4096 + ksize]
open(sys.argv[2], "wb").write(payload[:0x1000])
EOF
fi
: >"$WORK/empty-list"
(cd "$WORK" && find empty -type f >/dev/null 2>&1 || true)
mkdir -p "$WORK/empty"
(cd "$WORK/empty" && find . | cpio -o -H newc -R 0:0 --quiet | gzip -9n) >"$WORK/empty.cpio.gz"
python3 "$ROOT/tapas-bringup/scripts/pack-shim.py" "$WORK/shim.bin" \
	"$KO/$DTS_DIR/sm6225-xiaomi-xun.dtb" "$KO/arch/arm64/boot/Image" "$WORK/shim-xun.bin"
python3 "$ROOT/tapas-bringup/scripts/mkbootimg.py" --header_version 4 \
	--kernel "$WORK/shim-xun.bin" --ramdisk "$WORK/empty.cpio.gz" \
	--os_version 13.0.0 --os_patch_level 2026-10 \
	--cmdline "" --output "$OUT/boot-xun.img" | tail -1
ls -l "$OUT/boot-xun.img"
