#!/bin/bash
# Refresh the pmaports packages from this bring-up tree.
#
#   ./scripts/sync-pmaports.sh
#
# Copies the on-device files, the kernel board files/patches/configs and the
# firmware into pmaports/, rebuilds the source tarballs and updates the
# checksums in the APKBUILDs. The firmware and busybox blobs stay out of git;
# only the package metadata and source files are committed.
set -eu
BASE=$(cd "$(dirname "$0")/.." && pwd)
ROOT=$BASE/build
PM=$BASE/pmaports
DEV=$PM/device/testing/device-xiaomi-xun
KPKG=$PM/device/testing/linux-postmarketos-qcom-sm6225
FWPKG=$PM/device/testing/firmware-xiaomi-xun

# --- device package ------------------------------------------------------
rm -rf "$DEV/files/etc" "$DEV/files/usr"
mkdir -p "$DEV/files"
cp -a "$ROOT"/device/etc "$DEV/files/"
cp -a "$ROOT"/device/usr "$DEV/files/"
mkdir -p "$DEV/files/usr/share/alsa"
# The packaged image keeps the distribution binaries; the bring-up rootfs
# uses these wrappers around gnome-session.real/gnome-shell.real.
rm -f "$DEV/files/usr/bin/gnome-session" "$DEV/files/usr/bin/gnome-shell"
cp -a "$ROOT"/ucm2/. "$DEV/files/usr/share/alsa/ucm2/"
cat >"$DEV/files/etc/local.d/40-dconf.start" <<'EOF'
#!/bin/sh
# Compile /etc/dconf/db/local.d into the system database installed by the
# device package; dconf only reads the compiled database.
dconf update
EOF
chmod 755 "$DEV/files/etc/local.d/40-dconf.start"

# --- kernel package sources ----------------------------------------------
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/xun/patches"
cp "$ROOT/kernel-dts/sm6225-xiaomi-xun.dts" "$STAGE/xun/"
cp "$ROOT"/kernel-panel/*.c "$STAGE/xun/"
cp "$ROOT"/kernel-touch/*.c "$STAGE/xun/"
cp "$ROOT"/kernel-audio/*.c "$STAGE/xun/"
cp "$ROOT"/kernel-battery/*.c "$STAGE/xun/"
cp "$ROOT"/patches/*.patch "$STAGE/xun/patches/"
cp "$ROOT/config/xun.config" "$STAGE/xun/xun.config"
cp "$ROOT/tapas-bringup/config/base.config" "$STAGE/xun/base.config"
cp "$ROOT/tapas-bringup/config/tapas.config" "$STAGE/xun/tapas.config"
cp "$ROOT/tapas-bringup/initramfs/init" "$STAGE/xun/init"
cp "$ROOT/tools/bin/busybox.static" "$STAGE/xun/busybox.static"
tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$STAGE" -cf - xun | gzip -9n > "$KPKG/xun-files.tar.gz"

# The kernel boot initramfs carries the ADSP firmware and the topology, and
# CONFIG_EXTRA_FIRMWARE points at these files during the build.
tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$ROOT" -cf - firmware | gzip -9n > "$KPKG/firmware.tar.gz"

# --- firmware package ----------------------------------------------------
tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$ROOT" -cf - firmware | gzip -9n > "$FWPKG/firmware-xiaomi-xun.tar.gz"

# --- checksums -----------------------------------------------------------
python3 - "$ROOT" "$DEV" "$KPKG" "$FWPKG" <<'PY'
import hashlib, os, sys
root, dev, kpkg, fwpkg = sys.argv[1:5]

def sha(path):
    h = hashlib.sha512()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()

def set_sums(path, sums):
    text = open(path).read()
    start = text.index('sha512sums="')
    end = text.index('"', start + len('sha512sums="'))
    block = 'sha512sums="\n' + ''.join(
        f'{digest}  {name}\n' for name, digest in sums) + '"'
    out = text[:start] + block + text[end + 1:]
    if not out.endswith('\n'):
        out += '\n'
    open(path, 'w').write(out)

# Device package: only the metadata files are sources.
set_sums(os.path.join(dev, 'APKBUILD'), [
    ('deviceinfo', sha(os.path.join(dev, 'deviceinfo'))),
    ('kernel-cmdline.conf', sha(os.path.join(dev, 'kernel-cmdline.conf'))),
])

# Kernel package: the upstream tarball checksum is pinned to the fork commit;
# the two generated tarballs are local.
_kernel_tar = 'f8b9677542b1d477401d0cb2c2f3586898fc8490ca084b113172cb44d2a78b9e67192b6b1d92033ad939fcf0c260fb23de011f6aea9684e4e3d95c313f700d1a'
set_sums(os.path.join(kpkg, 'APKBUILD'), [
    ('linux-postmarketos-qcom-sm6225-f5a976a8334c7563137352eaafe8d49a2a3b8c6a.tar.gz', _kernel_tar),
    ('xun-files.tar.gz', sha(os.path.join(kpkg, 'xun-files.tar.gz'))),
    ('firmware.tar.gz', sha(os.path.join(kpkg, 'firmware.tar.gz'))),
])

set_sums(os.path.join(fwpkg, 'APKBUILD'), [
    ('firmware-xiaomi-xun.tar.gz', sha(os.path.join(fwpkg, 'firmware-xiaomi-xun.tar.gz'))),
    ('30-initramfs-firmware.files', sha(os.path.join(fwpkg, '30-initramfs-firmware.files'))),
])
print('pmaports synced')
PY

ls -lh "$KPKG/xun-files.tar.gz" "$KPKG/firmware.tar.gz" "$FWPKG/firmware-xiaomi-xun.tar.gz"
