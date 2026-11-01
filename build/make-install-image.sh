#!/bin/bash
# Build a full Alpine + GNOME install image for the xun tablet.
#
#   ./build/make-install-image.sh [out/xun-install.img] [size]
#
# The rootfs is installed inside an aarch64 Alpine container so package
# scripts run natively (the host needs Docker and a registered qemu-aarch64
# binfmt entry), then wrapped in a sparse ext4 image labelled tapas_root.
# The image is deliberately smaller than the userdata partition; /sbin/init
# grows the filesystem on the first boot.
set -eu
cd "$(dirname "$0")"
BASE=$PWD
IMG=${1:-$BASE/out/xun-install.img}
SIZE=${2:-10G}

if ! docker run --rm --platform linux/arm64 alpine:3.24 uname -m 2>/dev/null |
	grep -q aarch64; then
	echo "registering aarch64 binfmt handlers..."
	docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
fi

mkdir -p "$(dirname "$IMG")"
rm -f "$IMG" "$IMG.sparse"
IMG_REL=${IMG#"$BASE"/}
docker run --rm --platform linux/arm64 \
	-v "$BASE:/build" -w /build \
	-e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" \
	-e IMG="/build/$IMG_REL" -e SIZE="$SIZE" \
	alpine:3.24 /bin/sh /build/install-image-inner.sh

img2simg "$IMG" "$IMG.sparse"
chown "$(id -u):$(id -g)" "$IMG.sparse"
echo "image: $IMG ($(du -h "$IMG" | cut -f1) on disk, sparse $IMG.sparse $(du -h "$IMG.sparse" | cut -f1))"
