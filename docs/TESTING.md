# Testing Strategy

## Overview

This document outlines the comprehensive testing strategy for the DuoClean Energy IoT monitoring platform, covering unit tests, integration tests, end-to-end tests, and manual testing procedures.

## Testing Pyramid

```
        /\
       /  \      E2E Tests (Few, Slow, High Value)
      /____\
     /      \    Integration Tests (Some, Medium Speed)
    /________\
   /          \  Unit Tests (Many, Fast, Focused)
  /____________\
```

## Test Environments

### Local Development
- Docker Compose setup on developer machine
- Local MQTT broker
- Test database (separate from dev data)

### CI/CD Pipeline
- Automated tests on every commit
- GitHub Actions, GitLab CI, or similar
- Test containers spin up for each test run

### Staging
- Production-like environment
- Full dataset (anonymized)
- Performance testing

### Production
- Monitoring and alerting
- Canary deployments
- Smoke tests after deployment

## Unit Tests

### Database Layer (SQLAlchemy Models)

**Tools:** pytest, pytest-asyncio

**Test file:** `web-api/tests/test_models.py`

```python
import pytest
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from src.models import User, Device, UserDeviceAccess, Base

@pytest.fixture
def db_session():
    # Create in-memory SQLite database for testing
    engine = create_engine("sqlite:///:memory:")
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)
    session = Session()
    yield session
    session.close()

def test_create_user(db_session):
    user = User(
        username="testuser",
        email="test@example.com",
        password_hash="hashed_password",
        role="user"
    )
    db_session.add(user)
    db_session.commit()

    assert user.id is not None
    assert user.username == "testuser"

def test_user_device_relationship(db_session):
    user = User(username="testuser", email="test@example.com", password_hash="hash", role="user")
    device = Device(device_id="device_001", name="Test Device")

    db_session.add_all([user, device])
    db_session.commit()

    # Create association
    access = UserDeviceAccess(user_id=user.id, device_id=device.id)
    db_session.add(access)
    db_session.commit()

    # Test relationship
    assert len(user.devices) == 1
    assert user.devices[0].device_id == "device_001"
```

### Authentication (Password Hashing, JWT)

**Test file:** `web-api/tests/test_auth.py`

```python
import pytest
from src.auth import verify_password, hash_password, create_access_token, decode_token

def test_password_hashing():
    password = "SecurePassword123"
    hashed = hash_password(password)

    assert hashed != password
    assert verify_password(password, hashed) is True
    assert verify_password("WrongPassword", hashed) is False

def test_jwt_token_creation():
    data = {"sub": "user_id", "username": "testuser", "role": "user"}
    token = create_access_token(data)

    assert token is not None
    assert isinstance(token, str)

    # Decode and verify
    decoded = decode_token(token)
    assert decoded["username"] == "testuser"
    assert decoded["role"] == "user"

def test_jwt_token_expiration():
    from datetime import timedelta
    data = {"sub": "user_id"}
    token = create_access_token(data, expires_delta=timedelta(seconds=-1))

    # Token should be expired
    with pytest.raises(Exception):
        decode_token(token)
```

### MQTT Message Validation

**Test file:** `mqtt-ingestor/tests/test_mqtt_client.py`

```python
import pytest
from src.mqtt_client import MQTTIngestor

@pytest.fixture
def ingestor():
    # Create ingestor with mock db_writer
    return MQTTIngestor(db_writer=None)

def test_validate_valid_message(ingestor):
    payload = {
        "device_id": "device_001",
        "load": 45.2,
        "battery_voltage": 12.6,
        "temperature": 23.5
    }
    assert ingestor._validate_message(payload, "device_001") is True

def test_validate_missing_device_id(ingestor):
    payload = {"load": 45.2}
    assert ingestor._validate_message(payload, "device_001") is False

def test_validate_device_id_mismatch(ingestor):
    payload = {"device_id": "device_001"}
    assert ingestor._validate_message(payload, "device_002") is False

def test_validate_invalid_sensor_value(ingestor):
    payload = {
        "device_id": "device_001",
        "load": "not_a_number"
    }
    assert ingestor._validate_message(payload, "device_001") is False

def test_validate_partial_reading(ingestor):
    # Only one sensor value is valid
    payload = {
        "device_id": "device_001",
        "temperature": 23.5
    }
    assert ingestor._validate_message(payload, "device_001") is True
```

### API Endpoints

**Test file:** `web-api/tests/test_api.py`

```python
import pytest
from fastapi.testclient import TestClient
from src.main import app

client = TestClient(app)

def test_login_success():
    response = client.post("/auth/login", json={
        "username": "admin",
        "password": "admin123"
    })
    assert response.status_code == 200
    data = response.json()
    assert "access_token" in data
    assert "refresh_token" in data

def test_login_invalid_credentials():
    response = client.post("/auth/login", json={
        "username": "admin",
        "password": "wrongpassword"
    })
    assert response.status_code == 401

def test_get_devices_unauthorized():
    response = client.get("/devices")
    assert response.status_code == 401

def test_get_devices_authorized(auth_token):
    headers = {"Authorization": f"Bearer {auth_token}"}
    response = client.get("/devices", headers=headers)
    assert response.status_code == 200
    data = response.json()
    assert "devices" in data

@pytest.fixture
def auth_token():
    # Login and return token
    response = client.post("/auth/login", json={
        "username": "admin",
        "password": "admin123"
    })
    return response.json()["access_token"]
```

## Integration Tests

### Database + MQTT Ingestor

**Goal:** Test that messages are correctly written to database

**Test file:** `tests/integration/test_mqtt_to_db.py`

```python
import pytest
import paho.mqtt.publish as publish
import psycopg2
from time import sleep

@pytest.fixture(scope="module")
def db_connection():
    conn = psycopg2.connect(
        host="localhost",
        database="duoclean_test",
        user="duoclean_user",
        password="testpass"
    )
    yield conn
    conn.close()

def test_mqtt_message_stored_in_database(db_connection):
    # Publish MQTT message
    message = {
        "device_id": "test_device_integration",
        "load": 50.0,
        "battery_voltage": 12.5,
        "temperature": 22.0
    }

    publish.single(
        "sensors/test_device_integration/data",
        payload=json.dumps(message),
        hostname="localhost",
        port=1883
    )

    # Wait for ingestor to process
    sleep(2)

    # Check database
    cursor = db_connection.cursor()
    cursor.execute(
        "SELECT * FROM sensor_readings WHERE device_id = %s",
        ("test_device_integration",)
    )
    result = cursor.fetchone()

    assert result is not None
    assert result[2] == 50.0  # load
    assert result[3] == 12.5  # battery_voltage
    assert result[4] == 22.0  # temperature

def test_device_auto_registration(db_connection):
    device_id = "auto_registered_device"

    # Publish message for new device
    publish.single(
        f"sensors/{device_id}/data",
        payload=json.dumps({"device_id": device_id, "load": 10.0}),
        hostname="localhost"
    )

    sleep(2)

    # Check if device was auto-registered
    cursor = db_connection.cursor()
    cursor.execute("SELECT * FROM devices WHERE device_id = %s", (device_id,))
    result = cursor.fetchone()

    assert result is not None
```

### API + Database

**Goal:** Test API endpoints with real database

**Test file:** `tests/integration/test_api_db.py`

```python
import pytest
from fastapi.testclient import TestClient
from src.main import app

client = TestClient(app)

@pytest.fixture(scope="module")
def admin_token():
    response = client.post("/auth/login", json={
        "username": "admin",
        "password": "admin123"
    })
    return response.json()["access_token"]

def test_create_user(admin_token):
    headers = {"Authorization": f"Bearer {admin_token}"}
    response = client.post("/users", headers=headers, json={
        "username": "newuser",
        "email": "newuser@example.com",
        "password": "SecurePass123",
        "role": "user"
    })

    assert response.status_code == 201
    data = response.json()
    assert data["username"] == "newuser"
    assert "password" not in data  # Password should not be returned

def test_assign_device_to_user(admin_token):
    headers = {"Authorization": f"Bearer {admin_token}"}

    # Create device
    device_response = client.post("/devices", headers=headers, json={
        "device_id": "test_device_001",
        "name": "Test Device"
    })
    assert device_response.status_code == 201

    # Create user
    user_response = client.post("/users", headers=headers, json={
        "username": "testuser2",
        "email": "testuser2@example.com",
        "password": "Pass123",
        "role": "user"
    })
    user_id = user_response.json()["id"]

    # Assign device to user
    assign_response = client.post(
        "/devices/test_device_001/assign",
        headers=headers,
        json={"user_id": user_id}
    )
    assert assign_response.status_code == 200

def test_user_can_only_access_assigned_devices():
    # Login as regular user
    user_response = client.post("/auth/login", json={
        "username": "testuser2",
        "password": "Pass123"
    })
    user_token = user_response.json()["access_token"]

    headers = {"Authorization": f"Bearer {user_token}"}

    # Get devices
    devices_response = client.get("/devices", headers=headers)
    devices = devices_response.json()["devices"]

    # Should only see assigned device
    assert len(devices) == 1
    assert devices[0]["device_id"] == "test_device_001"
```

## End-to-End Tests

### Full System Flow

**Goal:** Test complete data flow from IoT device to frontend

**Test file:** `tests/e2e/test_full_flow.py`

```python
import pytest
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import paho.mqtt.publish as publish
import json
from time import sleep

@pytest.fixture(scope="module")
def browser():
    driver = webdriver.Chrome()  # or Firefox
    driver.implicitly_wait(10)
    yield driver
    driver.quit()

def test_complete_flow(browser):
    # Step 1: Publish MQTT message
    device_id = "e2e_test_device"
    message = {
        "device_id": device_id,
        "load": 75.5,
        "battery_voltage": 13.2,
        "temperature": 25.0
    }

    publish.single(
        f"sensors/{device_id}/data",
        payload=json.dumps(message),
        hostname="localhost"
    )

    sleep(3)  # Wait for ingestion

    # Step 2: Login to web UI
    browser.get("http://localhost:3000")

    username_input = browser.find_element(By.ID, "username")
    password_input = browser.find_element(By.ID, "password")
    login_button = browser.find_element(By.ID, "login-button")

    username_input.send_keys("admin")
    password_input.send_keys("admin123")
    login_button.click()

    # Wait for redirect to dashboard
    WebDriverWait(browser, 10).until(
        EC.url_contains("dashboard.html")
    )

    # Step 3: Verify device appears on dashboard
    device_card = WebDriverWait(browser, 10).until(
        EC.presence_of_element_located((By.CSS_SELECTOR, f"[data-device-id='{device_id}']"))
    )

    assert device_card is not None
    assert "75.5" in device_card.text  # Load value
    assert "13.2" in device_card.text  # Battery voltage

    # Step 4: Click device to view details
    device_card.click()

    WebDriverWait(browser, 10).until(
        EC.url_contains("device-detail.html")
    )

    # Step 5: Verify charts loaded
    load_chart = browser.find_element(By.ID, "load-chart")
    assert load_chart.is_displayed()
```

### Admin Workflow

**Test file:** `tests/e2e/test_admin_workflow.py`

```python
def test_admin_create_user_and_assign_device(browser):
    # Login as admin
    browser.get("http://localhost:3000")
    # ... login steps ...

    # Navigate to admin panel
    admin_button = browser.find_element(By.ID, "admin-button")
    admin_button.click()

    # Create new user
    username_input = browser.find_element(By.ID, "new-user-username")
    email_input = browser.find_element(By.ID, "new-user-email")
    password_input = browser.find_element(By.ID, "new-user-password")
    create_button = browser.find_element(By.ID, "create-user-button")

    username_input.send_keys("selenium_test_user")
    email_input.send_keys("selenium@example.com")
    password_input.send_keys("TestPass123")
    create_button.click()

    # Wait for success message
    WebDriverWait(browser, 5).until(
        EC.presence_of_element_located((By.CLASS_NAME, "success-message"))
    )

    # Switch to assignments tab
    assignments_tab = browser.find_element(By.ID, "assignments-tab")
    assignments_tab.click()

    # Select user
    user_select = browser.find_element(By.ID, "user-select")
    user_select.select_by_visible_text("selenium_test_user")

    # Assign device
    device_checkbox = browser.find_element(By.CSS_SELECTOR, "[data-device='e2e_test_device']")
    device_checkbox.click()

    assign_button = browser.find_element(By.ID, "assign-button")
    assign_button.click()

    # Verify assignment
    assigned_devices = browser.find_elements(By.CLASS_NAME, "assigned-device")
    assert len(assigned_devices) > 0
```

## Performance Tests

### API Load Testing

**Tool:** Locust

**Test file:** `tests/performance/locustfile.py`

```python
from locust import HttpUser, task, between

class APIUser(HttpUser):
    wait_time = between(1, 3)

    def on_start(self):
        # Login
        response = self.client.post("/auth/login", json={
            "username": "admin",
            "password": "admin123"
        })
        self.token = response.json()["access_token"]
        self.headers = {"Authorization": f"Bearer {self.token}"}

    @task(3)
    def get_dashboard(self):
        self.client.get("/dashboard", headers=self.headers)

    @task(2)
    def get_devices(self):
        self.client.get("/devices", headers=self.headers)

    @task(1)
    def get_device_readings(self):
        self.client.get(
            "/devices/device_001/readings?start_time=2026-01-20T00:00:00Z&end_time=2026-01-21T00:00:00Z",
            headers=self.headers
        )
```

**Run test:**
```bash
locust -f tests/performance/locustfile.py --host=http://localhost:8000 --users=100 --spawn-rate=10
```

**Metrics to monitor:**
- Requests per second
- Response time (p50, p95, p99)
- Error rate
- Database query time
- Memory usage
- CPU usage

### MQTT Throughput Testing

**Tool:** Custom Python script

**Test file:** `tests/performance/test_mqtt_throughput.py`

```python
import paho.mqtt.publish as publish
import json
import time
from concurrent.futures import ThreadPoolExecutor

def publish_message(device_id, count):
    message = {
        "device_id": device_id,
        "load": 50.0,
        "battery_voltage": 12.5,
        "temperature": 22.0
    }

    for i in range(count):
        publish.single(
            f"sensors/{device_id}/data",
            payload=json.dumps(message),
            hostname="localhost"
        )

def test_mqtt_throughput():
    num_devices = 100
    messages_per_device = 100

    start_time = time.time()

    with ThreadPoolExecutor(max_workers=10) as executor:
        futures = []
        for i in range(num_devices):
            device_id = f"perf_test_device_{i:03d}"
            future = executor.submit(publish_message, device_id, messages_per_device)
            futures.append(future)

        # Wait for all to complete
        for future in futures:
            future.result()

    end_time = time.time()

    total_messages = num_devices * messages_per_device
    duration = end_time - start_time
    throughput = total_messages / duration

    print(f"Published {total_messages} messages in {duration:.2f}s")
    print(f"Throughput: {throughput:.2f} messages/second")

    # Verify in database
    # ...
```

## Manual Testing

### Test Cases

#### TC01: User Login
1. Navigate to http://localhost:3000
2. Enter username: admin
3. Enter password: admin123
4. Click Login
5. **Expected:** Redirect to dashboard

#### TC02: View Dashboard
1. Login as admin
2. **Expected:** See all devices with latest readings
3. Wait 30 seconds
4. **Expected:** Dashboard auto-refreshes

#### TC03: View Device Details
1. Login as admin
2. Click on a device card
3. **Expected:** Navigate to device detail page
4. **Expected:** See latest readings (large numbers)
5. **Expected:** See three charts (load, voltage, temperature)
6. Click "7d" time range
7. **Expected:** Charts update with 7-day data

#### TC04: Admin Create User
1. Login as admin
2. Click "Admin Panel"
3. Fill in new user form
4. Click "Create User"
5. **Expected:** User appears in users table
6. **Expected:** Success message displayed

#### TC05: Admin Assign Device
1. Login as admin
2. Go to Admin Panel → Assignments tab
3. Select a user from dropdown
4. Click checkbox next to unassigned device
5. Click "Assign"
6. **Expected:** Device moves to assigned list

#### TC06: User Permission Check
1. Create test user via admin panel
2. Assign one device to test user
3. Logout
4. Login as test user
5. **Expected:** Dashboard shows only assigned device
6. Try to access admin panel directly (admin.html)
7. **Expected:** Redirect to dashboard

#### TC07: MQTT Data Ingestion
1. Publish test MQTT message:
   ```bash
   mosquitto_pub -t sensors/manual_test/data -m '{"device_id":"manual_test","load":100,"battery_voltage":12.0,"temperature":30.0}'
   ```
2. Wait 5 seconds
3. Refresh dashboard
4. **Expected:** Device "manual_test" appears with correct values

#### TC08: Token Expiration
1. Login as admin
2. Wait 61 minutes (access token expires after 60 min)
3. Click on a device
4. **Expected:** Token auto-refreshes, request succeeds
5. If refresh token also expired, **Expected:** Redirect to login

### Browser Testing Matrix

| Browser | Version | Desktop | Mobile | Status |
|---------|---------|---------|--------|--------|
| Chrome | Latest | ✓ | ✓ | |
| Firefox | Latest | ✓ | ✓ | |
| Safari | Latest | ✓ | ✓ | |
| Edge | Latest | ✓ | N/A | |

### Device Testing

- Desktop (1920x1080)
- Laptop (1366x768)
- Tablet (768x1024)
- Mobile (375x667)

## Continuous Integration

### GitHub Actions Example

**File:** `.github/workflows/test.yml`

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    services:
      postgres:
        image: timescale/timescaledb:latest-pg15
        env:
          POSTGRES_DB: duoclean_test
          POSTGRES_USER: duoclean_user
          POSTGRES_PASSWORD: testpass
        ports:
          - 5432:5432
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5

      mosquitto:
        image: eclipse-mosquitto:2
        ports:
          - 1883:1883

    steps:
    - uses: actions/checkout@v3

    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.11'

    - name: Install dependencies
      run: |
        python -m pip install --upgrade pip
        pip install -r web-api/requirements.txt
        pip install pytest pytest-cov

    - name: Run unit tests
      run: |
        pytest web-api/tests/unit --cov=src --cov-report=xml

    - name: Run integration tests
      run: |
        pytest tests/integration
      env:
        DATABASE_URL: postgresql://duoclean_user:testpass@localhost:5432/duoclean_test
        MQTT_BROKER_HOST: localhost

    - name: Upload coverage
      uses: codecov/codecov-action@v3
```

## Test Data Management

### Fixtures and Seed Data

**File:** `tests/fixtures/seed_data.sql`

```sql
-- Test users
INSERT INTO users (username, email, password_hash, role)
VALUES
  ('testadmin', 'testadmin@example.com', '$2b$12$...', 'admin'),
  ('testuser', 'testuser@example.com', '$2b$12$...', 'user');

-- Test devices
INSERT INTO devices (device_id, name, description)
VALUES
  ('test_device_001', 'Test Device 1', 'Test device for integration tests'),
  ('test_device_002', 'Test Device 2', 'Another test device');

-- Test sensor readings
INSERT INTO sensor_readings (time, device_id, load, battery_voltage, temperature)
SELECT
  NOW() - (interval '1 hour' * i),
  'test_device_001',
  45 + (i % 10),
  12.5 + (i % 5) * 0.1,
  23 + (i % 8)
FROM generate_series(1, 100) AS i;
```

### Test Database Setup

```bash
# Create test database
docker-compose exec postgres psql -U duoclean_user -c "CREATE DATABASE duoclean_test;"

# Load schema
docker-compose exec postgres psql -U duoclean_user -d duoclean_test -f /path/to/init.sql

# Load test data
docker-compose exec postgres psql -U duoclean_user -d duoclean_test -f /path/to/seed_data.sql
```

## Code Coverage

### Target Coverage

- **Unit tests:** > 80%
- **Integration tests:** > 60%
- **Overall:** > 70%

### Generate Coverage Report

```bash
# Run tests with coverage
pytest --cov=src --cov-report=html

# Open report
open htmlcov/index.html
```

## Troubleshooting Tests

### Common Issues

**Issue: Tests fail to connect to database**
```bash
# Check if postgres is running
docker-compose ps postgres

# Check connection
psql -h localhost -U duoclean_user -d duoclean_test
```

**Issue: MQTT tests timeout**
```bash
# Check if mosquitto is running
sudo systemctl status mosquitto

# Test MQTT connection
mosquitto_sub -h localhost -t '#' -v
```

**Issue: Frontend tests fail (Selenium)**
```bash
# Install ChromeDriver
# On Ubuntu:
sudo apt-get install chromium-chromedriver

# On macOS:
brew install chromedriver
```

## Test Maintenance

### Regular Tasks

- **Weekly:** Review and update test cases
- **Monthly:** Update test data to reflect production patterns
- **Quarterly:** Performance test with increased load
- **Annually:** Full security penetration testing

### When to Update Tests

- Feature addition: Add tests for new feature
- Bug fix: Add regression test
- Performance issue: Add performance test
- Security issue: Add security test

## Resources

- [pytest Documentation](https://docs.pytest.org/)
- [FastAPI Testing](https://fastapi.tiangolo.com/tutorial/testing/)
- [Selenium Documentation](https://www.selenium.dev/documentation/)
- [Locust Documentation](https://docs.locust.io/)
- [Testing Best Practices](https://testingjavascript.com/)
