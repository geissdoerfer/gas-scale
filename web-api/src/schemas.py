"""
Pydantic schemas for request/response validation.
"""
from pydantic import BaseModel, EmailStr, Field, ConfigDict
from typing import Optional, List
from datetime import datetime
from enum import Enum


# Enums
class UserRole(str, Enum):
    ADMIN = "admin"
    USER = "user"


# User Schemas
class UserBase(BaseModel):
    username: str = Field(..., min_length=3, max_length=50)
    email: EmailStr
    role: UserRole


class UserCreate(UserBase):
    password: str = Field(..., min_length=8)


class UserUpdate(BaseModel):
    username: Optional[str] = Field(None, min_length=3, max_length=50)
    email: Optional[EmailStr] = None
    password: Optional[str] = Field(None, min_length=8)
    role: Optional[UserRole] = None


class UserResponse(UserBase):
    id: int
    created_at: datetime
    updated_at: datetime

    model_config = ConfigDict(from_attributes=True)


class UserWithDevices(UserResponse):
    assigned_devices: List['DeviceResponse'] = []


# Device Schemas
class DeviceBase(BaseModel):
    device_id: str = Field(..., min_length=1, max_length=50)
    name: Optional[str] = Field(None, max_length=100)
    description: Optional[str] = None


class DeviceCreate(DeviceBase):
    pass


class DeviceUpdate(BaseModel):
    name: Optional[str] = Field(None, max_length=100)
    description: Optional[str] = None


class DeviceResponse(DeviceBase):
    id: int
    created_at: datetime
    updated_at: datetime

    model_config = ConfigDict(from_attributes=True)


class DeviceWithLatestReading(DeviceResponse):
    latest_reading: Optional['SensorReadingResponse'] = None
    status: Optional[str] = "unknown"


# Sensor Reading Schemas
class SensorReadingResponse(BaseModel):
    time: datetime
    device_id: str
    weight: Optional[float] = None
    battery_voltage: Optional[float] = None
    temperature: Optional[float] = None

    model_config = ConfigDict(from_attributes=True)


class SensorReadingList(BaseModel):
    device_id: str
    readings: List[SensorReadingResponse]
    count: int
    start_time: Optional[datetime] = None
    end_time: Optional[datetime] = None


class AggregateReading(BaseModel):
    bucket: datetime
    device_id: str
    avg_weight: Optional[float] = None
    min_weight: Optional[float] = None
    max_weight: Optional[float] = None
    avg_battery_voltage: Optional[float] = None
    min_battery_voltage: Optional[float] = None
    max_battery_voltage: Optional[float] = None
    avg_temperature: Optional[float] = None
    min_temperature: Optional[float] = None
    max_temperature: Optional[float] = None
    reading_count: int

    model_config = ConfigDict(from_attributes=True)


class AggregateList(BaseModel):
    device_id: str
    interval: str
    aggregates: List[AggregateReading]
    count: int


# Authentication Schemas
class LoginRequest(BaseModel):
    username: str
    password: str


class TokenResponse(BaseModel):
    access_token: str
    refresh_token: str
    token_type: str = "bearer"
    user: UserResponse


class RefreshTokenRequest(BaseModel):
    refresh_token: str


class TokenRefreshResponse(BaseModel):
    access_token: str
    token_type: str = "bearer"


# Device Assignment Schemas
class DeviceAssignment(BaseModel):
    user_id: int


class DeviceAssignmentResponse(BaseModel):
    message: str
    device_id: str
    user_id: int


# Dashboard Schemas
class DashboardDevice(DeviceWithLatestReading):
    pass


class DashboardResponse(BaseModel):
    devices: List[DashboardDevice]
    total: int
    user: UserResponse


# Generic Response Schemas
class MessageResponse(BaseModel):
    message: str


class HealthResponse(BaseModel):
    status: str
    timestamp: datetime


# Rebuild models with forward references
UserWithDevices.model_rebuild()
DeviceWithLatestReading.model_rebuild()
DashboardDevice.model_rebuild()
DashboardResponse.model_rebuild()
