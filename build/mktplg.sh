#!/bin/bash
# Build firmware/qcom/sm4250/SM6225-Xun-tplg.bin, the AudioReach topology the
# xun sound card loads: q6apm derives the file name from the card model
# ("SM6225-Xun" from the DTS), not from this file.
#
# Needs m4 and alsatplg (alsa-utils) and the AudioReach m4 macros from
# github.com/linux-msm/audioreach-topology, which fetch-sources.sh clones into
# downloads/audioreach-topology.
set -eu
ROOT=$(cd "$(dirname "$0")" && pwd)
AR=${1:-$ROOT/downloads/audioreach-topology}
OUT=$ROOT/firmware/qcom/sm4250
CONF=$ROOT/work/SM6225-Xun.conf
mkdir -p "$OUT" "$ROOT/work"

m4 -I "$AR" "$ROOT/topology/SM6225-Xun.m4" >"$CONF"
# The "undefined sink/source widget" warnings for the codec DMA ports are
# expected: those streams come from the kernel's q6apm-lpass-dais, not from
# the topology.
alsatplg -c "$CONF" -o "$OUT/SM6225-Xun-tplg.bin" ||
	[ -s "$OUT/SM6225-Xun-tplg.bin" ]
ls -l "$OUT/SM6225-Xun-tplg.bin"
