"""
Pytest configuration and fixtures for Web API tests.
"""
import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool

from src.main import app
from src.database import Base, get_db
from src.auth import hash_password
from src import models

# Use in-memory SQLite for tests
SQLALCHEMY_DATABASE_URL = "sqlite:///:memory:"

engine = create_engine(
    SQLALCHEMY_DATABASE_URL,
    connect_args={"check_same_thread": False},
    poolclass=StaticPool,
)
TestingSessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)


@pytest.fixture(scope="function")
def db_session():
    """Create a fresh database session for each test."""
    Base.metadata.create_all(bind=engine)
    session = TestingSessionLocal()
    try:
        yield session
    finally:
        session.close()
        Base.metadata.drop_all(bind=engine)


@pytest.fixture(scope="function")
def client(db_session):
    """Create a test client with database session override."""
    def override_get_db():
        try:
            yield db_session
        finally:
            pass

    app.dependency_overrides[get_db] = override_get_db
    with TestClient(app) as test_client:
        yield test_client
    app.dependency_overrides.clear()


@pytest.fixture
def admin_user(db_session):
    """Create an admin user for testing."""
    user = models.User(
        username="admin",
        email="admin@test.com",
        password_hash=hash_password("admin123"),
        role="admin"
    )
    db_session.add(user)
    db_session.commit()
    db_session.refresh(user)
    return user


@pytest.fixture
def regular_user(db_session):
    """Create a regular user for testing."""
    user = models.User(
        username="testuser",
        email="testuser@test.com",
        password_hash=hash_password("password123"),
        role="user"
    )
    db_session.add(user)
    db_session.commit()
    db_session.refresh(user)
    return user


@pytest.fixture
def admin_token(client, admin_user):
    """Get admin authentication token."""
    response = client.post(
        "/auth/login",
        json={"username": "admin", "password": "admin123"}
    )
    return response.json()["access_token"]


@pytest.fixture
def user_token(client, regular_user):
    """Get regular user authentication token."""
    response = client.post(
        "/auth/login",
        json={"username": "testuser", "password": "password123"}
    )
    return response.json()["access_token"]


@pytest.fixture
def auth_headers_admin(admin_token):
    """Get authorization headers for admin user."""
    return {"Authorization": f"Bearer {admin_token}"}


@pytest.fixture
def auth_headers_user(user_token):
    """Get authorization headers for regular user."""
    return {"Authorization": f"Bearer {user_token}"}


@pytest.fixture
def sample_device(db_session):
    """Create a sample device for testing."""
    device = models.Device(
        device_id="test_device_001",
        name="Test Device",
        description="A test device"
    )
    db_session.add(device)
    db_session.commit()
    db_session.refresh(device)
    return device


@pytest.fixture
def sample_reading(db_session, sample_device):
    """Create a sample sensor reading for testing."""
    from datetime import datetime
    reading = models.SensorReading(
        time=datetime.utcnow(),
        device_id=sample_device.device_id,
        load=45.2,
        battery_voltage=12.6,
        temperature=23.5
    )
    db_session.add(reading)
    db_session.commit()
    db_session.refresh(reading)
    return reading
