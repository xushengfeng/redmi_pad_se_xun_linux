#!/bin/bash
# Download everything the xun port needs but does not keep in the repository:
# the kernel tarball, the SM6225 patches, the tapas bring-up tree and its boot
# image, the Alpine tooling for the rootfs, and the tools used to regenerate
# the panel driver and the audio topology.
#
# Everything lands where build-xun.sh and the scripts expect it, and existing
# files are kept, so re-running only fetches what is missing.
#
# Firmware is not here: it is pulled from the tablet itself, see docs/BUILD.md.
set -eu
ROOT=$(cd "$(dirname "$0")" && pwd)
SRC=$ROOT/src
DL=$ROOT/downloads
mkdir -p "$SRC/patches" "$DL" "$ROOT/tools/bin" "$ROOT/tapas-images" \
	"$ROOT/rootfs-work"

KERNEL=7.2.1
KERNEL_URL="https://mirrors.tuna.tsinghua.edu.cn/kernel/v7.x/linux-$KERNEL.tar.xz"
SM6225_URL="https://gitlab.postmarketos.org/sm6225-mainline/kernels/linux-v.$KERNEL"
ALPINE_URL="https://mirrors.tuna.tsinghua.edu.cn/alpine/latest-stable"

# Commits of the sm6225-mainline stable-sm6225 branch that the plain kernel
# tarball needs to support xun, applied in this order. They are SM6225 SoC
# support (DPU/DSI/DSC, GPU, remoteprocs, GENI, WCN3990, PMIC). Patches for
# later kernels (ae725a2c, b8463e4b) do not apply to 7.2.1.
SM6225_COMMITS="
650f459de5b9eb6f4f73728fd935d34677074965
8b6545f79b5ff3e589436c0746fc3014739dc12c
f5a976a8334c7563137352eaafe8d49a2a3b8c6a
e127de2a6297931b1abce1fb5b13a415163c325e
"

# Bring-up tree for the Redmi Note 12 4G (tapas): its initramfs init, v4 boot
# shim and kernel base config are the starting point of this port.
TAPAS_REPO=https://github.com/Sype0/pmos-tapas.git
TAPAS_REV=5dee84cc5d7939a1042cf9daf2a21e67ab5c3579
TAPAS_IMAGE_URL=https://github.com/Sype0/pmos-tapas/releases/download/latest/boot-tapas-safe.img

# Generates a mainline DRM panel driver from a stock DSI panel node; used once
# to produce kernel-panel/*.c.
PANELGEN_REPO=https://github.com/msm8916-mainline/linux-mdss-dsi-panel-driver-generator.git
PANELGEN_REV=28c4df25628f6241ce925af15637a443ea2b3bb5

# AudioReach m4 macros; mktplg.sh builds the sound card topology from them.
AUDIOREACH_REPO=https://github.com/linux-msm/audioreach-topology.git
AUDIOREACH_REV=d6bb205aca5f4b8fa6b2f44cf8b5f6a081cbf037

# FT8203 over GENI SPI, the reference the touch driver started from.
FTP8203_REPO=https://github.com/RigoLigoRLC/unixnu-nativefocaltech.git

fetch() { # fetch <url> <file>
	[ -s "$2" ] && return 0
	echo "fetching $2"
	curl -L --fail --retry 3 --progress-bar -o "$2.part" "$1"
	mv "$2.part" "$2"
}

# --- kernel and SM6225 support -------------------------------------------
fetch "$KERNEL_URL" "$SRC/linux-$KERNEL.tar.xz"
if [ ! -d "$SRC/linux-$KERNEL" ]; then
	echo "extracting linux-$KERNEL"
	tar -xJf "$SRC/linux-$KERNEL.tar.xz" -C "$SRC"
fi
for sha in $SM6225_COMMITS; do
	fetch "$SM6225_URL/-/commit/$sha.patch" "$SRC/patches/${sha:0:8}.patch"
done

# --- tapas bring-up tree and its boot image (source of the shim) ---------
if [ ! -d "$ROOT/tapas-bringup" ]; then
	git clone "$TAPAS_REPO" "$ROOT/tapas-bringup"
	git -C "$ROOT/tapas-bringup" checkout -q "$TAPAS_REV"
fi
fetch "$TAPAS_IMAGE_URL" "$ROOT/tapas-images/boot-tapas-safe.img"

# --- tools ----------------------------------------------------------------
[ -d "$DL/panel-generator" ] ||
	git clone "$PANELGEN_REPO" "$DL/panel-generator"
git -C "$DL/panel-generator" checkout -q "$PANELGEN_REV"
[ -d "$DL/audioreach-topology" ] ||
	git clone "$AUDIOREACH_REPO" "$DL/audioreach-topology"
git -C "$DL/audioreach-topology" checkout -q "$AUDIOREACH_REV"
[ -d "$DL/ft8203-reference" ] ||
	git clone "$FTP8203_REPO" "$DL/ft8203-reference"

# --- Alpine bits the rootfs needs ----------------------------------------
# busybox-static for the kernel initramfs, apk-tools-static to build a rootfs
# on a non-Alpine host, and the Alpine minirootfs itself.
alpine_pkg() { # alpine_pkg <name>  -> the .apk file name on the mirror
	curl -s "$ALPINE_URL/main/x86_64/" | grep -o "$1-[0-9][^\"]*\.apk" | head -n1
}

if [ ! -x "$ROOT/tools/bin/busybox.static" ]; then
	pkg=$(alpine_pkg busybox-static)
	fetch "$ALPINE_URL/main/x86_64/$pkg" "$ROOT/tools/busybox-static.apk"
	tar -xzf "$ROOT/tools/busybox-static.apk" -C "$ROOT/tools" bin/busybox.static
	chmod 755 "$ROOT/tools/bin/busybox.static"
fi
fetch "$ALPINE_URL/main/x86_64/$(alpine_pkg apk-tools-static)" \
	"$ROOT/rootfs-work/apk-tools.apk"
if [ ! -s "$ROOT/rootfs-work/mini.tar.gz" ]; then
	mini=$(curl -s "$ALPINE_URL/releases/aarch64/latest-releases.yaml" |
		grep -o 'alpine-minirootfs-[0-9.]*-aarch64\.tar\.gz' | head -n1)
	fetch "$ALPINE_URL/releases/aarch64/$mini" "$ROOT/rootfs-work/mini.tar.gz"
fi

echo "sources are in place"
