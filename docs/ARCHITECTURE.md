# System Architecture

## Overview

The DuoClean Energy IoT monitoring platform is a microservices-based architecture designed to handle thousands of IoT devices reporting sensor data irregularly over time.

## Architecture Diagram

```
┌─────────────────┐
│  IoT Devices    │
│  (thousands)    │
└────────┬────────┘
         │ MQTT (sensors/{device_id}/data)
         │
         ▼
┌────────────────────┐
│  MQTT Broker       │
│  (Mosquitto)       │
│  localhost:1883    │
└────────┬───────────┘
         │
         │ Subscribe
         ▼
┌────────────────────┐         ┌─────────────────────┐
│  MQTT Ingestor     │────────▶│  PostgreSQL +       │
│  (Python)          │  Write  │  TimescaleDB        │
│                    │         │                     │
└────────────────────┘         └──────────┬──────────┘
                                          │
                                          │ Query
                                          ▼
                               ┌──────────────────────┐
                               │  Web API             │
                               │  (FastAPI)           │
                               │  Port 8000           │
                               └──────────┬───────────┘
                                          │
                                          │ REST API + JWT
                                          ▼
                               ┌──────────────────────┐
                               │  Web Frontend        │
                               │  (Vue.js + Nginx)    │
                               │  Port 3000           │
                               └──────────────────────┘
                                          │
                                          ▼
                               ┌──────────────────────┐
                               │  End Users           │
                               │  (Admin/Users)       │
                               └──────────────────────┘
```

## Components

### 1. IoT Devices

**Characteristics:**
- Thousands of devices in the field
- Send readings when values change (irregular intervals)
- Report load, battery voltage, and temperature
- Each device has a unique ID

**Communication:**
- Protocol: MQTT
- Broker: localhost:1883 (no TLS for MVP)
- Topic pattern: `sensors/{device_id}/data`
- Message format: JSON

**Example message:**
```json
{
  "device_id": "device_001",
  "load": 45.2,
  "battery_voltage": 12.6,
  "temperature": 23.5,
  "timestamp": "2026-01-21T10:30:00Z"
}
```

### 2. MQTT Broker (Mosquitto)

**Role:** Message broker for IoT devices

**Configuration:**
- Host: localhost
- Port: 1883
- Protocol: MQTT v3.1.1
- Authentication: None (MVP)
- TLS: Disabled (MVP)

**Topics:**
- `sensors/+/data` - Wildcard subscription for all devices
- `+` matches any device_id

**Features:**
- Message persistence (optional)
- QoS support (recommend QoS 1 for at-least-once delivery)

### 3. MQTT Ingestor Service

**Technology:** Python 3.11+ with paho-mqtt

**Responsibilities:**
1. Connect to MQTT broker
2. Subscribe to all sensor topics (`sensors/+/data`)
3. Parse and validate JSON messages
4. Auto-register new devices in database
5. Insert sensor readings into TimescaleDB hypertable
6. Handle connection failures and reconnection
7. Logging and error handling

**Key Features:**
- Async message processing
- Connection retry with exponential backoff
- Structured logging
- Health monitoring

**Database Operations:**
- INSERT into `sensor_readings` hypertable
- INSERT into `devices` table (if new device)
- Uses connection pooling

**Error Handling:**
- Invalid JSON: Log and skip message
- Database errors: Retry with backoff
- MQTT disconnection: Auto-reconnect
- Duplicate messages: Ignored (ON CONFLICT DO NOTHING)

**Configuration:**
- Environment variables for MQTT broker connection
- Database connection parameters
- Logging level

### 4. PostgreSQL + TimescaleDB

**Technology:** PostgreSQL 15 + TimescaleDB 2.x extension

**Role:** Primary data store for all sensor readings and application data

**Schema:**

#### Tables

**users**
- id (SERIAL PRIMARY KEY)
- username (UNIQUE)
- email (UNIQUE)
- password_hash (bcrypt)
- role (admin | user)
- created_at, updated_at

**devices**
- id (SERIAL PRIMARY KEY)
- device_id (UNIQUE) - the actual device identifier
- name, description
- created_at, updated_at

**user_device_access**
- user_id, device_id (composite PRIMARY KEY)
- assigned_at
- Defines which users can access which devices

**sensor_readings** (TimescaleDB Hypertable)
- time (TIMESTAMPTZ, partition key)
- device_id (TEXT)
- load (FLOAT)
- battery_voltage (FLOAT)
- temperature (FLOAT)
- PRIMARY KEY (time, device_id)

**TimescaleDB Features:**

1. **Hypertables:** Automatic time-based partitioning
   - Partitioned by time (default: 7 days per chunk)
   - Efficient queries on time ranges
   - Automatic chunk management

2. **Continuous Aggregates:** Pre-computed hourly statistics
   - `sensor_readings_hourly` view
   - AVG, MIN, MAX for all metrics
   - Refreshes every hour
   - Significantly faster queries for historical data

3. **Retention Policy:** Automatic data cleanup
   - Raw data: 1 year retention
   - Aggregates: Indefinite
   - Runs automatically in background

4. **Indexes:**
   - `(device_id, time DESC)` - Fast device-specific queries
   - Automatic time-based indexes from hypertable

**Performance:**
- Handles millions of readings per day
- Sub-second queries for device history
- Efficient aggregations over long time periods

### 5. Web API Service

**Technology:** FastAPI (Python 3.11+)

**Role:** RESTful API backend with authentication and authorization

**Port:** 8000

**Key Features:**
- Automatic API documentation (Swagger UI at `/docs`)
- Pydantic request/response validation
- SQLAlchemy ORM for database access
- JWT-based authentication
- Role-based access control (RBAC)
- CORS middleware for frontend access

**Architecture Pattern:** Layered architecture
```
routers/ (API endpoints)
    ↓
schemas.py (Pydantic models - validation)
    ↓
dependencies.py (auth, permissions)
    ↓
models.py (SQLAlchemy ORM)
    ↓
database.py (connection management)
```

**Authentication Flow:**
1. User POSTs credentials to `/auth/login`
2. API validates password (bcrypt comparison)
3. API generates JWT access token (1 hour expiry)
4. API generates refresh token (7 days expiry)
5. Client stores tokens
6. Subsequent requests include `Authorization: Bearer <token>`
7. Middleware validates JWT and extracts user info
8. Permission checks based on user role and device access

**Authorization Levels:**
- **Public:** Login endpoint
- **Authenticated:** All other endpoints require valid JWT
- **Admin Only:** User management, device management
- **Device Access:** Users can only query devices assigned to them

**API Groups:**
- `/auth/*` - Authentication endpoints
- `/users/*` - User management (admin only)
- `/devices/*` - Device management and queries
- `/readings/*` - Sensor data queries

**Database Connection:**
- Connection pooling via SQLAlchemy
- Dependency injection for session management
- Automatic session cleanup

### 6. Web Frontend

**Technology:** Vue.js 3 (CDN), vanilla JavaScript, Chart.js

**Delivery:** Nginx static file server

**Port:** 3000 (configurable)

**Architecture:** Single Page Application (SPA) concept with multiple HTML pages

**Pages:**
1. **index.html** - Login page
2. **dashboard.html** - Main device dashboard
3. **device-detail.html** - Individual device view with charts
4. **admin.html** - Admin panel (admin users only)

**State Management:**
- localStorage for JWT tokens
- Session storage for current user info
- No complex state management library needed for MVP

**API Communication:**
- Fetch API for HTTP requests
- Centralized API client (`api.js`)
- Automatic JWT injection in headers
- Error handling (401 → redirect to login)
- Token refresh logic

**Visualization:**
- Chart.js for time-series line charts
- Responsive canvas charts
- Multiple datasets per chart (load, voltage, temperature)
- Time range selection

**Auto-refresh:**
- Dashboard polls API every 30 seconds
- Fetches latest readings for all devices
- Updates device cards without full page reload

**Security:**
- XSS protection via CSP headers (Nginx)
- Tokens in localStorage (acceptable for MVP)
- HTTPS in production (recommended)

## Data Flow

### Ingestion Flow (MQTT → Database)

```
1. IoT Device measures values
2. Device publishes JSON to MQTT broker: sensors/{device_id}/data
3. MQTT Ingestor receives message
4. Ingestor parses JSON and validates structure
5. Ingestor checks if device exists in database
   - If not: INSERT INTO devices (device_id)
6. Ingestor INSERTs reading into sensor_readings hypertable
7. TimescaleDB automatically partitions data by time
8. Continuous aggregate updates hourly
```

### Query Flow (User → API → Database)

```
1. User logs in via frontend
2. Frontend stores JWT token
3. User navigates to dashboard
4. Frontend requests GET /dashboard (with JWT)
5. API middleware validates JWT
6. API identifies user and their accessible devices
7. API queries database for latest readings (filtered by device access)
8. API returns JSON response
9. Frontend renders device cards
10. User clicks device
11. Frontend requests GET /devices/{id}/readings?start_time=...
12. API validates user has access to this device
13. API queries sensor_readings hypertable (or hourly aggregate)
14. API returns time-series data
15. Frontend renders Chart.js visualizations
```

### Authentication Flow

```
1. User enters username/password
2. Frontend POSTs to /auth/login
3. API queries users table by username
4. API verifies password_hash with bcrypt
5. If valid:
   - Generate JWT access token (signed with secret)
   - Generate refresh token
   - Return both tokens
6. Frontend stores tokens in localStorage
7. Frontend redirects to dashboard
8. All subsequent API calls include: Authorization: Bearer <access_token>
9. API middleware decodes JWT, extracts user_id and role
10. API uses user context for permission checks
```

## Scalability Considerations

### Current Bottlenecks (MVP)
1. **Single MQTT ingestor instance**
   - Can handle ~1000s of messages/second
   - Good enough for MVP with irregular device updates

2. **Database writes**
   - TimescaleDB can handle 100,000+ inserts/second on modest hardware
   - Not a bottleneck for this use case

3. **API queries**
   - Continuous aggregates enable fast queries
   - Device access filtering adds minimal overhead

### Scaling Strategy (Post-MVP)

**Horizontal Scaling:**
1. **Multiple MQTT ingestors**
   - Use MQTT broker shared subscriptions
   - Each ingestor processes subset of messages
   - Requires no code changes

2. **Multiple API instances**
   - Stateless API design enables easy horizontal scaling
   - Load balancer in front (Nginx, HAProxy, AWS ALB)
   - Database connection pooling per instance

3. **Database replication**
   - Read replicas for query scaling
   - Write to primary, read from replicas
   - TimescaleDB supports streaming replication

**Caching:**
- Redis for latest device readings (hot data)
- Reduces database queries for dashboard
- TTL-based cache invalidation

**Message Queue (if needed):**
- Insert Kafka/RabbitMQ between MQTT ingestor and database
- Decouple ingestion from storage
- Enables data replay and additional consumers

## Network Architecture (Docker)

**Docker Compose Network:**
- All services on custom bridge network: `duoclean-network`
- Service discovery via service names (e.g., `postgres:5432`)
- External access only to web-frontend (port 3000) and web-api (port 8000)

**Service Dependencies:**
```
mqtt-ingestor: depends_on: [postgres]
web-api: depends_on: [postgres]
web-frontend: depends_on: [web-api]
```

**MQTT Broker Access:**
- Broker runs on host (localhost:1883)
- Ingestor uses `host.docker.internal` to reach host's localhost from container
- Alternative: Run Mosquitto in Docker and expose port

**Volumes:**
- `postgres-data`: Persistent database storage
- `./database/init.sql`: Database initialization script

## Security Architecture

### Authentication
- JWT-based token authentication
- Access tokens: Short-lived (1 hour)
- Refresh tokens: Long-lived (7 days)
- Tokens signed with HS256 algorithm

### Authorization
- Role-Based Access Control (RBAC)
  - Admin: Full access
  - User: Limited to assigned devices
- Row-level security via SQL JOINs with user_device_access

### Password Security
- bcrypt hashing with salt (cost factor: 12)
- Passwords never stored in plaintext
- No password recovery in MVP (admin resets)

### API Security
- CORS configured for specific origins
- Input validation via Pydantic schemas
- SQL injection prevention via SQLAlchemy parameterized queries
- No rate limiting in MVP (add in production)

### MQTT Security (MVP: None)
- Post-MVP: TLS encryption
- Post-MVP: Per-device authentication
- Post-MVP: ACLs for topic access

## Monitoring and Observability

### Logging
- Structured logging (JSON format recommended)
- Log levels: DEBUG, INFO, WARNING, ERROR
- Centralized via Docker logs

**Log Sources:**
- MQTT Ingestor: Connection events, message processing, errors
- Web API: Request logs, authentication events, errors
- Database: Query logs (if enabled)

### Health Checks
- Docker Compose health checks for each service
- Database: `pg_isready`
- API: `/health` endpoint (returns 200 OK)
- Ingestor: MQTT connection status

### Metrics (Post-MVP)
- Messages processed per second
- API request latency
- Database query performance
- Active user sessions

## Deployment Architecture

### Development
- Docker Compose on local machine
- Hot-reload for rapid development
- Local MQTT broker

### Production (Recommended)
- Docker Compose or Kubernetes
- Reverse proxy (Nginx/Traefik) with HTTPS/TLS
- Managed PostgreSQL (AWS RDS, GCP Cloud SQL)
- Managed MQTT broker (AWS IoT Core, HiveMQ Cloud) or self-hosted with TLS
- Environment-specific configuration
- Secrets management (Docker secrets, Kubernetes secrets)
- Automated backups
- Monitoring (Prometheus, Grafana)

## Technology Choices - Rationale

### Why TimescaleDB?
- Purpose-built for time-series data
- SQL familiar to developers
- Automatic partitioning and compression
- Continuous aggregates for fast queries
- Mature and production-ready

### Why FastAPI?
- High performance (async)
- Automatic API documentation
- Excellent Python typing support
- Built-in Pydantic validation
- Easy to learn and use

### Why Vue.js (via CDN)?
- Lightweight and fast
- No build step needed for MVP
- Progressive enhancement
- Easy to add components
- Good documentation

### Why MQTT?
- Lightweight protocol ideal for IoT
- Supports thousands of concurrent connections
- QoS levels for reliability
- Industry standard for IoT

### Why Docker Compose?
- Simple multi-container orchestration
- Easy development and deployment
- Service isolation
- Reproducible environments

## Future Architecture Enhancements

1. **Message Queue:** Add Kafka between MQTT and database for better throughput
2. **Caching Layer:** Redis for hot data and session storage
3. **CDN:** Serve static frontend assets from CDN
4. **WebSockets:** Real-time updates to dashboard (instead of polling)
5. **Microservices:** Split API into separate services (auth, devices, readings)
6. **Event Sourcing:** Store all state changes as events for audit trail
7. **GraphQL:** Alternative API for complex queries
8. **Service Mesh:** Istio/Linkerd for service-to-service communication
