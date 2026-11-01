#!/usr/bin/env python3
"""Unpack an Android dtbo.img (dt_table) into individual .dtb files.

The stock Xiaomi dtbo.img stores its fields big-endian and puts the 32-byte
entry table at header_size (0x20 for the xun image), even though some
descriptions of the format call the field "dt_entries_offset". This parser
tries the header_size first and falls back to the 0x20 table used by the
stock xun image.

Usage:
    parse_dtbo.py dtbo.img out-dir [--dts]

--dts also decompiles each entry with dtc (must be installed).
"""

from __future__ import annotations

import argparse
import struct
import subprocess
import sys
from pathlib import Path


def parse(path: Path, out_dir: Path, decompile: bool) -> int:
    data = path.read_bytes()
    if len(data) < 32 or data[0:4] != b"\xd7\xb7\xab\x1e":
        print(f"{path}: not a dtbo image", file=sys.stderr)
        return 1

    # magic, total_size, header_size, entry_count, entries_offset, page_size, version
    magic, total_size, header_size, count, entries_offset, page_size, version = (
        struct.unpack_from(">7I", data, 0)
    )
    if count == 0 or count > 4096:
        print(f"{path}: implausible entry count {count}", file=sys.stderr)
        return 1

    # The stock xun image has entries at 0x20 (header_size). A standards-
    # compliant image may use entries_offset; use it only if it is sane.
    candidates = [header_size, entries_offset, 0x20]
    table = None
    for candidate in candidates:
        if 32 <= candidate <= len(data) - count * 32:
            # Validate the first entry: offset/size must fit in the file.
            size, offset = struct.unpack_from(">II", data, candidate)
            if size and offset + size <= len(data):
                table = candidate
                break
    if table is None:
        print(f"{path}: could not locate the entry table", file=sys.stderr)
        return 1

    out_dir.mkdir(parents=True, exist_ok=True)
    print(
        f"{path}: total={total_size} header={header_size} entries={count} "
        f"table=0x{table:x} page={page_size} version={version}"
    )

    for i in range(count):
        base = table + i * 32
        size, offset, dt_id, rev, c0, c1, c2, c3 = struct.unpack_from(
            ">8I", data, base
        )
        if size == 0 or offset + size > len(data):
            print(f"  entry {i}: invalid size/offset ({size:#x}/{offset:#x})",
                  file=sys.stderr)
            continue
        name = f"{i:02d}_id{dt_id:x}_rev{rev:x}.dtb"
        target = out_dir / name
        target.write_bytes(data[offset:offset + size])
        print(f"  {name} size={size} offset={offset:#x}")

        if decompile:
            dts = target.with_suffix(".dts")
            try:
                subprocess.run(
                    ["dtc", "-I", "dtb", "-O", "dts", "-o", str(dts), str(target)],
                    check=True,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
            except FileNotFoundError:
                print("  dtc not found; skipping decompilation", file=sys.stderr)
                decompile = False
            except subprocess.CalledProcessError:
                print(f"  warning: dtc failed for {name}", file=sys.stderr)

    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dtbo", type=Path)
    parser.add_argument("out_dir", type=Path)
    parser.add_argument("--dts", action="store_true", help="decompile with dtc")
    args = parser.parse_args()
    if not args.dtbo.is_file():
        parser.error(f"{args.dtbo} does not exist")
    return parse(args.dtbo, args.out_dir, args.dts)


if __name__ == "__main__":
    sys.exit(main())
