#!/usr/bin/env python3
"""Generate a printable A4 sheet of device labels (QR code + device ID).

Layout: 3 columns x 7 rows per A4 page. The grid is centered on the page,
so with an odd number of rows the middle row's vertical center automatically
coincides with the page's vertical center.

Each label's QR code encodes "<device_id>:<secret>" -- a plain colon-
separated string, not JSON, since embedding quotes in QR payloads has
caused parsing trouble before. Split on the first ":" to recover the two
fields.

Secrets are persisted to a CSV file (device_id,secret) alongside the PDF.
Re-running the script for the same --secrets-out file reuses secrets
already on record for a device ID instead of regenerating them, so
existing devices already provisioned with a printed QR code aren't
invalidated when the sheet is regenerated (e.g. to add more devices).

Usage:
    python generate_labels.py --count 30
    python generate_labels.py --count 30 --start 10 --prefix eagle
"""

import argparse
import csv
import io
import secrets
import string
from pathlib import Path

import qrcode
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib.utils import ImageReader
from reportlab.pdfgen import canvas

COLS = 3
ROWS = 7

# Characters used for generated secrets. Excludes visually ambiguous
# characters (0/O, 1/I/l) purely so a human transcribing one by hand from
# a screen is less likely to make a mistake; the QR code itself doesn't
# care.
SECRET_ALPHABET = "".join(
    c for c in string.ascii_letters + string.digits if c not in "0O1Il"
)


def generate_secret(length: int) -> str:
    return "".join(secrets.choice(SECRET_ALPHABET) for _ in range(length))


def load_existing_secrets(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    with path.open(newline="") as f:
        return {row["device_id"]: row["secret"] for row in csv.DictReader(f)}


def save_secrets(path: Path, records: list[tuple[str, str]]) -> None:
    with path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["device_id", "secret"])
        writer.writerows(records)


def make_qr_image(payload: str):
    qr = qrcode.QRCode(
        error_correction=qrcode.constants.ERROR_CORRECT_M,
        box_size=10,
        border=2,
    )
    qr.add_data(payload)
    qr.make(fit=True)
    return qr.make_image(fill_color="black", back_color="white").convert("RGB")


def draw_label(c: canvas.Canvas, x: float, y: float, w: float, h: float, device_id: str, secret: str, show_border: bool) -> None:
    """Draw one label with its bottom-left corner at (x, y)."""
    pad = 3 * mm

    if show_border:
        c.setStrokeColorRGB(0.75, 0.75, 0.75)
        c.setLineWidth(0.3)
        c.rect(x, y, w, h)

    qr_size = h - 2 * pad
    qr_img = make_qr_image(f"{device_id}:{secret}")
    c.drawImage(
        ImageReader(qr_img),
        x + pad,
        y + pad,
        width=qr_size,
        height=qr_size,
        preserveAspectRatio=True,
    )

    text_x = x + pad + qr_size + pad
    text_center_y = y + h / 2
    font_size = min(16, h * 0.35 / mm)
    c.setFont("Helvetica-Bold", font_size)
    c.setFillColorRGB(0, 0, 0)
    c.drawString(text_x, text_center_y - font_size * 0.35, device_id)


def build_sheet(
    out_path: Path,
    device_ids: list[str],
    secrets_by_id: dict[str, str],
    margin_x: float,
    margin_y: float,
    gap: float,
    show_border: bool,
) -> None:
    page_w, page_h = A4
    usable_w = page_w - 2 * margin_x
    usable_h = page_h - 2 * margin_y
    label_w = (usable_w - (COLS - 1) * gap) / COLS
    label_h = (usable_h - (ROWS - 1) * gap) / ROWS

    c = canvas.Canvas(str(out_path), pagesize=A4)

    labels_per_page = COLS * ROWS
    for page_start in range(0, len(device_ids), labels_per_page):
        page_ids = device_ids[page_start : page_start + labels_per_page]
        for i, device_id in enumerate(page_ids):
            row = i // COLS
            col = i % COLS
            x = margin_x + col * (label_w + gap)
            # row 0 is the top row; y is measured from the bottom of the page.
            y = page_h - margin_y - (row + 1) * label_h - row * gap
            draw_label(c, x, y, label_w, label_h, device_id, secrets_by_id[device_id], show_border)
        c.showPage()

    c.save()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--count", type=int, required=True, help="Number of devices to generate labels for")
    parser.add_argument("--start", type=int, default=0, help="First device index (default: 0)")
    parser.add_argument("--prefix", default="eagle", help="Device ID prefix (default: eagle)")
    parser.add_argument("--pad", type=int, default=2, help="Zero-padding width for the device index (default: 2)")
    parser.add_argument("--secret-length", type=int, default=16, help="Length of generated secrets (default: 16)")
    parser.add_argument("--out", default="output/labels.pdf", help="Output PDF path")
    parser.add_argument("--secrets-out", default="output/device_secrets.csv", help="Output CSV path for device_id,secret")
    parser.add_argument("--margin-x", type=float, default=10.0, help="Left/right page margin in mm (default: 10)")
    parser.add_argument("--margin-y", type=float, default=10.0, help="Top/bottom page margin in mm (default: 10)")
    parser.add_argument("--gap", type=float, default=2.0, help="Gap between labels in mm (default: 2)")
    parser.add_argument("--no-borders", action="store_true", help="Don't draw cutting-guide borders around labels")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    out_path = (script_dir / args.out).resolve()
    secrets_path = (script_dir / args.secrets_out).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    secrets_path.parent.mkdir(parents=True, exist_ok=True)

    device_ids = [f"{args.prefix}{i:0{args.pad}d}" for i in range(args.start, args.start + args.count)]

    existing = load_existing_secrets(secrets_path)
    secrets_by_id = dict(existing)
    new_count = 0
    for device_id in device_ids:
        if device_id not in secrets_by_id:
            secrets_by_id[device_id] = generate_secret(args.secret_length)
            new_count += 1

    # Persist the full known set (existing entries untouched, so devices
    # provisioned in earlier runs keep their printed secret) but only
    # write out the ids we actually know about.
    all_records = sorted(secrets_by_id.items())
    save_secrets(secrets_path, all_records)

    build_sheet(
        out_path,
        device_ids,
        secrets_by_id,
        margin_x=args.margin_x * mm,
        margin_y=args.margin_y * mm,
        gap=args.gap * mm,
        show_border=not args.no_borders,
    )

    print(f"Generated {len(device_ids)} labels ({new_count} new secret(s)) across "
          f"{-(-len(device_ids) // (COLS * ROWS))} page(s).")
    print(f"PDF:     {out_path}")
    print(f"Secrets: {secrets_path}")
    print(f"QR payload format: <device_id>:<secret>, e.g. {device_ids[0]}:{secrets_by_id[device_ids[0]]}")


if __name__ == "__main__":
    main()
