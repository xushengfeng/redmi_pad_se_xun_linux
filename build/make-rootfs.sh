#!/bin/bash
# Build a minimal Alpine aarch64 rootfs for the xun bring-up kernel and wrap
# it in an ext4 image labelled tapas_root, which the bring-up init picks up.
set -eu
cd "$(dirname "$0")"
WORK=$PWD/rootfs-work
ROOT=$WORK/rootfs
IMG=${1:-$PWD/xun-rootfs.img}
SIZE=${2:-2G}
MIRROR=https://mirrors.tuna.tsinghua.edu.cn/alpine/latest-stable

# apk.static from the host arch: it can install other architectures into --root
if [ ! -x "$WORK/apk.static" ]; then
	tar -xzf "$WORK/apk-tools.apk" -C "$WORK" sbin/apk.static
	mv "$WORK/sbin/apk.static" "$WORK/apk.static"
fi

rm -rf "$ROOT"
mkdir -p "$ROOT"
tar -xzf "$WORK/mini.tar.gz" -C "$ROOT"

"$WORK/apk.static" --root "$ROOT" --arch aarch64 --no-cache -U --allow-untrusted \
	--repository "$MIRROR/main" --repository "$MIRROR/community" \
	add alpine-base e2fsprogs e2fsprogs-extra openssh sshfs util-linux \
	iproute2 curl nano htop tmux bash procps kmod || echo "apk reported errors (triggers cannot run on x86_64), continuing"

# Repos for the installed system (the device has no internet yet, but this
# makes later `apk add` work as soon as networking does).
mkdir -p "$ROOT/etc/apk"
printf '%s\n' "$MIRROR/main" "$MIRROR/community" >"$ROOT/etc/apk/repositories"

echo xun >"$ROOT/etc/hostname"
printf 'nameserver 223.5.5.5\nnameserver 1.1.1.1\n' >"$ROOT/etc/resolv.conf"

# root password: xun
HASH=$(openssl passwd -6 xun)
sed -i "s|^root:[^:]*:|root:$HASH:|" "$ROOT/etc/shadow"
sed -i 's|^root:[^:]*:|root:x:|' "$ROOT/etc/passwd" 2>/dev/null || true

# init: rootfs-init.sh is the /sbin/init the installed system runs
rm -f "$ROOT/sbin/init"
install -m 755 rootfs-init.sh "$ROOT/sbin/init"

# inittab: console on the screen and on the USB serial function
cat >"$ROOT/etc/inittab" <<'EOF'
::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default
tty0::respawn:/sbin/getty -L 115200 tty0 vt100
ttyGS0::respawn:/sbin/getty -L 115200 ttyGS0 vt100
tty1::respawn:/sbin/getty 38400 tty1
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/openrc shutdown
EOF

# sshd with the root password; telnetd already gives a password-less shell
if [ -f "$ROOT/etc/ssh/sshd_config" ]; then
	sed -i 's/^#*PermitRootLogin.*/PermitRootLogin yes/' "$ROOT/etc/ssh/sshd_config"
fi

mkdir -p "$ROOT/proc" "$ROOT/sys" "$ROOT/dev" "$ROOT/tmp"
chmod 1777 "$ROOT/tmp"

echo "rootfs: $(du -sh "$ROOT" | cut -f1)"

# mke2fs -d runs as the calling user and must be able to read everything
find "$ROOT" -type f ! -perm -u+r -exec chmod u+r {} +

rm -f "$IMG"
truncate -s "$SIZE" "$IMG"
mke2fs -q -t ext4 -L tapas_root -E root_owner=0:0 -d "$ROOT" -F "$IMG"
echo "image: $IMG ($(du -h "$IMG" | cut -f1), label tapas_root)"
