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
| Fast charge | BQ2589X HVDCP handshake, 9 V from a Quick Charge adapter |

The node in `build/kernel-dts/sm6225-xiaomi-xun.dts` uses the stock board
values in the mainline units (µA/µV):

```
ti,charge-current = <3600000>;              /* 3.6 A into the cell */
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
device tree. `build/patches/bq2589x-quickcharge.patch` adds the HVDCP setup
and the fast-charge input limits. `build-xun.sh` applies both;
`CONFIG_CHARGER_BQ25890=y` is already in the base config.

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

## Fast charging (18 W)

The tablet's 18 W mode is Quick Charge 2.0/3.0 at 9 V, negotiated by the
BQ2589X over D+/D-. The driver enables the HVDCP handshake and keeps the
MaxCharge 12 V bit clear, so a QC adapter settles at 9 V. It also raises the
input limit from the chip's 1.5 A MaxCharge default to 2 A. With the 3.6 A cell
current from the DTS this draws the full 18 W from a 9 V / 2 A adapter (about
16 W reaches the cell; the rest covers conversion losses and the system load).

With a QC adapter attached, `usb_type` still reports `DCP` — the power-supply
class has no separate HVDCP value — but the input limit and the charger
registers show the negotiation:

```sh
P=/sys/class/power_supply/bq25890-charger-0
cat $P/input_current_limit      # 2000000 on a 9 V adapter
cat $P/constant_charge_current  # 3584000 (3.584 A)

# regmap debugfs: 0b[7:5] = 100 (MaxCharge), 11[6:0] = 64 -> 9.0 V
cat /sys/kernel/debug/regmap/2-006a/registers | sed -n '12p;18p'
```

`0b` (VBUS_STAT) changes from `0x34` (SDP, 5 V) to a value with bits 7:5 set
to `100`; `11` (VBUS ADC) reads 24 at 5 V and 64 at 9 V in bits 6:0 (100 mV
steps above 2.6 V, bit 7 is VBUS_GD).

The input limit follows the detected source: 500 mA on SDP, 1.5 A on CDP and
2 A on DCP/HVDCP. A 5 V DCP adapter therefore charges at up to 10 W; only a QC
adapter that raises VBUS to 9 V reaches 18 W.

A PD-only charger stays at 5 V. The stock device tree contains an SC2150
Type-C PD controller at 0x4e, but the part does not answer on `i2c1` on this
board and there is no mainline driver for it. Quick Charge is the supported
fast-charge path.

## Limits

- No USB Power Delivery / PPS negotiation: the 18 W mode uses the BQ2589X's
  Quick Charge handshake, not the Type-C CC lines.
- The OTG boost regulator is registered by the driver but has not been used.
- Charging while the tablet is powered off is still the bootloader's
  configuration: Linux only runs when the tablet is on.
- The charger's JEITA temperature windows are left at the chip defaults; the
  vendor driver implements software JEITA that is not ported.
