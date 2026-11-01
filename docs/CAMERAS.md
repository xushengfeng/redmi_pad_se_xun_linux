# Cameras

The tablet has two camera modules behind the Qualcomm CAMSS/CCI block. The
sensors are supported by mainline V4L2 drivers, but they do not answer on I2C
yet; this file records what is known and what was tried.

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

So the rear 8 MP camera is a GalaxyCore **GC08A3** and the front 5 MP camera
is a Hynix **HI556** (not the GC05A2 that also appears in the vendor camera
libraries; that is the module for other variants). Both modules answer at
EEPROM address `0x51`: the fuse IDs above are read back from that EEPROM.

The matching mainline drivers are `drivers/media/i2c/gc08a3.c` and
`drivers/media/i2c/hi556.c`; `ov08d10.c` and `gc05a2.c` cover the other module
variants and stay enabled as well.

## Wiring (stock device tree)

| | Rear | Front |
| --- | --- | --- |
| CCI bus | `cci_i2c0` (master 0) | `cci_i2c1` (master 1) |
| CSI PHY | CSIPHY0 | CSIPHY2 |
| MCLK | MCLK0, GPIO 20, 19.2 MHz | MCLK1, GPIO 21, 19.2 MHz |
| Reset | GPIO 18, active low | GPIO 24, active low |
| AVDD (2.8 V) | PM6125 L21 | PM6125 L21 |
| DVDD (1.2 V) | PM6125 L1 | PM6125 L1 |
| DOVDD (1.8 V) | PM6125 L13 | PM6125 L13 |
| Data lanes | 4, 336 MHz link | 2, 437 MHz link |

The aperture is described in `build/kernel-dts/sm6225-xiaomi-xun.dts`:
`rear_camera` at CCI0 address 0x31 and `front_camera` at CCI1 address 0x20,
plus the CAMSS endpoints and the MCLK pinmux on GPIO 20/21.

`build/patches/hi556-dt.patch` adds OF matching to the HI556 driver and makes
it identify at probe; upstream the driver is only reachable through ACPI.

## Current state

- The CAMSS, CCI, VFE and CSIPHY devices probe and `camss` has its power
  domains on.
- The GC08A3 driver reads the chip ID register (`0x03f0`) at 0x31 and gets
  `-ENXIO` (no ACK). The HI556 driver does the same at 0x20 and gets `-EIO`.
- The modules are installed: the rear module's **CN3927V actuator** answers
  at CCI0 address `0x0c`, and the module EEPROM at `0x51` answers on both
  buses with the fuse IDs from `persist`. The rear EEPROM stops answering
  when the three camera rails are disabled, so the module is powered from
  the board rails.
- A full scan of 0x08..0x77 on both buses, with all four MCLK outputs
  running (gpio 20/21/27/28), the three rails at 2.8/1.2/1.8 V, VAF at 2.8 V
  and the reset candidates 18/24/19/65 at both levels, finds only `0x0c`
  (rear actuator), `0x51` (module EEPROM) and `0x59` on CCI1 (answers 0xff,
  not the HI556 chip ID).

### Vendor power sequence

The power-up sequence is in the module's Chromatix data
(`com.qti.sensormodule.m84_aac_gc08a3_i.bin`, symbol `sensorDriverData` ->
`powerSetting`). It decodes as:

| Step | Rail | Value | Delay |
| --- | --- | --- | --- |
| 1 | VIO | on | 1 ms |
| 2 | VDIG | on | 5 ms |
| 3 | VANA | on | 5 ms |
| 4 | RESET | 1 | 1 ms |
| 5 | MCLK | 19200000 | 5 ms |
| 6 | VAF | on | 5 ms |

The GC08A3 driver was changed to follow this order, the MCLK was set to
19.2 MHz (the value in the sequence) and L19 (VAF) was kept at 2.8 V.
The sensor still does not answer, so the remaining difference is not the
power sequence.

The sequence was read with
[go-chromatix2libcamera](https://github.com/W-Floyd/go-chromatix2libcamera):
`go-chromatix2libcamera dump -name sensorDriverData <module>.bin` shows the
`powerSetting` list.

### What is left

The sensor dies are the only part of the module that does not respond while
the actuator and EEPROM on the same flex do. Either their VANA/DVDD feed is
not reaching the module (the EEPROM shows VIO works, the actuator shows VAF
works) or the dies themselves are damaged. The next checks need the
hardware: measure VANA/DVDD at the camera connector, reseat the module, or
boot the stock image to see whether the camera works there.

## Testing on the tablet

```
dmesg | grep -iE 'gc08a3|hi556|camss|cci'
ls /dev/media* /dev/video*
```

When a sensor probes, the next step is to check the media graph and stream:

```
media-ctl -p -d /dev/media0
v4l2-ctl -d /dev/video0 --list-formats-ext
```

`v4l-utils` and `libcamera` are the tools to use; both need the sensor
subdevice to be present before the CAMSS pipeline can be linked.
