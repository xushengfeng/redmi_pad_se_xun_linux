#!/bin/bash
# Copy a boot image to the tablet and write it to slot B (the active slot).
#
#   deploy-boot.sh out/boot-xun.img
#
# The tablet runs the same image in slot B; slot A still holds Android.
set -eu
IMG=${1:?boot image}
ROOT=$(cd "$(dirname "$0")" && pwd)
KEY=$ROOT/ssh/xun_key
TABLET=${TABLET:-root@172.16.42.1}
SSH="ssh -i $KEY -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR"
SCP="scp -i $KEY -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR"

$SCP "$IMG" "$TABLET:/tmp/boot-xun.img"
# The eMMC is mmcblk0 or mmcblk1 depending on probe order: use the by-name link
$SSH "$TABLET" 'set -e; dd if=/tmp/boot-xun.img of=/dev/disk/by-partlabel/boot_b bs=1M; sync; echo written'
md5sum "$IMG"
$SSH "$TABLET" 'dd if=/dev/disk/by-partlabel/boot_b bs=1M 2>/dev/null | head -c '"$(stat -c%s "$IMG")"' | md5sum'
