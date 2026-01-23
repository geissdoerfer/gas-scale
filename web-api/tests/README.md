# Web API Test Suite

Comprehensive test suite for the DuoClean Energy Web API.

## Test Coverage

### Authentication Tests (`test_auth.py`)
- ✅ Login with valid credentials
- ✅ Login with invalid credentials
- ✅ Token refresh flow
- ✅ Get current user information
- ✅ JWT token validation
- ✅ Invalid token rejection

### User Management Tests (`test_users.py`)
- ✅ List users (admin only)
- ✅ Create new users
- ✅ Update user details
- ✅ Update passwords
- ✅ Delete users
- ✅ Permission checks (admin vs regular user)
- ✅ Validation (duplicate username/email, invalid email, short password)

### Device Management Tests (`test_devices.py`)
- ✅ List devices (filtered by user access)
- ✅ Get device details
- ✅ Create devices (admin only)
- ✅ Assign devices to users
- ✅ Unassign devices from users
- ✅ Get latest device reading
- ✅ Access control (users can only see assigned devices)

### Sensor Readings Tests (`test_readings.py`)
- ✅ Get device readings
- ✅ Time range filtering
- ✅ Limit results
- ✅ Dashboard endpoint
- ✅ Permission checks for readings

### Integration Tests (`test_readings.py`)
- ✅ Complete admin workflow
- ✅ Complete user workflow

## Running Tests

### Install Test Dependencies

```bash
cd web-api
pip install -r requirements-test.txt
```

### Run All Tests

```bash
pytest
```

### Run with Coverage Report

```bash
pytest --cov=src --cov-report=html
```

Then open `htmlcov/index.html` in your browser.

### Run Specific Test Categories

```bash
# Only authentication tests
pytest -m auth

# Only user management tests
pytest -m users

# Only device tests
pytest -m devices

# Only reading tests
pytest -m readings

# Only dashboard tests
pytest -m dashboard

# Only integration tests
pytest -m integration
```

### Run Specific Test File

```bash
pytest tests/test_auth.py
pytest tests/test_users.py
pytest tests/test_devices.py
pytest tests/test_readings.py
```

### Run Specific Test Class or Function

```bash
# Run all tests in TestLogin class
pytest tests/test_auth.py::TestLogin

# Run specific test
pytest tests/test_auth.py::TestLogin::test_login_success
```

### Verbose Output

```bash
pytest -v
```

### Show Print Statements

```bash
pytest -s
```

### Stop on First Failure

```bash
pytest -x
```

### Run Last Failed Tests

```bash
pytest --lf
```

## Test Structure

```
tests/
├── __init__.py
├── conftest.py              # Fixtures and test configuration
├── test_auth.py             # Authentication endpoint tests
├── test_users.py            # User management tests
├── test_devices.py          # Device management tests
├── test_readings.py         # Sensor readings & dashboard tests
└── README.md                # This file
```

## Test Fixtures

Defined in `conftest.py`:

- `db_session`: Fresh database session for each test
- `client`: FastAPI test client
- `admin_user`: Pre-created admin user
- `regular_user`: Pre-created regular user
- `admin_token`: Admin authentication token
- `user_token`: Regular user authentication token
- `auth_headers_admin`: Authorization headers for admin
- `auth_headers_user`: Authorization headers for regular user
- `sample_device`: Pre-created test device
- `sample_reading`: Pre-created sensor reading

## Test Database

Tests use an in-memory SQLite database that is created fresh for each test. This ensures:
- Tests are isolated from each other
- No pollution of production/development database
- Fast test execution
- No cleanup required

## Continuous Integration

To run tests in CI/CD:

```yaml
# Example GitHub Actions workflow
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-python@v2
        with:
          python-version: '3.11'
      - name: Install dependencies
        run: |
          pip install -r requirements.txt
          pip install -r requirements-test.txt
      - name: Run tests
        run: pytest --cov=src --cov-report=xml
      - name: Upload coverage
        uses: codecov/codecov-action@v2
```

## Writing New Tests

### Test Class Structure

```python
@pytest.mark.your_category
class TestYourFeature:
    """Test your feature."""

    def test_something(self, client, auth_headers_admin):
        """Test something specific."""
        response = client.get("/endpoint", headers=auth_headers_admin)
        assert response.status_code == 200
        assert response.json()["field"] == "expected_value"
```

### Common Patterns

**Testing authenticated endpoints:**
```python
def test_endpoint(self, client, auth_headers_admin):
    response = client.get("/endpoint", headers=auth_headers_admin)
    assert response.status_code == 200
```

**Testing permission denied:**
```python
def test_endpoint_as_user(self, client, auth_headers_user):
    response = client.get("/admin-endpoint", headers=auth_headers_user)
    assert response.status_code == 403
```

**Testing with database objects:**
```python
def test_with_data(self, client, auth_headers_admin, sample_device, db_session):
    # Use sample_device or create new objects
    new_obj = models.Something(...)
    db_session.add(new_obj)
    db_session.commit()

    response = client.get("/endpoint")
    assert response.status_code == 200
```

## Troubleshooting

### Import Errors

If you get import errors, make sure you're running pytest from the `web-api` directory:

```bash
cd web-api
pytest
```

### Database Errors

If you get SQLAlchemy errors, check that:
1. Models are properly imported in conftest.py
2. Database tables are created in the fixture
3. You're using `db_session` fixture for database operations

### Token Errors

If authentication tests fail:
1. Check that JWT_SECRET is consistent
2. Verify password hashing is working
3. Check that tokens contain correct payload format

## Test Metrics

Current test coverage:
- **Total tests**: 50+
- **Test files**: 4
- **Endpoints covered**: All major endpoints
- **Line coverage**: Target 80%+

## Next Steps

Suggested additional tests:
- [ ] Performance tests for high-volume sensor data
- [ ] Load tests for concurrent users
- [ ] Security tests (SQL injection, XSS, etc.)
- [ ] API rate limiting tests
- [ ] WebSocket tests (if added)
- [ ] MQTT integration tests
