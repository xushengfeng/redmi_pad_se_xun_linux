#!/bin/sh
# Build the Alpine rootfs for the xun install image. Runs as root inside an
# aarch64 Alpine container (build/make-install-image.sh starts it), so package
# scripts execute natively instead of failing with Exec format errors.
set -eu

BASE=${BASE:-/build}
ROOT=$BASE/rootfs-install/rootfs
WORK=$BASE/rootfs-install
MIRROR=${MIRROR:-https://mirrors.tuna.tsinghua.edu.cn/alpine/latest-stable}
HOST_UID=${HOST_UID:-1000}
HOST_GID=${HOST_GID:-1000}

# Use the same mirror inside the container; dl-cdn is slow or unreachable from
# here. openssl is for the password hash, e2fsprogs builds the image.
printf '%s\n' "$MIRROR/main" "$MIRROR/community" >/etc/apk/repositories
apk add --no-cache openssl e2fsprogs zstd >/dev/null

rm -rf "$ROOT"
mkdir -p "$ROOT" "$WORK/apk-cache"
tar -xzf "$BASE/rootfs-work/mini.tar.gz" -C "$ROOT"

# Install into the target root. `apk --root` runs the package scripts inside
# the target (aarch64 binaries through binfmt), so triggers compile schemas,
# icon caches and font caches for us.
printf '%s\n' "$MIRROR/main" "$MIRROR/community" >"$ROOT/etc/apk/repositories"
PKGS=$( { sed -e 's/#.*//' -e 's/[[:space:]]*$//' "$BASE/install-packages.txt"
	sed -e 's/#.*//' -e 's/[[:space:]]*$//' "$BASE/lang-packages.txt"; } |
	grep -v '^$' | sort -u | tr '\n' ' ')
# shellcheck disable=SC2086
apk --root "$ROOT" --arch aarch64 --allow-untrusted \
	--cache-dir "$WORK/apk-cache" \
	--repository "$MIRROR/main" --repository "$MIRROR/community" \
	add $PKGS
rm -rf "$ROOT/var/cache/apk"

# --- board files ---------------------------------------------------------
# The bring-up rootfs renamed the distribution session binaries and put
# wrappers in front of them to force the Chinese locale and log the session.
for b in gnome-session gnome-shell; do
	if [ -f "$ROOT/usr/bin/$b" ] && [ ! -e "$ROOT/usr/bin/$b.real" ]; then
		mv "$ROOT/usr/bin/$b" "$ROOT/usr/bin/$b.real"
	fi
done
cp -a "$BASE/device/." "$ROOT/"
chmod 755 "$ROOT"/etc/local.d/*.start "$ROOT"/usr/local/bin/time-sync \
	"$ROOT"/usr/libexec/elogind/system-sleep/*.sh

mkdir -p "$ROOT/usr/share/alsa/ucm2"
cp -a "$BASE/ucm2/." "$ROOT/usr/share/alsa/ucm2/"

mkdir -p "$ROOT/lib/firmware"
cp -a "$BASE/firmware/." "$ROOT/lib/firmware/"

# cp -a preserves the source owner (the build user); the installed files
# belong to root except for the home directory and the private SSH keys.
# Only the copied files are touched here: a recursive chown of /usr/libexec
# would clear the setuid bit on dbus-daemon-launch-helper.
chown -R root:root "$ROOT/etc/local.d" "$ROOT/etc/dconf" \
	"$ROOT/etc/gdm" "$ROOT/etc/NetworkManager" "$ROOT/etc/bluetooth" \
	"$ROOT/etc/xdg" "$ROOT/etc/greetd" "$ROOT/etc/dnsmasq-xun.conf" \
	"$ROOT/usr/local" "$ROOT/usr/libexec/elogind" \
	"$ROOT/usr/share/alsa/ucm2" "$ROOT/lib/firmware" \
	"$ROOT/usr/bin/gnome-session" "$ROOT/usr/bin/gnome-shell"
# cp -a also copied the directory attributes of build/device/etc and
# build/device/usr onto /etc and /usr themselves.
chown root:root "$ROOT/etc" "$ROOT/usr" "$ROOT/usr/bin" "$ROOT/usr/local"

rm -f "$ROOT/sbin/init"
install -m 755 "$BASE/rootfs-init.sh" "$ROOT/sbin/init"

# --- system configuration ------------------------------------------------
echo xun >"$ROOT/etc/hostname"
cat >"$ROOT/etc/hosts" <<'HOSTS'
127.0.0.1 localhost xun
::1 localhost
HOSTS
printf 'nameserver 223.5.5.5\nnameserver 1.1.1.1\n' >"$ROOT/etc/resolv.conf"
cat >"$ROOT/etc/fstab" <<'FSTAB'
LABEL=tapas_root / ext4 defaults,noatime 0 1
FSTAB
cat >"$ROOT/etc/locale.conf" <<'LOCALE'
LANG=zh_CN.UTF-8
LC_ALL=zh_CN.UTF-8
LOCALE
cat >"$ROOT/etc/environment" <<'ENV'
LANG=zh_CN.UTF-8
LC_ALL=zh_CN.UTF-8
ENV
ln -sf /usr/share/zoneinfo/Asia/Shanghai "$ROOT/etc/localtime"
echo Asia/Shanghai >"$ROOT/etc/timezone"

cat >"$ROOT/etc/inittab" <<'INITTAB'
::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default
tty0::respawn:/sbin/getty -L 115200 tty0 vt100
ttyGS0::respawn:/sbin/getty -L 115200 ttyGS0 vt100
tty1::respawn:/sbin/agetty --autologin xun --noclear tty1 38400
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/openrc shutdown
INITTAB

# --- accounts ------------------------------------------------------------
HASH=$(openssl passwd -6 xun)
if ! grep -q '^xun:' "$ROOT/etc/passwd"; then
	echo 'xun:x:1000:1000::/home/xun:/bin/bash' >>"$ROOT/etc/passwd"
	echo 'xun:x:1000:' >>"$ROOT/etc/group"
	# pam_unix treats a zero last-change day as an expired password, which
	# makes GDM autologin fail; use day 1 of the epoch instead.
	echo "xun:$HASH:1:0:99999:7:::" >>"$ROOT/etc/shadow"
	echo 'xun:!::' >>"$ROOT/etc/gshadow"
	mkdir -p "$ROOT/home/xun"
	cp -a "$ROOT/etc/skel/." "$ROOT/home/xun/" 2>/dev/null || true
fi
sed -i "s|^root:[^:]*:[^:]*:|root:$HASH:1:|" "$ROOT/etc/shadow"
for g in wheel audio video input netdev plugdev render bluetooth; do
	grep -q "^$g:" "$ROOT/etc/group" || continue
	awk -F: -v g="$g" -v OFS=: '
		$1 == g {
			if ($4 == "") $4 = "xun"
			else if ($4 !~ /(^|,)xun(,|$)/) $4 = $4 ",xun"
		}
		{ print }
	' "$ROOT/etc/group" >"$ROOT/etc/group.new"
	mv "$ROOT/etc/group.new" "$ROOT/etc/group"
done
chown -R 1000:1000 "$ROOT/home/xun"
chmod 700 "$ROOT/root"
chmod 640 "$ROOT/etc/shadow" "$ROOT/etc/gshadow"

# GDM's home is not created by the package's OpenRC subpackage, and
# accounts-service wants its user database directory before GDM starts.
GDM_UID=$(awk -F: '$1 == "gdm" { print $3 }' "$ROOT/etc/passwd")
GDM_GID=$(awk -F: '$1 == "gdm" { print $4 }' "$ROOT/etc/passwd")
install -d -m 755 -o "${GDM_UID:-32}" -g "${GDM_GID:-32}" "$ROOT/var/lib/gdm"
install -d -m 775 "$ROOT/var/lib/AccountsService"
install -d -m 700 "$ROOT/var/lib/AccountsService/users"
cat >"$ROOT/var/lib/AccountsService/users/xun" <<'ACCOUNTS'
[User]
Session=
Icon=/home/xun/.face
SystemAccount=false
ACCOUNTS
chmod 600 "$ROOT/var/lib/AccountsService/users/xun"

# --- SSH access ----------------------------------------------------------
install -d -m 700 "$ROOT/root/.ssh"
install -m 600 "$BASE/ssh/xun_key.pub" "$ROOT/root/.ssh/authorized_keys"
install -d -m 700 -o 1000 -g 1000 "$ROOT/home/xun/.ssh"
install -m 600 -o 1000 -g 1000 "$BASE/ssh/xun_key.pub" \
	"$ROOT/home/xun/.ssh/authorized_keys"

SSHD=$ROOT/etc/ssh/sshd_config
sed -i 's/^#*PermitRootLogin.*/PermitRootLogin prohibit-password/' "$SSHD"
sed -i 's/^#*PubkeyAuthentication.*/PubkeyAuthentication yes/' "$SSHD"
sed -i 's/^#*PasswordAuthentication.*/PasswordAuthentication yes/' "$SSHD"
grep -q '^PermitRootLogin' "$SSHD" || echo 'PermitRootLogin prohibit-password' >>"$SSHD"
grep -q '^PubkeyAuthentication' "$SSHD" || echo 'PubkeyAuthentication yes' >>"$SSHD"
grep -q '^PasswordAuthentication' "$SSHD" || echo 'PasswordAuthentication yes' >>"$SSHD"

# --- boot services -------------------------------------------------------
# Keep the proven runlevel from the bring-up rootfs; the GDM start lives in
# local.d/50-gnome.start so it runs after dbus, elogind and polkit.
for s in dbus elogind polkit local networkmanager bluetooth crond \
	udev-postmount sshd chronyd; do
	[ -x "$ROOT/etc/init.d/$s" ] || { echo "missing init script: $s" >&2; continue; }
	ln -sf "/etc/init.d/$s" "$ROOT/etc/runlevels/default/$s"
done

# --- caches and compiled data -------------------------------------------
chroot "$ROOT" /bin/sh -c '
set -e
command -v glib-compile-schemas >/dev/null && glib-compile-schemas /usr/share/glib-2.0/schemas
command -v gio-querymodules >/dev/null && gio-querymodules /usr/lib/gio/modules || true
command -v update-desktop-database >/dev/null && update-desktop-database -q /usr/share/applications || true
command -v update-mime-database >/dev/null && update-mime-database /usr/share/mime || true
command -v fc-cache >/dev/null && fc-cache -f >/dev/null 2>&1 || true
for d in /usr/share/icons/*; do
	[ -f "$d/index.theme" ] || continue
	command -v gtk-update-icon-cache >/dev/null && gtk-update-icon-cache -f -t "$d" >/dev/null 2>&1 || true
done
if command -v gdk-pixbuf-query-loaders >/dev/null; then
	mkdir -p /usr/lib/gdk-pixbuf-2.0/2.10.0
	gdk-pixbuf-query-loaders > /usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
fi
command -v update-ca-certificates >/dev/null && update-ca-certificates || true
command -v udevadm >/dev/null && udevadm hwdb --update || true
command -v dconf >/dev/null && dconf update || true
'

# Alpine ships the WCN3990 and regulatory firmware zstd-compressed, but the
# xun kernel only has the XZ decompressor. Leave the .zst files in place and
# add the uncompressed names the firmware loader looks for.
if [ -d "$ROOT/lib/firmware/ath10k/WCN3990" ]; then
	find "$ROOT/lib/firmware/ath10k/WCN3990" -name '*.zst' | while read -r f; do
		zstd -d -f "$f" -o "${f%.zst}" >/dev/null 2>&1 || true
	done
fi
for f in "$ROOT"/lib/firmware/regulatory.db.zst \
	"$ROOT"/lib/firmware/regulatory.db.p7s.zst; do
	[ -f "$f" ] && zstd -d -f "$f" -o "${f%.zst}" >/dev/null 2>&1 || true
done
chown -R root:root "$ROOT/lib/firmware"

# Build the filesystem while the tree is still owned by root: mke2fs -d copies
# the ownership and the setuid bits into the image. root can also read the
# execute-only helpers without changing their modes.
if [ -n "${IMG:-}" ]; then
	rm -f "$IMG"
	truncate -s "${SIZE:-10G}" "$IMG"
	mke2fs -q -t ext4 -L tapas_root -E root_owner=0:0 -d "$ROOT" -F "$IMG"
	chown "$HOST_UID:$HOST_GID" "$IMG"
fi

# Leave the tree owned by the host user so the work directory can be cleaned
# without sudo.
chown -R "$HOST_UID:$HOST_GID" "$ROOT"
echo "rootfs: $(du -sh "$ROOT" | cut -f1)"
