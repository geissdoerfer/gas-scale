"""
MQTT client for receiving and processing sensor messages.
"""
import json
import logging
from datetime import datetime
import paho.mqtt.client as mqtt
from config import Config
from db_writer import DatabaseWriter

logger = logging.getLogger(__name__)


class MQTTIngestor:
    def __init__(self, db_writer: DatabaseWriter):
        """
        Initialize MQTT client.

        Args:
            db_writer: DatabaseWriter instance
        """
        self.db_writer = db_writer
        self.client = mqtt.Client(client_id=Config.MQTT_CLIENT_ID)

        # Set callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

        # Set authentication if provided
        if Config.MQTT_USERNAME and Config.MQTT_PASSWORD:
            self.client.username_pw_set(Config.MQTT_USERNAME, Config.MQTT_PASSWORD)
            logger.info("MQTT authentication configured")

    def connect(self):
        """Connect to MQTT broker"""
        try:
            logger.info(f"Connecting to MQTT broker at {Config.MQTT_BROKER_HOST}:{Config.MQTT_BROKER_PORT}")
            self.client.connect(
                Config.MQTT_BROKER_HOST,
                Config.MQTT_BROKER_PORT,
                Config.MQTT_KEEPALIVE
            )
        except Exception as e:
            logger.error(f"Failed to connect to MQTT broker: {e}")
            raise

    def _on_connect(self, client, userdata, flags, rc):
        """Callback when connected to MQTT broker"""
        if rc == 0:
            logger.info("Connected to MQTT broker successfully")
            # Subscribe to topic
            client.subscribe(Config.MQTT_TOPIC, qos=Config.MQTT_QOS)
            logger.info(f"Subscribed to topic: {Config.MQTT_TOPIC}")
        else:
            logger.error(f"Failed to connect to MQTT broker. Return code: {rc}")

    def _on_disconnect(self, client, userdata, rc):
        """Callback when disconnected from MQTT broker"""
        if rc != 0:
            logger.warning(f"Unexpected disconnect from MQTT broker. Return code: {rc}")
            logger.info("Attempting to reconnect...")

    def _on_message(self, client, userdata, msg):
        """
        Callback when message received.

        Args:
            client: MQTT client instance
            userdata: User data
            msg: Message object
        """
        try:
            # Parse topic to extract device_id
            topic_parts = msg.topic.split('/')
            if len(topic_parts) != 3 or topic_parts[0] != 'sensors' or topic_parts[2] != 'data':
                logger.warning(f"Invalid topic format: {msg.topic}")
                return

            device_id = topic_parts[1]

            # Parse JSON payload
            try:
                payload = json.loads(msg.payload.decode('utf-8'))
            except json.JSONDecodeError as e:
                logger.error(f"Invalid JSON in message from {msg.topic}: {e}")
                return

            # Validate message
            if not self._validate_message(payload):
                return

            # Process message with device_id from topic
            self._process_message(payload, device_id)

        except Exception as e:
            logger.error(f"Error processing message from {msg.topic}: {e}", exc_info=True)

    def _validate_message(self, payload: dict) -> bool:
        """
        Validate message format and content.

        Args:
            payload: Message payload dictionary

        Returns:
            True if valid, False otherwise
        """
        # Check that at least one sensor value is present
        sensor_fields = ['value', 'battery_voltage', 'temperature']
        has_sensor_value = any(field in payload for field in sensor_fields)

        if not has_sensor_value:
            logger.warning("No sensor values in message")
            # Still valid, but unusual

        # Validate sensor values are numeric (if present)
        for field in sensor_fields:
            if field in payload:
                value = payload[field]
                if value is not None and not isinstance(value, (int, float)):
                    logger.error(f"Invalid {field} value: {value} (not numeric)")
                    return False

        return True

    def _process_message(self, payload: dict, device_id: str):
        """
        Process valid message and write to database.

        Args:
            payload: Message payload dictionary
            device_id: Device identifier from topic
        """
        # Extract timestamp or use current time
        if 'timestamp' in payload:
            try:
                timestamp = datetime.fromisoformat(payload['timestamp'].replace('Z', '+00:00'))
            except (ValueError, AttributeError) as e:
                logger.warning(f"Invalid timestamp format: {payload.get('timestamp')}, using server time")
                timestamp = datetime.utcnow()
        else:
            timestamp = datetime.utcnow()

        # Extract sensor values (firmware sends 'value' for raw HX711 reading)
        raw_value = payload.get('value')
        battery_voltage = payload.get('battery_voltage')
        temperature = payload.get('temperature')

        # Write to database
        try:
            self.db_writer.insert_reading(
                device_id=device_id,
                timestamp=timestamp,
                raw_value=raw_value,
                battery_voltage=battery_voltage,
                temperature=temperature
            )
            logger.debug(f"Successfully processed reading for device {device_id}")
        except Exception as e:
            logger.error(f"Failed to write reading to database: {e}")

    def loop_forever(self):
        """Start MQTT client loop (blocking)"""
        logger.info("Starting MQTT client loop")
        self.client.loop_forever()
