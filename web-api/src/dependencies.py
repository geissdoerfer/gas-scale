"""
FastAPI dependencies for authentication and authorization.
"""
from fastapi import Depends, HTTPException, status
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from jose import JWTError
from sqlalchemy.orm import Session
from typing import Optional

from src.database import get_db
from src.auth import decode_token
from src import models
from src import schemas

# HTTP Bearer security scheme
security = HTTPBearer()


def get_current_user(
    credentials: HTTPAuthorizationCredentials = Depends(security),
    db: Session = Depends(get_db)
) -> models.User:
    """
    Get current authenticated user from JWT token.

    Args:
        credentials: HTTP authorization credentials with Bearer token
        db: Database session

    Returns:
        Current user object

    Raises:
        HTTPException: If token is invalid or user not found
    """
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )

    try:
        token = credentials.credentials
        payload = decode_token(token)
        user_id_str: Optional[str] = payload.get("sub")

        if user_id_str is None:
            raise credentials_exception

        # Convert string back to integer
        user_id = int(user_id_str)

    except (JWTError, ValueError):
        raise credentials_exception

    # Get user from database
    user = db.query(models.User).filter(models.User.id == user_id).first()

    if user is None:
        raise credentials_exception

    return user


def require_admin(current_user: models.User = Depends(get_current_user)) -> models.User:
    """
    Dependency to require admin role.

    Args:
        current_user: Current authenticated user

    Returns:
        Current user if admin

    Raises:
        HTTPException: If user is not admin
    """
    if current_user.role != "admin":
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not enough permissions. Admin role required."
        )

    return current_user


def check_device_access(device_id: str, current_user: models.User, db: Session) -> bool:
    """
    Check if user has access to a specific device.

    Args:
        device_id: Device identifier (device_id field, not internal id)
        current_user: Current authenticated user
        db: Database session

    Returns:
        True if user has access

    Raises:
        HTTPException: If user doesn't have access
    """
    # Admin has access to all devices
    if current_user.role == "admin":
        return True

    # Get device internal id
    device = db.query(models.Device).filter(models.Device.device_id == device_id).first()

    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {device_id} not found"
        )

    # Check if user has access to this device
    access = db.query(models.UserDeviceAccess).filter(
        models.UserDeviceAccess.user_id == current_user.id,
        models.UserDeviceAccess.device_id == device.id
    ).first()

    if not access:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail=f"Access denied to device {device_id}"
        )

    return True
