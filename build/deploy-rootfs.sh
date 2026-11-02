#!/bin/sh
# Install the on-device files of the port on the tablet and apply the settings
# that live outside them (account, display manager, dconf). Run it as root on
# the tablet, from this directory:
#
#   cd /path/to/build && ./deploy-rootfs.sh
#
# It is safe to run again: files are copied over, the user is only created
# when missing, and the dconf database is recompiled.
set -eu
ROOT=$(cd "$(dirname "$0")" && pwd)

# Account for autologin and the desktop session (see docs/SESSION.md).
if ! id xun >/dev/null 2>&1; then
	adduser -D -u 1000 -s /bin/bash xun
	echo 'xun:xun' | chpasswd
	for g in wheel audio video input netdev plugdev; do
		addgroup xun "$g" 2>/dev/null || true
	done
fi

# GDM runs the session; greetd stays installed as a manual fallback.
rc-update del greetd default 2>/dev/null || true

# gnome-session/gnome-shell are wrappers around the distribution binaries
# (see build/device/usr/bin). A package upgrade replaces the wrapper with the
# new binary; move that binary aside so the wrapper can go back in front of
# it instead of copying the old .real over the new version.
for b in gnome-session gnome-shell; do
	if [ -f "/usr/bin/$b" ] &&
		! grep -q "/usr/bin/$b.real" "/usr/bin/$b" 2>/dev/null; then
		mv "/usr/bin/$b" "/usr/bin/$b.real"
	fi
done

cp -a "$ROOT/device/." /
cp -a "$ROOT/ucm2/." /usr/share/alsa/ucm2/
chmod +x /etc/local.d/*.start 2>/dev/null || true

# System dconf defaults: power key action, idle lock.
if command -v dconf >/dev/null 2>&1; then
	dconf update
fi

# /sbin/init is deployed by the rootfs image, not by this script: replacing
# the running system's init with a copy that may not match is not useful.
echo "session files installed; reboot to start the session with them"
