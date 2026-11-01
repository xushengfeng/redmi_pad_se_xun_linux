# Bring-up runbook (unlocked bootloader)

The working build and install steps are in [`BUILD.md`](BUILD.md); this is the
first-contact runbook.

This assumes the tablet is already bootloader-unlocked and still boots Android.
The goal of the first pass is **not** a usable desktop; it is to prove the boot
chain and get a mainline kernel log over USB serial without writing anything to
the device.

## 0. Safety and recovery

Before any `fastboot flash`:

- Download the exact `xun` fastboot ROM (or at least keep a known-good
  `boot.img`, `init_boot.img`, `vendor_boot.img`, `dtbo.img` for the current
  slot).
- Know the EDL/test-point procedure for `xun`.
- Prefer `fastboot boot <image>` first. It writes nothing.
- If you must flash, use the **inactive slot** and keep Android on the active
  slot:

```sh
fastboot getvar current-slot        # e.g. a
fastboot flash boot_b boot-xun.img  # inactive slot
fastboot --set-active=b
fastboot reboot
# back to Android:
fastboot --set-active=a
fastboot reboot
```

Do not enable charging writes or battery-profile writes until the basic kernel
is stable.

## 1. Confirm unlock and collect live data

```sh
adb devices
adb shell getprop > props.txt
adb shell getprop ro.boot.project_name
adb shell getprop ro.boot.hwc
adb shell getprop ro.boot.hardware
adb shell getprop ro.boot.slot_suffix
adb shell dumpsys sensorservice > sensors.txt
adb shell dumpsys SurfaceFlinger > sf.txt
adb shell cat /proc/cmdline
adb shell cat /proc/version
adb shell ls -l /dev/block/by-name

fastboot devices
fastboot getvar unlocked
fastboot getvar current-slot
fastboot getvar all > fastboot-vars.txt
```

If the stock ROM is rooted, also save:

```sh
adb shell su -c 'cat /sys/firmware/fdt' > fdt.dtb
adb shell su -c 'cat /proc/config.gz' > config.gz
adb shell su -c 'dd if=/dev/block/by-name/firmware of=/sdcard/firmware.img'
```

If not rooted, get the `firmware` images from the official fastboot ROM. The
`Xiaomi-Redmi-Pad-SE-Resources/android_device_xiaomi_xun` repository's
`proprietary-firmware.txt` lists them: `abl.img`, `bluetooth.img`, `dsp.img`,
`modem.img`, `tz.img`, `xbl.img`, and so on.

Useful things to record:

- panel/touch variant (stock `dmesg` if available, or the panel ID later)
- RAM size
- `fastboot getvar all` slot/anti-rollback state
- whether `fastboot boot` accepts a v4 image

## 2. Build environment

Host packages (names vary by distro):

```sh
clang lld llvm make bc bison flex openssl-dev \
dtc python3 git android-tools \
mkbootimg unpack_bootimg
```

For postmarketOS packaging later, install `pmbootstrap`.

Clone the kernel and the tapas bring-up tree:

```sh
git clone --depth 1 -b stable-sm6225-tapas \
    https://gitlab.postmarketos.org/sm6225-mainline/kernels/linux-v.7.2.1.git linux-xun

git clone --depth 1 https://github.com/Sype0/pmos-tapas.git tapas-bringup
```

The tapas tree contains:

- `shim/` — the v4 boot shim
- `initramfs/init` — USB-serial bring-up init
- `config/` — known-good SM6225 kernel config
- `scripts/build.sh` — builds the shim + kernel + initramfs
- `scripts/mkbootimg.py` — v4 image packing

## 3. Smoke-test the v4 boot chain

Two low-risk tests, in this order:

### 3.1 Tapas shim image

Build or obtain the tapas safe image, then:

```sh
fastboot boot boot-tapas-safe.img
```

Watch USB for a new `/dev/ttyACM0` (or a COM port) and the tablet screen for
the shim's colour bands. This proves the xun ABL accepts a v4 image and that
`fastboot boot` works. It does not prove the xun hardware works.

### 3.2 U-Boot (optional, very useful)

If the tapas shim gets nowhere, try the SM6225 U-Boot port:

```sh
git clone --depth 1 -b sm6225 \
    https://gitlab.postmarketos.org/sm6225-mainline/u-boot.git
cd u-boot
make O=.output qcom_defconfig qcom-phone.config
make O=.output CROSS_COMPILE=aarch64-linux-gnu- -j"$(nproc)"
fastboot boot .output/u-boot.img
```

If U-Boot starts, it gives a boot menu, USB serial gadget, UMS and fastboot.
Its `spes` DTS is not xun-correct, but basic boot/console/display is enough to
validate the chain.

If neither image starts, stop and compare the boot headers with the stock
images (`unpack_bootimg`), then investigate ABL/vbmta/slot requirements before
flashing anything.

## 4. Minimal xun DTS

Start with only what is needed to reach a shell. Do **not** port display,
touch, audio, charger or cameras yet.

Create `arch/arm64/boot/dts/qcom/sm6225-xiaomi-xun.dts` based on
`sm6225-xiaomi-tapas.dts` plus the stock xun merged tree:

- `model`, `compatible = "xiaomi,xun", "qcom,sm6225"`
- `qcom,msm-id = <518 0x10000>`
- `qcom,board-id = <0x20022 0>`
- `qcom,pmic-id = <0x2d 0 0 0>`
- `chosen` with `console=tty0 pmos.nosplash`, simple-framebuffer at
  `0x5c000000`, 1200×1920, stride 4800, format `a8r8g8b8`
- `reserved-memory` / splash / ramoops from the xun tree
- PM6125 regulators
- UFS (`ufs_mem_hc`, `ufs_mem_phy`)
- SD (`sdhc_2`, CD GPIO 88)
- USB (`usb`, `usb_dwc3`, `usb_hsphy`, `usb_qmpphy`)
- PMIC keys / volume up / power
- UART for console
- a placeholder `memory` node (the shim needs a layout matching the unit's
  4/6/8 GB RAM; U-Boot fixes the memory node up automatically)

Build with a config based on
`linux-postmarketos-qcom-sm6225/config-postmarketos-qcom-sm6225.aarch64`,
plus a small xun fragment enabling:

- `CONFIG_SERIAL_QCOM_GENI` / `CONFIG_SERIAL_QCOM_GENI_CONSOLE`
- `CONFIG_USB_CONFIGFS_F_ACM` / `CONFIG_USB_GADGET`
- `CONFIG_SCSI_UFS_QCOM`
- `CONFIG_MMC_SDHCI_MSM`
- `CONFIG_PINCTRL_SM6225`
- `CONFIG_REGULATOR_RPM_SMD`
- `CONFIG_DRM_MSM` + `CONFIG_DRM_MSM_KMS_FBDEV` (for simplefb/console)
- `CONFIG_INITRAMFS_SOURCE` pointing at the tapas initramfs

Pack as a v4 image with the shim. Boot:

```sh
fastboot boot boot-xun-safe.img
```

Success criteria:

- kernel log appears on the display and/or `/dev/ttyACM0`
- the initramfs shell is reachable
- UFS block devices are visible (`/dev/sda*` or `/dev/block/...`)
- microSD is visible
- USB gadget works

If it fails, report the last visible screen lines, whether USB enumerated, and
whether the device rebooted or stayed in fastboot.

## 5. Next milestones after the shell

### 5.1 Display

- Generate `m84_36` panel driver with
  `linux-mdss-dsi-panel-driver-generator` from the official display DT
  (`MiCode/vendor_qcom_proprietary_display-devicetree`, branch `xun-t-oss`).
- Enable `mdss`, `mdss_dsi0`, `mdss_dsi0_phy`, `dispcc`, 4 lanes, DSC.
- Add brightness (DCS first; KTZ8866 if needed).
- Repeat for `m84_35`, `m84_42`.

### 5.2 Touch

- Port the FT8203 SPI driver first (more common); then NT36532.
- SPI0 CS0, IRQ GPIO 80, reset GPIO 86.
- Test with `evtest`/`libinput debug-events`.

### 5.3 Power and storage

- UFS rootfs.
- BQ2589X charger + SH366101 fuel gauge + NOPMI glue.
- SC2150/WUSB3801 USB-C.
- Thermal zones; only then enable charge writes.

### 5.4 GPU/Wi-Fi/BT/audio

- Adreno 610 zap.
- WCN3990 firmware + calibration from the stock `firmware` partition.
- FS1815 + WCD937x audio, ADSP firmware/topology.

## 6. What to send back from each test

For every boot attempt, keep:

- the exact image name/commit
- `fastboot getvar all`
- a photo of the last screen state
- whether USB enumerated and the last kernel lines
- `dmesg` if the shell is reachable

That is enough to decide whether the next fix is DTS, boot header, memory
layout, regulator, or a driver.
