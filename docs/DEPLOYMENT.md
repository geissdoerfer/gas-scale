# Deployment Guide

## Overview

The DuoClean Energy platform is deployed using Docker Compose, which orchestrates all microservices in containers. This guide covers local development deployment and production considerations.

## Prerequisites

### Required Software

- **Docker** (version 20.10+)
- **Docker Compose** (version 2.0+)
- **Git**
- **MQTT Broker** (Mosquitto) running at localhost:1883

### System Requirements

**Minimum:**
- 2 CPU cores
- 4 GB RAM
- 20 GB disk space

**Recommended:**
- 4 CPU cores
- 8 GB RAM
- 50 GB disk space (for data retention)

## Project Structure

```
duocleanenergy/server/
├── docker-compose.yml          # Main orchestration file
├── .env                        # Environment variables (do not commit!)
├── .env.example                # Template for environment variables
├── .dockerignore               # Files to exclude from Docker builds
├── README.md
│
├── database/
│   └── init.sql                # Database initialization script
│
├── mqtt-ingestor/
│   ├── Dockerfile
│   ├── requirements.txt
│   └── src/
│
├── web-api/
│   ├── Dockerfile
│   ├── requirements.txt
│   └── src/
│
└── web-frontend/
    ├── Dockerfile
    ├── nginx.conf
    └── public/
```

## Environment Configuration

### Create .env File

```bash
cp .env.example .env
# Edit .env with your configuration
nano .env
```

### Environment Variables

```.env
# ===== PostgreSQL Database =====
POSTGRES_HOST=postgres
POSTGRES_PORT=5432
POSTGRES_DB=duoclean
POSTGRES_USER=duoclean_user
POSTGRES_PASSWORD=your_secure_password_here_change_this

# ===== MQTT Broker =====
MQTT_BROKER_HOST=host.docker.internal  # Access host's localhost from Docker
MQTT_BROKER_PORT=1883
MQTT_TOPIC=sensors/+/data
MQTT_CLIENT_ID=duoclean-ingestor
MQTT_QOS=1
MQTT_KEEPALIVE=60

# Optional MQTT authentication (MVP: not used)
MQTT_USERNAME=
MQTT_PASSWORD=

# ===== Web API =====
JWT_SECRET=your_jwt_secret_key_change_this_to_random_string
JWT_ALGORITHM=HS256
JWT_ACCESS_TOKEN_EXPIRE_MINUTES=60
JWT_REFRESH_TOKEN_EXPIRE_DAYS=7

# CORS origins (comma-separated)
API_CORS_ORIGINS=http://localhost:3000,http://localhost

# ===== Database Connection Pool =====
DB_POOL_MIN=2
DB_POOL_MAX=10

# ===== Logging =====
LOG_LEVEL=INFO

# ===== Frontend =====
VUE_APP_API_URL=http://localhost:8000
```

### Security Notes

**IMPORTANT:** Change these before deploying:
- `POSTGRES_PASSWORD` - Use a strong password
- `JWT_SECRET` - Generate a random string (e.g., `openssl rand -hex 32`)

**Generate strong secrets:**
```bash
# Generate JWT secret
openssl rand -hex 32

# Generate PostgreSQL password
openssl rand -base64 32
```

## Docker Compose Configuration

### docker-compose.yml

```yaml
version: '3.8'

services:
  # PostgreSQL + TimescaleDB
  postgres:
    image: timescale/timescaledb:latest-pg15
    container_name: duoclean-postgres
    environment:
      POSTGRES_DB: ${POSTGRES_DB}
      POSTGRES_USER: ${POSTGRES_USER}
      POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
    ports:
      - "5432:5432"  # Expose for debugging (remove in production)
    volumes:
      - postgres-data:/var/lib/postgresql/data
      - ./database/init.sql:/docker-entrypoint-initdb.d/init.sql
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U ${POSTGRES_USER} -d ${POSTGRES_DB}"]
      interval: 10s
      timeout: 5s
      retries: 5
    restart: unless-stopped
    networks:
      - duoclean-network

  # MQTT Ingestor
  mqtt-ingestor:
    build:
      context: ./mqtt-ingestor
      dockerfile: Dockerfile
    container_name: duoclean-mqtt-ingestor
    environment:
      MQTT_BROKER_HOST: ${MQTT_BROKER_HOST}
      MQTT_BROKER_PORT: ${MQTT_BROKER_PORT}
      MQTT_TOPIC: ${MQTT_TOPIC}
      MQTT_CLIENT_ID: ${MQTT_CLIENT_ID}
      MQTT_QOS: ${MQTT_QOS}
      MQTT_KEEPALIVE: ${MQTT_KEEPALIVE}
      MQTT_USERNAME: ${MQTT_USERNAME}
      MQTT_PASSWORD: ${MQTT_PASSWORD}
      POSTGRES_HOST: ${POSTGRES_HOST}
      POSTGRES_PORT: ${POSTGRES_PORT}
      POSTGRES_DB: ${POSTGRES_DB}
      POSTGRES_USER: ${POSTGRES_USER}
      POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
      DB_POOL_MIN: ${DB_POOL_MIN}
      DB_POOL_MAX: ${DB_POOL_MAX}
      LOG_LEVEL: ${LOG_LEVEL}
    depends_on:
      postgres:
        condition: service_healthy
    restart: unless-stopped
    networks:
      - duoclean-network

  # Web API (FastAPI)
  web-api:
    build:
      context: ./web-api
      dockerfile: Dockerfile
    container_name: duoclean-web-api
    environment:
      POSTGRES_HOST: ${POSTGRES_HOST}
      POSTGRES_PORT: ${POSTGRES_PORT}
      POSTGRES_DB: ${POSTGRES_DB}
      POSTGRES_USER: ${POSTGRES_USER}
      POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
      JWT_SECRET: ${JWT_SECRET}
      JWT_ALGORITHM: ${JWT_ALGORITHM}
      JWT_ACCESS_TOKEN_EXPIRE_MINUTES: ${JWT_ACCESS_TOKEN_EXPIRE_MINUTES}
      JWT_REFRESH_TOKEN_EXPIRE_DAYS: ${JWT_REFRESH_TOKEN_EXPIRE_DAYS}
      API_CORS_ORIGINS: ${API_CORS_ORIGINS}
      LOG_LEVEL: ${LOG_LEVEL}
    ports:
      - "8000:8000"
    depends_on:
      postgres:
        condition: service_healthy
    restart: unless-stopped
    networks:
      - duoclean-network

  # Web Frontend (Nginx)
  web-frontend:
    build:
      context: ./web-frontend
      dockerfile: Dockerfile
    container_name: duoclean-web-frontend
    environment:
      VUE_APP_API_URL: ${VUE_APP_API_URL}
    ports:
      - "3000:80"
    depends_on:
      - web-api
    restart: unless-stopped
    networks:
      - duoclean-network

networks:
  duoclean-network:
    driver: bridge

volumes:
  postgres-data:
    driver: local
```

## Deployment Steps

### 1. Initial Setup

```bash
# Clone repository
git clone <repository-url>
cd duocleanenergy/server

# Create environment file
cp .env.example .env

# Edit environment variables
nano .env

# Generate secrets
echo "JWT_SECRET=$(openssl rand -hex 32)" >> .env
echo "POSTGRES_PASSWORD=$(openssl rand -base64 32)" >> .env
```

### 2. Start MQTT Broker (if not running)

```bash
# Install Mosquitto (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients

# Start Mosquitto
sudo systemctl start mosquitto
sudo systemctl enable mosquitto

# Verify it's running
mosquitto_sub -t '#' -v
```

### 3. Build and Start Services

```bash
# Build all images
docker-compose build

# Start all services
docker-compose up -d

# View logs
docker-compose logs -f

# Check status
docker-compose ps
```

### 4. Verify Deployment

```bash
# Check if all services are running
docker-compose ps

# Expected output:
# NAME                      STATUS
# duoclean-postgres         Up (healthy)
# duoclean-mqtt-ingestor    Up
# duoclean-web-api          Up
# duoclean-web-frontend     Up

# Check database initialization
docker-compose exec postgres psql -U duoclean_user -d duoclean -c "\dt"

# Test API
curl http://localhost:8000/docs

# Test frontend
curl http://localhost:3000
```

### 5. Test MQTT Ingestion

```bash
# Send test message
mosquitto_pub -h localhost -t sensors/test_device/data -m '{
  "device_id": "test_device",
  "load": 50.0,
  "battery_voltage": 12.5,
  "temperature": 22.0
}'

# Check database
docker-compose exec postgres psql -U duoclean_user -d duoclean -c "SELECT * FROM sensor_readings WHERE device_id = 'test_device';"

# Check if device was auto-registered
docker-compose exec postgres psql -U duoclean_user -d duoclean -c "SELECT * FROM devices WHERE device_id = 'test_device';"
```

### 6. Access the Application

- **Web UI:** http://localhost:3000
- **API Docs:** http://localhost:8000/docs
- **Default credentials:**
  - Username: `admin`
  - Password: `admin123` (CHANGE IMMEDIATELY!)

## Management Commands

### Service Control

```bash
# Start all services
docker-compose up -d

# Stop all services
docker-compose down

# Restart a specific service
docker-compose restart web-api

# View logs
docker-compose logs -f
docker-compose logs -f mqtt-ingestor

# Check service status
docker-compose ps

# View resource usage
docker stats
```

### Database Management

```bash
# Access PostgreSQL shell
docker-compose exec postgres psql -U duoclean_user -d duoclean

# Backup database
docker-compose exec postgres pg_dump -U duoclean_user duoclean > backup_$(date +%Y%m%d).sql

# Restore database
cat backup_20260121.sql | docker-compose exec -T postgres psql -U duoclean_user duoclean

# View database size
docker-compose exec postgres psql -U duoclean_user -d duoclean -c "SELECT pg_size_pretty(pg_database_size('duoclean'));"
```

### Logs and Debugging

```bash
# View all logs
docker-compose logs

# Follow logs in real-time
docker-compose logs -f

# View logs for specific service
docker-compose logs web-api

# View last 100 lines
docker-compose logs --tail=100

# Check service health
docker-compose exec postgres pg_isready -U duoclean_user

# Enter container shell
docker-compose exec web-api bash
docker-compose exec postgres bash
```

### Data Management

```bash
# Remove all data (WARNING: destroys all data)
docker-compose down -v

# Remove only containers (keeps data)
docker-compose down

# Rebuild after code changes
docker-compose up -d --build

# Rebuild specific service
docker-compose up -d --build web-api
```

## Updating the Application

### Update Code

```bash
# Pull latest changes
git pull origin main

# Rebuild and restart services
docker-compose up -d --build

# Check logs for errors
docker-compose logs -f
```

### Database Migrations

For schema changes:

```bash
# Create new migration SQL file
nano database/migrations/002_add_new_column.sql

# Apply migration manually
docker-compose exec postgres psql -U duoclean_user -d duoclean -f /path/to/migration.sql

# Or use a migration tool (Alembic, Flyway, etc.)
```

## Monitoring

### Health Checks

```bash
# Check if services are healthy
docker-compose ps

# Check API health endpoint
curl http://localhost:8000/health

# Check database connection
docker-compose exec postgres pg_isready -U duoclean_user
```

### Resource Usage

```bash
# View resource usage
docker stats

# View disk usage
docker system df

# Clean up unused resources
docker system prune -a
```

### Application Logs

```bash
# API logs
docker-compose logs -f web-api

# Ingestor logs
docker-compose logs -f mqtt-ingestor

# Database logs
docker-compose logs -f postgres

# All logs
docker-compose logs -f
```

## Production Deployment

### Additional Considerations

1. **HTTPS/TLS:**
   - Use a reverse proxy (Nginx, Traefik) with Let's Encrypt
   - Terminate SSL at the proxy
   - Update CORS origins

2. **Database:**
   - Use managed PostgreSQL (AWS RDS, GCP Cloud SQL)
   - Enable automated backups
   - Set up replication
   - Don't expose port 5432 externally

3. **Secrets Management:**
   - Use Docker secrets or Kubernetes secrets
   - Don't commit .env file
   - Rotate secrets regularly

4. **Monitoring:**
   - Set up Prometheus + Grafana
   - Configure alerts (PagerDuty, Slack)
   - Log aggregation (ELK stack, CloudWatch)

5. **Scaling:**
   - Multiple API instances behind load balancer
   - Multiple ingestor instances (MQTT shared subscriptions)
   - Database read replicas
   - Consider Kubernetes for orchestration

6. **Security:**
   - Enable MQTT TLS and authentication
   - Set up firewall rules
   - Use private networks for inter-service communication
   - Regular security updates

### Production docker-compose.yml

```yaml
version: '3.8'

services:
  web-api:
    image: duoclean/web-api:latest
    deploy:
      replicas: 3
      resources:
        limits:
          cpus: '1'
          memory: 1G
    environment:
      - POSTGRES_HOST=your-rds-endpoint.amazonaws.com
      - JWT_SECRET=${JWT_SECRET}
    ports:
      - "8000"  # Don't expose directly, use load balancer
    networks:
      - private-network

  mqtt-ingestor:
    image: duoclean/mqtt-ingestor:latest
    deploy:
      replicas: 2
    environment:
      - MQTT_BROKER_HOST=mqtt-broker.example.com
      - MQTT_USERNAME=${MQTT_USERNAME}
      - MQTT_PASSWORD=${MQTT_PASSWORD}
    networks:
      - private-network

  web-frontend:
    image: duoclean/web-frontend:latest
    networks:
      - private-network

  nginx-proxy:
    image: nginx:alpine
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx-prod.conf:/etc/nginx/nginx.conf
      - /etc/letsencrypt:/etc/letsencrypt
    networks:
      - private-network

networks:
  private-network:
    driver: overlay
```

## Troubleshooting

### Common Issues

**Issue: Services won't start**
```bash
# Check logs
docker-compose logs

# Check if ports are available
sudo lsof -i :5432
sudo lsof -i :8000
sudo lsof -i :3000

# Rebuild images
docker-compose build --no-cache
docker-compose up -d
```

**Issue: Database connection failed**
```bash
# Check if postgres is healthy
docker-compose ps postgres

# Check database logs
docker-compose logs postgres

# Verify credentials
docker-compose exec postgres psql -U duoclean_user -d duoclean

# Wait for database to be ready
docker-compose exec postgres pg_isready -U duoclean_user
```

**Issue: MQTT ingestor can't connect**
```bash
# Check if MQTT broker is running on host
sudo systemctl status mosquitto

# Test MQTT connection from host
mosquitto_sub -h localhost -t '#' -v

# Check ingestor logs
docker-compose logs mqtt-ingestor

# Verify MQTT_BROKER_HOST is set correctly
docker-compose exec mqtt-ingestor env | grep MQTT
```

**Issue: Frontend can't reach API**
```bash
# Check if API is accessible
curl http://localhost:8000/docs

# Check CORS configuration
docker-compose logs web-api | grep CORS

# Verify API_CORS_ORIGINS includes frontend URL
```

**Issue: Out of disk space**
```bash
# Check disk usage
docker system df

# Clean up
docker system prune -a
docker volume prune

# Remove old logs
docker-compose logs --tail=0 > /dev/null
```

### Reset Everything

```bash
# Stop and remove all containers, networks, and volumes
docker-compose down -v

# Remove all images
docker-compose down --rmi all

# Start fresh
docker-compose up -d --build
```

## Backup and Restore

### Automated Backup Script

```bash
#!/bin/bash
# backup.sh

BACKUP_DIR="./backups"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Backup database
docker-compose exec -T postgres pg_dump -U duoclean_user duoclean | gzip > $BACKUP_DIR/db_backup_$DATE.sql.gz

# Backup environment file
cp .env $BACKUP_DIR/env_backup_$DATE

# Remove backups older than 30 days
find $BACKUP_DIR -name "*.sql.gz" -mtime +30 -delete

echo "Backup completed: $BACKUP_DIR/db_backup_$DATE.sql.gz"
```

### Restore from Backup

```bash
#!/bin/bash
# restore.sh

BACKUP_FILE=$1

if [ -z "$BACKUP_FILE" ]; then
  echo "Usage: ./restore.sh <backup_file.sql.gz>"
  exit 1
fi

# Stop services
docker-compose down

# Start only database
docker-compose up -d postgres

# Wait for database to be ready
sleep 10

# Restore database
gunzip < $BACKUP_FILE | docker-compose exec -T postgres psql -U duoclean_user duoclean

# Start all services
docker-compose up -d

echo "Restore completed"
```

### Cron Job for Daily Backups

```bash
# Edit crontab
crontab -e

# Add daily backup at 2 AM
0 2 * * * /path/to/backup.sh >> /var/log/duoclean-backup.log 2>&1
```

## Performance Tuning

### PostgreSQL Configuration

Edit `postgresql.conf` via volume mount or environment variables:

```yaml
postgres:
  environment:
    - POSTGRES_MAX_CONNECTIONS=200
    - POSTGRES_SHARED_BUFFERS=256MB
    - POSTGRES_EFFECTIVE_CACHE_SIZE=1GB
    - POSTGRES_WORK_MEM=4MB
```

### Docker Resource Limits

```yaml
services:
  web-api:
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 2G
        reservations:
          cpus: '0.5'
          memory: 512M
```

## Migration from Development to Production

1. **Update environment variables** for production
2. **Enable HTTPS** with reverse proxy
3. **Set up managed database** (AWS RDS, etc.)
4. **Configure monitoring and alerts**
5. **Set up automated backups**
6. **Enable MQTT security** (TLS, authentication)
7. **Review and harden security settings**
8. **Set up CI/CD pipeline** for automated deployments
9. **Load testing** to verify performance
10. **Document runbooks** for operations team

## Support

For deployment issues, check:
- [README.md](./README.md) - General project information
- [ARCHITECTURE.md](./ARCHITECTURE.md) - System architecture
- [SECURITY.md](./SECURITY.md) - Security best practices
- Docker logs: `docker-compose logs -f`
- GitHub Issues: [repository-url]/issues
