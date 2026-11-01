#!/bin/sh
# /sbin/init of the Alpine rootfs installed on the tablet.
#
# The init in the kernel initramfs unbinds the USB gadget before it switches
# root and starts no userspace at all, so this script mounts what the desktop
# needs, brings up udev (without it logind has no seat and mutter does not
# start), puts the USB gadget back (serial console plus the ECM link the host
# reaches 172.16.42.1 over) and then hands over to busybox init, which runs
# /etc/inittab and the /etc/local.d/*.start scripts.
mountpoint -q /proc || mount -t proc proc /proc
mountpoint -q /sys || mount -t sysfs sysfs /sys
mountpoint -q /dev || mount -t devtmpfs devtmpfs /dev
mkdir -p /sys/kernel/config /dev/pts /var/log /var/lib/misc /run
mountpoint -q /sys/kernel/config || mount -t configfs configfs /sys/kernel/config
mountpoint -q /dev/pts || mount -t devpts devpts /dev/pts
# elogind, greetd and dbus want a writable /run
mountpoint -q /run || mount -t tmpfs -o mode=0755,nosuid,nodev tmpfs /run
mkdir -p /run/systemd/seats /run/user

# udev: devtmpfs only creates the nodes, the seat devices have to be coldplugged
if command -v udevd >/dev/null 2>&1; then
	udevd --daemon
	udevadm trigger --action=add
	udevadm settle --timeout=30
fi

# USB gadget: serial console on ttyGS0 and the ECM link to the host
G=/sys/kernel/config/usb_gadget/g1
if [ -d "$G" ]; then
	udc=$(ls /sys/class/udc 2>/dev/null | head -n1)
	if [ -n "$udc" ]; then
		echo "" >"$G/UDC" 2>/dev/null
		sleep 0.2
		echo "$udc" >"$G/UDC"
	fi
	sleep 1
fi
if [ -e /sys/class/net/usb0 ]; then
	ip link set usb0 up 2>/dev/null
	ip addr replace 172.16.42.1/24 dev usb0 2>/dev/null
	/usr/sbin/dnsmasq --conf-file=/etc/dnsmasq.conf
fi
if [ -x /usr/sbin/sshd ]; then
	ssh-keygen -A 2>/dev/null
	mkdir -p /run/sshd
	/usr/sbin/sshd 2>/dev/null
fi

exec /bin/busybox init
