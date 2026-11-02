# Updating the tablet

## What lives where

| Part | Contents | Updating it touches user data? |
| --- | --- | --- |
| `boot_b` | kernel, DTB, initramfs (board drivers, ADSP firmware, topology) | no |
| `userdata` (`mmcblk0p91`) | Alpine rootfs: `/usr`, `/etc`, `/var`, `/home/xun` | **yes, if reflashed** |
| slot A | Android, untouched by the Linux install | no |

There is no separate data partition. `/home/xun`, Wi-Fi passwords in
`/etc/NetworkManager/system-connections`, Bluetooth pairings in
`/var/lib/bluetooth` and the accounts database all sit on `userdata` with the
rest of the rootfs. Flashing a new rootfs image erases them; flashing
`boot_b` does not.

## The safe update paths

### Kernel, DTB, board drivers

Most driver work only changes the boot image. The tablet keeps its rootfs and
all user data:

```sh
cd build
./build-xun.sh
fastboot boot out/boot-xun.img     # optional one-shot test, writes nothing
./deploy-boot.sh out/boot-xun.img  # writes boot_b over SSH, verifies the md5
ssh -i ssh/xun_key root@172.16.42.1 reboot
```

`deploy-boot.sh` saves the current `boot_b` to `out/boot-xun.prev.img` before
writing, so a kernel that does not boot can be rolled back from fastboot:

```sh
fastboot flash boot_b out/boot-xun.prev.img
```

If the new kernel fails, the rootfs and data are still on the eMMC. Enter
fastboot with **power + volume-down** and flash the previous boot image or
any known-good `out/boot-xun.img`.

Use `fastboot boot` first when testing a risky change: it boots the image
without writing either slot. A plain reboot returns to the installed kernel.

### Alpine packages and desktop applications

```sh
apk update
apk upgrade
```

This only replaces files under `/usr`, `/etc` and `/var`; `/home/xun` is not
touched. Review `/etc/*.apk-new` after the upgrade, then reboot (or restart
the affected service). The kernel is not an Alpine package here, so
`apk upgrade` never changes `boot_b`.

The port puts wrappers at `/usr/bin/gnome-session` and
`/usr/bin/gnome-shell` to force the Chinese locale and keep session logs. An
upgrade of those two packages replaces the wrapper with the new binary.
After such an upgrade run `./deploy-rootfs.sh` on the tablet: it moves the
new binary to `.real` and puts the wrapper back.

Keep `/etc/apk/repositories` on Alpine 3.24 (`latest-stable`). Switching to
`edge` may pull a GNOME/GDM combination that the xun packages are not tested
with; if you do, back up first.

### Device files, UCM and desktop defaults

The on-device files live in `build/device` and `build/ucm2`:

From the host:

```sh
ssh -i build/ssh/xun_key root@172.16.42.1 'rm -rf /tmp/xun-deploy; mkdir -p /tmp/xun-deploy'
scp -r -i build/ssh/xun_key build/device build/ucm2 build/deploy-rootfs.sh \
	root@172.16.42.1:/tmp/xun-deploy/
ssh -i build/ssh/xun_key root@172.16.42.1 'cd /tmp/xun-deploy && ./deploy-rootfs.sh'
```

It copies the files, creates the `xun` account if missing, runs `dconf
update` and leaves `/sbin/init` alone. Reboot afterwards; for session-only
changes `rc-service gdm restart` is enough.

### Firmware

| Files | Where they are installed | How to update |
| --- | --- | --- |
| `qcom/a610_zap.*`, `a630_sqe.fw`, touch firmware, QCA Bluetooth | embedded in the kernel/initramfs | rebuild and flash `boot_b` |
| `qcom/sm4250/SM6225-Xun-tplg.bin`, `qcom/sm6225/xiaomi/xun/adsp.*` | kernel initramfs (and `/lib/firmware`) | rebuild and flash `boot_b` |
| `qcom/sm6225/xiaomi/xun/modem.*`, `wlanmdsp.mbn`, `bdwlan*`, `ath10k/WCN3990/*` | `/lib/firmware` on the rootfs | copy or package update, restart the modem/ath10k or reboot |

Updating `/lib/firmware` does not touch user data. The modem can be restarted
without a reboot:

```sh
echo stop  > /sys/class/remoteproc/remoteproc0/state
echo start > /sys/class/remoteproc/remoteproc0/state
```

### A new full rootfs image

This is the only operation that erases data. Back up first:

```sh
./scripts/backup-tablet.sh
fastboot flash userdata build/out/xun-install.img.sparse
```

Restore after the first boot (selective restore avoids replacing the new
image's configuration with old files):

```sh
scp -i build/ssh/xun_key backups/<stamp>/system.tar.gz root@172.16.42.1:/tmp/
ssh -i build/ssh/xun_key root@172.16.42.1 \
  'tar -C / -xzf /tmp/system.tar.gz home/xun var/lib/bluetooth var/lib/AccountsService'
ssh -i build/ssh/xun_key root@172.16.42.1 \
  'tar -C / -xzf /tmp/system.tar.gz etc/NetworkManager/system-connections'
ssh -i build/ssh/xun_key root@172.16.42.1 'chown -R 1000:1000 /home/xun'
```

`backup-tablet.sh` stores the user's home, `/etc`, Bluetooth pairings, the
accounts database and the package list under `backups/` on the host. The
directory is gitignored because it contains Wi-Fi passwords and SSH keys.

## Quick reference

```sh
# driver / kernel / DTB change: data-safe
./build/build-xun.sh
./build/deploy-boot.sh out/boot-xun.img
ssh -i build/ssh/xun_key root@172.16.42.1 reboot

# user-space package update: data-safe
ssh -i build/ssh/xun_key root@172.16.42.1 'apk update && apk upgrade'

# rootfs files or UCM: data-safe
ssh -i build/ssh/xun_key root@172.16.42.1 'cd /path/to/build && ./deploy-rootfs.sh'

# backup before a full image flash
./scripts/backup-tablet.sh
```
