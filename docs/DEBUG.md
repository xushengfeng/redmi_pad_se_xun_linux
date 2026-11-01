# Debugging the tablet

How to get into a running tablet, watch what it does, and replace parts of it
while it is running. The USB cable carries both the network link the SSH
session runs over and a serial console, so a broken desktop is still
reachable.

## The USB link

The bring-up initramfs creates the configfs gadget (`/sbin/init` in
`build/tapas-bringup/initramfs/init` does the setup) and binds it to the UDC at
boot, before the rootfs is mounted:

- `ecm.usb0`: an Ethernet link to the host; the tablet takes
  `172.16.42.1/24` on `usb0` and runs `dnsmasq` (`build/device/etc/dnsmasq.conf`)
  which hands the host `172.16.42.2`.
- `acm.usb0`: a serial port, `ttyGS0` on the tablet and `/dev/ttyACM0` on the
  host, with a getty at 115200.

The gadget is already bound after the boot, so **plugging the cable into a
running tablet needs nothing enabled on it**. Nothing has to be plugged in at
boot either; unplugging and replugging the cable makes the host see a
disconnect and a new device a few seconds later. The UDC state tells whether
the tablet side is attached:

```sh
cat /sys/class/udc/*/state     # "configured" once the host has enumerated it
```

On the host, after plugging in:

```sh
ip -br link            # a new interface, e.g. enp0s20f0u7
ip -4 addr show dev enp0s20f0u7
ping 172.16.42.1
```

NetworkManager usually takes the DHCP lease by itself. If the interface stays
without an address:

```sh
sudo ip addr add 172.16.42.2/24 dev enp0s20f0u7
sudo ip link set enp0s20f0u7 up
```

If no USB device appears at all:

- `lsusb` and `dmesg | tail` on the host: no enumeration means the cable has no
  data lines (charge-only cable) or the port is a charging port;
- the tablet may simply be off: press the power key for a second. The kernel
  clears the charger power-on triggers before a shutdown (see
  [`SESSION.md`](SESSION.md)), so plugging the cable does not necessarily start
  a tablet that was shut down on purpose;
- if the tablet is running, look at it: `ip -br link` should list `usb0` and
  `cat /sys/class/udc/*/state` should say `configured`. Unplug and replug the
  cable; the controller re-enumerates on connect. If the host still shows
  nothing, reboot the tablet with the cable connected, then check again;
- on a tablet connected over the serial console, the gadget can be checked and
  rebound by hand: `cat /sys/kernel/config/usb_gadget/g1/UDC` (the UDC name, or
  empty when it is not bound) and
  `echo $(ls /sys/class/udc | head -1) >/sys/kernel/config/usb_gadget/g1/UDC`.

## SSH

```sh
ssh -i build/ssh/xun_key -o StrictHostKeyChecking=no root@172.16.42.1
```

`build/ssh/xun_key` is a development key and is not in git (`build/` is
ignored). To recreate it, and install it on the tablet, use the serial console
or the root password (`xun`):

```sh
ssh-keygen -t ed25519 -f build/ssh/xun_key -N ''
# on the tablet:
mkdir -p /root/.ssh && cat >>/root/.ssh/authorized_keys   # paste xun_key.pub
chmod 700 /root/.ssh && chmod 600 /root/.ssh/authorized_keys
```

Useful commands on the tablet:

| What | Command |
| --- | --- |
| kernel log | `dmesg` (also `/tmp/last_kmsg` from the previous boot, from pstore) |
| boot and service state | `rc-status`, `rc-service <name> status` |
| sessions | `loginctl list-sessions`, `loginctl lock-session` |
| GNOME Shell log | `/tmp/gs.log` (the `build/device/usr/bin/gnome-shell` wrapper) |
| session log | `/tmp/session.log` (the `gnome-session` wrapper) |
| battery | `cat /sys/class/power_supply/sh366101/{capacity,voltage_now,temp,status}` |
| sound card | `cat /proc/asound/cards`, `aplay -l` |
| remoteprocs | `cat /sys/class/remoteproc/remoteproc*/{name,state}` |

`su xun -c` does not inherit the shell's session bus. The shell's own address
is in its environment:

```sh
P=$(pgrep -u xun -f '/usr/bin/gnome-shell.real$')
BUS=$(tr '\0' '\n' < /proc/$P/environ | sed -n 's/^DBUS_SESSION_BUS_ADDRESS=//p')
su xun -c "DBUS_SESSION_BUS_ADDRESS=$BUS XDG_RUNTIME_DIR=/run/user/1000 \
    gsettings get org.gnome.settings-daemon.plugins.power power-button-action"
```

## Serial console

The same cable exposes `/dev/ttyACM0` (the acm gadget). It is independent of
the network link and works when `usb0` does not:

```sh
sudo picocom -b 115200 /dev/ttyACM0     # or: screen /dev/ttyACM0 115200
```

The kernel also prints to the display (`console=tty0 consoleblank=0
loglevel=7` in `build/config/xun.config`), so the last lines are visible on the
tablet itself when nothing else works.

## Replacing the kernel and the rootfs files

```sh
cd build
./build-xun.sh                      # -> out/boot-xun.img
./deploy-boot.sh out/boot-xun.img   # writes boot_b over SSH, verifies the md5
ssh -i ssh/xun_key root@172.16.42.1 reboot
```

`deploy-boot.sh` needs the tablet at `172.16.42.1`; a boot takes 60-70 s,
GNOME included.

The userspace files (`build/device`, `build/ucm2`) are installed from the
tablet:

```sh
cd build
./deploy-rootfs.sh                  # run on the tablet, as root
```

It copies the files, creates `xun` if missing, runs `dconf update` and drops
greetd from the default runlevel. Reboot afterwards; restarting just GDM
(`rc-service gdm restart`) is enough for session settings but starts a new
session.

`/sbin/init` itself is not replaced by that script: it comes from
`build/rootfs-init.sh` when the rootfs image is built, and replacing the init of
a running system is only worth it when the image itself is being reinstalled.

## Fastboot and ADB

The bootloader accepts the image directly, which writes nothing:

```sh
fastboot boot out/boot-xun.img
```

Flashing to the inactive slot and switching to it:

```sh
fastboot flash boot_b out/boot-xun.img
fastboot --set-active=b
fastboot reboot
```

If the tablet ends up in the stock recovery, `adb reboot bootloader` (with USB
debugging working there) or a long power-key press gets it back. The boot slot
is marked good at boot by `build/device/etc/local.d/10-boot-success.start`
(`qbootctl -m`), so ABL does not fall back to recovery after a few boots.

## Collecting state for a report

```sh
dmesg > /tmp/dmesg.txt
cat /proc/asound/cards /proc/asound/pcm > /tmp/audio.txt
cat /sys/class/power_supply/sh366101/* > /tmp/battery.txt 2>/dev/null
cat /proc/device-tree/model
```

Copy them over with `scp -i build/ssh/xun_key root@172.16.42.1:/tmp/dmesg.txt .`
