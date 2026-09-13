"""
Sensor readings query endpoints.
"""
from fastapi import APIRouter, Depends, HTTPException, Query, status
from sqlalchemy.orm import Session
from sqlalchemy import desc, text
from typing import Optional
from datetime import datetime, timedelta

from src.database import get_db
from src.dependencies import get_current_user, check_device_access
from src import models
from src import schemas
from src.calibration import reading_to_dict
from src.cylinder_specs import LOW_BATTERY_VOLTAGE

router = APIRouter(tags=["Sensor Readings"])


@router.get("/devices/{device_id}/latest", response_model=schemas.SensorReadingResponse)
def get_latest_reading(
    device_id: str,
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db)
):
    """Get the latest sensor reading for a device with calibration applied"""
    check_device_access(device_id, current_user, db)

    reading = db.query(models.SensorReading).filter(
        models.SensorReading.device_id == device_id
    ).order_by(desc(models.SensorReading.time)).first()

    if not reading:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="No readings found for this device"
        )

    # Get device calibration
    device = db.query(models.Device).filter(models.Device.device_id == device_id).first()
    offset = device.offset if device else 0.0
    gain = device.gain if device else 1.0

    return reading_to_dict(reading, offset, gain)


@router.get("/devices/{device_id}/readings", response_model=schemas.SensorReadingList)
def get_readings(
    device_id: str,
    start_time: Optional[datetime] = Query(None),
    end_time: Optional[datetime] = Query(None),
    limit: int = Query(1000, le=10000),
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db)
):
    """Get historical sensor readings for a device with calibration applied"""
    check_device_access(device_id, current_user, db)

    # Default to last 24 hours if not specified
    if not start_time:
        start_time = datetime.utcnow() - timedelta(days=1)
    if not end_time:
        end_time = datetime.utcnow()

    # Get device calibration
    device = db.query(models.Device).filter(models.Device.device_id == device_id).first()
    offset = device.offset if device else 0.0
    gain = device.gain if device else 1.0

    # Query readings
    readings = db.query(models.SensorReading).filter(
        models.SensorReading.device_id == device_id,
        models.SensorReading.time >= start_time,
        models.SensorReading.time <= end_time
    ).order_by(desc(models.SensorReading.time)).limit(limit).all()

    # Apply calibration to each reading
    calibrated_readings = [reading_to_dict(reading, offset, gain) for reading in readings]

    return {
        "device_id": device_id,
        "readings": calibrated_readings,
        "count": len(calibrated_readings),
        "start_time": start_time,
        "end_time": end_time
    }


@router.get("/dashboard", response_model=schemas.DashboardResponse)
def get_dashboard(
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db)
):
    """
    Get dashboard with all accessible devices and their latest readings with calibration applied.
    Admin: all devices
    User: only assigned devices
    """
    # Get devices based on role
    if current_user.role == "admin":
        devices = db.query(models.Device).all()
    else:
        devices = db.query(models.Device).join(
            models.UserDeviceAccess
        ).filter(
            models.UserDeviceAccess.user_id == current_user.id
        ).all()

    dashboard_devices = []

    for device in devices:
        # Get latest reading
        latest = db.query(models.SensorReading).filter(
            models.SensorReading.device_id == device.device_id
        ).order_by(desc(models.SensorReading.time)).first()

        # Apply calibration to latest reading if it exists
        latest_reading_dict = reading_to_dict(latest, device.offset, device.gain) if latest else None

        # Determine status
        status_value = "no_data"
        if latest:
            time_diff = datetime.utcnow() - latest.time.replace(tzinfo=None)
            if time_diff > timedelta(hours=24):
                status_value = "no_data"
            elif latest.battery_voltage and latest.battery_voltage < LOW_BATTERY_VOLTAGE:
                status_value = "low_battery"
            else:
                status_value = "ok"

        dashboard_devices.append({
            "id": device.id,
            "device_id": device.device_id,
            "name": device.name,
            "description": device.description,
            "offset": device.offset,
            "gain": device.gain,
            "created_at": device.created_at,
            "updated_at": device.updated_at,
            "latest_reading": latest_reading_dict,
            "status": status_value
        })

    return {
        "devices": dashboard_devices,
        "total": len(dashboard_devices),
        "user": current_user
    }
