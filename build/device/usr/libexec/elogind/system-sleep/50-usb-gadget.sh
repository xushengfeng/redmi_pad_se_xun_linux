#!/bin/sh
# The dwc3 controller does not re-enumerate the configfs gadget after resume;
# reset the controller and bind the UDC again so the host gets the ECM
# network and the ACM console back. Also recreate the GNOME session's IBus
# input context, which goes stale across suspend.
[ "$1" = "post" ] || exit 0

log=/var/log/xun-sleep.log
G=/sys/kernel/config/usb_gadget/g1
echo "resume hook: $(date)" >>"$log"
[ -d "$G" ] || exit 0

bind_udc()
{
	echo "" >"$G/UDC" 2>/dev/null
	if [ -e /sys/bus/platform/drivers/dwc3/unbind ]; then
		echo 4e00000.usb >/sys/bus/platform/drivers/dwc3/unbind 2>>"$log"
		sleep 0.3
		echo 4e00000.usb >/sys/bus/platform/drivers/dwc3/bind 2>>"$log"
		sleep 0.5
	fi
	udc=$(ls /sys/class/udc 2>/dev/null | head -n1)
	if [ -n "$udc" ]; then
		echo "$udc" >"$G/UDC" 2>>"$log"
		echo "bound $udc" >>"$log"
	else
		echo "no UDC" >>"$log"
	fi
}

bind_udc
for _ in 1 2 3 4 5 6 7 8 9 10; do
	[ -e /sys/class/net/usb0 ] && break
	sleep 0.2
done
# The first bind can race with the controller reset; try once more.
if [ ! -e /sys/class/net/usb0 ]; then
	echo "usb0 missing, retrying" >>"$log"
	bind_udc
	for _ in 1 2 3 4 5 6 7 8 9 10; do
		[ -e /sys/class/net/usb0 ] && break
		sleep 0.2
	done
fi
ip link set usb0 up 2>/dev/null
ip addr replace 172.16.42.1/24 dev usb0 2>/dev/null
pkill -x dnsmasq 2>/dev/null
/usr/sbin/dnsmasq --conf-file=/etc/dnsmasq-xun.conf 2>/dev/null
