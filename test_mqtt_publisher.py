#!/usr/bin/env python3
"""
MQTT Test Data Publisher

Publishes random sensor data to MQTT broker for testing purposes.
Simulates the firmware (firmware/scale/Core/Src/app.c), which publishes
{"val": <raw HX711 reading>, "temp": <float>, "bat": <battery voltage>, "cnt": <radio init count>}.

Usage:
    python test_mqtt_publisher.py --device-id sensor_001 --rate 1.0
    python test_mqtt_publisher.py --device-id device_123 --rate 0.5 --duration 60
"""

import argparse
import json
import random
import time
import sys
from datetime import datetime, timezone

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Error: paho-mqtt not installed. Install with: pip install paho-mqtt")
    sys.exit(1)


class SensorDataGenerator:
    """Generates realistic sensor data with some variation"""

    def __init__(self, base_val=14.0, base_voltage=3.7, base_temp=23.0):
        self.base_val = base_val
        self.base_voltage = base_voltage
        self.base_temp = base_temp
        self.val_trend = 0.0
        self.voltage_trend = 0.0
        self.temp_trend = 0.0
        self.cnt = random.randint(0, 100)

    def generate(self):
        """Generate a reading with realistic variations"""
        # Add random walk to trends
        self.val_trend += random.uniform(-1.1, 1.1)
        self.val_trend = max(0.0, min(12.0, self.val_trend))  # Limit trend

        self.voltage_trend += random.uniform(-0.02, 0.02)
        self.voltage_trend = max(-0, min(0.5, self.voltage_trend))

        self.temp_trend += random.uniform(-0.1, 0.1)
        self.temp_trend = max(-2.0, min(2.0, self.temp_trend))

        # Generate values with noise
        val = self.base_val + self.val_trend + random.uniform(-0.5, 0.5)
        voltage = self.base_voltage + self.voltage_trend + random.uniform(-0.1, 0.1)
        temp = self.base_temp + self.temp_trend + random.uniform(-0.5, 0.5)

        # Ensure realistic bounds
        val = max(0.0, min(100.0, val))  # raw HX711 reading (arbitrary test scale)
        voltage = max(3.0, min(4.3, voltage))
        temp = max(-10.0, min(50.0, temp))

        self.cnt += 1

        return {
            "val": round(val, 3),
            "temp": round(temp, 3),
            "bat": round(voltage, 3),
            "cnt": self.cnt,
        }


def on_connect(client, userdata, flags, rc, properties=None):
    """Callback for when client connects to broker"""
    if rc == 0:
        print(f"✓ Connected to MQTT broker")
    else:
        print(f"✗ Connection failed with code {rc}")


def on_publish(client, userdata, mid, reason_codes=None, properties=None):
    """Callback for when message is published"""
    userdata["publish_count"] += 1


def publish_data(
    device_id,
    rate,
    duration,
    broker,
    port,
    topic_prefix,
    mode,
    username=None,
    password=None,
    tls=False,
):
    """
    Publish sensor data at specified rate

    Args:
        device_id: Device identifier
        rate: Messages per second
        duration: Duration in seconds (None for infinite)
        broker: MQTT broker hostname
        port: MQTT broker port
        topic_prefix: Topic prefix (device_id will be appended)
        mode: Data generation mode (realistic, random, or stable)
        username: MQTT username (optional)
        password: MQTT password (optional)
        tls: Connect using TLS (required by brokers like HiveMQ Cloud, typically on port 8883)
    """
    # Create client with callback data
    userdata = {"publish_count": 0}
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=f"test_publisher_{device_id}",
        userdata=userdata,
    )

    if username is not None:
        client.username_pw_set(username, password)

    if tls:
        client.tls_set()

    client.on_connect = on_connect
    client.on_publish = on_publish

    # Connect to broker
    print(f"Connecting to MQTT broker at {broker}:{port}...")
    try:
        client.connect(broker, port, 60)
    except Exception as e:
        print(f"✗ Failed to connect: {e}")
        return

    # Start network loop in background
    client.loop_start()

    # Wait for connection
    time.sleep(1)

    # Generate topic
    topic = f"{topic_prefix}/{device_id}/data"

    # Create data generator
    if mode == "realistic":
        generator = SensorDataGenerator()
    elif mode == "random":
        generator = SensorDataGenerator(
            base_val=random.uniform(500, 3000),
            base_voltage=random.uniform(11.5, 13.0),
            base_temp=random.uniform(15, 30),
        )
    else:  # stable
        generator = SensorDataGenerator()
        # Override generate to return stable values
        generator.generate = lambda: {
            "val": 1500,
            "temp": 23.0,
            "bat": 12.6,
            "cnt": generator.cnt,
        }

    print(f"\nPublishing to topic: {topic}")
    print(f"Rate: {rate} msg/sec")
    print(f"Duration: {'infinite' if duration is None else f'{duration} seconds'}")
    print(f"Mode: {mode}")
    print("\nPress Ctrl+C to stop\n")
    print("-" * 60)

    interval = 1.0 / rate
    start_time = time.time()
    message_count = 0

    try:
        while True:
            # Check duration
            if duration is not None and (time.time() - start_time) >= duration:
                break

            # Generate data
            data = generator.generate()

            # Publish
            payload = json.dumps(data)
            result = client.publish(topic, payload, qos=1)

            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                message_count += 1
                print(
                    f"[{message_count:4d}] Published: val={data['val']:5.2f}  "
                    f"bat={data['bat']:5.2f}V  "
                    f"temp={data['temp']:5.2f}°C  "
                    f"cnt={data['cnt']}"
                )
            else:
                print(f"✗ Publish failed: {result.rc}")

            # Wait for next interval
            time.sleep(interval)

    except KeyboardInterrupt:
        print("\n" + "-" * 60)
        print("Interrupted by user")

    finally:
        # Cleanup
        elapsed = time.time() - start_time
        client.loop_stop()
        client.disconnect()

        print("\nStatistics:")
        print(f"  Messages sent: {message_count}")
        print(f"  Duration: {elapsed:.2f} seconds")
        print(f"  Average rate: {message_count/elapsed:.2f} msg/sec")
        print(f"  Messages published: {userdata['publish_count']}")


def main():
    parser = argparse.ArgumentParser(
        description="MQTT Test Data Publisher - Send random sensor data to MQTT broker",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Publish as device_001 at 1 msg/sec
  %(prog)s --device-id device_001 --rate 1.0

  # Publish at 0.5 msg/sec for 60 seconds
  %(prog)s --device-id sensor_123 --rate 0.5 --duration 60

  # Publish with random data mode at 2 msg/sec
  %(prog)s --device-id test_device --rate 2.0 --mode random

  # Publish to custom broker
  %(prog)s --device-id sensor_001 --broker 192.168.1.100 --port 1883

  # Publish with broker authentication
  %(prog)s --device-id sensor_001 --username myuser --password mypass

  # Publish to HiveMQ Cloud (TLS on port 8883)
  %(prog)s --device-id sensor_001 --broker xxxx.s1.eu.hivemq.cloud --port 8883 --tls --username myuser --password mypass
        """,
    )

    parser.add_argument(
        "--device-id",
        type=str,
        required=True,
        help="Device ID to use for publishing (e.g., sensor_001, device_123)",
    )

    parser.add_argument(
        "--rate",
        type=float,
        default=1.0,
        help="Publishing rate in messages per second (default: 1.0)",
    )

    parser.add_argument(
        "--duration",
        type=float,
        default=None,
        help="Duration in seconds (default: infinite, run until Ctrl+C)",
    )

    parser.add_argument(
        "--broker",
        type=str,
        default="localhost",
        help="MQTT broker hostname (default: localhost)",
    )

    parser.add_argument(
        "--port", type=int, default=1883, help="MQTT broker port (default: 1883)"
    )

    parser.add_argument(
        "--username",
        type=str,
        default=None,
        help="MQTT username (default: none, connect anonymously)",
    )

    parser.add_argument(
        "--password",
        type=str,
        default=None,
        help="MQTT password (default: none)",
    )

    parser.add_argument(
        "--tls",
        action="store_true",
        help="Connect using TLS (required by brokers like HiveMQ Cloud, typically port 8883)",
    )

    parser.add_argument(
        "--topic-prefix",
        type=str,
        default="sensors",
        help="MQTT topic prefix (default: <sensors)",
    )

    parser.add_argument(
        "--mode",
        type=str,
        choices=["realistic", "random", "stable"],
        default="realistic",
        help="Data generation mode: realistic (trending values), random (completely random), stable (constant values) (default: realistic)",
    )

    args = parser.parse_args()

    # Validate rate
    if args.rate <= 0:
        parser.error("Rate must be positive")
    if args.rate > 10:
        print(f"Warning: High rate ({args.rate} msg/sec) may overwhelm the system")

    # Validate duration
    if args.duration is not None and args.duration <= 0:
        parser.error("Duration must be positive")

    # Validate credentials
    if args.password is not None and args.username is None:
        parser.error("--password requires --username")

    # Run publisher
    publish_data(
        device_id=args.device_id,
        rate=args.rate,
        duration=args.duration,
        broker=args.broker,
        port=args.port,
        topic_prefix=args.topic_prefix,
        mode=args.mode,
        username=args.username,
        password=args.password,
        tls=args.tls,
    )


if __name__ == "__main__":
    main()
