"""
Tests for sensor readings and dashboard endpoints.
"""
import pytest
from datetime import datetime, timedelta
from src import models


@pytest.mark.readings
class TestGetDeviceReadings:
    """Test get device readings endpoint."""

    def test_get_readings_as_admin(
        self, client, auth_headers_admin, sample_device, db_session
    ):
        """Test getting device readings as admin."""
        # Create multiple readings
        base_time = datetime.utcnow()
        for i in range(5):
            reading = models.SensorReading(
                time=base_time - timedelta(hours=i),
                device_id=sample_device.device_id,
                load=40.0 + i,
                battery_voltage=12.0 + i * 0.1,
                temperature=20.0 + i
            )
            db_session.add(reading)
        db_session.commit()

        response = client.get(
            f"/devices/{sample_device.device_id}/readings",
            headers=auth_headers_admin
        )
        assert response.status_code == 200
        data = response.json()

        assert "readings" in data
        assert len(data["readings"]) == 5
        assert data["count"] == 5

    def test_get_readings_with_time_range(
        self, client, auth_headers_admin, sample_device, db_session
    ):
        """Test getting readings with time range filter."""
        base_time = datetime.utcnow()
        for i in range(10):
            reading = models.SensorReading(
                time=base_time - timedelta(hours=i),
                device_id=sample_device.device_id,
                load=40.0,
                battery_voltage=12.0,
                temperature=20.0
            )
            db_session.add(reading)
        db_session.commit()

        # Get only last 5 hours
        start_time = (base_time - timedelta(hours=5)).isoformat()
        end_time = base_time.isoformat()

        response = client.get(
            f"/devices/{sample_device.device_id}/readings",
            params={"start_time": start_time, "end_time": end_time},
            headers=auth_headers_admin
        )
        assert response.status_code == 200
        data = response.json()

        # Should get readings from last 5 hours (inclusive)
        assert len(data["readings"]) <= 6  # 0-5 hours = 6 readings

    def test_get_readings_with_limit(
        self, client, auth_headers_admin, sample_device, db_session
    ):
        """Test getting readings with limit."""
        base_time = datetime.utcnow()
        for i in range(10):
            reading = models.SensorReading(
                time=base_time - timedelta(hours=i),
                device_id=sample_device.device_id,
                load=40.0,
                battery_voltage=12.0,
                temperature=20.0
            )
            db_session.add(reading)
        db_session.commit()

        response = client.get(
            f"/devices/{sample_device.device_id}/readings",
            params={"limit": 3},
            headers=auth_headers_admin
        )
        assert response.status_code == 200
        data = response.json()

        assert len(data["readings"]) == 3

    def test_get_readings_as_user_with_access(
        self, client, auth_headers_user, regular_user, sample_device, sample_reading, db_session
    ):
        """Test getting readings as user with access."""
        # Assign device to user
        access = models.UserDeviceAccess(
            user_id=regular_user.id,
            device_id=sample_device.id
        )
        db_session.add(access)
        db_session.commit()

        response = client.get(
            f"/devices/{sample_device.device_id}/readings",
            headers=auth_headers_user
        )
        assert response.status_code == 200

    def test_get_readings_as_user_no_access(
        self, client, auth_headers_user, sample_device, sample_reading
    ):
        """Test that users cannot access readings for unassigned devices."""
        response = client.get(
            f"/devices/{sample_device.device_id}/readings",
            headers=auth_headers_user
        )
        assert response.status_code == 403


@pytest.mark.dashboard
class TestDashboard:
    """Test dashboard endpoint."""

    def test_dashboard_as_admin(
        self, client, auth_headers_admin, admin_user, sample_device, sample_reading
    ):
        """Test dashboard as admin."""
        response = client.get("/dashboard", headers=auth_headers_admin)
        assert response.status_code == 200
        data = response.json()

        assert "devices" in data
        assert "total" in data
        assert "user" in data
        assert data["user"]["username"] == "admin"
        assert isinstance(data["devices"], list)

    def test_dashboard_as_user_with_devices(
        self, client, auth_headers_user, regular_user, sample_device, sample_reading, db_session
    ):
        """Test dashboard as user with assigned devices."""
        # Assign device to user
        access = models.UserDeviceAccess(
            user_id=regular_user.id,
            device_id=sample_device.id
        )
        db_session.add(access)
        db_session.commit()

        response = client.get("/dashboard", headers=auth_headers_user)
        assert response.status_code == 200
        data = response.json()

        assert len(data["devices"]) == 1
        assert data["total"] == 1
        assert data["devices"][0]["device_id"] == "test_device_001"
        # Should include latest reading
        assert "latest_reading" in data["devices"][0]

    def test_dashboard_as_user_no_devices(self, client, auth_headers_user, regular_user):
        """Test dashboard as user with no assigned devices."""
        response = client.get("/dashboard", headers=auth_headers_user)
        assert response.status_code == 200
        data = response.json()

        assert len(data["devices"]) == 0
        assert data["total"] == 0

    def test_dashboard_no_auth(self, client):
        """Test that dashboard requires authentication."""
        response = client.get("/dashboard")
        assert response.status_code == 401


@pytest.mark.integration
class TestCompleteFlow:
    """Integration tests for complete user flows."""

    def test_complete_admin_flow(self, client, admin_user):
        """Test complete admin workflow."""
        # Login
        login_response = client.post(
            "/auth/login",
            json={"username": "admin", "password": "admin123"}
        )
        assert login_response.status_code == 200
        token = login_response.json()["access_token"]
        headers = {"Authorization": f"Bearer {token}"}

        # Create a user
        user_response = client.post(
            "/users",
            json={
                "username": "testuser2",
                "email": "testuser2@test.com",
                "password": "password123",
                "role": "user"
            },
            headers=headers
        )
        assert user_response.status_code == 201
        user_id = user_response.json()["id"]

        # Create a device
        device_response = client.post(
            "/devices",
            json={
                "device_id": "device_002",
                "name": "Test Device 2",
                "description": "Another test device"
            },
            headers=headers
        )
        assert device_response.status_code == 201
        device_string_id = device_response.json()["device_id"]

        # Assign device to user
        assign_response = client.post(
            f"/devices/{device_string_id}/assign",
            json={"user_id": user_id},
            headers=headers
        )
        assert assign_response.status_code == 200

        # Check dashboard
        dashboard_response = client.get("/dashboard", headers=headers)
        assert dashboard_response.status_code == 200
        assert dashboard_response.json()["total"] >= 1

    def test_complete_user_flow(self, client, regular_user, sample_device, sample_reading, db_session):
        """Test complete user workflow."""
        # Assign device to user
        access = models.UserDeviceAccess(
            user_id=regular_user.id,
            device_id=sample_device.id
        )
        db_session.add(access)
        db_session.commit()

        # Login
        login_response = client.post(
            "/auth/login",
            json={"username": "testuser", "password": "password123"}
        )
        assert login_response.status_code == 200
        token = login_response.json()["access_token"]
        headers = {"Authorization": f"Bearer {token}"}

        # View dashboard
        dashboard_response = client.get("/dashboard", headers=headers)
        assert dashboard_response.status_code == 200
        assert len(dashboard_response.json()["devices"]) == 1

        # View device details
        device_response = client.get(
            f"/devices/{sample_device.device_id}",
            headers=headers
        )
        assert device_response.status_code == 200

        # View device readings
        readings_response = client.get(
            f"/devices/{sample_device.device_id}/readings",
            headers=headers
        )
        assert readings_response.status_code == 200
        assert len(readings_response.json()["readings"]) >= 1
