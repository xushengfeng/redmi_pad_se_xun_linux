# Cameras

Status: the modules and their wiring are known, the mainline drivers and
device-tree nodes are in place, but the sensor dies do not answer on I2C.
The CAMSS/CCI side works; the module EEPROM and the rear actuator on the same
flex do answer. The remaining fault is below the configuration level (the
sensor's VANA/DVDD feed or the dies themselves).

## Modules on this unit

`build/live/props.txt` holds the factory module selection and the per-module
fuse IDs:

```
persist.vendor.camera.mi.module.info = wide=aac_gc08a3_i;front=aac_hi556_i;
persist.vendor.camera.wide            = aac_gc08a3_i
persist.vendor.camera.front           = aac_hi556_i
vendor.camera.sensor.rearMain.fuseID  = _aac0024 0A1E2543 30333436 3332…
vendor.camera.sensor.frontMain.fuseID = _aac0022 05563830 32371600 002F21…
```

The rear 8 MP camera is a GalaxyCore **GC08A3**; the front 5 MP camera is a
Hynix **HI556**. The GC05A2 and HI556 entries in the vendor camera libraries
are the other module variants. The fuse IDs above are read back from the
module EEPROM at I2C address `0x51`, which matches the factory data.

The matching mainline drivers are `drivers/media/i2c/gc08a3.c` and
`drivers/media/i2c/hi556.c`; `ov08d10.c` and `gc05a2.c` cover the variants
and are enabled too.

## Board wiring

| | Rear | Front |
| --- | --- | --- |
| CCI bus | `cci_i2c0` (master 0) | `cci_i2c1` (master 1) |
| CSI PHY | CSIPHY0 | CSIPHY2 |
| MCLK | MCLK0, GPIO 20, 19.2 MHz | MCLK1, GPIO 21, 19.2 MHz |
| Reset | GPIO 18 | GPIO 24 |
| AVDD / VANA (2.8 V) | PM6125 L21 | PM6125 L21 |
| DVDD / VDIG (1.2 V) | PM6125 L1 | PM6125 L1 |
| DOVDD / VIO (1.8 V) | PM6125 L13 | PM6125 L13 |
| VAF (2.8 V) | PM6125 L19 | — |
| Data lanes | 4, 336 MHz link | 2, 437 MHz link |

The nodes are in `build/kernel-dts/sm6225-xiaomi-xun.dts`:
`rear_camera` at CCI0 address 0x31 and `front_camera` at CCI1 address 0x20,
the CAMSS endpoints, and the MCLK pinmux on GPIO 20/21.

The wiring was cross-checked against the xun-specific DTBO overlay in the
LineageOS device tree prebuilts (`prebuilts/dtbo.img`, entries with model
"KHAJE IDP nopmi xun"). It declares the same sensor nodes and the same
`L1A`/`L21A`/`L13A` rails, so this is not a generic IDP configuration.

`build/patches/hi556-dt.patch` adds OF matching to the HI556 driver and makes
it identify at probe; upstream the driver is only reachable through ACPI.

## What answers on the CCI buses

| Bus | Address | Device | Note |
| --- | --- | --- | --- |
| CCI0 | 0x0c | CN3927V actuator | Only answers while L19 (VAF) is on |
| CCI0 | 0x51 | module EEPROM | Returns the rear fuse ID |
| CCI1 | 0x51 | module EEPROM | Returns the front fuse ID |
| CCI1 | 0x59 | unknown | Answers 0xff, not a HI556 chip ID |

The sensor addresses (rear 0x31/0x36/0x37, front 0x20) do not ACK. A
0x08..0x77 scan on both buses, with all four MCLK outputs, all rails on and
the reset candidates 18/24/19/65 at both levels, finds only the devices in
the table above.

## Rail isolation

With the module EEPROM as the probe, disabling one rail at a time gives:

| Rail removed | EEPROM |
| --- | --- |
| AVDD / L21 | still answers |
| DVDD / L1 | still answers |
| DOVDD / L13 | stops answering (`-ETIMEDOUT`) |

So VIO = L13 is confirmed at the module, and the actuator confirms
VAF = L19. The VANA (L21) and DVDD (L1) feed cannot be confirmed from the
AP.

## Vendor power sequence

The power-up sequence is in the module's Chromatix data
(`com.qti.sensormodule.m84_aac_gc08a3_i.bin`, symbol `sensorDriverData` →
`powerSetting`):

| Step | Rail | Value | Delay |
| --- | --- | --- | --- |
| 1 | VIO | on | 1 ms |
| 2 | VDIG | on | 5 ms |
| 3 | VANA | on | 5 ms |
| 4 | RESET | 1 | 1 ms |
| 5 | MCLK | 19200000 | 5 ms |
| 6 | VAF | on | 5 ms |

The driver was changed to follow this order, MCLK was set to 19.2 MHz and
L19 (VAF) was powered. This matches the vendor configuration; the sensor
still does not answer.

## Configurations tested without effect

- all four MCLK outputs (GPIO 20/21/27/28) at 19.2 and 24 MHz;
- reset candidates GPIO 18/24/19/65, both electrical levels;
- regulator load votes matching the vendor `rgltr-load-current` values;
- 400 kHz CCI instead of the default standard mode;
- VAF enabled last in the vendor order (and VAF always-on);
- CAMSS kept runtime-active so the top GDSC is on before MCLK;
- L20 driven at 2.8 V as an alternate VANA rail.

## Remaining hypotheses

1. The module's VANA/DVDD pins do not reach the board rails (connector or
   flex fault).
2. The sensor dies are damaged.

Both need a hardware check: measure VANA (2.8 V) and DVDD (1.2 V) at the
camera connector, reseat the module, or boot the stock image to see whether
the camera works there. Software cannot distinguish them further.

## Reproducing the vendor data

The vendor partition is the `vendor_a` logical partition inside `super`.
On the tablet:

```
L=$(losetup -f)
losetup -o $((11073536*512)) --sizelimit $((1009488*512)) $L \
	/dev/disk/by-partlabel/super
mount -t erofs -o ro $L /mnt/vendor
```

The module data and camera libraries are then under
`/mnt/vendor/lib64/camera/`. The power list is decoded with
[go-chromatix2libcamera](https://github.com/W-Floyd/go-chromatix2libcamera):

```
go-chromatix2libcamera dump -name sensorDriverData \
	com.qti.sensormodule.m84_aac_gc08a3_i.bin
```

The same tool decodes the EEPROM map and the actuator data.

The xun DTBO overlay is in `aosp-device-tree/prebuilts/dtbo.img` from the
device dump; the DTB entries with model "KHAJE IDP nopmi xun" are the
relevant ones.

## Testing when a sensor probes

```
dmesg | grep -iE 'gc08a3|hi556|camss|cci'
ls /dev/media* /dev/video*
```

Then check the media graph and stream:

```
media-ctl -p -d /dev/media0
v4l2-ctl -d /dev/video0 --list-formats-ext
```

`v4l-utils` and `libcamera` need the sensor subdevice to be present before
the CAMSS pipeline can be linked.
