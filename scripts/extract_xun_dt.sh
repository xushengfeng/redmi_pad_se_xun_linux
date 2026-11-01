#!/bin/sh
# Extract the stock xun DTB/DTBO from the public partition dump and merge them.
#
# Usage: extract_xun_dt.sh [output-dir]
#
# The dump's extracted dtbo/ directory is used because its root dtbo.img is a
# generic 32-entry Qualcomm image without the xun overlay.
set -eu

out=${1:-xun-dt}
repo=https://github.com/gm-stuffs/redmi_xun_dump.git
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

for tool in git dtc fdtoverlay; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: $tool not found" >&2
        echo "install git and device-tree-compiler" >&2
        exit 1
    fi
done

mkdir -p "$out"
if [ ! -d "$out/dump/.git" ]; then
    git clone --depth 1 --filter=blob:none --no-checkout "$repo" "$out/dump"
fi

# Fetch only the small DT files.
git -C "$out/dump" checkout HEAD -- dtbo aosp-device-tree/prebuilts

git -C "$out/dump" show HEAD:aosp-device-tree/prebuilts/dtb.img > "$out/base.dtb"

mkdir -p "$out/dtbo-dts"
for f in "$out"/dump/dtbo/*.dtb; do
    name=$(basename "${f%.dtb}")
    dtc -I dtb -O dts -o "$out/dtbo-dts/$name.dts" "$f" 2>/dev/null || true
done

xun=$(grep -l 'KHAJE IDP nopmi xun' "$out"/dtbo-dts/*.dts | head -n 1 || true)
if [ -z "$xun" ]; then
    echo "error: xun overlay not found in the extracted dtbo set" >&2
    exit 1
fi

overlay_dtb="$out/dump/dtbo/$(basename "${xun%.dts}").dtb"
echo "xun overlay: $xun"
"$script_dir/merge_dts.sh" "$out/base.dtb" "$overlay_dtb" "$out/xun-merged"

cat > "$out/README.txt" <<EOF
base:    $out/base.dtb (Khaje SoC from vendor_boot)
overlay: $overlay_dtb (KHAJE IDP nopmi xun)
merged:  $out/xun-merged.dtb / $out/xun-merged.dts

Note: the dump's root dtbo.img is a generic 32-entry image; the extracted
dtbo/ set is what contains the xun overlay.
EOF
