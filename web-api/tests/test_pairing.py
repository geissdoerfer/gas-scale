"""
Tests for the account-less device pairing endpoints (/devices/pair, /devices/me/*).
"""
import pytest
from datetime import datetime

from src import models


@pytest.fixture
def device_token(client, sample_device):
    """Pair with sample_device and return its device-scoped access token."""
    response = client.post(
        "/devices/pair",
        json={"device_id": sample_device.device_id, "secret": sample_device.pairing_secret},
    )
    return response.json()["access_token"]


@pytest.fixture
def device_auth_headers(device_token):
    return {"Authorization": f"Bearer {device_token}"}


@pytest.mark.devices
class TestPairDevice:
    def test_pair_with_correct_secret(self, client, sample_device):
        response = client.post(
            "/devices/pair",
            json={"device_id": sample_device.device_id, "secret": sample_device.pairing_secret},
        )
        assert response.status_code == 200
        data = response.json()
        assert data["device_id"] == sample_device.device_id
        assert data["token_type"] == "bearer"
        assert data["access_token"]

    def test_pair_with_wrong_secret(self, client, sample_device):
        response = client.post(
            "/devices/pair",
            json={"device_id": sample_device.device_id, "secret": "wrong-secret"},
        )
        assert response.status_code == 401

    def test_pair_with_unknown_device(self, client):
        response = client.post(
            "/devices/pair",
            json={"device_id": "does-not-exist", "secret": "whatever"},
        )
        assert response.status_code == 401

    def test_device_token_rejected_by_user_endpoints(self, client, device_token):
        """A device-scoped token must not work as a user token."""
        response = client.get("/devices", headers={"Authorization": f"Bearer {device_token}"})
        assert response.status_code == 401

    def test_user_token_rejected_by_device_endpoints(self, client, auth_headers_admin):
        """A user token must not work as a device token."""
        response = client.get("/devices/me/status", headers=auth_headers_admin)
        assert response.status_code == 401


@pytest.mark.devices
class TestCylinderType:
    def test_set_cylinder_type(self, client, device_auth_headers):
        response = client.patch(
            "/devices/me/cylinder",
            json={"cylinder_type": "12kg"},
            headers=device_auth_headers,
        )
        assert response.status_code == 200
        data = response.json()
        assert data["cylinder_type"] == "12kg"
        assert data["status"] == "no_data"  # no sensor reading yet

    def test_set_invalid_cylinder_type(self, client, device_auth_headers):
        response = client.patch(
            "/devices/me/cylinder",
            json={"cylinder_type": "9001kg"},
            headers=device_auth_headers,
        )
        assert response.status_code == 422

    def test_set_cylinder_type_requires_device_token(self, client):
        response = client.patch("/devices/me/cylinder", json={"cylinder_type": "12kg"})
        assert response.status_code in (401, 403)


@pytest.mark.devices
class TestDeviceStatus:
    def test_status_not_configured(self, client, device_auth_headers):
        response = client.get("/devices/me/status", headers=device_auth_headers)
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "not_configured"
        assert data["cylinder_type"] is None
        assert data["percentage_remaining"] is None

    def test_status_no_data(self, client, device_auth_headers):
        client.patch("/devices/me/cylinder", json={"cylinder_type": "12kg"}, headers=device_auth_headers)
        response = client.get("/devices/me/status", headers=device_auth_headers)
        data = response.json()
        assert data["status"] == "no_data"
        assert data["percentage_remaining"] is None

    def test_status_with_reading(self, client, device_auth_headers, sample_device, db_session):
        client.patch("/devices/me/cylinder", json={"cylinder_type": "12kg"}, headers=device_auth_headers)

        # 12kg cylinder spec (web-api/src/cylinder_specs.py): tare=14.0, full=26.0.
        # raw_value passes straight through since offset=0.0/gain=1.0 by default.
        reading = models.SensorReading(
            time=datetime.utcnow(),
            device_id=sample_device.device_id,
            raw_value=20.0,  # halfway between tare and full
            battery_voltage=12.6,
            temperature=23.0,
        )
        db_session.add(reading)
        db_session.commit()

        response = client.get("/devices/me/status", headers=device_auth_headers)
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "ok"
        assert data["percentage_remaining"] == 50
        assert data["battery_percentage"] is not None
        assert data["battery_voltage"] == 12.6

    def test_status_low_battery(self, client, device_auth_headers, sample_device, db_session):
        client.patch("/devices/me/cylinder", json={"cylinder_type": "12kg"}, headers=device_auth_headers)

        reading = models.SensorReading(
            time=datetime.utcnow(),
            device_id=sample_device.device_id,
            raw_value=20.0,
            battery_voltage=10.0,
            temperature=23.0,
        )
        db_session.add(reading)
        db_session.commit()

        response = client.get("/devices/me/status", headers=device_auth_headers)
        data = response.json()
        assert data["status"] == "low_battery"
