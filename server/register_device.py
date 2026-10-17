#!/usr/bin/env python3
"""
Device Registration Script

Provisions a new device in the Web API. mqtt-ingestor no longer auto-registers devices on
first MQTT message - a device must be created here (or via the admin UI's "Create Device"
form) before it can publish sensor readings. This is also the only time the device's
pairing_secret is available, so print/save the QR payload this script prints - the API never
returns the secret again after this call.

Usage:
    python register_device.py --device-id bbc456 --name "Kitchen bottle" \\
        --admin-username admin --admin-password admin123

    # Prompts for the admin password instead of passing it on the command line:
    python register_device.py --device-id bbc456 --admin-username admin
"""

import argparse
import getpass
import json
import sys
import urllib.error
import urllib.request


def api_post(base_url: str, path: str, payload: dict, token: str = None) -> dict:
    """POST JSON to the API and return the decoded JSON response."""
    url = f"{base_url.rstrip('/')}{path}"
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"

    request = urllib.request.Request(
        url, data=json.dumps(payload).encode("utf-8"), headers=headers, method="POST"
    )

    try:
        with urllib.request.urlopen(request) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        detail = e.read().decode("utf-8", errors="replace")
        try:
            detail = json.loads(detail).get("detail", detail)
        except json.JSONDecodeError:
            pass
        print(f"✗ {e.code} from {path}: {detail}", file=sys.stderr)
        sys.exit(1)
    except urllib.error.URLError as e:
        print(f"✗ Could not reach {url}: {e.reason}", file=sys.stderr)
        sys.exit(1)


def print_qr(payload: str):
    """Print an ASCII QR code for the pairing payload, if the optional `qrcode` package is installed."""
    try:
        import qrcode
    except ImportError:
        print("(install the 'qrcode' package - `pip install qrcode` - to print a scannable QR code here)")
        return

    qr = qrcode.QRCode(border=1)
    qr.add_data(payload)
    qr.make(fit=True)
    qr.print_ascii(invert=True)


def main():
    parser = argparse.ArgumentParser(
        description="Provision a new device and print its pairing QR payload",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Provision a device, prompting for the admin password
  %(prog)s --device-id bbc456 --name "Kitchen bottle" --admin-username admin

  # Non-interactive (e.g. from another script)
  %(prog)s --device-id bbc456 --admin-username admin --admin-password admin123

  # Against a non-default API URL
  %(prog)s --device-id bbc456 --admin-username admin --api-url https://api.example.com

  # With a known calibration fit (weight = (raw_value + offset) * gain)
  %(prog)s --device-id bbc456 --admin-username admin --offset -256620.0 --gain -0.09121
        """,
    )

    parser.add_argument("--device-id", required=True, help="Unique device ID (matches what the firmware publishes to sensors/<device_id>/data)")
    parser.add_argument("--name", default=None, help="Human-readable device name")
    parser.add_argument("--description", default=None, help="Optional description")
    parser.add_argument(
        "--offset", type=float, default=0.0,
        help="Calibration offset: weight = (raw_value + offset) * gain (default: 0.0, i.e. not calibrated yet)"
    )
    parser.add_argument(
        "--gain", type=float, default=1.0,
        help="Calibration gain: weight = (raw_value + offset) * gain (default: 1.0, i.e. not calibrated yet)"
    )
    parser.add_argument("--api-url", default="http://localhost:8000", help="Web API base URL (default: http://localhost:8000)")
    parser.add_argument("--admin-username", required=True, help="Admin username to authenticate with")
    parser.add_argument("--admin-password", default=None, help="Admin password (omit to be prompted)")

    args = parser.parse_args()

    if args.gain == 0:
        parser.error("--gain must not be zero")

    password = args.admin_password or getpass.getpass("Admin password: ")

    print(f"Logging in as {args.admin_username}...")
    login = api_post(args.api_url, "/auth/login", {
        "username": args.admin_username,
        "password": password,
    })
    token = login["access_token"]

    print(f"Creating device '{args.device_id}'...")
    device = api_post(args.api_url, "/devices", {
        "device_id": args.device_id,
        "name": args.name,
        "description": args.description,
        "offset": args.offset,
        "gain": args.gain,
    }, token=token)

    pairing_payload = f"verolic://pair?device_id={device['device_id']}&secret={device['pairing_secret']}"

    print("\n" + "-" * 60)
    print(f"✓ Device created: {device['device_id']}")
    print(f"  Calibration:    offset={device['offset']}, gain={device['gain']}")
    print(f"  Pairing secret: {device['pairing_secret']}")
    print(f"  QR payload:     {pairing_payload}")
    print("-" * 60)
    print("\nThis secret is shown ONLY now - the API never returns it again. Print the QR")
    print("below (or encode the payload above) as the device's label.\n")
    print_qr(pairing_payload)


if __name__ == "__main__":
    main()
