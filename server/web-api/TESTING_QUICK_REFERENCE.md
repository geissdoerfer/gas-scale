# Testing Quick Reference

## Installation

```bash
cd web-api
pip install -r requirements-test.txt
```

## Common Commands

```bash
# Run all tests
pytest

# Run with coverage
pytest --cov=src

# Run specific category
pytest -m auth
pytest -m users
pytest -m devices

# Run specific file
pytest tests/test_auth.py

# Run specific test
pytest tests/test_auth.py::TestLogin::test_login_success

# Verbose output
pytest -v

# Stop on first failure
pytest -x

# Show print statements
pytest -s

# Run only failed tests
pytest --lf
```

## Test Categories

- `auth` - Authentication tests
- `users` - User management tests
- `devices` - Device management tests
- `readings` - Sensor readings tests
- `dashboard` - Dashboard tests
- `integration` - End-to-end tests

## Available Fixtures

```python
client              # FastAPI test client
db_session          # Database session
admin_user          # Admin user object
regular_user        # Regular user object
admin_token         # Admin JWT token
user_token          # User JWT token
auth_headers_admin  # Admin auth headers
auth_headers_user   # User auth headers
sample_device       # Test device
sample_reading      # Test sensor reading
```

## Example Test

```python
@pytest.mark.auth
class TestLogin:
    def test_login_success(self, client, admin_user):
        response = client.post(
            "/auth/login",
            json={"username": "admin", "password": "admin123"}
        )
        assert response.status_code == 200
        assert "access_token" in response.json()
```

## Coverage Report

```bash
# Generate HTML coverage report
pytest --cov=src --cov-report=html

# Open report
open htmlcov/index.html
```

## Troubleshooting

**Import errors?**
```bash
# Run from web-api directory
cd web-api
pytest
```

**Tests failing unexpectedly?**
```bash
# Run with verbose output
pytest -vv -s
```

**Want to see which tests exist?**
```bash
pytest --collect-only
```
