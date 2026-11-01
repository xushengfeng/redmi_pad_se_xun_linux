#!/bin/sh
# Merge a base DTB with an overlay and decompile the result.
#
# Usage: merge_dts.sh <base.dtb> <overlay.dtb> <out-prefix>
#
# Produces <out-prefix>.dtb and <out-prefix>.dts.
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 <base.dtb> <overlay.dtb> <out-prefix>" >&2
    exit 2
fi

base=$1
overlay=$2
out=$3

for tool in fdtoverlay dtc; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: $tool not found (install device-tree-compiler)" >&2
        exit 1
    fi
done

fdtoverlay -i "$base" -o "$out.dtb" "$overlay"
dtc -q -I dtb -O dts -o "$out.dts" "$out.dtb"
echo "wrote $out.dtb and $out.dts"
