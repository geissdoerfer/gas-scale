"""
Tests for device management endpoints.
"""
import pytest
from src import models


@pytest.mark.devices
class TestListDevices:
    """Test list devices endpoint."""

    def test_list_devices_as_admin(self, client, auth_headers_admin, sample_device):
        """Test listing devices as admin."""
        response = client.get("/devices", headers=auth_headers_admin)
        assert response.status_code == 200
        devices = response.json()

        assert isinstance(devices, list)
        assert len(devices) >= 1

    def test_list_devices_as_user_with_access(
        self, client, auth_headers_user, regular_user, sample_device, db_session
    ):
        """Test listing devices as user with assigned devices."""
        # Assign device to user
        access = models.UserDeviceAccess(
            user_id=regular_user.id,
            device_id=sample_device.id
        )
        db_session.add(access)
        db_session.commit()

        response = client.get("/devices", headers=auth_headers_user)
        assert response.status_code == 200
        devices = response.json()

        assert len(devices) == 1
        assert devices[0]["device_id"] == "test_device_001"

    def test_list_devices_as_user_no_access(self, client, auth_headers_user, sample_device):
        """Test that users only see their assigned devices."""
        response = client.get("/devices", headers=auth_headers_user)
        assert response.status_code == 200
        devices = response.json()

        assert len(devices) == 0  # User has no assigned devices


@pytest.mark.devices
class TestGetDevice:
    """Test get single device endpoint."""

    def test_get_device_as_admin(self, client, auth_headers_admin, sample_device):
        """Test getting device details as admin."""
        response = client.get(f"/devices/{sample_device.device_id}", headers=auth_headers_admin)
        assert response.status_code == 200
        data = response.json()

        assert data["device_id"] == "test_device_001"
        assert data["name"] == "Test Device"

    def test_get_device_as_user_with_access(
        self, client, auth_headers_user, regular_user, sample_device, db_session
    ):
        """Test getting device as user with access."""
        # Assign device to user
        access = models.UserDeviceAccess(
            user_id=regular_user.id,
            device_id=sample_device.id
        )
        db_session.add(access)
        db_session.commit()

        response = client.get(f"/devices/{sample_device.device_id}", headers=auth_headers_user)
        assert response.status_code == 200

    def test_get_device_as_user_no_access(self, client, auth_headers_user, sample_device):
        """Test that users cannot access unassigned devices."""
        response = client.get(f"/devices/{sample_device.device_id}", headers=auth_headers_user)
        assert response.status_code == 403

    def test_get_nonexistent_device(self, client, auth_headers_admin):
        """Test getting non-existent device."""
        response = client.get("/devices/99999", headers=auth_headers_admin)
        assert response.status_code == 404


@pytest.mark.devices
class TestCreateDevice:
    """Test create device endpoint."""

    def test_create_device_as_admin(self, client, auth_headers_admin):
        """Test creating a device as admin."""
        device_data = {
            "device_id": "new_device_001",
            "name": "New Device",
            "description": "A new test device"
        }
        response = client.post("/devices", json=device_data, headers=auth_headers_admin)
        assert response.status_code == 201
        data = response.json()

        assert data["device_id"] == "new_device_001"
        assert data["name"] == "New Device"

    def test_create_device_duplicate_id(self, client, auth_headers_admin, sample_device):
        """Test creating device with duplicate ID."""
        device_data = {
            "device_id": "test_device_001",  # Already exists
            "name": "Duplicate",
            "description": "Duplicate device"
        }
        response = client.post("/devices", json=device_data, headers=auth_headers_admin)
        assert response.status_code == 400

    def test_create_device_as_regular_user(self, client, auth_headers_user):
        """Test that regular users cannot create devices."""
        device_data = {
            "device_id": "new_device_001",
            "name": "New Device"
        }
        response = client.post("/devices", json=device_data, headers=auth_headers_user)
        assert response.status_code == 403


@pytest.mark.devices
class TestAssignDevice:
    """Test device assignment endpoint."""

    def test_assign_device_to_user(
        self, client, auth_headers_admin, sample_device, regular_user
    ):
        """Test assigning device to user."""
        assignment_data = {"user_id": regular_user.id}
        response = client.post(
            f"/devices/{sample_device.device_id}/assign",
            json=assignment_data,
            headers=auth_headers_admin
        )
        assert response.status_code == 200
        data = response.json()

        assert data["message"] == "Device assigned successfully"

    def test_assign_device_nonexistent_user(self, client, auth_headers_admin, sample_device):
        """Test assigning device to non-existent user."""
        assignment_data = {"user_id": 99999}
        response = client.post(
            f"/devices/{sample_device.device_id}/assign",
            json=assignment_data,
            headers=auth_headers_admin
        )
        assert response.status_code == 404

    def test_assign_device_as_regular_user(self, client, auth_headers_user, sample_device, regular_user):
        """Test that regular users cannot assign devices."""
        assignment_data = {"user_id": regular_user.id}
        response = client.post(
            f"/devices/{sample_device.device_id}/assign",
            json=assignment_data,
            headers=auth_headers_user
        )
        assert response.status_code == 403


@pytest.mark.devices
class TestUnassignDevice:
    """Test device unassignment endpoint."""

    def test_unassign_device_from_user(
        self, client, auth_headers_admin, sample_device, regular_user, db_session
    ):
        """Test unassigning device from user."""
        # First assign it
        access = models.UserDeviceAccess(
            user_id=regular_user.id,
            device_id=sample_device.id
        )
        db_session.add(access)
        db_session.commit()

        # Now unassign
        response = client.delete(
            f"/devices/{sample_device.device_id}/unassign/{regular_user.id}",
            headers=auth_headers_admin
        )
        assert response.status_code == 200

    def test_unassign_device_not_assigned(
        self, client, auth_headers_admin, sample_device, regular_user
    ):
        """Test unassigning device that's not assigned."""
        response = client.delete(
            f"/devices/{sample_device.device_id}/unassign/{regular_user.id}",
            headers=auth_headers_admin
        )
        assert response.status_code == 404

    def test_unassign_device_as_regular_user(
        self, client, auth_headers_user, sample_device, regular_user
    ):
        """Test that regular users cannot unassign devices."""
        response = client.delete(
            f"/devices/{sample_device.device_id}/unassign/{regular_user.id}",
            headers=auth_headers_user
        )
        assert response.status_code == 403


@pytest.mark.devices
class TestGetLatestReading:
    """Test get latest reading endpoint."""

    def test_get_latest_reading_as_admin(
        self, client, auth_headers_admin, sample_device, sample_reading
    ):
        """Test getting latest reading as admin."""
        response = client.get(
            f"/devices/{sample_device.device_id}/latest",
            headers=auth_headers_admin
        )
        assert response.status_code == 200
        data = response.json()

        assert data["device_id"] == "test_device_001"
        assert data["load"] == 45.2
        assert data["battery_voltage"] == 12.6
        assert data["temperature"] == 23.5

    def test_get_latest_reading_no_data(self, client, auth_headers_admin, sample_device):
        """Test getting latest reading when no data exists."""
        response = client.get(
            f"/devices/{sample_device.device_id}/latest",
            headers=auth_headers_admin
        )
        assert response.status_code == 404
