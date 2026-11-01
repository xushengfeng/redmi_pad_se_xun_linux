#!/bin/sh
# Install the xun board files into a freshly extracted sm6225-mainline tree
# and generate its kernel configuration.
#
#   prepare-kernel.sh <kernel-tree> <xun-dir> <srcdir>
#
# <xun-dir> holds the DTS, panel/touch/audio/battery drivers, the patches,
# the base/tapas/xun configs, the tapas initramfs init and the static busybox.
# <srcdir> also holds firmware/ (extracted from firmware.tar.gz).
set -eu

K=$1
X=$2
S=$3
DTS_DIR="$K/arch/arm64/boot/dts/qcom"

# --- board files ---------------------------------------------------------
cp "$X/sm6225-xiaomi-xun.dts" "$K/$DTS_DIR/"
cp "$X"/panel-xiaomi-m84-*.c "$K/drivers/gpu/drm/panel/"

PANEL="$K/drivers/gpu/drm/panel"
if ! grep -q 'PANEL_XIAOMI_M84_35_03_0B' "$PANEL/Kconfig"; then
	cat >"$S/panel-kconfig" <<'EOF'

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
	python3 - "$PANEL/Kconfig" "$S/panel-kconfig" <<'PY'
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

# --- SH366101 fuel gauge -------------------------------------------------
cp "$X/sh366101.c" "$K/drivers/power/supply/"
PS="$K/drivers/power/supply"
if ! grep -q 'SH366101' "$PS/Kconfig"; then
	cat >"$S/sh366101-kconfig" <<'EOF'

config SH366101
	tristate "SinoWealth SH366101 battery fuel gauge"
	depends on I2C
	help
	  Say Y here to enable the SH366101 SBS battery fuel gauge used in the
	  Redmi Pad SE.

EOF
	python3 - "$PS/Kconfig" "$S/sh366101-kconfig" <<'PY'
import sys
kconfig, extra = sys.argv[1], open(sys.argv[2]).read()
text = open(kconfig).read()
idx = text.rfind("endif")
open(kconfig, "w").write(text[:idx] + extra + text[idx:])
PY
fi
grep -q 'sh366101' "$PS/Makefile" ||
	echo 'obj-$(CONFIG_SH366101)	+= sh366101.o' >>"$PS/Makefile"

# --- FS1815 speaker amplifiers -------------------------------------------
cp "$X/fs1815.c" "$K/sound/soc/codecs/"
SC="$K/sound/soc/codecs"
if ! grep -q 'SND_SOC_FS1815' "$SC/Kconfig"; then
	cat >"$S/fs1815-kconfig" <<'EOF'

config SND_SOC_FS1815
	tristate "Foursemi FS1815 speaker amplifier"
	depends on I2C
	help
	  Say Y here to enable the Foursemi FS1815 speaker amplifier of the
	  Redmi Pad SE. The driver initialises the amplifier from the board
	  preset and leaves it running behind the WCD937x AUX output.

EOF
	python3 - "$SC/Kconfig" "$S/fs1815-kconfig" <<'PY'
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
cp "$X/ft8203-spi.c" "$K/drivers/input/touchscreen/"
TS="$K/drivers/input/touchscreen"
if ! grep -q 'TOUCHSCREEN_FT8203' "$TS/Kconfig"; then
	cat >"$S/ft8203-kconfig" <<'EOF'

config TOUCHSCREEN_FT8203
	tristate "FocalTech FT8203 SPI touchscreen"
	depends on SPI
	help
	  Say Y here if you want to enable support for the FocalTech FT8203
	  touchscreen controller connected over SPI (Redmi Pad SE).

EOF
	python3 - "$TS/Kconfig" "$S/ft8203-kconfig" <<'PY'
import sys
kconfig, extra = sys.argv[1], open(sys.argv[2]).read()
text = open(kconfig).read()
idx = text.rfind("endif")
open(kconfig, "w").write(text[:idx] + extra + text[idx:])
PY
fi
grep -q 'ft8203-spi' "$TS/Makefile" ||
	echo 'obj-$(CONFIG_TOUCHSCREEN_FT8203)	+= ft8203-spi.o' >>"$TS/Makefile"

grep -q 'sm6225-xiaomi-xun.dtb' "$K/$DTS_DIR/Makefile" ||
	echo 'dtb-$(CONFIG_ARCH_QCOM) += sm6225-xiaomi-xun.dtb' >>"$K/$DTS_DIR/Makefile"

# --- patches -------------------------------------------------------------
for p in "$X"/patches/*.patch; do
	patch -d "$K" -p1 -s -N <"$p"
done

# --- initramfs -----------------------------------------------------------
cat >"$S/initramfs.list" <<EOF
dir /bin 755 0 0
file /bin/busybox $X/busybox.static 755 0 0
file /init $X/init 755 0 0
dir /dev 755 0 0
nod /dev/console 600 0 0 c 5 1
EOF
if [ -d "$S/firmware/qcom/sm6225/xiaomi/xun" ]; then
	cat >>"$S/initramfs.list" <<'EOF'
dir /lib 755 0 0
dir /lib/firmware 755 0 0
dir /lib/firmware/qcom 755 0 0
dir /lib/firmware/qcom/sm6225 755 0 0
dir /lib/firmware/qcom/sm6225/xiaomi 755 0 0
dir /lib/firmware/qcom/sm6225/xiaomi/xun 755 0 0
EOF
	for f in "$S"/firmware/qcom/sm6225/xiaomi/xun/adsp.*; do
		[ -f "$f" ] || continue
		echo "file /lib/firmware/qcom/sm6225/xiaomi/xun/${f##*/} $f 644 0 0" >>"$S/initramfs.list"
	done
	if [ -f "$S/firmware/qcom/sm4250/SM6225-Xun-tplg.bin" ]; then
		echo 'dir /lib/firmware/qcom/sm4250 755 0 0' >>"$S/initramfs.list"
		echo "file /lib/firmware/qcom/sm4250/SM6225-Xun-tplg.bin $S/firmware/qcom/sm4250/SM6225-Xun-tplg.bin 644 0 0" >>"$S/initramfs.list"
	fi
fi

# --- config --------------------------------------------------------------
cp "$X/base.config" "$K/.config"
sed "s|@BUILD_DIR@|$S|" "$X/xun.config" >"$S/xun-fragment.config"
cat "$X/tapas.config" "$S/xun-fragment.config" >"$S/xun-merged.config"
echo "CONFIG_INITRAMFS_SOURCE=\"$S/initramfs.list\"" >>"$S/xun-merged.config"

sh "$K/scripts/kconfig/merge_config.sh" -m -O "$K" "$K/.config" "$S/xun-merged.config"
make -C "$K" ARCH=arm64 LLVM=1 olddefconfig
