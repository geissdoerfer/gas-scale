# DuoClean Energy - IoT Sensor Monitoring Platform

## Overview

A scalable server application for monitoring IoT sensor devices that report load, battery voltage, and temperature readings. The system consists of three main components:

1. **MQTT Ingestor** - Subscribes to MQTT broker and stores sensor data
2. **REST API** - FastAPI backend with authentication and authorization
3. **Web Application** - Responsive Vue.js frontend for data visualization

## Key Features

- **Time-series data storage** with PostgreSQL + TimescaleDB
- **Real-time MQTT ingestion** from thousands of IoT devices
- **Role-based access control** (Admin and User roles)
- **Device assignment system** - Users only see their assigned devices
- **Historical data visualization** with interactive charts
- **Admin panel** for user and device management
- **Dockerized deployment** with Docker Compose

## Technology Stack

### Backend
- Python 3.11+
- FastAPI (REST API)
- PostgreSQL 15 + TimescaleDB 2.x
- SQLAlchemy (ORM)
- paho-mqtt (MQTT client)
- JWT authentication (python-jose)
- bcrypt password hashing (passlib)

### Frontend
- Vue.js 3 (via CDN)
- Chart.js (data visualization)
- Vanilla JavaScript
- Responsive CSS

### Infrastructure
- Docker & Docker Compose
- Nginx (static file serving)
- Local MQTT broker (Mosquitto at localhost:1883)

## Quick Start

### Prerequisites

- Docker and Docker Compose installed
- MQTT broker running at localhost:1883
- Git

### Installation

1. Clone the repository:
```bash
git clone <repository-url>
cd duocleanenergy/server
```

2. Create environment file:
```bash
cp .env.example .env
# Edit .env with your configuration
```

3. Start all services:
```bash
docker-compose up -d
```

4. Access the application:
- Web UI: http://localhost:3000
- API Documentation: http://localhost:8000/docs
- Default admin credentials: `admin` / `admin123` (CHANGE IMMEDIATELY!)

### Sending Test Data

Send a test MQTT message:
```bash
mosquitto_pub -h localhost -t sensors/device_001/data -m '{"device_id":"device_001","load":45.2,"battery_voltage":12.6,"temperature":23.5}'
```

## Project Structure

```
duocleanenergy/server/
├── docker-compose.yml
├── .env.example
├── .env
├── README.md
├── docs/                    # Comprehensive documentation
├── database/                # Database initialization
│   └── init.sql
├── mqtt-ingestor/          # MQTT ingestion service
│   ├── Dockerfile
│   ├── requirements.txt
│   └── src/
├── web-api/                # FastAPI backend
│   ├── Dockerfile
│   ├── requirements.txt
│   └── src/
└── web-frontend/           # Vue.js frontend
    ├── Dockerfile
    ├── nginx.conf
    └── public/
```

## Documentation

Detailed documentation is available in the `/docs` directory:

- [ARCHITECTURE.md](./ARCHITECTURE.md) - System architecture and component overview
- [DATABASE.md](./DATABASE.md) - Database schema and TimescaleDB setup
- [API.md](./API.md) - Complete API endpoint documentation
- [MQTT.md](./MQTT.md) - MQTT ingestor implementation details
- [FRONTEND.md](./FRONTEND.md) - Frontend architecture and implementation
- [DEPLOYMENT.md](./DEPLOYMENT.md) - Docker Compose setup and deployment guide
- [IMPLEMENTATION_PHASES.md](./IMPLEMENTATION_PHASES.md) - Step-by-step implementation plan
- [SECURITY.md](./SECURITY.md) - Security considerations and best practices
- [TESTING.md](./TESTING.md) - Testing strategy and examples

## Default Credentials

**Admin Account** (created during database initialization):
- Username: `admin`
- Password: `admin123`
- Email: `admin@duocleanenergy.com`

**IMPORTANT:** Change the default admin password immediately after first login!

## MQTT Message Format

IoT devices should publish JSON messages to topic `sensors/{device_id}/data`:

```json
{
  "device_id": "device_001",
  "load": 45.2,
  "battery_voltage": 12.6,
  "temperature": 23.5,
  "timestamp": "2026-01-21T10:30:00Z"
}
```

- `device_id`: Unique device identifier (required)
- `load`: Load reading in appropriate units (optional)
- `battery_voltage`: Battery voltage in volts (optional)
- `temperature`: Temperature in Celsius (optional)
- `timestamp`: ISO 8601 timestamp (optional, server time used if not provided)

## User Roles

### Admin
- Access all devices and data
- Create, edit, and delete users
- Add, edit, and delete devices
- Assign/unassign devices to users
- Full system access

### User
- Access only assigned devices
- View device data and historical trends
- View charts and analytics
- No administrative capabilities

## Key Workflows

### Admin: Add New User and Assign Devices
1. Login as admin
2. Navigate to Admin Panel
3. Create new user with username, email, password, and role
4. Go to Assignment tab
5. Select user and assign devices

### User: View Device Data
1. Login with user credentials
2. Dashboard shows all assigned devices with latest readings
3. Click device to view historical charts
4. Select time range (1h, 6h, 24h, 7d, 30d)

### Device Auto-Registration
- When a new device sends its first MQTT message, it's automatically added to the database
- Admin can then assign the device to users

## Data Retention

- Raw sensor readings: **1 year** (configurable in init.sql)
- Hourly aggregates: Indefinite (much smaller storage footprint)
- Continuous aggregates refresh every hour

## Performance Considerations

- TimescaleDB hypertables optimize time-series queries
- Continuous aggregates pre-compute hourly statistics
- Indexes on device_id and time for fast lookups
- Dashboard auto-refreshes every 30 seconds (configurable)

## Monitoring

Check service health:
```bash
docker-compose ps
docker-compose logs -f mqtt-ingestor
docker-compose logs -f web-api
```

## Troubleshooting

### MQTT Ingestor Can't Connect
- Verify MQTT broker is running at localhost:1883
- Check `MQTT_BROKER_HOST` in .env (should be `host.docker.internal` to access host from container)
- Check firewall settings

### API Returns 401 Unauthorized
- Token may have expired (access tokens expire after 1 hour)
- Use refresh token to get new access token
- Re-login if refresh token also expired

### Database Connection Errors
- Ensure postgres service is healthy: `docker-compose ps postgres`
- Check database credentials in .env
- Verify init.sql ran successfully: `docker-compose logs postgres`

### Frontend Can't Reach API
- Check `VUE_APP_API_URL` in .env
- Verify web-api service is running
- Check CORS settings in web-api configuration

## Contributing

This is an MVP. Future enhancements:
- PWA features (offline support, installable)
- Native mobile apps (Flutter/React Native)
- Alert system with email/SMS notifications
- MQTT TLS and device authentication
- Advanced analytics and anomaly detection
- Data export (CSV/Excel)
- HTTPS/SSL for production

## License

[Your License Here]

## Support

For issues or questions, please contact [Your Contact Info]
