#!/bin/bash
# Vendor sources from the Xiaomi GPL drops, kept as references while porting
# drivers. They are not needed to build the port and are not in git; this
# script fetches the exact files next to the kernel checkout.
#
#   ./fetch-vendor.sh
#
# They land in downloads/xun-vendor/<path>. Set MIRROR to a replacement for
# https://raw.githubusercontent.com (for example
# MIRROR=https://ghproxy.imciel.com/https://raw.githubusercontent.com) if
# GitHub is slow from where you are.
set -eu
ROOT=$(cd "$(dirname "$0")" && pwd)
OUT=$ROOT/downloads/xun-vendor
GITHUB_RAW=${MIRROR:-https://raw.githubusercontent.com}

# xun-t-oss of MiCode/Xiaomi_Kernel_OpenSource (GKI tree)
MI_REV=0418ef63b947c0cfe144074fae559ea9dfc3638e
MI_URL=$GITHUB_RAW/MiCode/Xiaomi_Kernel_OpenSource/$MI_REV

# lineage-22.2 of the vendor module tree (audio-kernel with FS1815)
MOD_REV=819001a03074d4ba3befd21b996f23cda06e6abb
MOD_URL=$GITHUB_RAW/Xiaomi-Redmi-Pad-SE-Resources/android_kernel_xiaomi_sm6225-modules/$MOD_REV

# The files the xun drivers were written from:
#   qpnp-power-on      PON/PS_HOLD registers and the PS_HOLD shutdown action
#   msm-poweroff       vendor power-off sequence (PON setup, SCM PS_HOLD release)
#   qcom_scm           SCM service and command IDs
#   qcom-pon           vendor PON driver
#   sh366101           fuel gauge register map and SBS protocol
#   bq2589x            charger, for the driver that is still missing
MI_FILES="
drivers/input/misc/qpnp-power-on.c
include/linux/input/qpnp-power-on.h
include/dt-bindings/input/qcom,qpnp-power-on.h
drivers/firmware/qcom_scm.c
drivers/firmware/qcom_scm.h
drivers/power/reset/msm-poweroff.c
drivers/power/reset/qcom-pon.c
drivers/power/supply/nopmi/sh366101_fg.c
drivers/power/supply/nopmi/sh366101_fg.h
drivers/power/supply/nopmi/sh366101_iio.h
drivers/power/supply/nopmi/bq2589x_charger.c
drivers/power/supply/nopmi/bq2589x_charger.h
drivers/power/supply/nopmi/bq2589x_reg.h
"

# FS1815 speaker amplifier: register access, preset parser and the M84 tables
# behind build/kernel-audio/fs1815.c.
FS1815_FILES="
Kbuild
Makefile
fs1815.c
fsm-dev.h
fsm_core.c
fsm_firmware.c
fsm_i2c.c
fsm_misc.c
fsm_public.h
"

fetch() { # fetch <url> <file>
	[ -s "$2" ] && return 0
	mkdir -p "$(dirname "$2")"
	echo "  ${2#$OUT/}"
	curl -L --fail --retry 3 -s -o "$2.part" "$1"
	mv "$2.part" "$2"
}

echo "vendor sources -> $OUT"
for f in $MI_FILES; do
	fetch "$MI_URL/$f" "$OUT/$f"
done
for f in $FS1815_FILES; do
	fetch "$MOD_URL/qcom/opensource/audio-kernel/asoc/codecs/fs1815/$f" \
		"$OUT/audio/fs1815/$f"
done
du -sh "$OUT"
