# Scripts

| Script | Purpose |
| --- | --- |
| `parse_dtbo.py` | Unpack any Android `dtbo.img` into individual `.dtb` files (optional `--dts` decompile) |
| `merge_dts.sh` | Apply an overlay DTB to a base DTB and decompile the result |
| `extract_xun_dt.sh` | Clone the public xun partition dump, extract the base DTB and xun overlay, merge and decompile |

`extract_xun_dt.sh` needs `git`, `dtc` and `fdtoverlay` (the latter two are in
the `device-tree-compiler` package). It writes to `xun-dt/` by default; pass
another directory as the first argument.

The dump's root `dtbo.img` is a generic 32-entry Qualcomm image. The xun
overlay is in the dump's pre-extracted `dtbo/` set, which the script uses.
