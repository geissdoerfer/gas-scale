"""
Authentication endpoints for login and token management.
"""
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from jose import JWTError

from src.database import get_db
from src.dependencies import get_current_user
from src import models
from src import schemas
from src import auth

router = APIRouter(prefix="/auth", tags=["Authentication"])


@router.post("/login", response_model=schemas.TokenResponse)
def login(
    credentials: schemas.LoginRequest,
    db: Session = Depends(get_db)
):
    """
    Authenticate user and return JWT tokens.

    **Parameters:**
    - username: User's username
    - password: User's password

    **Returns:**
    - access_token: Short-lived JWT for API requests (1 hour)
    - refresh_token: Long-lived JWT for refreshing access token (7 days)
    - user: User information
    """
    # Find user by username
    user = db.query(models.User).filter(models.User.username == credentials.username).first()

    if not user:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password"
        )

    # Verify password
    if not auth.verify_password(credentials.password, user.password_hash):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password"
        )

    # Create tokens
    token_data = {
        "sub": str(user.id),  # JWT spec requires sub to be a string
        "username": user.username,
        "role": user.role
    }

    access_token = auth.create_access_token(token_data)
    refresh_token = auth.create_refresh_token({"sub": str(user.id)})

    return {
        "access_token": access_token,
        "refresh_token": refresh_token,
        "token_type": "bearer",
        "user": user
    }


@router.post("/refresh", response_model=schemas.TokenRefreshResponse)
def refresh_token(
    request: schemas.RefreshTokenRequest,
    db: Session = Depends(get_db)
):
    """
    Refresh an access token using a refresh token.

    **Parameters:**
    - refresh_token: Valid refresh token

    **Returns:**
    - access_token: New access token
    """
    try:
        payload = auth.decode_token(request.refresh_token)

        # Verify it's a refresh token
        if payload.get("type") != "refresh":
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Invalid refresh token"
            )

        user_id = payload.get("sub")
        if user_id is None:
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Invalid refresh token"
            )

        # Verify user still exists
        user = db.query(models.User).filter(models.User.id == user_id).first()
        if not user:
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="User not found"
            )

        # Create new access token
        token_data = {
            "sub": str(user.id),  # JWT spec requires sub to be a string
            "username": user.username,
            "role": user.role
        }
        access_token = auth.create_access_token(token_data)

        return {
            "access_token": access_token,
            "token_type": "bearer"
        }

    except JWTError:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid refresh token"
        )


@router.get("/me", response_model=schemas.UserResponse)
def get_current_user_info(current_user: models.User = Depends(get_current_user)):
    """
    Get current authenticated user information.

    **Returns:**
    - User information
    """
    return current_user
