#!/usr/bin/env python3
"""
MQTT Test Data Publisher

Publishes random sensor data to MQTT broker for testing purposes.
Simulates IoT device sending weight (in grams), battery voltage, and temperature readings.

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

    def __init__(self, base_weight=1500.0, base_voltage=12.6, base_temp=23.0):
        self.base_weight = base_weight
        self.base_voltage = base_voltage
        self.base_temp = base_temp
        self.weight_trend = 0.0
        self.voltage_trend = 0.0
        self.temp_trend = 0.0

    def generate(self):
        """Generate a reading with realistic variations"""
        # Add random walk to trends
        self.weight_trend += random.uniform(-10.0, 10.0)
        self.weight_trend = max(-200.0, min(200.0, self.weight_trend))  # Limit trend

        self.voltage_trend += random.uniform(-0.02, 0.02)
        self.voltage_trend = max(-0.5, min(0.5, self.voltage_trend))

        self.temp_trend += random.uniform(-0.1, 0.1)
        self.temp_trend = max(-2.0, min(2.0, self.temp_trend))

        # Generate values with noise
        weight = self.base_weight + self.weight_trend + random.uniform(-50.0, 50.0)
        voltage = self.base_voltage + self.voltage_trend + random.uniform(-0.1, 0.1)
        temp = self.base_temp + self.temp_trend + random.uniform(-0.5, 0.5)

        # Ensure realistic bounds
        weight = max(0.0, min(5000.0, weight))  # 0 to 5kg in grams
        voltage = max(10.0, min(14.0, voltage))
        temp = max(-10.0, min(50.0, temp))

        return {
            "weight": round(weight, 2),
            "battery_voltage": round(voltage, 2),
            "temperature": round(temp, 2),
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


def publish_data(device_id, rate, duration, broker, port, topic_prefix, mode):
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
    """
    # Create client with callback data
    userdata = {"publish_count": 0}
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=f"test_publisher_{device_id}",
        userdata=userdata,
    )

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
            base_weight=random.uniform(500, 3000),
            base_voltage=random.uniform(11.5, 13.0),
            base_temp=random.uniform(15, 30),
        )
    else:  # stable
        generator = SensorDataGenerator()
        # Override generate to return stable values
        generator.generate = lambda: {
            "weight": 1500.0,
            "battery_voltage": 12.6,
            "temperature": 23.0,
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
                    f"[{message_count:4d}] Published: Weight={data['weight']:7.2f}g  "
                    f"Voltage={data['battery_voltage']:5.2f}V  "
                    f"Temp={data['temperature']:5.2f}°C"
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

    # Run publisher
    publish_data(
        device_id=args.device_id,
        rate=args.rate,
        duration=args.duration,
        broker=args.broker,
        port=args.port,
        topic_prefix=args.topic_prefix,
        mode=args.mode,
    )


if __name__ == "__main__":
    main()
