# DuoClean Energy - IoT Sensor Monitoring Platform

## Project Status

✅ **Phase 1: Database Setup** - Complete
✅ **Phase 2: MQTT Ingestor** - Complete
⏳ **Phase 3: Web API** - Not started
⏳ **Phase 4: Web Frontend** - Not started

## What's Been Implemented

### Database (PostgreSQL + TimescaleDB)
- Complete schema with users, devices, user_device_access, sensor_readings
- TimescaleDB hypertable for sensor_readings
- Continuous aggregate for hourly statistics
- Data retention policy (1 year)
- Default admin user created
- Full init.sql script: [database/init.sql](database/init.sql)

### MQTT Ingestor Service
- MQTT client with paho-mqtt
- Database writer with connection pooling
- Auto-device registration
- Message validation
- Error handling and logging
- Dockerized service
- Source code: [mqtt-ingestor/src/](mqtt-ingestor/src/)

## Quick Start

### Prerequisites
- Docker and Docker Compose
- MQTT broker running at localhost:1883 (or adjust MQTT_BROKER_HOST in .env)

### Installation

1. **Configure environment:**
   ```bash
   # .env file already created from .env.example
   # Edit if needed:
   nano .env
   ```

2. **Start services:**
   ```bash
   docker-compose up -d
   ```

3. **View logs:**
   ```bash
   docker-compose logs -f
   ```

4. **Check status:**
   ```bash
   docker-compose ps
   ```

### Testing

#### Test Database
```bash
# Access PostgreSQL
docker-compose exec postgres psql -U duoclean_user -d duoclean

# List tables
\dt

# Check admin user
SELECT * FROM users;

# Exit
\q
```

#### Test MQTT Ingestion

1. **Install mosquitto clients** (if not already installed):
   ```bash
   # Ubuntu/Debian
   sudo apt-get install mosquitto-clients

   # macOS
   brew install mosquitto
   ```

2. **Send test message:**
   ```bash
   mosquitto_pub -h localhost -t sensors/test_device_001/data -m '{
     "load": 50.5,
     "battery_voltage": 12.6,
     "temperature": 23.5
   }'
   ```

3. **Verify in database:**
   ```bash
   docker-compose exec postgres psql -U duoclean_user -d duoclean -c "SELECT * FROM sensor_readings WHERE device_id = 'test_device_001';"
   ```

4. **Check device auto-registration:**
   ```bash
   docker-compose exec postgres psql -U duoclean_user -d duoclean -c "SELECT * FROM devices WHERE device_id = 'test_device_001';"
   ```

## Project Structure

```
duocleanenergy/server/
├── docker-compose.yml          # Orchestrates all services
├── .env                        # Environment configuration
├── .env.example                # Template for environment variables
├── README.md                   # This file
│
├── docs/                       # Comprehensive documentation
│   ├── README.md              # Project overview
│   ├── ARCHITECTURE.md        # System architecture
│   ├── DATABASE.md            # Database design
│   ├── API.md                 # API documentation
│   ├── MQTT.md                # MQTT ingestor details
│   ├── FRONTEND.md            # Frontend implementation
│   ├── DEPLOYMENT.md          # Deployment guide
│   ├── IMPLEMENTATION_PHASES.md  # Step-by-step plan
│   ├── SECURITY.md            # Security considerations
│   └── TESTING.md             # Testing strategy
│
├── database/
│   └── init.sql               # Database initialization script
│
└── mqtt-ingestor/
    ├── Dockerfile
    ├── requirements.txt
    ├── .dockerignore
    └── src/
        ├── __init__.py
        ├── main.py            # Entry point
        ├── config.py          # Configuration
        ├── mqtt_client.py     # MQTT client
        └── db_writer.py       # Database writer
```

## Default Credentials

**Admin User:**
- Username: `admin`
- Password: `admin123`
- **IMPORTANT:** Change this password immediately after deployment!

## MQTT Message Format

Devices should publish JSON messages to topic: `sensors/{device_id}/data`

**Important:** The device_id is derived from the topic path, NOT from the message payload. This is more secure and prevents spoofing.

Example:
```bash
# Topic: sensors/device_001/data
# Payload:
{
  "load": 45.2,
  "battery_voltage": 12.6,
  "temperature": 23.5,
  "timestamp": "2026-01-21T10:30:00Z"
}
```

Fields:
- `load` (optional): Load reading
- `battery_voltage` (optional): Battery voltage in volts
- `temperature` (optional): Temperature in Celsius
- `timestamp` (optional): ISO 8601 timestamp (server time if omitted)

**Note:** At least one sensor value should be present in the message.

## Management Commands

```bash
# Start services
docker-compose up -d

# Stop services
docker-compose down

# View logs
docker-compose logs -f
docker-compose logs -f mqtt-ingestor  # Specific service

# Restart a service
docker-compose restart mqtt-ingestor

# Rebuild after code changes
docker-compose up -d --build

# Check resource usage
docker stats

# Clean up everything (WARNING: destroys data)
docker-compose down -v
```

## Next Steps

1. **Implement Web API (FastAPI)** - Phase 3
   - Authentication with JWT
   - User management endpoints
   - Device management endpoints
   - Sensor data query endpoints
   - See [docs/IMPLEMENTATION_PHASES.md](docs/IMPLEMENTATION_PHASES.md)

2. **Implement Web Frontend (Vue.js)** - Phase 4
   - Login page
   - Dashboard
   - Device detail views
   - Admin panel

## Documentation

Comprehensive documentation is available in the `/docs` directory:

- **[docs/README.md](docs/README.md)** - Project overview and quick start
- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System architecture and design
- **[docs/DATABASE.md](docs/DATABASE.md)** - Database schema and queries
- **[docs/MQTT.md](docs/MQTT.md)** - MQTT ingestor implementation details
- **[docs/IMPLEMENTATION_PHASES.md](docs/IMPLEMENTATION_PHASES.md)** - Detailed implementation plan
- **[docs/DEPLOYMENT.md](docs/DEPLOYMENT.md)** - Deployment and operations guide
- **[docs/SECURITY.md](docs/SECURITY.md)** - Security best practices
- **[docs/TESTING.md](docs/TESTING.md)** - Testing strategy

## Troubleshooting

### Services won't start
```bash
# Check logs
docker-compose logs

# Check if ports are available
sudo lsof -i :5432
sudo lsof -i :1883
```

### MQTT ingestor can't connect to broker
```bash
# Verify MQTT broker is running
sudo systemctl status mosquitto

# Test MQTT connection
mosquitto_sub -h localhost -t '#' -v

# Check MQTT_BROKER_HOST in .env
# Should be "host.docker.internal" to access host's localhost from Docker
```

### Database connection failed
```bash
# Check if postgres is healthy
docker-compose ps postgres

# Verify credentials
docker-compose exec postgres psql -U duoclean_user -d duoclean
```

### Messages not appearing in database
```bash
# Check ingestor logs
docker-compose logs -f mqtt-ingestor

# Verify message format
# Ensure device_id in payload matches device_id in topic
```

## Support

For detailed information, refer to:
- Implementation guide: [docs/IMPLEMENTATION_PHASES.md](docs/IMPLEMENTATION_PHASES.md)
- Troubleshooting: [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md#troubleshooting)
- Architecture details: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

## License

[Your License Here]
