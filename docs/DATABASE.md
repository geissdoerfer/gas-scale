# Database Design

## Overview

The application uses **PostgreSQL 15** with the **TimescaleDB 2.x** extension for efficient time-series data storage and querying.

## Why TimescaleDB?

TimescaleDB is an extension that turns PostgreSQL into a time-series database while maintaining full SQL compatibility.

**Advantages for this project:**
- Automatic time-based partitioning (hypertables)
- Continuous aggregates for pre-computed statistics
- Data retention policies
- Excellent compression
- Native SQL queries (no new query language to learn)
- Handles millions of time-series data points efficiently

## Schema Overview

```
users (authentication)
  ↓
user_device_access (many-to-many)
  ↓
devices (IoT device metadata)
  ↓
sensor_readings (hypertable - time-series data)
  ↓
sensor_readings_hourly (continuous aggregate)
```

## Table Definitions

### 1. users

Stores user accounts with authentication information.

```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL CHECK (role IN ('admin', 'user')),
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index for fast username lookups during authentication
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_users_email ON users(email);
```

**Fields:**
- `id`: Auto-incrementing primary key
- `username`: Unique login name (max 50 chars)
- `email`: User email address (unique)
- `password_hash`: bcrypt hash of password (never store plaintext!)
- `role`: Either 'admin' or 'user'
  - **admin**: Can access all devices, manage users, manage devices
  - **user**: Can only access assigned devices
- `created_at`: Timestamp of account creation
- `updated_at`: Timestamp of last update

**Sample data:**
```sql
-- Default admin user (password: admin123)
INSERT INTO users (username, email, password_hash, role)
VALUES ('admin', 'admin@duocleanenergy.com',
        '$2b$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewY5NU7MvbBjqG3u',
        'admin');
```

### 2. devices

Stores metadata about IoT devices.

```sql
CREATE TABLE devices (
    id SERIAL PRIMARY KEY,
    device_id VARCHAR(50) UNIQUE NOT NULL,
    name VARCHAR(100),
    description TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index for fast device_id lookups
CREATE INDEX idx_devices_device_id ON devices(device_id);
```

**Fields:**
- `id`: Auto-incrementing primary key (internal use)
- `device_id`: Actual device identifier from MQTT messages (unique)
- `name`: Human-readable device name (optional)
- `description`: Additional device information (optional)
- `created_at`: When device was first registered
- `updated_at`: Last metadata update

**Notes:**
- Devices are auto-registered when first MQTT message arrives
- Admin can update name and description later
- `device_id` is what appears in MQTT topics and messages

### 3. user_device_access

Many-to-many relationship between users and devices. Defines which users can access which devices.

```sql
CREATE TABLE user_device_access (
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    device_id INTEGER REFERENCES devices(id) ON DELETE CASCADE,
    assigned_at TIMESTAMPTZ DEFAULT NOW(),
    PRIMARY KEY (user_id, device_id)
);

-- Indexes for fast lookups
CREATE INDEX idx_user_device_access_user_id ON user_device_access(user_id);
CREATE INDEX idx_user_device_access_device_id ON user_device_access(device_id);
```

**Fields:**
- `user_id`: Foreign key to users table
- `device_id`: Foreign key to devices table
- `assigned_at`: When device was assigned to user

**Behavior:**
- Composite primary key ensures no duplicate assignments
- CASCADE DELETE: If user or device is deleted, access is automatically removed
- Admin users bypass this table (access all devices by role)

**Sample data:**
```sql
-- Assign device 1 to user 2
INSERT INTO user_device_access (user_id, device_id)
VALUES (2, 1);
```

### 4. sensor_readings (TimescaleDB Hypertable)

Time-series data from IoT devices. This is the main data table.

```sql
CREATE TABLE sensor_readings (
    time TIMESTAMPTZ NOT NULL,
    device_id VARCHAR(50) NOT NULL,
    load FLOAT,
    battery_voltage FLOAT,
    temperature FLOAT,
    PRIMARY KEY (time, device_id)
);

-- Convert to TimescaleDB hypertable (partitioned by time)
SELECT create_hypertable('sensor_readings', 'time');

-- Index for fast device-specific queries
CREATE INDEX idx_sensor_readings_device_id ON sensor_readings(device_id, time DESC);
```

**Fields:**
- `time`: Timestamp of reading (TIMESTAMPTZ = timestamp with timezone)
- `device_id`: Device identifier (matches devices.device_id)
- `load`: Load reading (float, nullable)
- `battery_voltage`: Battery voltage reading (float, nullable)
- `temperature`: Temperature reading (float, nullable)

**Notes:**
- Primary key is `(time, device_id)` to ensure uniqueness per device per timestamp
- Nullable sensor values allow partial readings
- `create_hypertable()` converts this to a TimescaleDB hypertable
  - Automatically partitions data by time (default: 7-day chunks)
  - Significantly improves query performance on time ranges
- Index on `(device_id, time DESC)` optimizes common query pattern: "get recent readings for device X"

**Hypertable Benefits:**
- Efficient inserts (append-only pattern)
- Fast time-range queries
- Automatic data management (compression, retention)
- Transparent to application (still just SQL)

### 5. sensor_readings_hourly (Continuous Aggregate)

Pre-computed hourly statistics for faster queries on historical data.

```sql
CREATE MATERIALIZED VIEW sensor_readings_hourly
WITH (timescaledb.continuous) AS
SELECT
    time_bucket('1 hour', time) AS bucket,
    device_id,
    AVG(load) AS avg_load,
    AVG(battery_voltage) AS avg_battery_voltage,
    AVG(temperature) AS avg_temperature,
    MIN(load) AS min_load,
    MAX(load) AS max_load,
    MIN(battery_voltage) AS min_battery_voltage,
    MAX(battery_voltage) AS max_battery_voltage,
    MIN(temperature) AS min_temperature,
    MAX(temperature) AS max_temperature,
    COUNT(*) AS reading_count
FROM sensor_readings
GROUP BY bucket, device_id;
```

**What is a Continuous Aggregate?**
- A materialized view that automatically updates as new data arrives
- TimescaleDB incrementally refreshes the aggregate
- Much faster than computing aggregates on-the-fly for large time ranges

**Fields:**
- `bucket`: Start of the 1-hour time bucket
- `device_id`: Device identifier
- `avg_*`: Average values over the hour
- `min_*`: Minimum values over the hour
- `max_*`: Maximum values over the hour
- `reading_count`: Number of readings in that hour

**Usage:**
```sql
-- Query hourly averages instead of raw data for long time periods
SELECT bucket, avg_temperature, min_temperature, max_temperature
FROM sensor_readings_hourly
WHERE device_id = 'device_001'
  AND bucket >= NOW() - INTERVAL '30 days'
ORDER BY bucket DESC;
```

**Refresh Policy:**
```sql
SELECT add_continuous_aggregate_policy('sensor_readings_hourly',
    start_offset => INTERVAL '3 hours',
    end_offset => INTERVAL '1 hour',
    schedule_interval => INTERVAL '1 hour');
```

This policy:
- Refreshes data from 3 hours ago to 1 hour ago
- Runs every hour
- Ensures recent data is always available in the aggregate

## Data Retention

### Retention Policy

Automatically drop old data to manage storage:

```sql
SELECT add_retention_policy('sensor_readings', INTERVAL '1 year');
```

**Behavior:**
- Deletes chunks of data older than 1 year
- Runs automatically in background
- Only affects raw `sensor_readings` table
- Continuous aggregates are not affected (keep indefinitely or set separate policy)

**Why this works:**
- Recent data (< 1 month): Query raw `sensor_readings` for full detail
- Historical data (> 1 month): Query `sensor_readings_hourly` for trends
- Very old data (> 1 year): Only hourly aggregates remain

**Adjust retention:**
```sql
-- Change to 2 years
SELECT remove_retention_policy('sensor_readings');
SELECT add_retention_policy('sensor_readings', INTERVAL '2 years');
```

## Complete init.sql

This script is run once during database initialization:

```sql
-- Enable TimescaleDB extension
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- Create users table
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL CHECK (role IN ('admin', 'user')),
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_users_email ON users(email);

-- Create devices table
CREATE TABLE devices (
    id SERIAL PRIMARY KEY,
    device_id VARCHAR(50) UNIQUE NOT NULL,
    name VARCHAR(100),
    description TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_devices_device_id ON devices(device_id);

-- Create user_device_access table
CREATE TABLE user_device_access (
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    device_id INTEGER REFERENCES devices(id) ON DELETE CASCADE,
    assigned_at TIMESTAMPTZ DEFAULT NOW(),
    PRIMARY KEY (user_id, device_id)
);

CREATE INDEX idx_user_device_access_user_id ON user_device_access(user_id);
CREATE INDEX idx_user_device_access_device_id ON user_device_access(device_id);

-- Create sensor_readings table
CREATE TABLE sensor_readings (
    time TIMESTAMPTZ NOT NULL,
    device_id VARCHAR(50) NOT NULL,
    load FLOAT,
    battery_voltage FLOAT,
    temperature FLOAT,
    PRIMARY KEY (time, device_id)
);

-- Convert to hypertable
SELECT create_hypertable('sensor_readings', 'time');

-- Index for device queries
CREATE INDEX idx_sensor_readings_device_id ON sensor_readings(device_id, time DESC);

-- Create continuous aggregate for hourly statistics
CREATE MATERIALIZED VIEW sensor_readings_hourly
WITH (timescaledb.continuous) AS
SELECT
    time_bucket('1 hour', time) AS bucket,
    device_id,
    AVG(load) AS avg_load,
    AVG(battery_voltage) AS avg_battery_voltage,
    AVG(temperature) AS avg_temperature,
    MIN(load) AS min_load,
    MAX(load) AS max_load,
    MIN(battery_voltage) AS min_battery_voltage,
    MAX(battery_voltage) AS max_battery_voltage,
    MIN(temperature) AS min_temperature,
    MAX(temperature) AS max_temperature,
    COUNT(*) AS reading_count
FROM sensor_readings
GROUP BY bucket, device_id;

-- Add refresh policy for continuous aggregate
SELECT add_continuous_aggregate_policy('sensor_readings_hourly',
    start_offset => INTERVAL '3 hours',
    end_offset => INTERVAL '1 hour',
    schedule_interval => INTERVAL '1 hour');

-- Add retention policy (1 year)
SELECT add_retention_policy('sensor_readings', INTERVAL '1 year');

-- Insert default admin user
-- Password: admin123 (CHANGE IN PRODUCTION!)
-- Hash generated with: python -c "from passlib.hash import bcrypt; print(bcrypt.hash('admin123'))"
INSERT INTO users (username, email, password_hash, role)
VALUES ('admin', 'admin@duocleanenergy.com',
        '$2b$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewY5NU7MvbBjqG3u',
        'admin');

-- Optional: Insert sample devices for testing
-- INSERT INTO devices (device_id, name, description)
-- VALUES
--     ('device_001', 'Sensor 001', 'Test device in location A'),
--     ('device_002', 'Sensor 002', 'Test device in location B');
```

## Common Queries

### Get Latest Reading for Device

```sql
SELECT time, load, battery_voltage, temperature
FROM sensor_readings
WHERE device_id = 'device_001'
ORDER BY time DESC
LIMIT 1;
```

### Get Readings for Last 24 Hours

```sql
SELECT time, load, battery_voltage, temperature
FROM sensor_readings
WHERE device_id = 'device_001'
  AND time >= NOW() - INTERVAL '24 hours'
ORDER BY time DESC;
```

### Get Hourly Averages for Last 7 Days

```sql
SELECT bucket, avg_load, avg_battery_voltage, avg_temperature
FROM sensor_readings_hourly
WHERE device_id = 'device_001'
  AND bucket >= NOW() - INTERVAL '7 days'
ORDER BY bucket DESC;
```

### Get All Devices Accessible by User

```sql
-- For regular user (user_id = 2)
SELECT d.id, d.device_id, d.name, d.description
FROM devices d
JOIN user_device_access uda ON d.id = uda.device_id
WHERE uda.user_id = 2;

-- For admin (access all devices)
SELECT id, device_id, name, description
FROM devices;
```

### Get Latest Reading for All Devices (Dashboard)

```sql
SELECT DISTINCT ON (device_id)
    device_id,
    time,
    load,
    battery_voltage,
    temperature
FROM sensor_readings
ORDER BY device_id, time DESC;
```

**Note:** `DISTINCT ON` is a PostgreSQL feature that returns the first row per group.

### Check Device Access for User

```sql
-- Check if user_id 2 can access device_id 1
SELECT EXISTS (
    SELECT 1
    FROM user_device_access
    WHERE user_id = 2 AND device_id = 1
) AS has_access;

-- Or check if user is admin
SELECT role = 'admin' AS is_admin
FROM users
WHERE id = 2;
```

## Performance Optimization

### Index Usage

The application relies on these indexes:

1. **idx_sensor_readings_device_id** - For device-specific queries
2. **idx_devices_device_id** - For device lookups by MQTT device_id
3. **idx_user_device_access_user_id** - For permission checks
4. **idx_users_username** - For login queries

**Verify index usage:**
```sql
EXPLAIN ANALYZE
SELECT * FROM sensor_readings
WHERE device_id = 'device_001'
  AND time >= NOW() - INTERVAL '24 hours';
```

Look for "Index Scan" (good) vs "Seq Scan" (bad for large tables).

### Query Optimization Tips

1. **Always filter by time range** - Hypertables are optimized for time-based queries
2. **Use continuous aggregates for long time periods** (> 1 week)
3. **Limit result sets** - Add `LIMIT` clause when possible
4. **Use appropriate time buckets** - 1 hour for 30 days, 1 day for 1 year, etc.

### Hypertable Chunk Management

Check chunk information:
```sql
SELECT * FROM timescaledb_information.chunks
WHERE hypertable_name = 'sensor_readings';
```

Manually compress old chunks (optional):
```sql
SELECT compress_chunk(i)
FROM show_chunks('sensor_readings', older_than => INTERVAL '30 days') i;
```

## Backup and Restore

### Backup

```bash
# Backup entire database
docker-compose exec postgres pg_dump -U duoclean_user duoclean > backup.sql

# Backup with compression
docker-compose exec postgres pg_dump -U duoclean_user duoclean | gzip > backup.sql.gz
```

### Restore

```bash
# Restore from backup
docker-compose exec -T postgres psql -U duoclean_user duoclean < backup.sql

# Restore from compressed backup
gunzip < backup.sql.gz | docker-compose exec -T postgres psql -U duoclean_user duoclean
```

### Continuous Backups (Production)

- Use WAL archiving and point-in-time recovery
- Managed PostgreSQL services (AWS RDS, GCP Cloud SQL) handle this automatically
- Consider pgBackRest or Barman for self-hosted solutions

## Database Migrations

For MVP, we use `init.sql` which runs once. For production with schema changes:

### Option 1: Alembic (Recommended)

```bash
pip install alembic
alembic init migrations
# Edit alembic.ini and env.py
alembic revision -m "Add new column"
# Edit migration file
alembic upgrade head
```

### Option 2: Manual SQL Scripts

Create versioned migration scripts:
```
migrations/
  001_initial_schema.sql
  002_add_device_location.sql
  003_add_user_preferences.sql
```

Track applied migrations in a table:
```sql
CREATE TABLE schema_migrations (
    version INTEGER PRIMARY KEY,
    applied_at TIMESTAMPTZ DEFAULT NOW()
);
```

## Troubleshooting

### Connection Issues

```bash
# Check if PostgreSQL is running
docker-compose ps postgres

# Check logs
docker-compose logs postgres

# Test connection
docker-compose exec postgres psql -U duoclean_user -d duoclean -c "SELECT NOW();"
```

### Slow Queries

```sql
-- Enable query logging (for debugging)
ALTER SYSTEM SET log_min_duration_statement = 1000; -- Log queries > 1 second
SELECT pg_reload_conf();

-- View slow queries (requires pg_stat_statements extension)
SELECT query, calls, mean_exec_time, max_exec_time
FROM pg_stat_statements
ORDER BY mean_exec_time DESC
LIMIT 10;
```

### Hypertable Not Working

```sql
-- Verify hypertable was created
SELECT * FROM timescaledb_information.hypertables;

-- Check if table exists
\d sensor_readings

-- Recreate hypertable if needed (WARNING: drops existing data)
DROP TABLE sensor_readings CASCADE;
-- Then re-run create table and create_hypertable()
```

### Disk Space Issues

```sql
-- Check database size
SELECT pg_size_pretty(pg_database_size('duoclean'));

-- Check table sizes
SELECT
    schemaname,
    tablename,
    pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename)) AS size
FROM pg_tables
WHERE schemaname = 'public'
ORDER BY pg_total_relation_size(schemaname||'.'||tablename) DESC;

-- Check chunk sizes
SELECT
    chunk_name,
    pg_size_pretty(total_bytes) AS size
FROM timescaledb_information.chunks
ORDER BY total_bytes DESC;
```

## Resources

- [TimescaleDB Documentation](https://docs.timescale.com/)
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [TimescaleDB Hypertables](https://docs.timescale.com/use-timescale/latest/hypertables/)
- [Continuous Aggregates](https://docs.timescale.com/use-timescale/latest/continuous-aggregates/)
- [Data Retention](https://docs.timescale.com/use-timescale/latest/data-retention/)
