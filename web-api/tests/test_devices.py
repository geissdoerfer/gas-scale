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

    def test_create_device_with_calibration(self, client, auth_headers_admin):
        """Offset/gain supplied at creation time must actually be persisted, not silently
        dropped in favor of the column defaults."""
        device_data = {
            "device_id": "calibrated_device_001",
            "name": "Pre-calibrated device",
            "offset": -256620.0,
            "gain": -0.09121,
        }
        response = client.post("/devices", json=device_data, headers=auth_headers_admin)
        assert response.status_code == 201
        data = response.json()

        assert data["offset"] == -256620.0
        assert data["gain"] == -0.09121

    def test_create_device_default_calibration(self, client, auth_headers_admin):
        """Omitting offset/gain at creation time means 'not calibrated yet' (0.0/1.0)."""
        device_data = {"device_id": "uncalibrated_device_001"}
        response = client.post("/devices", json=device_data, headers=auth_headers_admin)
        assert response.status_code == 201
        data = response.json()

        assert data["offset"] == 0.0
        assert data["gain"] == 1.0

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
        assert data["raw_value"] == 1500.0
        assert data["weight"] == 1500.0  # sample_device has default offset=0/gain=1
        assert data["battery_voltage"] == 12.6
        assert data["temperature"] == 23.5

    def test_get_latest_reading_no_data(self, client, auth_headers_admin, sample_device):
        """Test getting latest reading when no data exists."""
        response = client.get(
            f"/devices/{sample_device.device_id}/latest",
            headers=auth_headers_admin
        )
        assert response.status_code == 404


@pytest.mark.devices
class TestCalibration:
    """Test the PATCH /devices/{device_id}/calibration endpoint."""

    def test_set_calibration(self, client, auth_headers_admin, sample_device):
        response = client.patch(
            f"/devices/{sample_device.device_id}/calibration",
            json={"offset": 10.0, "gain": 2.0},
            headers=auth_headers_admin,
        )
        assert response.status_code == 200
        data = response.json()
        assert data["offset"] == 10.0
        assert data["gain"] == 2.0

    def test_set_negative_gain(self, client, auth_headers_admin, sample_device):
        """A load cell can legitimately need a negative gain to invert its polarity -
        this must not be rejected the way a plain gt=0 constraint would."""
        response = client.patch(
            f"/devices/{sample_device.device_id}/calibration",
            json={"offset": -256620.0, "gain": -0.09121},
            headers=auth_headers_admin,
        )
        assert response.status_code == 200
        data = response.json()
        assert data["offset"] == -256620.0
        assert data["gain"] == -0.09121

    def test_set_zero_gain_rejected(self, client, auth_headers_admin, sample_device):
        response = client.patch(
            f"/devices/{sample_device.device_id}/calibration",
            json={"offset": 0.0, "gain": 0.0},
            headers=auth_headers_admin,
        )
        assert response.status_code == 422

    def test_calibration_requires_admin(self, client, auth_headers_user, sample_device):
        response = client.patch(
            f"/devices/{sample_device.device_id}/calibration",
            json={"offset": 10.0, "gain": 2.0},
            headers=auth_headers_user,
        )
        assert response.status_code == 403
