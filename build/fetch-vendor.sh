#!/bin/bash
# Vendor sources from the Xiaomi GPL drop, kept as references while porting
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
# xun-t-oss as of this port
REV=0418ef63b947c0cfe144074fae559ea9dfc3638e
GITHUB_RAW=${MIRROR:-https://raw.githubusercontent.com}
URL=$GITHUB_RAW/MiCode/Xiaomi_Kernel_OpenSource/$REV

# The files the xun drivers were written from:
#   qpnp-power-on      PON/PS_HOLD registers and the PS_HOLD shutdown action
#   msm-poweroff       vendor power-off sequence (PON setup, SCM PS_HOLD release)
#   qcom_scm           SCM service and command IDs
#   qcom-pon           vendor PON driver
#   sh366101           fuel gauge register map and SBS protocol
#   bq2589x            charger, for the driver that is still missing
FILES="
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

echo "vendor sources at xun-t-oss $REV -> $OUT"
for f in $FILES; do
	[ -s "$OUT/$f" ] && continue
	mkdir -p "$OUT/$(dirname "$f")"
	echo "  $f"
	curl -L --fail --retry 3 -s -o "$OUT/$f.part" "$URL/$f"
	mv "$OUT/$f.part" "$OUT/$f"
done
du -sh "$OUT"
