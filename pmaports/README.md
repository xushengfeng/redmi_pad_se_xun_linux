# pmaports packages for xun

These are the postmarketOS packages for the Xiaomi Redmi Pad SE 11
(`xiaomi-xun`), kept next to the bring-up tree so the board files, kernel
patches and configs have a single source.

## Layout

```
device/testing/device-xiaomi-xun/                 device package
device/testing/linux-postmarketos-qcom-sm6225/    kernel package (xun fork)
device/testing/firmware-xiaomi-xun/               firmware from the stock unit
```

`device-xiaomi-xun` installs the on-device files from `files/`: the local.d
services, GDM/dconf/NetworkManager/Bluetooth configuration, the GNOME session
wrappers, the elogind sleep hook and the ALSA UCM profile. `40-dconf.start`
compiles the dconf database on boot.

`linux-postmarketos-qcom-sm6225` is a fork of the sm6225-mainline package with
the xun board files. `prepare-kernel.sh` copies the DTS and the panel, touch,
amplifier and fuel-gauge drivers into the tree, applies the xun patches and
merges `base.config` + `tapas.config` + `xun.config`. The kernel image carries
the tapas initramfs (busybox + init), the ADSP firmware and the AudioReach
topology, so it boots through the v4 shim.

`firmware-xiaomi-xun` packages the files in `build/firmware` (18 MB,
proprietary, not committed). The full stock firmware can be extracted from the
`firmware`/`vendor` partitions as described in `docs/DOWNLOADS.md`.

## Refreshing the packages

The kernel sources, firmware blobs and busybox are generated from the bring-up
tree:

```sh
./build/sync-pmaports.sh
```

It copies `build/device` and `build/ucm2` into the device package, rebuilds
`xun-files.tar.gz` (DTS, drivers, patches, configs, init, busybox) and the two
firmware tarballs, and rewrites the `sha512sums` blocks. The tarballs are
ignored by git; the metadata and source files are committed.

## Using the packages

Copy the three directories into a pmaports checkout (the kernel package keeps
the sm6225-mainline package name, so it replaces that directory), then build
with pmbootstrap:

```sh
pmbootstrap init          # device xiaomi-xun, UI gnome
pmbootstrap install
```

## Boot image

`deviceinfo_generate_bootimg="false"`: xun needs the tapas **v4 shim** because
ABL hands the kernel its own DTB/vendor ramdisk. The boot image is still
assembled by the bring-up tree:

```sh
./build/build-xun.sh
./build/deploy-boot.sh out/boot-xun.img
```

`deploy-boot.sh` writes `boot_b`; slot A keeps Android. A boot-deploy/installer
hook that builds the shim image from the installed `/boot/Image` and
`/boot/dtbs/qcom/sm6225-xiaomi-xun.dtb` is the remaining packaging step, so a
plain `pmbootstrap install` does not yet produce a bootable slot by itself.

The kernel package installs both `/boot/vmlinuz` (Image.gz) and `/boot/Image`
(the uncompressed image the shim packs).

## Firmware

`build/sync-pmaports.sh` builds the firmware tarballs from `build/firmware`,
which contains only the files this port uses (ADSP, GPU zap, touch, QCA BT and
the AudioReach topology). For a complete stock set, mount the vendor partition
and copy the `firmware` and `vendor/firmware` trees before syncing.
