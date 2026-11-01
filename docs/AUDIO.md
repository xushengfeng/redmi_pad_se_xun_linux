# Speaker audio

The tablet has four speakers, each driven by a Foursemi FS1815 amplifier
behind the WCD937x codec. This is the path from an application to the
speakers and what had to be built for it.

## Signal path

```
application
  -> PulseAudio (UCM profile "HiFi", device "Speaker")
  -> MultiMedia1 Playback (q6apm front end, hw:0,0)
  -> RX_CODEC_DMA_RX_0 (LPASS codec DMA back end)
  -> LPASS RX macro -> SoundWire -> WCD937x (RDAC/AUX output)
  -> four FS1815 amplifiers -> speakers
```

The sound card is `SM6225-Xun` (`snd-sm8250`, the `qcom,qrb4210-rb2-sndcard`
machine driver). The ADSP runs the AudioReach firmware and the front ends and
graphs come from `qcom/sm4250/SM6225-Xun-tplg.bin`, built by `mktplg.sh` from
`build/topology/SM6225-Xun.m4`.

## Microphones

Capture uses the reverse path:

```
internal microphone -> WCD937x ADC1 -> SoundWire uplink (SWR_MIC0)
  -> TX macro DEC0 -> MultiMedia2 Capture (hw:0,1) -> application
```

The UCM `Mic` device programs that path. The important control values are
`TX DEC0 MUX` = `SWR_MIC`, `TX SMIC MUX0` = `SWR_MIC0` and
`TX_AIF1_CAP Mixer DEC0` = `1`. The `TX SMIC MUX0` items on this machine are
`SWR_MIC0..11` (not `ADC0`, which some upstream `wcd937x` sequences use), so
the generic `codecs/wcd937x/HeadphoneMicEnableSeq.conf` values do not apply
here.

The microphone is mono; PulseAudio exposes it as a stereo source with the
signal on the left channel.

## Sound server

The image uses PulseAudio (`pulseaudio --start`) started per user session by
`build/device/etc/xdg/autostart/pulseaudio.desktop`; PipeWire is not installed.
PulseAudio reads the same UCM profile and exposes
`alsa_output.platform-sound.HiFi__Speaker__sink` and
`alsa_input.platform-sound.HiFi__Mic__source`. A recording made while playing
a 1 kHz tone shows the tone clearly in the microphone spectrum.

## The amplifiers

| Bus | I2C address | Preset entry | Reset line |
| --- | --- | --- | --- |
| `i2c1` (4a84000) | 0x34 | 0x34 | GPIO 106 (shared with 0x35) |
| `i2c1` (4a84000) | 0x35 | 0x35 | GPIO 106 |
| `i2c2` (4a88000) | 0x34 | 0x36 | GPIO 70 (shared with 0x35) |
| `i2c2` (4a88000) | 0x35 | 0x37 | GPIO 70 |

The DT nodes are in `build/kernel-dts/sm6225-xiaomi-xun.dts`; each carries
`foursemi,dev-addr` to pick its preset entry (the two buses use the same I2C
addresses) and its shared `reset-gpios`.

`build/kernel-audio/fs1815.c` is the driver:

- register access is one byte of address plus a 16-bit big-endian value;
- probe pulses the reset line, soft-resets the chip (`0x10` = 2, wait, `0x10`
  = 1) and polls `CHIPINI` (`0x0e`) for `0x0003`;
- it then writes the speaker temperature coefficient (`0x14`), the common
  register table and the "music" scene, and powers the amplifier up
  (`SYSCTRL` `0x11` = `0x008f`, `PWRCTRL` `0x10` = 0);
- only register `0x19` differs between the four positions (channel select);
- the amplifier then stays on. Volume is done in software: PulseAudio scales
  the PCM samples before the DSP.

The register tables are the ones the vendor driver reads from `fs1815.fsm`,
the preset file in the Android vendor partition. `build/kernel-audio/fsm-preset.py`
parses that file and prints them; the driver carries the result, so the file
is not needed at runtime.

**Shared reset lines:** the two amplifiers of one bus share a reset GPIO. Newer
kernels handle shared lines through `gpio-shared-proxy`, which is a module
(`=m`) by default — without `CONFIG_GPIO_SHARED_PROXY=y` the probe defers
forever with `reset gpio: -517`. The xun config enables it.

## Routing: the UCM profile

`build/ucm2` installs the ALSA UCM profile
(`conf.d/sm4250/SM6225-Xun.conf` and `Qualcomm/sm6225/xun/HiFi.conf`). The
verb connects the front end to the back end:

```
RX_CODEC_DMA_RX_0 Audio Mixer MultiMedia1   on
MultiMedia2 Mixer TX_CODEC_DMA_TX_3         on
```

and the "Speaker" device enables the codec side:

```
RX_MACRO RX0 MUX        AIF1_PB
RX INT2_1 MIX1 INP0     RX0
RX_RX2 Digital Volume   84
LO Switch               on
AUX_RDAC Switch         on
```

Without the verb, opening MultiMedia1 fails with "no backend DAIs enabled for
MultiMedia1 Playback". PulseAudio reads the UCM and exposes the card as
"HiFi (Mic, Speaker)" with the sink
`alsa_output.platform-sound.HiFi__Speaker__sink`, but it does not always
program the sequences on a fresh profile, so
`build/device/etc/local.d/45-audio.start` applies the verb and the Speaker
device once the card is registered:

```sh
printf 'set _verb HiFi\nset _enadev Speaker\n' >/run/ucm-speaker.txt
alsaucm -c SM6225-Xun -b /run/ucm-speaker.txt
```

## Testing

Apply the profile by hand on a running tablet (a batch file, because
`alsaucm` opens the card per invocation):

```sh
printf 'set _verb HiFi\nset _enadev Speaker\n' >/tmp/ucm.txt
alsaucm -c SM6225-Xun -b /tmp/ucm.txt

# bypass PulseAudio
speaker-test -D hw:0,0 -c 2 -t sine -l 1

# through PulseAudio
su xun -c "XDG_RUNTIME_DIR=/run/user/1000 paplay tone.wav"

su xun -c "XDG_RUNTIME_DIR=/run/user/1000 pactl list cards" | grep -E 'Profile|Speaker'
```

The amplifiers must show up in `dmesg`:

```
fs1815 0-0034: FS1815 at 0x34 (preset 0x34), power on
fs1815 0-0035: FS1815 at 0x35 (preset 0x35), power on
fs1815 1-0034: FS1815 at 0x34 (preset 0x36), power on
fs1815 1-0035: FS1815 at 0x35 (preset 0x37), power on
```

## Extracting fs1815.fsm

The file lives in the Android `vendor` logical partition inside `super`, which
is EROFS. To get it (the driver does not need it, this is for changing the
tables):

1. copy the super metadata and find the `vendor_a` extent:
   `dd if=/dev/disk/by-partlabel/super bs=1M count=16 of=/tmp/super-meta`,
   `lpdump /tmp/super-meta` (host, `android-tools`/`lpunpack`);
2. loop-mount that extent read-only on the tablet (this needs
   `CONFIG_EROFS_FS=y`, which the xun config enables):
   `losetup -o <offset> --sizelimit <size> /dev/loop0 /dev/disk/by-partlabel/super`
   and `mount -t erofs -o ro /dev/loop0 /mnt/vendor`;
3. copy `/mnt/vendor/firmware/fs1815.fsm` and dump it with
   `python3 build/kernel-audio/fsm-preset.py fs1815.fsm`.

## Limits

- The amplifiers stay powered after probe; they are not part of the DAPM graph
  and there is no runtime power management yet.
- Amplifier gain is the fixed preset value; volume is the PCM volume.
- The earpiece and headphone ports have UCM sections but were not tested
  (the tablet has no jack detection populated).
- The headset microphone UCM section still uses the generic upstream control
  names and needs the same `SWR_MIC*` adaptation as the internal microphone
  once a headset is available for testing.
