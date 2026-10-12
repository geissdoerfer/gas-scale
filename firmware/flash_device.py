#!/usr/bin/env python3
"""Patch the dev_name placeholder in a built scale.bin and flash the result.

Usage: ./flash_device.py <dev_num>

Requires build/Debug/scale.bin to already exist (run the "build" task first).
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
BIN_PATH = PROJECT_ROOT / "build" / "Debug" / "scale.bin"

FLASH_BASE = 0x08000000
CONFIG_PAGE_ADDR = 0x0803F800
CONFIG_PAGE_SIZE = 0x800

PLACEHOLDER = b"eagleXX\x00"


def patch_dev_name(data: bytes, dev_num: int) -> bytes:
    replacement = f"eagle{dev_num:02d}".encode("ascii") + b"\x00"
    assert len(replacement) == len(PLACEHOLDER)

    count = data.count(PLACEHOLDER)
    if count != 1:
        raise ValueError(
            f"expected exactly one occurrence of {PLACEHOLDER!r} in the binary, found {count}"
        )
    return data.replace(PLACEHOLDER, replacement)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dev_num", type=int, help="Device number, e.g. 3 -> eagle03")
    parser.add_argument("--bin", type=Path, default=BIN_PATH, help="Path to the built .bin file")
    args = parser.parse_args()

    if not 0 <= args.dev_num <= 99:
        parser.error("dev_num must be between 0 and 99 to fit eagleXX's two digits")

    if not args.bin.exists():
        parser.error(f"{args.bin} does not exist; build the project first")

    data = args.bin.read_bytes()
    try:
        patched = patch_dev_name(data, args.dev_num)
    except ValueError as e:
        parser.error(str(e))

    dev_name = f"eagle{args.dev_num:02d}"

    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as tmp:
        tmp.write(patched)
        tmp_path = Path(tmp.name)

    try:
        print(f"Flashing {args.bin} as {dev_name} via {tmp_path}")
        subprocess.run(
            [
                "openocd",
                "-f", "interface/stlink.cfg",
                "-f", "target/stm32u0x.cfg",
                "-c",
                (
                    "init; reset halt; "
                    f"flash erase_address {CONFIG_PAGE_ADDR:#x} {CONFIG_PAGE_SIZE:#x}; "
                    f"program {tmp_path} {FLASH_BASE:#x} verify reset exit"
                ),
            ],
            cwd=PROJECT_ROOT,
            check=True,
        )
    except subprocess.CalledProcessError as e:
        return e.returncode
    finally:
        tmp_path.unlink(missing_ok=True)

    return 0


if __name__ == "__main__":
    sys.exit(main())
