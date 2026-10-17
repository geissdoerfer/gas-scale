"""
Device management and query endpoints.
"""
from datetime import timedelta

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from sqlalchemy import desc
from typing import List

from src.database import get_db
from src.dependencies import require_admin, get_current_user, get_current_device, check_device_access
from src import models
from src import schemas
from src import auth
from src.calibration import calculate_weight, reading_to_dict
from src.cylinder_specs import gas_percentage_remaining, battery_percentage, LOW_BATTERY_VOLTAGE

router = APIRouter(prefix="/devices", tags=["Devices"])

# Device-scoped pairing tokens (see POST /devices/pair) have no refresh flow, so they're
# issued long-lived rather than expiring like user access tokens do.
DEVICE_TOKEN_EXPIRE = timedelta(days=365)


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


@router.post("", response_model=schemas.DeviceCreateResponse, status_code=status.HTTP_201_CREATED)
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
        description=device_data.description,
        offset=device_data.offset,
        gain=device_data.gain
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
    latest_reading_dict = reading_to_dict(latest, device.offset, device.gain) if latest else None

    # Determine status
    status_value = "unknown"
    if latest:
        if latest.battery_voltage and latest.battery_voltage < LOW_BATTERY_VOLTAGE:
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


# --- Account-less device pairing (Android app; see android/app_plan.md) --------------------
#
# These endpoints authenticate with a device-scoped token (get_current_device) rather than
# a user JWT — pairing with the correct device_id + pairing_secret *is* the credential, there
# is no separate user account involved.

def _build_device_status(device: models.Device, db: Session) -> dict:
    """Shared status payload for the cylinder-update and status endpoints."""
    latest = db.query(models.SensorReading).filter(
        models.SensorReading.device_id == device.device_id
    ).order_by(desc(models.SensorReading.time)).first()

    if device.cylinder_type is None:
        return {
            "device_id": device.device_id,
            "cylinder_type": None,
            "percentage_remaining": None,
            "battery_percentage": None,
            "battery_voltage": latest.battery_voltage if latest else None,
            "last_updated": latest.time if latest else None,
            "status": "not_configured",
        }

    if latest is None:
        return {
            "device_id": device.device_id,
            "cylinder_type": device.cylinder_type,
            "percentage_remaining": None,
            "battery_percentage": None,
            "battery_voltage": None,
            "last_updated": None,
            "status": "no_data",
        }

    percentage_remaining = None
    weight_kg = calculate_weight(latest.raw_value, device.offset, device.gain)
    if weight_kg is not None:
        percentage_remaining = gas_percentage_remaining(weight_kg, device.cylinder_type)

    battery_pct = None
    if latest.battery_voltage is not None:
        battery_pct = battery_percentage(latest.battery_voltage)

    status_value = "ok"
    if latest.battery_voltage is not None and latest.battery_voltage < LOW_BATTERY_VOLTAGE:
        status_value = "low_battery"

    return {
        "device_id": device.device_id,
        "cylinder_type": device.cylinder_type,
        "percentage_remaining": percentage_remaining,
        "battery_percentage": battery_pct,
        "battery_voltage": latest.battery_voltage,
        "last_updated": latest.time,
        "status": status_value,
    }


@router.post("/pair", response_model=schemas.DevicePairResponse)
def pair_device(
    pairing: schemas.DevicePairRequest,
    db: Session = Depends(get_db)
):
    """
    Pair with a device using the device_id + pairing_secret printed on its QR code label
    (scanned or entered manually). No user account required — returns a device-scoped
    token to use for /devices/me/* endpoints.
    """
    device = db.query(models.Device).filter(models.Device.device_id == pairing.device_id).first()

    if not device or device.pairing_secret != pairing.secret:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid device ID or pairing secret"
        )

    access_token = auth.create_access_token(
        {"sub": device.device_id, "scope": "device"},
        expires_delta=DEVICE_TOKEN_EXPIRE
    )

    return {
        "access_token": access_token,
        "token_type": "bearer",
        "device_id": device.device_id,
    }


@router.patch("/me/cylinder", response_model=schemas.DeviceStatusResponse)
def set_cylinder_type(
    update: schemas.CylinderUpdateRequest,
    current_device: models.Device = Depends(get_current_device),
    db: Session = Depends(get_db)
):
    """Set or change the cylinder type for the paired device (device-token authenticated)."""
    current_device.cylinder_type = update.cylinder_type.value
    db.commit()
    db.refresh(current_device)

    return _build_device_status(current_device, db)


@router.get("/me/status", response_model=schemas.DeviceStatusResponse)
def get_paired_device_status(
    current_device: models.Device = Depends(get_current_device),
    db: Session = Depends(get_db)
):
    """
    Get the paired device's current gas percentage and battery level (device-token
    authenticated). This is the endpoint the app polls for its main gauge screen.
    """
    return _build_device_status(current_device, db)
