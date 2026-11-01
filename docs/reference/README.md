# Reference data

Generated from the public stock partition dump
[`gm-stuffs/redmi_xun_dump`](https://github.com/gm-stuffs/redmi_xun_dump)
(branch `missi-user-15-AQ3A.240829.003-OS2.0.1.0.VMUMIXM`).

| File | Contents |
| --- | --- |
| `xun-overlay.dts` | Decompiled xun DTBO overlay (`KHAJE IDP nopmi xun`), entries 46/47 of the dump's extracted `dtbo/` set |
| `xun-merged.dts` | Base Khaje SoC DTB merged with the xun overlay, decompiled |
| `xun-stock-info.txt` | Boot headers, partition sizes, board IDs, relevant stock modules and firmware |

Notes:

- These are **generated reference files**, not the source of truth for the
  mainline DTS. The mainline DTS will be written by hand against
  `sm6225.dtsi`/`pm6125.dtsi` and the tapas board DTS.
- The decompiled DTC output contains numeric phandles and compacted values; it
  is intended for cross-checking wiring, init sequences and properties.
- The base DTB is the SoC tree from `vendor_boot`; the bootloader fills the
  memory node and selects the overlay at boot. Do not treat the merged file's
  memory node as final.
- Proprietary firmware blobs are not included.
