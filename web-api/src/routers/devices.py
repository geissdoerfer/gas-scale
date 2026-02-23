"""
Device management and query endpoints.
"""
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from sqlalchemy import desc
from typing import List

from src.database import get_db
from src.dependencies import require_admin, get_current_user, check_device_access
from src import models
from src import schemas

router = APIRouter(prefix="/devices", tags=["Devices"])


@router.get("", response_model=List[schemas.DeviceResponse])
def list_devices(
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db)
):
    """
    List devices (filtered by user permissions).
    Admin: returns all devices
    User: returns only assigned devices
    """
    if current_user.role == "admin":
        devices = db.query(models.Device).all()
    else:
        # Get devices assigned to user
        devices = db.query(models.Device).join(
            models.UserDeviceAccess
        ).filter(
            models.UserDeviceAccess.user_id == current_user.id
        ).all()

    return devices


@router.post("", response_model=schemas.DeviceResponse, status_code=status.HTTP_201_CREATED)
def create_device(
    device_data: schemas.DeviceCreate,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """Create a new device (admin only)"""
    # Check if device_id already exists
    if db.query(models.Device).filter(models.Device.device_id == device_data.device_id).first():
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Device ID already exists"
        )

    new_device = models.Device(
        device_id=device_data.device_id,
        name=device_data.name,
        description=device_data.description
    )

    db.add(new_device)
    db.commit()
    db.refresh(new_device)

    return new_device


@router.get("/{device_id}", response_model=schemas.DeviceWithLatestReading)
def get_device(
    device_id: str,
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db)
):
    """Get device with latest reading and calibration applied"""
    device = db.query(models.Device).filter(models.Device.device_id == device_id).first()
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Device not found"
        )

    # Check access
    check_device_access(device_id, current_user, db)

    # Get latest reading
    latest = db.query(models.SensorReading).filter(
        models.SensorReading.device_id == device_id
    ).order_by(desc(models.SensorReading.time)).first()

    # Apply calibration to latest reading
    latest_reading_dict = None
    if latest:
        weight = None
        if latest.raw_value is not None:
            weight = (latest.raw_value + device.offset) * device.gain

        latest_reading_dict = {
            "time": latest.time,
            "device_id": latest.device_id,
            "raw_value": latest.raw_value,
            "weight": weight,
            "battery_voltage": latest.battery_voltage,
            "temperature": latest.temperature
        }

    # Determine status
    status_value = "unknown"
    if latest:
        if latest.battery_voltage and latest.battery_voltage < 11.5:
            status_value = "low_battery"
        else:
            status_value = "ok"

    device_dict = {
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
    }

    return device_dict


@router.post("/{device_id}/assign", response_model=schemas.MessageResponse)
def assign_device(
    device_id: str,
    assignment: schemas.DeviceAssignment,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """Assign device to user (admin only)"""
    device = db.query(models.Device).filter(models.Device.device_id == device_id).first()
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Device not found"
        )

    user = db.query(models.User).filter(models.User.id == assignment.user_id).first()
    if not user:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="User not found"
        )

    # Check if already assigned
    existing = db.query(models.UserDeviceAccess).filter(
        models.UserDeviceAccess.user_id == assignment.user_id,
        models.UserDeviceAccess.device_id == device.id
    ).first()

    if existing:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Device already assigned to this user"
        )

    # Create assignment
    access = models.UserDeviceAccess(
        user_id=assignment.user_id,
        device_id=device.id
    )
    db.add(access)
    db.commit()

    return {"message": "Device assigned successfully"}


@router.delete("/{device_id}/unassign/{user_id}", response_model=schemas.MessageResponse)
def unassign_device(
    device_id: str,
    user_id: int,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """Unassign device from user (admin only)"""
    device = db.query(models.Device).filter(models.Device.device_id == device_id).first()
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Device not found"
        )

    access = db.query(models.UserDeviceAccess).filter(
        models.UserDeviceAccess.user_id == user_id,
        models.UserDeviceAccess.device_id == device.id
    ).first()

    if not access:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Assignment not found"
        )

    db.delete(access)
    db.commit()

    return {"message": "Device unassigned successfully"}


@router.patch("/{device_id}/calibration", response_model=schemas.DeviceResponse)
def update_device_calibration(
    device_id: str,
    calibration: schemas.DeviceCalibration,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """
    Update device calibration parameters (admin only).

    Calibration formula: weight = (raw_value + offset) * gain

    Args:
        device_id: Device identifier
        calibration: Calibration parameters (offset and gain)

    Returns:
        Updated device information
    """
    device = db.query(models.Device).filter(models.Device.device_id == device_id).first()
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Device not found"
        )

    # Update calibration values
    device.offset = calibration.offset
    device.gain = calibration.gain

    db.commit()
    db.refresh(device)

    return device
