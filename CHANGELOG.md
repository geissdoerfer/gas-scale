# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Complete database schema with PostgreSQL + TimescaleDB
- MQTT ingestor service with auto-device registration
- Docker Compose orchestration for database and ingestor
- Comprehensive documentation in `/docs` directory
- Environment configuration with .env file

### Changed
- **BREAKING:** Device ID is now derived from MQTT topic path only, not from message payload
  - Old format: `{"device_id": "device_001", "load": 45.2, ...}`
  - New format: `{"load": 45.2, ...}` (device_id from topic `sensors/device_001/data`)
  - This change improves security by preventing device spoofing

### Security
- Device ID validation now prevents spoofing by using only the topic path
- Devices cannot impersonate other devices by including a different device_id in payload

## [0.1.0] - 2026-01-21

### Added
- Initial project setup
- Phase 1: Database with TimescaleDB complete
- Phase 2: MQTT Ingestor complete

### Components
- PostgreSQL 15 + TimescaleDB 2.x
- Python 3.11 MQTT Ingestor with paho-mqtt
- Docker containerization
- Connection pooling for database
- Automatic device registration
- Continuous aggregates for hourly statistics
- 1-year data retention policy
