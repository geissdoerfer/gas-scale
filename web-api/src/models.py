"""
SQLAlchemy ORM models for database tables.
"""
from sqlalchemy import Column, Integer, String, Float, ForeignKey, DateTime, Table, TIMESTAMP
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from src.database import Base


class User(Base):
    """User model for authentication and authorization"""
    __tablename__ = "users"

    id = Column(Integer, primary_key=True, index=True)
    username = Column(String(50), unique=True, nullable=False, index=True)
    email = Column(String(100), unique=True, nullable=False, index=True)
    password_hash = Column(String(255), nullable=False)
    role = Column(String(20), nullable=False)  # 'admin' or 'user'
    created_at = Column(TIMESTAMP(timezone=True), server_default=func.now())
    updated_at = Column(TIMESTAMP(timezone=True), server_default=func.now(), onupdate=func.now())

    # Relationships
    device_access = relationship("UserDeviceAccess", back_populates="user", cascade="all, delete-orphan")


class Device(Base):
    """Device model for IoT devices"""
    __tablename__ = "devices"

    id = Column(Integer, primary_key=True, index=True)
    device_id = Column(String(50), unique=True, nullable=False, index=True)
    name = Column(String(100))
    description = Column(String)
    offset = Column(Float, default=0.0, nullable=False, server_default="0.0")
    gain = Column(Float, default=1.0, nullable=False, server_default="1.0")
    created_at = Column(TIMESTAMP(timezone=True), server_default=func.now())
    updated_at = Column(TIMESTAMP(timezone=True), server_default=func.now(), onupdate=func.now())

    # Relationships
    user_access = relationship("UserDeviceAccess", back_populates="device", cascade="all, delete-orphan")


class UserDeviceAccess(Base):
    """Many-to-many relationship between users and devices"""
    __tablename__ = "user_device_access"

    user_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), primary_key=True)
    device_id = Column(Integer, ForeignKey("devices.id", ondelete="CASCADE"), primary_key=True)
    assigned_at = Column(TIMESTAMP(timezone=True), server_default=func.now())

    # Relationships
    user = relationship("User", back_populates="device_access")
    device = relationship("Device", back_populates="user_access")


class SensorReading(Base):
    """Sensor reading model (read-only, managed by MQTT ingestor)"""
    __tablename__ = "sensor_readings"

    time = Column(TIMESTAMP(timezone=True), primary_key=True)
    device_id = Column(String(50), primary_key=True, index=True)
    raw_value = Column(Float, nullable=True)
    battery_voltage = Column(Float, nullable=True)
    temperature = Column(Float, nullable=True)
