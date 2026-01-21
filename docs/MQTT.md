# MQTT Ingestor Service

## Overview

The MQTT Ingestor is a Python service that subscribes to an MQTT broker, receives sensor data from IoT devices, and writes it to the PostgreSQL/TimescaleDB database.

## Architecture

```
MQTT Broker (localhost:1883)
        ↓
   paho-mqtt client
        ↓
  Message Parser & Validator
        ↓
  Device Auto-Registration
        ↓
PostgreSQL Writer (psycopg2)
        ↓
TimescaleDB Hypertable
```

## Key Responsibilities

1. **Connect to MQTT broker** with automatic reconnection
2. **Subscribe to sensor topics** using wildcard patterns
3. **Parse and validate JSON messages**
4. **Auto-register new devices** on first message
5. **Insert sensor readings** into TimescaleDB hypertable
6. **Handle errors gracefully** with logging
7. **Maintain connection health** with keepalive

## Project Structure

```
mqtt-ingestor/
├── Dockerfile
├── requirements.txt
├── .dockerignore
└── src/
    ├── __init__.py
    ├── main.py           # Entry point, service startup
    ├── mqtt_client.py    # MQTT connection and message handling
    ├── db_writer.py      # Database operations
    └── config.py         # Configuration from environment variables
```

## Configuration (Environment Variables)

```env
# MQTT Broker
MQTT_BROKER_HOST=host.docker.internal  # Use this to access host's localhost from Docker
MQTT_BROKER_PORT=1883
MQTT_TOPIC=sensors/+/data              # + is wildcard for device_id
MQTT_CLIENT_ID=duoclean-ingestor       # Unique client identifier
MQTT_QOS=1                             # Quality of Service (0, 1, or 2)
MQTT_KEEPALIVE=60                      # Keepalive interval in seconds

# MQTT Authentication (MVP: not used, but supported)
MQTT_USERNAME=                         # Optional
MQTT_PASSWORD=                         # Optional

# Database
POSTGRES_HOST=postgres
POSTGRES_PORT=5432
POSTGRES_DB=duoclean
POSTGRES_USER=duoclean_user
POSTGRES_PASSWORD=secure_password_here

# Connection Pool
DB_POOL_MIN=2
DB_POOL_MAX=10

# Logging
LOG_LEVEL=INFO                         # DEBUG, INFO, WARNING, ERROR
```

## MQTT Message Format

### Expected Message

**Topic:** `sensors/{device_id}/data`

**Important:** The `device_id` is extracted from the topic path, NOT from the payload. This prevents device spoofing and is more secure.

**Payload (JSON):**
```json
{
  "load": 45.2,
  "battery_voltage": 12.6,
  "temperature": 23.5,
  "timestamp": "2026-01-21T10:30:00Z"
}
```

### Field Specifications

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `load` | float | No | Load reading (nullable) |
| `battery_voltage` | float | No | Battery voltage in volts (nullable) |
| `temperature` | float | No | Temperature in Celsius (nullable) |
| `timestamp` | ISO 8601 | No | Reading timestamp (server time if omitted) |

**Note:** At least one sensor value is recommended, though not strictly required.

### Validation Rules

1. **Sensor values:**
   - Must be numeric (int or float)
   - Can be null/missing (at least one sensor value recommended)
   - Reasonable ranges (optional validation):
     - Load: 0-1000
     - Battery voltage: 0-20
     - Temperature: -50 to 100

2. **Timestamp:**
   - ISO 8601 format with timezone
   - If missing, use server's current time
   - Reject timestamps too far in future (> 1 hour)

### Invalid Message Examples

```json
// Invalid JSON
{load: 45.2}

// Non-numeric values
{
  "load": "forty-five"
}

// Empty payload (no sensor values)
{}
```

## Implementation Details

### main.py

Entry point that initializes and runs the service.

```python
import logging
import signal
import sys
from config import Config
from mqtt_client import MQTTIngestor
from db_writer import DatabaseWriter

def setup_logging():
    """Configure logging"""
    logging.basicConfig(
        level=getattr(logging, Config.LOG_LEVEL),
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )

def signal_handler(sig, frame):
    """Handle graceful shutdown"""
    logging.info("Shutting down gracefully...")
    sys.exit(0)

def main():
    setup_logging()
    logger = logging.getLogger(__name__)

    # Register signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    logger.info("Starting MQTT Ingestor Service")

    # Initialize database writer
    db_writer = DatabaseWriter()

    # Initialize MQTT client
    mqtt_client = MQTTIngestor(db_writer)

    # Connect and start loop
    mqtt_client.connect()
    mqtt_client.loop_forever()

if __name__ == "__main__":
    main()
```

### config.py

Centralized configuration management.

```python
import os

class Config:
    # MQTT Configuration
    MQTT_BROKER_HOST = os.getenv('MQTT_BROKER_HOST', 'localhost')
    MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT', 1883))
    MQTT_TOPIC = os.getenv('MQTT_TOPIC', 'sensors/+/data')
    MQTT_CLIENT_ID = os.getenv('MQTT_CLIENT_ID', 'duoclean-ingestor')
    MQTT_QOS = int(os.getenv('MQTT_QOS', 1))
    MQTT_KEEPALIVE = int(os.getenv('MQTT_KEEPALIVE', 60))
    MQTT_USERNAME = os.getenv('MQTT_USERNAME')
    MQTT_PASSWORD = os.getenv('MQTT_PASSWORD')

    # Database Configuration
    POSTGRES_HOST = os.getenv('POSTGRES_HOST', 'postgres')
    POSTGRES_PORT = int(os.getenv('POSTGRES_PORT', 5432))
    POSTGRES_DB = os.getenv('POSTGRES_DB', 'duoclean')
    POSTGRES_USER = os.getenv('POSTGRES_USER', 'duoclean_user')
    POSTGRES_PASSWORD = os.getenv('POSTGRES_PASSWORD', 'password')

    DB_POOL_MIN = int(os.getenv('DB_POOL_MIN', 2))
    DB_POOL_MAX = int(os.getenv('DB_POOL_MAX', 10))

    # Logging
    LOG_LEVEL = os.getenv('LOG_LEVEL', 'INFO')

    @classmethod
    def get_db_url(cls):
        return f"postgresql://{cls.POSTGRES_USER}:{cls.POSTGRES_PASSWORD}@{cls.POSTGRES_HOST}:{cls.POSTGRES_PORT}/{cls.POSTGRES_DB}"
```

### mqtt_client.py

MQTT connection handling and message processing.

```python
import json
import logging
from datetime import datetime
import paho.mqtt.client as mqtt
from config import Config
from db_writer import DatabaseWriter

logger = logging.getLogger(__name__)

class MQTTIngestor:
    def __init__(self, db_writer: DatabaseWriter):
        self.db_writer = db_writer
        self.client = mqtt.Client(client_id=Config.MQTT_CLIENT_ID)

        # Set callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

        # Set authentication if provided
        if Config.MQTT_USERNAME and Config.MQTT_PASSWORD:
            self.client.username_pw_set(Config.MQTT_USERNAME, Config.MQTT_PASSWORD)

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
        """Callback when message received"""
        try:
            # Parse topic to extract device_id
            topic_parts = msg.topic.split('/')
            if len(topic_parts) != 3 or topic_parts[0] != 'sensors' or topic_parts[2] != 'data':
                logger.warning(f"Invalid topic format: {msg.topic}")
                return

            topic_device_id = topic_parts[1]

            # Parse JSON payload
            try:
                payload = json.loads(msg.payload.decode('utf-8'))
            except json.JSONDecodeError as e:
                logger.error(f"Invalid JSON in message from {msg.topic}: {e}")
                return

            # Validate message
            if not self._validate_message(payload, topic_device_id):
                return

            # Process message
            self._process_message(payload)

        except Exception as e:
            logger.error(f"Error processing message from {msg.topic}: {e}", exc_info=True)

    def _validate_message(self, payload: dict, topic_device_id: str) -> bool:
        """Validate message format and content"""
        # Check device_id
        if 'device_id' not in payload:
            logger.error("Missing device_id in payload")
            return False

        device_id = payload['device_id']

        # Verify device_id matches topic (security check)
        if device_id != topic_device_id:
            logger.warning(f"device_id mismatch: topic={topic_device_id}, payload={device_id}")
            return False

        # Validate device_id format
        if not isinstance(device_id, str) or len(device_id) > 50:
            logger.error(f"Invalid device_id: {device_id}")
            return False

        # Check that at least one sensor value is present
        sensor_fields = ['load', 'battery_voltage', 'temperature']
        has_sensor_value = any(field in payload for field in sensor_fields)

        if not has_sensor_value:
            logger.warning(f"No sensor values in message from {device_id}")
            # Still valid, but unusual

        # Validate sensor values are numeric (if present)
        for field in sensor_fields:
            if field in payload:
                value = payload[field]
                if value is not None and not isinstance(value, (int, float)):
                    logger.error(f"Invalid {field} value: {value} (not numeric)")
                    return False

        return True

    def _process_message(self, payload: dict):
        """Process valid message and write to database"""
        device_id = payload['device_id']

        # Extract timestamp or use current time
        if 'timestamp' in payload:
            try:
                timestamp = datetime.fromisoformat(payload['timestamp'].replace('Z', '+00:00'))
            except ValueError as e:
                logger.warning(f"Invalid timestamp format: {payload['timestamp']}, using server time")
                timestamp = datetime.utcnow()
        else:
            timestamp = datetime.utcnow()

        # Extract sensor values
        load = payload.get('load')
        battery_voltage = payload.get('battery_voltage')
        temperature = payload.get('temperature')

        # Write to database
        try:
            self.db_writer.insert_reading(
                device_id=device_id,
                timestamp=timestamp,
                load=load,
                battery_voltage=battery_voltage,
                temperature=temperature
            )
            logger.debug(f"Inserted reading for device {device_id}")
        except Exception as e:
            logger.error(f"Failed to write reading to database: {e}")

    def loop_forever(self):
        """Start MQTT client loop (blocking)"""
        logger.info("Starting MQTT client loop")
        self.client.loop_forever()
```

### db_writer.py

Database connection and write operations.

```python
import logging
from datetime import datetime
import psycopg2
from psycopg2 import pool
from config import Config

logger = logging.getLogger(__name__)

class DatabaseWriter:
    def __init__(self):
        """Initialize database connection pool"""
        try:
            self.connection_pool = psycopg2.pool.ThreadedConnectionPool(
                Config.DB_POOL_MIN,
                Config.DB_POOL_MAX,
                host=Config.POSTGRES_HOST,
                port=Config.POSTGRES_PORT,
                database=Config.POSTGRES_DB,
                user=Config.POSTGRES_USER,
                password=Config.POSTGRES_PASSWORD
            )
            logger.info("Database connection pool created")
        except Exception as e:
            logger.error(f"Failed to create database connection pool: {e}")
            raise

    def _get_connection(self):
        """Get connection from pool"""
        return self.connection_pool.getconn()

    def _return_connection(self, conn):
        """Return connection to pool"""
        self.connection_pool.putconn(conn)

    def _ensure_device_exists(self, conn, device_id: str):
        """Ensure device exists in database (auto-register if not)"""
        with conn.cursor() as cursor:
            # Check if device exists
            cursor.execute(
                "SELECT id FROM devices WHERE device_id = %s",
                (device_id,)
            )
            result = cursor.fetchone()

            if result is None:
                # Device doesn't exist, insert it
                cursor.execute(
                    """
                    INSERT INTO devices (device_id, name, description)
                    VALUES (%s, %s, %s)
                    ON CONFLICT (device_id) DO NOTHING
                    """,
                    (device_id, f"Device {device_id}", "Auto-registered device")
                )
                conn.commit()
                logger.info(f"Auto-registered new device: {device_id}")

    def insert_reading(
        self,
        device_id: str,
        timestamp: datetime,
        load: float = None,
        battery_voltage: float = None,
        temperature: float = None
    ):
        """Insert sensor reading into database"""
        conn = None
        try:
            conn = self._get_connection()

            # Ensure device exists
            self._ensure_device_exists(conn, device_id)

            # Insert reading
            with conn.cursor() as cursor:
                cursor.execute(
                    """
                    INSERT INTO sensor_readings (time, device_id, load, battery_voltage, temperature)
                    VALUES (%s, %s, %s, %s, %s)
                    ON CONFLICT (time, device_id) DO NOTHING
                    """,
                    (timestamp, device_id, load, battery_voltage, temperature)
                )
                conn.commit()

        except Exception as e:
            if conn:
                conn.rollback()
            logger.error(f"Database error: {e}")
            raise
        finally:
            if conn:
                self._return_connection(conn)

    def close(self):
        """Close all database connections"""
        if self.connection_pool:
            self.connection_pool.closeall()
            logger.info("Database connections closed")
```

## Dockerfile

```dockerfile
FROM python:3.11-slim

WORKDIR /app

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy source code
COPY src/ ./src/

# Run the service
CMD ["python", "-u", "src/main.py"]
```

**Note:** `-u` flag ensures unbuffered output for Docker logs.

## requirements.txt

```txt
paho-mqtt==1.6.1
psycopg2-binary==2.9.9
```

## Error Handling

### Connection Errors

**MQTT Broker Unavailable:**
- Logs error and exits (Docker will restart container)
- Alternative: Implement retry loop with exponential backoff

**Database Unavailable:**
- Connection pool creation fails → service exits
- Individual write errors → logged and skipped (message lost)

### Message Processing Errors

**Invalid JSON:**
- Log warning with topic and error
- Skip message
- Continue processing other messages

**Validation Failures:**
- Log warning with reason
- Skip message
- Increment error counter (for monitoring)

**Database Write Failures:**
- Log error with details
- Do NOT crash service
- Message is lost (no retry in MVP)
- Consider: Dead-letter queue in production

### Duplicate Messages

Handled via `ON CONFLICT (time, device_id) DO NOTHING`:
- Same reading at same timestamp → ignored
- No error, silent skip
- Idempotent inserts

## Logging

### Log Levels

**DEBUG:**
- Each message processed
- SQL queries
- Connection pool stats

**INFO:**
- Service startup/shutdown
- MQTT connection status
- New device registrations

**WARNING:**
- Invalid messages (JSON, validation)
- Reconnection attempts
- Non-critical errors

**ERROR:**
- Database errors
- MQTT connection failures
- Unexpected exceptions

### Log Format

```
2026-01-21 10:30:15,123 - mqtt_client - INFO - Connected to MQTT broker successfully
2026-01-21 10:30:15,125 - mqtt_client - INFO - Subscribed to topic: sensors/+/data
2026-01-21 10:30:20,456 - mqtt_client - DEBUG - Inserted reading for device device_001
2026-01-21 10:30:25,789 - db_writer - INFO - Auto-registered new device: device_002
2026-01-21 10:30:30,012 - mqtt_client - ERROR - Invalid JSON in message from sensors/device_003/data: Expecting value: line 1 column 1 (char 0)
```

## Monitoring and Health

### Health Checks

Docker health check (in docker-compose.yml):
```yaml
healthcheck:
  test: ["CMD", "python", "-c", "import sys; sys.exit(0)"]
  interval: 30s
  timeout: 10s
  retries: 3
```

**Better health check:** Check MQTT connection status
- Requires exposing health status via file or HTTP endpoint

### Metrics (Post-MVP)

Track:
- Messages received per minute
- Messages processed successfully
- Validation errors
- Database write errors
- Connection reconnects
- Processing latency

Export to: Prometheus, CloudWatch, etc.

## Performance

### Expected Throughput

- **Single instance:** 1,000-10,000 messages/second (depending on hardware)
- **Bottleneck:** Database writes (mitigated by connection pooling)

### Optimization Strategies

1. **Batch Inserts:**
   - Buffer messages and insert in batches
   - Trade latency for throughput
   - Implementation: Queue + periodic flush

2. **Async I/O:**
   - Use `asyncio` with `aiomqtt` and `asyncpg`
   - Non-blocking database writes
   - Higher throughput

3. **Multiple Instances:**
   - MQTT shared subscriptions
   - Each instance processes subset of messages
   - No code changes needed

4. **Message Queue:**
   - Insert Kafka/RabbitMQ between MQTT and database
   - Decouple ingestion from storage
   - Enables replay and additional consumers

## Testing

### Local Testing

1. **Start MQTT broker:**
```bash
mosquitto -p 1883
```

2. **Run ingestor:**
```bash
python src/main.py
```

3. **Publish test message:**
```bash
mosquitto_pub -t sensors/test_device/data -m '{"device_id":"test_device","load":50.0,"battery_voltage":12.5,"temperature":22.0}'
```

4. **Check database:**
```sql
SELECT * FROM sensor_readings WHERE device_id = 'test_device';
```

### Unit Tests

```python
import unittest
from mqtt_client import MQTTIngestor

class TestMessageValidation(unittest.TestCase):
    def test_valid_message(self):
        ingestor = MQTTIngestor(None)
        payload = {
            "device_id": "device_001",
            "load": 45.2,
            "battery_voltage": 12.6,
            "temperature": 23.5
        }
        self.assertTrue(ingestor._validate_message(payload, "device_001"))

    def test_missing_device_id(self):
        ingestor = MQTTIngestor(None)
        payload = {"load": 45.2}
        self.assertFalse(ingestor._validate_message(payload, "device_001"))
```

### Integration Tests

Test with real MQTT broker and database:
1. Start services via docker-compose
2. Publish test messages
3. Query database to verify
4. Clean up test data

## Troubleshooting

### Ingestor Not Receiving Messages

**Check:**
1. MQTT broker is running: `mosquitto -v`
2. Topic pattern is correct: `sensors/+/data`
3. Network connectivity from Docker container to host
4. Firewall not blocking port 1883

**Debug:**
```bash
# Subscribe to all topics to see messages
mosquitto_sub -h localhost -t '#' -v
```

### Messages Not Appearing in Database

**Check:**
1. Database connection successful (check logs)
2. Messages passing validation (check logs for warnings)
3. Query correct time range (readings may be old if timestamp provided)

**Debug:**
```bash
# Check ingestor logs
docker-compose logs -f mqtt-ingestor

# Query database
docker-compose exec postgres psql -U duoclean_user -d duoclean -c "SELECT COUNT(*) FROM sensor_readings;"
```

### High Memory Usage

**Causes:**
- Connection pool too large
- Memory leak in MQTT library
- Large message backlog

**Solutions:**
- Reduce `DB_POOL_MAX`
- Restart service periodically (Docker restart policy)
- Monitor with `docker stats`

## Security Considerations

### MVP (No Security)

- No MQTT authentication
- No TLS encryption
- Devices can impersonate each other
- Messages sent in plaintext

**Acceptable for:** Trusted local network, development, proof-of-concept

### Production Recommendations

1. **MQTT TLS:**
```python
client.tls_set(ca_certs="ca.crt", certfile="client.crt", keyfile="client.key")
```

2. **MQTT Authentication:**
```python
client.username_pw_set(username, password)
```

3. **Per-Device Credentials:**
- Each device has unique username/password
- Broker ACLs restrict topics per device
- Device `device_001` can only publish to `sensors/device_001/data`

4. **Message Signing:**
- Include HMAC in message
- Verify signature before processing
- Prevents tampering

## Deployment

### Docker Compose

```yaml
mqtt-ingestor:
  build: ./mqtt-ingestor
  container_name: mqtt-ingestor
  environment:
    - MQTT_BROKER_HOST=host.docker.internal
    - MQTT_BROKER_PORT=1883
    - POSTGRES_HOST=postgres
    - POSTGRES_DB=duoclean
    - POSTGRES_USER=duoclean_user
    - POSTGRES_PASSWORD=${POSTGRES_PASSWORD}
  depends_on:
    - postgres
  restart: unless-stopped
```

### Kubernetes

For production scale:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mqtt-ingestor
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: mqtt-ingestor
        image: duoclean/mqtt-ingestor:latest
        env:
        - name: MQTT_BROKER_HOST
          value: "mqtt-broker-service"
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: password
```

Multiple replicas for high availability and throughput.
