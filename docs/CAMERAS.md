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
- A sweep of every address from 0x08 to 0x77 on both CCI buses, with the
  MCLK running and the reset line at both levels, finds only the module
  EEPROMs at `0x51`. The EEPROMs answer even without the sensor rails, which
  shows that the bus itself is wired correctly.
- MCLK was verified with `clk_summary`: `gcc_camss_mclk0_clk` /
  `gcc_camss_mclk1_clk` run at 19.2 MHz, the pins are muxed to `cam_mclk`,
  the CAMSS GDSC `gcc_camss_top` is on, and the sensor rails report
  2.8/1.2/1.8 V while the drivers hold them.
- Re-enabling MCLK after CAMSS is powered, keeping the clocks always on from
  boot, and forcing CAMSS/CCI runtime-active did not change the result.

The sensors therefore either do not get one of their rails, are held in reset
by a line other than GPIO 18/24, or need a power-up step that the generic
drivers do not perform. The module EEPROMs and the stock camera stack are the
next sources of truth: booting the stock image and reading
`/sys/kernel/debug/regulator/regulator_summary` with the camera open would
show which regulators and GPIOs the vendor stack actually uses.

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
