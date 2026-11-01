# Session, lock screen, power key and power off

What happens between the kernel and the GNOME desktop on the tablet: which
account logs in, which display manager runs the session, how the lock screen
is reached, what the power key does, and how the PMIC is told to power the
board off instead of resetting it.

## Account

| Account | Password | Used for |
| --- | --- | --- |
| `xun` (uid/gid 1000, `/home/xun`, `/bin/bash`) | `xun` | the GNOME session, autologin |
| `root` | `xun` | the serial/USB console and SSH on `172.16.42.1` |

`build/make-rootfs.sh` creates both while it builds the rootfs image (the
passwords are SHA-512 hashes written into `/etc/shadow`), and
`build/deploy-rootfs.sh` creates `xun` if it is missing on an installed
system. Change the password on the tablet with `passwd` (and `passwd root` for
the console).

## Display manager and lock screen

The session is run by **GDM** in Wayland mode, with automatic login of `xun`;
`build/device/etc/gdm/custom.conf` holds that setting. greetd remains
installed as a manual fallback but is not in the default runlevel.

GDM is not only a convenience here. GNOME Shell 50 decides whether the
machine can lock at all in `js/misc/loginManager.js`:

```js
function haveSystemd() {
    return GLib.access('/run/systemd/seats', 0) >= 0;
}

export function canLock() {
    try {
        const version = ...;  // org.gnome.DisplayManager.Manager.Version
        return haveSystemd() && versionCompare('3.5.91', version);
    } catch {
        return false;
    }
}
```

When `canLock()` is false, `Main.screenShield` is never created: there is no
lock button in the quick settings, `Super+L` fails with "Could not lock
screen", and nothing owns `org.gnome.Shell.ScreenShield`. `haveSystemd()` only
checks for `/run/systemd/seats`, which systemd-logind creates and elogind does
not, so the bring-up creates that directory itself. Both
`build/rootfs-init.sh` and `50-gnome.start` do it before GDM starts.

With GDM and the seat directory in place:

- the quick settings menu (top right) shows a **lock icon** next to the power
  button, which is the touch-friendly way to lock;
- `Super+L` locks when a keyboard is attached;
- after five minutes of idle the screen blanks (`idle-delay=300`); automatic
  locking is off by default because the IBus input context does not survive a
  resume (see "Suspend and resume");
- unlocking asks for the `xun` password, with the GNOME on-screen keyboard on
  touch, when locking is enabled.

The unlock dialog authenticates through GDM (`Gdm.Client` /
`org.gnome.DisplayManager`); a lock screen without GDM cannot verify the
password, which is why the display manager is part of this setup.

Manual locking works with `lock-enabled=false`; only the automatic lock after
idle/suspend is off. `lock-enabled=true` can be set again once the resume
bug below is fixed.

Check from SSH (the session bus address is the one gnome-shell runs with):

```sh
P=$(pgrep -u xun -f '/usr/bin/gnome-shell.real$')
BUS=$(tr '\0' '\n' < /proc/$P/environ | sed -n 's/^DBUS_SESSION_BUS_ADDRESS=//p')
su xun -c "DBUS_SESSION_BUS_ADDRESS=$BUS XDG_RUNTIME_DIR=/run/user/1000 \
    gdbus call --session --dest org.gnome.ScreenSaver \
    --object-path /org/gnome/ScreenSaver \
    --method org.gnome.ScreenSaver.GetActive"
loginctl lock-session        # locks
loginctl unlock-session      # unlocks, as root
```

## Power key

Suspend works (see "Suspend and resume"), but a short press of the power key
opens the shutdown dialog rather than suspending. The system dconf database
(`build/device/etc/dconf/db/local.d/00-xun`, installed by
`deploy-rootfs.sh`) sets it to:

```ini
[org/gnome/settings-daemon/plugins/power]
power-button-action='interactive'
```

A short press opens the shutdown dialog with Restart / Power Off / Cancel;
suspend is in the system menu / quick settings and via `loginctl suspend`. A
long press still does what the PMIC does on its own (hard reset), independent
of the OS.

## Power off through the PMIC

On this SoC the AP releases **PS_HOLD** to tell the PMIC to power the board
down, and the PMIC decides what that means from the PON block:

| Register (PM6125 PON, base `0x800`) | Meaning |
| --- | --- |
| `0x85a` `PS_HOLD_RST_CTL` | action on PS_HOLD release: `0x1` warm reset, `0x4` shutdown, `0x7` hard reset (PMIC default) |
| `0x85b` `PS_HOLD_RST_CTL2` bit 7 | enable PS_HOLD reset action |
| `0x880` `PON_TRIGGER_EN` | power-on sources: SMPL, RTC, DC_CHG, USB_CHG, PON1, CBLPWR, KPDPWR |

Two things make the default path a reset:

1. the action is left at the hardware default, hard reset (`0x07`);
2. the PS_HOLD register in the TCSR block (`0x440b000`) does not release
   PS_HOLD when written from Linux alone, so the plain `writel(0)` of
   `msm-poweroff` changes nothing and firmware resets the SoC afterwards. The
   vendor driver asks the secure monitor to do it and keeps the write only as
   a fallback.

Two kernel patches, both applied by `build/build-xun.sh`, fix that:

- `build/patches/qcom-pon-poweroff.patch` (`drivers/power/reset/qcom-pon.c`):
  registers a `SYS_OFF_MODE_POWER_OFF_PREPARE` handler that clears
  `PS_HOLD_RST_CTL2` bit 7, sets `PS_HOLD_RST_CTL` to `0x04` (shutdown),
  re-enables the action, and clears the DC_CHG/USB_CHG/CBLPWR power-on
  triggers so external power cannot bring the board back by itself.
  It also adds two sysfs attributes for bring-up: `pon_regs` (a dump of the
  registers above) and `poweroff_prepare` (runs the configuration without
  powering off).
- `build/patches/msm-poweroff-scm.patch` (`drivers/power/reset/msm-poweroff.c`):
  before the fallback `writel`, calls the secure monitor with
  `QCOM_SCM_SVC_PWR` (`0x09`) / `QCOM_SCM_PWR_IO_DEASSERT_PS_HOLD` (`0x02`),
  which releases PS_HOLD even though the TCSR register is protected.

After a shutdown the PON block records the reason:

```sh
D=/sys/devices/platform/soc@0/1c40000.spmi/spmi-0/0-00/1c40000.spmi:pmic@0:pon@800
cat $D/pon_regs       # 5a=04 5b=80 ... c5=02 ... before power off
```

`c5` (`POFF_REASON1`) reads `0x02`, "PS_HOLD", and the following boot has no
hard reset bit in `c0` (`PON_REASON1`): the board really lost power.

**Caveat:** while the USB cable is plugged in, the PMIC sees external power
(CBL, power-on reason `0x40` in `c0`) and starts the board again right after
the shutdown. That is the normal Qualcomm behaviour of a charger-attached
device, not a failed power off; unplug the cable to keep the tablet off. The
power key always starts it.

Test it on the tablet:

```sh
D=/sys/devices/platform/soc@0/1c40000.spmi/spmi-0/0-00/1c40000.spmi:pmic@0:pon@800
cat $D/pon_regs                # 5a=07 5b=80 80=e4 ... on a fresh boot
echo 1 > $D/poweroff_prepare   # set the shutdown type, without powering off
cat $D/pon_regs                # 5a=04 5b=80 80=a4 ...
poweroff                       # with USB unplugged: stays off
```

## Double-tap to wake

The touch controller follows the panel state through the DRM panel follower
API (`panel = <&panel>` on the touchscreen node). When the panel is disabled
(the screen blanked), `build/kernel-touch/ft8203-spi.c` puts the FT8203 into
its gesture mode (`0xd0` = 1, all gesture types in `0xd1`/`0xd2`/`0xd5`-`0xd8`).
The controller then scans slowly and only reports gestures.

On a double tap it reads the gesture block from `0xd3` (id `0x24`), reports
`KEY_WAKEUP` through a small extra keyboard input device, and sends a
synthetic tap. Key events from the touchscreen device itself are dropped by
libinput, which is why the wake key is a separate device; GNOME wakes on the
key/tap and re-enables the panel, and the follower callback leaves gesture
mode again.

While the panel is off a single tap does nothing; the power key still wakes
the tablet. The "gesture mode on"/"double tap" messages are at debug level.

## Suspend and resume

`loginctl suspend` (and GNOME's power menu entry) puts the SoC into PSCI
deep suspend. The power key is the reliable wake source; a PMIC RTC alarm
woke the system only once out of several attempts, so it is not used.

After resuming, the dwc3 controller does not re-enumerate the configfs
gadget: the host loses the ECM network and the serial console. The elogind
hook `50-usb-gadget.sh` (in `/usr/libexec/elogind/system-sleep`) unbinds and
binds dwc3 again, binds the UDC and restores `usb0` and dnsmasq; the gadget
then comes back a few seconds after the wake. The hook logs to
`/var/log/xun-sleep.log` and retries the UDC bind once if `usb0` does not
appear. The same hook is the place to put anything else that has to be redone
after a resume.

The system dconf keeps the tablet awake on external power
(`sleep-inactive-ac-type='nothing'`) because the USB gadget is the
development link and the host does not reliably re-enumerate it after a
resume. On battery the GNOME default idle suspend (900 s) still applies.

**Known problem after a resume:** the shell's IBus input context is gone
(`Error processing key on IM: ... Object does not exist at path
/org/freedesktop/IBus/InputContext_2`), so the lock screen does not receive
a typed password and the session cannot be unlocked. Until that is fixed,
automatic locking is disabled (`org.gnome.desktop.screensaver lock-enabled
false`); the screen still blanks on idle and a touch brings it back.

## Files

| File | Role |
| --- | --- |
| `build/make-rootfs.sh` | creates `root` and `xun` with their passwords |
| `build/deploy-rootfs.sh` | installs `build/device` and `build/ucm2`, creates `xun` if missing, runs `dconf update`, drops greetd from the default runlevel |
| `build/device/etc/gdm/custom.conf` | GDM autologin of `xun` on Wayland |
| `build/device/etc/local.d/50-gnome.start` | dbus, elogind, polkit, `/run/systemd/seats`, GDM |
| `build/device/etc/dconf/db/local.d/00-xun` | power key action, idle time, lock settings |
| `build/device/usr/bin/gnome-session` | session wrapper: locale `zh_CN.UTF-8`, `XDG_RUNTIME_DIR`, log in `/tmp/session.log` |
| `build/device/usr/bin/gnome-shell` | shell wrapper, log in `/tmp/gs.log` |
| `build/patches/qcom-pon-poweroff.patch` | PMIC PS_HOLD action and power-on triggers |
| `build/patches/msm-poweroff-scm.patch` | PS_HOLD release through the secure monitor |
