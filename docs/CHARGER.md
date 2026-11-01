# Charger and battery state

The tablet charges through a TI BQ2589X charger (a BQ25890-compatible part
with part number 4). Linux runs the mainline `bq25890` driver for it, so
charging status, input current limit and the USB charger type reach upower
and GNOME.

## Hardware

| Part | |
| --- | --- |
| Charger | BQ2589X at address 0x6a on `i2c1` (4a84000) |
| Interrupt | GPIO 84, falling edge, pull-up |
| Battery gauge | SH366101 at 0x55 on `i2c2`, see the driver in `build/kernel-battery/` |
| USB type | BQ2589X auto D+/D- detection (SDP/CDP/DCP) |

The node in `build/kernel-dts/sm6225-xiaomi-xun.dts` uses the stock board
values in the mainline units (µA/µV):

```
ti,charge-current = <2000000>;              /* 2 A */
ti,battery-regulation-voltage = <4450000>;  /* 4.45 V */
ti,termination-current = <450000>;
ti,precharge-current = <128000>;
ti,minimum-sys-voltage = <3500000>;
ti,boost-voltage = <5000000>;               /* OTG boost */
ti,boost-max-current = <1500000>;
ti,enable-auto-dpdm;
ti,enable-termination;
```

## Driver change

The mainline driver knows the TI part numbers (0, 1 for BQ25890, 2/3 for
BQ25892/96, 5 for BQ25895) and refuses anything else. The BQ2589X reports
part number **4**, so `build/patches/bq2589x-id.patch` maps it to the
BQ25890 register map and adds the `ti,bq2589x-1` compatible used by the stock
device tree. `build-xun.sh` applies it; `CONFIG_CHARGER_BQ25890=y` is already
in the base config.

## What userspace sees

The charger registers as the power supply `bq25890-charger-0`:

```sh
P=/sys/class/power_supply/bq25890-charger-0
cat $P/online                       # 1 when a charger is connected
cat $P/status                       # Charging / Full / Not charging
cat $P/usb_type                     # SDP / CDP / DCP (or Unknown until DPDM)
cat $P/constant_charge_current      # limit in µA
cat $P/constant_charge_voltage      # regulation in µV
cat $P/input_current_limit          # USB input limit in µA
```

upower picks it up as line power:

```sh
upower -i /org/freedesktop/UPower/devices/line_power_bq25890_charger_0
upower -i /org/freedesktop/UPower/devices/battery_sh366101
```

`/proc/interrupts` should list `bq25890_irq` on GPIO 84; the driver updates
the state from that interrupt.

On a PC's USB port the charger detects SDP and limits the input to 500 mA; a
wall charger is detected as DCP and the driver allows the full 2 A. The
`usb_type` attribute shows which one was detected.

**Battery state on AC:** when the battery is full, the fuel gauge measures the
tablet's own consumption as a small discharge current (-34 mA here). The
SH366101 driver therefore reports `Full` when the charger is online and the
charge is at least 99%, so upower shows "fully charged" instead of
"discharging" while the cable is connected.

## Limits

- No USB Power Delivery / PPS negotiation: the charger runs from the 5 V VBUS
  that is present.
- The OTG boost regulator is registered by the driver but has not been used.
- Charging while the tablet is powered off is still the bootloader's
  configuration: Linux only runs when the tablet is on.
- The charger's JEITA temperature windows are left at the chip defaults; the
  vendor driver implements software JEITA that is not ported.
