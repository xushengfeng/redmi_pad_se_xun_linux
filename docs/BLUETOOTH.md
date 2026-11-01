# Bluetooth

The tablet's combo chip provides Bluetooth next to Wi-Fi (`ath10k_snoc`). The
Bluetooth half sits on a 4-wire UART and runs the Qualcomm QCA firmware; the
driver is the mainline `hci_qca` serdev driver with two local adjustments.

## Hardware and device tree

| Part | |
| --- | --- |
| Controller | QCA combo (reports `QCA SOC Version 0x40074130`, treated as WCN3990 by `hci_qca`) |
| UART | `serial@4a8c000` (`uart3`), 4-wire: GPIO 8 CTS, 9 RTS, 10 TX, 11 RX |
| Supplies | `vddio` = PM6125 L9, `vddxo` = L16, `vddrf` = L17, `vddch0` = L23 |
| Speed | 3200000 baud (`max-speed`) |
| Address | `local-bd-address = [02 40 00 12 50 39]` → `39:50:12:00:40:02` |

The node lives under `&uart3` in `build/kernel-dts/sm6225-xiaomi-xun.dts`. The
vendor's `qcom,bt-sw-ctrl-gpio` (GPIO 87) is kept for documentation only; the
mainline driver does not need it for this chip.

The address is the one baked into the stock NVM (`cmnv13.bin`, bytes at
offset 0x14). It is written back through the DT property so the kernel
programs it at setup and the controller starts out configured; without it the
kernel treats the controller as unconfigured and powers it down again.

## Firmware

`build/firmware/qca/crbtfw13.tlv` and `crnv13.bin` are embedded in the kernel
by `build/config/xun.config`. They are byte-identical to the stock partition's
`cmbtfw13.tlv` and `cmnv13.bin` (`/dev/disk/by-partlabel/bluetooth_a`,
`image/`): the Android driver uses the `cm` names while this ROM version makes
mainline ask for the `cr` names.

```sh
mkdir -p /mnt/bt
mount -t vfat -o ro /dev/disk/by-partlabel/bluetooth_a /mnt/bt
cp /mnt/bt/image/cmbtfw13.tlv build/firmware/qca/crbtfw13.tlv
cp /mnt/bt/image/cmnv13.bin  build/firmware/qca/crnv13.bin
```

The partition also carries `crnv20`/`crnv21` and the `ap*` variants for other
ROM versions; this chip wants ROM 13.

## Why 3.2 Mbaud

The GENI UART runs from 19.2 MHz; 3200000 divides it exactly (÷6). At
3000000 the divider is not integral and the firmware download/RESET
commands time out, at 115200 the setup also fails after the NVM download. The
vendor driver's default is 3 Mbaud; on this board 3.2 Mbaud is the value that
works.

## The in-band sleep workaround

Once the setup completes, `hci_qca` clears `QCA_IBS_DISABLED` and starts the
Qualcomm in-band sleep protocol (wake/sleep indications over the UART). With
this firmware the controller then stops answering: the first standard HCI
command (`Read Local Version`) times out, the kernel powers the device down,
and the next setup reads garbage (`Frame reassembly failed`, `unexpected event
for opcode 0x0000`).

`build/patches/qca-no-ibs.patch` keeps `QCA_IBS_DISABLED` set after setup,
which sends HCI frames directly without the wake handshake. The controller
stays awake (more power than the sleeping chip would use) but works. The
patch is applied by `build/build-xun.sh`.

## Runtime

`50-gnome.start` starts `bluetoothd`. PulseAudio loads `module-bluez5-discover`,
so a connected headset or speaker becomes a normal PulseAudio sink (A2DP or
HFP). The adapter name comes from `build/device/etc/bluetooth/main.conf`.

Check it:

```sh
hciconfig -a                 # hci0 ... UP RUNNING
bluetoothctl show            # Powered: yes, Controller 39:50:12:00:40:02
bluetoothctl --timeout 10 scan on
bluetoothctl devices
```

The boot log should show the firmware download and no retries:

```
Bluetooth: hci0: QCA Downloading qca/crbtfw13.tlv
Bluetooth: hci0: QCA Downloading qca/crnv13.bin
Bluetooth: hci0: QCA setup on UART is completed
```

## Limits

- The address is the stock NVM default; there is no per-device Bluetooth MAC
  (Android keeps one elsewhere, no Linux-accessible copy was found).
- In-band sleep is off (see above), so the controller does not enter its low
  power state.
- Scanning works from `bluetoothctl`; pairing and A2DP still need a test with a
  real headset or speaker.
