# Hardware inventory (`xun`)

This is derived from the stock Android device tree (base DTB from `vendor_boot`
plus the xun DTBO overlay, merged locally), the stock vendor/ODM partitions,
and the public SM6225 documentation. Items marked **TBD** still need live-device
confirmation.

## Device

| Item | Value |
| --- | --- |
| Marketing name | Redmi Pad SE 11" |
| Model | 23073RPBFG (global); 23073RPBFC (China) |
| Codename | `xun` |
| Android shipped | 13 (MIUI/HyperOS); dump used here is OS2.0.1.0.VMUMIXM (Android 15) |
| SoC | Qualcomm SM6225, Snapdragon 680 4G, 6 nm |
| CPU | 4× Cortex-A73 (Kryo 265 Gold) @ 2.4 GHz + 4× Cortex-A53 @ 1.9 GHz |
| GPU | Adreno 610 |
| RAM | 4 / 6 / 8 GB LPDDR4X |
| Storage | 128 / 256 GB UFS 2.2 |
| microSD | Yes, dedicated slot (SDHCI `sdhc_2`, card-detect GPIO 88) |
| Display | 11", 1200×1920, IPS, 90 Hz |
| Battery | 8000 mAh, non-removable |
| Cameras | 8 MP rear, 5 MP front |
| Speakers | Quad speakers |
| Wi-Fi / BT | Wi-Fi 5, Bluetooth 5.0 (WCN3990) |
| Ports | USB-C (USB 2.0 High-Speed in the stock configuration), microSD |
| Buttons | Power, volume up, volume down; hall/cover sensor |

## Platform and device-tree identity

| Property | Value |
| --- | --- |
| SoC compatible | `qcom,khaje` / `qcom,sm6225` |
| Board compatible | `qcom,khaje-idp`, `qcom,khaje`, `qcom,idp` |
| `qcom,msm-id` | `<518 0x10000>` (`0x206`) |
| `qcom,board-id` | `<0x20022 0>` |
| `qcom,pmic-id` | `<0x2d 0 0 0>` |
| PMIC | PM6125 (plus PM8010 variants in the generic DTBO) |
| Stock DTB source | `vendor_boot.img` (`aosp-device-tree/prebuilts/dtb.img`) |
| Stock xun overlay | `KHAJE IDP nopmi xun` in the dump's extracted `dtbo/` set (entries 46/47) |

The xun overlay is one of 47 entries in the dump's extracted `dtbo/` set. It is
the only entry with `model = "Qualcomm Technologies, Inc. KHAJE IDP nopmi xun"`,
board ID `0x20022`, and the M84 panel nodes. (The dump's root `dtbo.img` is a
generic 32-entry image and does not contain the xun overlay; see
`docs/DATA_COLLECTION.md`.)

## Display

### Panel variants

All three are 1200×1920 (FHDP), DSI video mode, 90 Hz, 4-lane, DSC compressed.
The default panel selected by the stock DT is **M84 36 02 0A**.

| Panel node | Panel name | Timing summary | DSC | Touch / firmware |
| --- | --- | --- | --- | --- |
| `m84_36_02_0a` (default) | `m84 36 02 0a video mode fhdp dsi panel` | HFP 90, HBP 92, HPW 4, VFP 54, VBP 16, VPW 4, 90 Hz | slice 600×40, 8 bpc/8 bpp, 2 per packet | Focaltech FT8203 (`focaltech_ts_fw_*_ft8203.bin`) or Novatek |
| `m84_35_03_0b` | `m84 35 03 0b video mode fhdp dsi panel` | HFP 50, HBP 36, HPW 20, VFP 410, VBP 10, VPW 6, 90 Hz | slice 600×40, 8 bpc/8 bpp, 2 per packet | Focaltech FT8203 or Novatek |
| `m84_42_03_0c` | `m84 42 03 0c video mode fhdp dsi panel` | HFP 50, HBP 36, HPW 20, VFP 410, VBP 10, VPW 6, 90 Hz | slice 1200×8, 8 bpc/8 bpp, 2 per packet | Focaltech FT8203 or Novatek |

Panel common properties:

| Property | Value |
| --- | --- |
| Resolution | 1200×1920 |
| Physical size | 147.4 × 235.8 mm (stock DT units) |
| Reset GPIO | TLMM 82, active state used by stock init sequence |
| TE GPIO | TLMM 81 (`mdp_vsync`) |
| VDDIO | PM6125 L9 (1.504–2.0 V) |
| Enable GPIO | None in the stock xun panel node (unlike tapas, which uses PM6125 GPIO 9) |
| Backlight | DCS brightness (`bl_ctrl_dcs`, 0x7FF max) and optionally KTZ8866 |
| Reset sequence | `1 10 / 0 10 / 1 10` (35) or `1 10 / 0 10 / 1 15` (42); 36 uses `1 10 / 0 10 / 1 10` |
| Dynamic FPS | 90 / 60 / 50 / 48 / 30 Hz list |

The full init/off command sequences and DSC settings are in
`docs/reference/xun-merged.dts` (search for `mdss_dsi_panel_m84`).

### Backlight

The panel nodes use DCS brightness, but the board also has a `bkl-bl@11`
node (`compatible = "bkl,bkl-bl"`, KTZ8866-compatible driver in the vendor
display tree) on I²C0, with `bkl,hwen-gpio = <&tlmm 102>`. This may be used by
some panel variants or for a separate LED string. Bring up DCS brightness
first, then add KTZ8866 if needed.

## Touch

The active touch controller is on **SPI**, not I²C:

| Property | Value |
| --- | --- |
| Controller | `spi@4a80000` (GENI SPI0 / `qupv3_se0_spi`), CS0 |
| Downstream compatible | `focaltech,fts`, `novatek,NVT-ts-spi` |
| IRQ | TLMM 80 |
| Reset | TLMM 86 |
| SPI clock | 18 MHz |
| Max touches | 10 |
| Downstream coordinates | 0..0x2EDF × 0..0x4AFF |
| Panel phandles | all three M84 panels |
| Touch firmware | `focaltech_ts_fw_boe_ft8203.bin`, `focaltech_ts_fw_huaxing_ft8203.bin`, `novatek_ts_fw_tianma.bin` (in `odm/firmware`) |
| Stock modules | `focaltech_ts_spi.ko`, `nt36532_spi.ko`, `lct_tp.ko`, `xiaomi_touch_game.ko` |

The I²C `novatek@62` and `synaptics_tcm@20` nodes are disabled. The SPI node
declares both Focaltech and Novatek compatibles; the actual chip depends on the
panel vendor (FT8203 for BOE/Huaxing, NT36532 for Tianma, based on the firmware
names and stock modules).

Official source: `MiCode/vendor_qcom_proprietary_display-devicetree` branch
`xun-t-oss` has `dsi_panel_m84_35_03_0b_fhdp_video.dtsi`,
`dsi_panel_m84_36_02_0a_fhdp_video.dtsi`, `touchscreen_m84.dtsi` and
`xun-sde-display-idp-nopmi-overlay.dts`. That branch's touch node lists only
the 35/36 panels; the stock dump also has the `m84_42_03_0c` panel, so keep the
dump as the cross-check.

Touch power/AVDD is not explicit in the merged tree; the stock pinctrl has
`pmx_ts_active/suspend/release` states. Confirm the rail on a live device.

## Power, charging and USB-C

| Function | Part | Bus / address | IRQ | Notes |
| --- | --- | --- | --- | --- |
| Main charger | TI BQ2589X | I²C0 `4a84000`, 0x6A | GPIO 84 | `ti,bq2589x-1`; DPDM to USB2 PHY; stock module `bq2589x_charger.ko` |
| Fuel gauge | Southchip SH366101 | I²C1 `4a88000`, 0x55 | TBD (stock node has an IRQ GPIO) | Stock module `sh366101_fg.ko`; battery parameters in the DT |
| USB-C PD | Southchip SC2150 | I²C0, 0x4E | GPIO 93 | `southchip,sc2150`; stock module `tcpc_sc2150.ko`; DP alt-mode config present |
| USB switch / BC1.2 | WillSemi WUSB3801 | I²C0, 0x60 | GPIO 93 / 93 | `WillSemi,wusb3801`; stock module `wusb3801x.ko` |
| Charger framework | NOPMI (`qcom,nopmi-chg`) | — | — | Mainline nopmi driver exists in the 7.2 tree; uses charger + fuel gauge + IIO channels |
| USB | DWC3 HS | `4e00000.dwc3` | — | USB 2.0 High-Speed; QMP/DP PHY present, USB3/DP TBD |
| USB VBUS/switch | — | PM6125 GPIO 1 / `usb_eldo9` | — | Stock `vdda_usb_ss_dp_core` fixed regulator |

The stock BQ2589X node uses vendor-specific properties (`ti,bq2589x,*`) and
different interrupt GPIO than tapas (GPIO 84 here, GPIO 105 on tapas). The
mainline `bq25890` driver needs a compatibility/register check for BQ2589X.

## Audio

| Function | Part | Bus / address | Notes |
| --- | --- | --- | --- |
| Codec | Qualcomm WCD937x (WCD9370/WCD9375 family) | SoundWire (`rx-macro`, `tx-macro`, `va-macro`) | Reset GPIO 92; supplies PM6125 L14 (buck), L9 (rxtx/px) |
| Speaker amplifiers | Foursemi FS1815 ×4 (two per I²C bus) | I²C0 and I²C1, 0x34/0x35 | Reset GPIO 106; firmware `fs1815.fsm`; stock module `fs1815_dlkm.ko` |
| Alternate amplifiers | SIA81xx ×4 (`sia8159`) | I²C0/I²C1, 0x28/0x29 | Nodes present in the stock DT, but no `sia81xx` module in the stock `vendor_dlkm`; likely unused on production xun |
| WSA881x | Disabled in stock DT | I²C0 0x0E/0x44 | Stock module exists but nodes are `status = "disable"` |
| ADSP audio | LPASS + q6apm | — | Firmware from the stock `firmware` partition; topology/UCM needed |

The stock sound card is `qcom,bengal-asoc-snd` with four SIA81xx aux-device
phandles, but the module evidence points to FS1815 as the populated amplifier.
Confirm on the live unit before investing in the SIA path.

## Cameras

| Position | Sensor variants | Bus | Notes |
| --- | --- | --- | --- |
| Rear 8 MP | GalaxyCore GC08A3 (`m84_aac_gc08a3_i`), HI556 (`m84_aac_hi556_i`) | CCI0, CSIPHY0 | EEPROM + actuator; flash on CCI0 |
| Front 5 MP | GalaxyCore GC05A2 (`m84_aac_gc05a2_ii`) | CCI0, CSIPHY2 | EEPROM |

Stock camera libraries and tuning data are in `vendor/lib{,64}/camera/`.
Mainline CAMSS/CCI support exists; sensor drivers are a later phase.

## Sensors, buttons and misc

| Item | Connection | Notes |
| --- | --- | --- |
| Volume up | PM6125 GPIO 5, active low | `gpio-keys` |
| Volume down | PMIC resin / PON | `pon_resin` |
| Power | PMIC PON | `pon_pwrkey` |
| Hall / cover | TLMM GPIO 66, active low | `hall_key`; stock module `hall_sensor.ko` |
| Accelerometer / gyro / light / proximity | ADSP SSC (not AP I²C in the stock DT) | Models TBD; stock configs use `bengal_*`/`lsm6dso`/`tmd2725` families. Direct IIO may not be possible without board rework |
| Vibration | None (tablet) | — |
| Thermal | PM6125 ADC + charger thermistor | `charger-therm-usr`, `rf_pa1_therm` |
| microSD | SDHCI `sdhc_2`, CD GPIO 88 | VMMC L22, VQMMC L5 |

## Buses and GPIO summary

| Bus | Address | Devices |
| --- | --- | --- |
| I²C0 (`i2c@4a84000`, `qupv3_se1_i2c`) | 0x11, 0x4E, 0x60, 0x6A, 0x34, 0x35, 0x28, 0x29, 0x0E, 0x44 | KTZ8866 backlight, SC2150 PD, WUSB3801, BQ2589X, FS1815 ×2, SIA81xx ×2 (unused), WSA881x (disabled) |
| I²C1 (`i2c@4a88000`, `qupv3_se2_i2c`) | 0x55, 0x34, 0x35, 0x28, 0x29 | SH366101 fuel gauge, FS1815 ×2, SIA81xx ×2 (unused) |
| SPI0 (`spi@4a80000`, `qupv3_se0_spi`) | CS0 | Touch (FT8203 or NT36532) |
| UART | `4a90000` (debug), `4a8c000` | Console / BT HCI |
| UFS | `ufshc@4804000` | 128/256 GB storage |
| SD | `sdhc@4784000` | microSD |

Key GPIOs:

| GPIO | Function |
| --- | --- |
| 80 | Touch IRQ |
| 81 | Panel TE |
| 82 | Panel reset |
| 84 | BQ2589X interrupt |
| 86 | Touch reset |
| 88 | microSD card detect |
| 92 | WCD937x reset |
| 93 | SC2150 interrupt (and WUSB3801 IRQ in the stock node) |
| 102 | KTZ8866 enable |
| 106 | FS1815 reset |
| 66 | Hall sensor |

## Boot and partitions

| Item | Value |
| --- | --- |
| Boot header | v4 |
| `boot` | 100663296 bytes, kernel only (stock ~47 MB) |
| `init_boot` | 8388608 bytes, generic ramdisk |
| `vendor_boot` | 100663296 bytes, vendor ramdisk + DTB + cmdline |
| `dtbo` | 8388608 bytes, 47 overlays including xun |
| `super` | 6979321856 bytes |
| Dynamic partitions | `odm product system system_dlkm system_ext vendor vendor_dlkm` |
| Slot layout | A/B (`boot_a/boot_b`, `init_boot_a/b`, `vendor_boot_a/b`, ...) |
| Stock vendor cmdline | `video=vfb:640x400,bpp=32,memsize=3072000 bootconfig` |
| Splash/framebuffer | 0x5C000000, `cont_splash` region ~15 MB; DFPS data at 0x5CF00000 |
| RAM node | Placeholder in the stock DTB (`memory { reg = <0 0 0 0>; }`); ABL fills it |

## Firmware inventory

Available from the stock `vendor`/`odm` dump:

- GPU: `a610_zap.{mdt,b00,b01,b02,elf}`.
- Video: `venus*.mbn`, `vpu20_*.mbn`.
- Touch: `focaltech_ts_fw_boe_ft8203.bin`, `focaltech_ts_fw_huaxing_ft8203.bin`,
  `novatek_ts_fw_tianma.bin`, `novatek_ts_mp_tianma.bin`.
- Audio amp: `fs1815.fsm`, `sipa.bin`.
- WLAN config: `vendor/firmware/wlan/qca_cld/WCNSS_qcom_cfg.ini`.
- Camera tuning/EEPROM data in `vendor/lib{,64}/camera/`.

Not present in the dump (needs a `firmware` partition dump or full ROM):

- WCN3990 Wi-Fi firmware (`firmware-5.bin`, `bdwlan*.bin`, board data).
- ADSP/CDSP/MPSS/IPA firmware (`adsp.mbn`, `cdsp.mbn`, `modem.mdt`, `ipa_fws.*`).
  The `firmware-xiaomi-spes` package can supply SM6225-family images, but xun
  calibration/board data should come from the device.
- `wlanmdsp.mbn` (the `firmware-spes` package has it; the dump only has
  `wlanmdsp.otaupdate`).

## Known hardware uncertainties

- Which panel/touch combination each retail unit has.
- Touch AVDD rail and whether it is controlled by GPIO/PMIC.
- Whether brightness is DCS-only or uses KTZ8866 on some panels.
- Actual speaker amplifier population (FS1815 vs SIA81xx) and speaker count.
- Sensor models and whether they are reachable from the AP.
- USB3/DisplayPort capability and QMP PHY configuration.
- Battery identification (`batt_id` ADC channel) and charge-profile selection.
