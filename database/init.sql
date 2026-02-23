-- DuoClean Energy - Database Initialization Script
-- PostgreSQL + TimescaleDB

-- Enable TimescaleDB extension
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- ===== USERS TABLE =====
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL CHECK (role IN ('admin', 'user')),
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Indexes for fast lookups
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_users_email ON users(email);

-- ===== DEVICES TABLE =====
CREATE TABLE devices (
    id SERIAL PRIMARY KEY,
    device_id VARCHAR(50) UNIQUE NOT NULL,
    name VARCHAR(100),
    description TEXT,
    "offset" FLOAT DEFAULT -274753.0 NOT NULL,
    gain FLOAT DEFAULT 0.0797197 NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index for fast device_id lookups
CREATE INDEX idx_devices_device_id ON devices(device_id);

-- Comments for calibration columns
COMMENT ON COLUMN devices."offset" IS 'Calibration offset: raw_value + offset';
COMMENT ON COLUMN devices.gain IS 'Calibration gain/scale factor: (raw_value + offset) * gain = weight';

-- ===== USER DEVICE ACCESS TABLE =====
CREATE TABLE user_device_access (
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    device_id INTEGER REFERENCES devices(id) ON DELETE CASCADE,
    assigned_at TIMESTAMPTZ DEFAULT NOW(),
    PRIMARY KEY (user_id, device_id)
);

-- Indexes for fast lookups
CREATE INDEX idx_user_device_access_user_id ON user_device_access(user_id);
CREATE INDEX idx_user_device_access_device_id ON user_device_access(device_id);

-- ===== SENSOR READINGS TABLE (HYPERTABLE) =====
CREATE TABLE sensor_readings (
    time TIMESTAMPTZ NOT NULL,
    device_id VARCHAR(50) NOT NULL,
    raw_value FLOAT,
    battery_voltage FLOAT,
    temperature FLOAT,
    PRIMARY KEY (time, device_id)
);

-- Convert to TimescaleDB hypertable (partitioned by time)
SELECT create_hypertable('sensor_readings', 'time');

-- Index for device-specific queries
CREATE INDEX idx_sensor_readings_device_id ON sensor_readings(device_id, time DESC);

-- Comment for raw_value column
COMMENT ON COLUMN sensor_readings.raw_value IS 'Raw ADC value from HX711 sensor (uncalibrated)';

-- ===== CONTINUOUS AGGREGATE FOR HOURLY STATISTICS =====
CREATE MATERIALIZED VIEW sensor_readings_hourly
WITH (timescaledb.continuous) AS
SELECT
    time_bucket('1 hour', time) AS bucket,
    device_id,
    AVG(raw_value) AS avg_raw_value,
    MIN(raw_value) AS min_raw_value,
    MAX(raw_value) AS max_raw_value,
    AVG(battery_voltage) AS avg_battery_voltage,
    MIN(battery_voltage) AS min_battery_voltage,
    MAX(battery_voltage) AS max_battery_voltage,
    AVG(temperature) AS avg_temperature,
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

-- ===== DATA RETENTION POLICY =====
-- Automatically drop data older than 1 year
SELECT add_retention_policy('sensor_readings', INTERVAL '1 year');

-- ===== HELPER VIEW FOR CALIBRATED READINGS =====
-- Optional view that automatically applies calibration
CREATE OR REPLACE VIEW sensor_readings_calibrated AS
SELECT
    sr.time,
    sr.device_id,
    sr.raw_value,
    (sr.raw_value + COALESCE(d."offset", 0.0)) * COALESCE(d.gain, 1.0) AS weight,
    sr.battery_voltage,
    sr.temperature,
    d."offset" AS device_offset,
    d.gain AS device_gain
FROM sensor_readings sr
LEFT JOIN devices d ON sr.device_id = d.device_id;

COMMENT ON VIEW sensor_readings_calibrated IS 'Sensor readings with calibration applied: weight = (raw_value + offset) * gain';

-- ===== DEFAULT ADMIN USER =====
-- Username: admin
-- Password: admin123 (CHANGE IN PRODUCTION!)
-- Password hash generated with bcrypt (cost factor 12)
INSERT INTO users (username, email, password_hash, role)
VALUES ('admin', 'admin@duocleanenergy.com',
        '$2b$12$GOizO1slRZouRc8PBGeq5.QQb7I6bGJfLUbRf5KLfEducu3IPSbqm',
        'admin');

-- ===== COMPLETE =====
-- Database initialization complete
