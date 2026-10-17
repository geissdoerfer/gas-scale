"""
User management endpoints (admin only).
"""
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from typing import List

from src.database import get_db
from src.dependencies import require_admin, get_current_user
from src import models
from src import schemas
from src import auth

router = APIRouter(prefix="/users", tags=["User Management"])


@router.get("", response_model=List[schemas.UserResponse])
def list_users(
    skip: int = 0,
    limit: int = 100,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """List all users (admin only)"""
    users = db.query(models.User).offset(skip).limit(limit).all()
    return users


@router.post("", response_model=schemas.UserResponse, status_code=status.HTTP_201_CREATED)
def create_user(
    user_data: schemas.UserCreate,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """Create a new user (admin only)"""
    # Check if username already exists
    if db.query(models.User).filter(models.User.username == user_data.username).first():
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Username already exists"
        )

    # Check if email already exists
    if db.query(models.User).filter(models.User.email == user_data.email).first():
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Email already exists"
        )

    # Create new user
    hashed_password = auth.hash_password(user_data.password)
    new_user = models.User(
        username=user_data.username,
        email=user_data.email,
        password_hash=hashed_password,
        role=user_data.role.value
    )

    db.add(new_user)
    db.commit()
    db.refresh(new_user)

    return new_user


@router.get("/{user_id}", response_model=schemas.UserResponse)
def get_user(
    user_id: int,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """Get user by ID (admin only)"""
    user = db.query(models.User).filter(models.User.id == user_id).first()
    if not user:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="User not found"
        )
    return user


@router.put("/{user_id}", response_model=schemas.UserResponse)
def update_user(
    user_id: int,
    user_data: schemas.UserUpdate,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """Update user (admin only)"""
    user = db.query(models.User).filter(models.User.id == user_id).first()
    if not user:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="User not found"
        )

    # Update fields if provided
    if user_data.username:
        # Check if new username already exists
        existing = db.query(models.User).filter(
            models.User.username == user_data.username,
            models.User.id != user_id
        ).first()
        if existing:
            raise HTTPException(
                status_code=status.HTTP_400_BAD_REQUEST,
                detail="Username already exists"
            )
        user.username = user_data.username

    if user_data.email:
        # Check if new email already exists
        existing = db.query(models.User).filter(
            models.User.email == user_data.email,
            models.User.id != user_id
        ).first()
        if existing:
            raise HTTPException(
                status_code=status.HTTP_400_BAD_REQUEST,
                detail="Email already exists"
            )
        user.email = user_data.email

    if user_data.password:
        user.password_hash = auth.hash_password(user_data.password)

    if user_data.role:
        user.role = user_data.role.value

    db.commit()
    db.refresh(user)

    return user


@router.delete("/{user_id}", response_model=schemas.MessageResponse)
def delete_user(
    user_id: int,
    current_user: models.User = Depends(require_admin),
    db: Session = Depends(get_db)
):
    """Delete user (admin only)"""
    # Prevent deleting yourself
    if user_id == current_user.id:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Cannot delete your own account"
        )

    user = db.query(models.User).filter(models.User.id == user_id).first()
    if not user:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="User not found"
        )

    db.delete(user)
    db.commit()

    return {"message": "User deleted successfully"}
