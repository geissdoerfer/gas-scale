# Web API Summary

## ✅ Phase 3 Complete!

The FastAPI Web API has been successfully implemented with full authentication and CRUD operations.

## What's Been Implemented

### Core Components
- **FastAPI Application** with automatic OpenAPI documentation
- **JWT Authentication** with access and refresh tokens
- **Role-Based Access Control** (Admin vs User permissions)
- **SQLAlchemy ORM** for database operations
- **Pydantic Schemas** for request/response validation
- **CORS Middleware** for frontend integration

### API Endpoints

#### Authentication (`/auth`)
- `POST /auth/login` - Login with username/password
- `POST /auth/refresh` - Refresh access token
- `GET /auth/me` - Get current user info

#### User Management (`/users`) - Admin Only
- `GET /users` - List all users
- `POST /users` - Create new user
- `GET /users/{user_id}` - Get user details
- `PUT /users/{user_id}` - Update user
- `DELETE /users/{user_id}` - Delete user

#### Device Management (`/devices`)
- `GET /devices` - List devices (filtered by permissions)
- `POST /devices` - Create device (admin only)
- `GET /devices/{device_id}` - Get device with latest reading
- `POST /devices/{device_id}/assign` - Assign device to user (admin only)
- `DELETE /devices/{device_id}/unassign/{user_id}` - Unassign device (admin only)

#### Sensor Readings
- `GET /devices/{device_id}/latest` - Latest reading
- `GET /devices/{device_id}/readings` - Historical readings with time range
- `GET /dashboard` - Dashboard with all devices and latest readings

#### Health & Info
- `GET /` - API information
- `GET /health` - Health check

## Testing the API

### 1. Start Services

```bash
# Build and start all services
docker-compose up -d --build

# View logs
docker-compose logs -f web-api
```

### 2. Access Swagger UI

Open your browser to: **http://localhost:8000/docs**

The interactive API documentation allows you to:
- View all endpoints
- Test endpoints directly in the browser
- See request/response schemas
- Authenticate with the "Authorize" button

### 3. Quick Test with cURL

```bash
# Login as admin
curl -X POST http://localhost:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'

# Save the access_token from response, then test:

# Get current user
curl -X GET http://localhost:8000/auth/me \
  -H "Authorization: Bearer YOUR_ACCESS_TOKEN"

# Get dashboard
curl -X GET http://localhost:8000/dashboard \
  -H "Authorization: Bearer YOUR_ACCESS_TOKEN"
```

### 4. Test with MQTT + API

```bash
# 1. Send MQTT message
mosquitto_pub -h localhost -t sensors/api_test_device/data -m '{
  "load": 75.0,
  "battery_voltage": 12.8,
  "temperature": 24.5
}'

# 2. Wait a few seconds for ingestion

# 3. Query via API
curl -X GET "http://localhost:8000/devices/api_test_device" \
  -H "Authorization: Bearer YOUR_ACCESS_TOKEN"
```

## Authentication Flow

1. **Login:** POST `/auth/login` with username/password
2. **Receive Tokens:**
   - `access_token` (expires in 1 hour) - use for API requests
   - `refresh_token` (expires in 7 days) - use to get new access token
3. **Use Access Token:** Include in Authorization header: `Bearer YOUR_TOKEN`
4. **Refresh When Expired:** POST `/auth/refresh` with refresh_token

## Authorization Rules

### Admin Role
- Can access ALL endpoints
- Can create/edit/delete users
- Can create/edit/delete devices
- Can assign devices to users
- Can see all devices

### User Role
- Can only see devices assigned to them
- Cannot create/edit/delete users
- Cannot create/edit/delete devices
- Cannot assign devices
- Limited to their own data

## Project Structure

```
web-api/
├── Dockerfile
├── requirements.txt
├── .dockerignore
└── src/
    ├── __init__.py
    ├── main.py              # FastAPI app entry point
    ├── config.py            # Configuration from environment
    ├── database.py          # SQLAlchemy setup
    ├── models.py            # Database models
    ├── schemas.py           # Pydantic schemas
    ├── auth.py              # JWT and password utilities
    ├── dependencies.py      # FastAPI dependencies
    └── routers/
        ├── __init__.py
        ├── auth.py          # Authentication endpoints
        ├── users.py         # User management
        ├── devices.py       # Device management
        └── readings.py      # Sensor data queries
```

## Environment Variables

The following environment variables are used by the Web API (already in `.env`):

```env
# Database
POSTGRES_HOST=postgres
POSTGRES_PORT=5432
POSTGRES_DB=duoclean
POSTGRES_USER=duoclean_user
POSTGRES_PASSWORD=change_this_secure_password

# JWT
JWT_SECRET=change_this_to_random_string
JWT_ALGORITHM=HS256
JWT_ACCESS_TOKEN_EXPIRE_MINUTES=60
JWT_REFRESH_TOKEN_EXPIRE_DAYS=7

# CORS
API_CORS_ORIGINS=http://localhost:3000,http://localhost

# Logging
LOG_LEVEL=INFO
```

**IMPORTANT:** Change `JWT_SECRET` and `POSTGRES_PASSWORD` before deploying!

## Default Credentials

- **Username:** `admin`
- **Password:** `admin123`
- **Role:** admin

**CHANGE IMMEDIATELY AFTER FIRST LOGIN!**

## Next Steps

With the API complete, the next phase is:

**Phase 4: Web Frontend (Vue.js)**
- Login page
- Dashboard with device cards
- Device detail views with charts
- Admin panel for user/device management

See [docs/IMPLEMENTATION_PHASES.md](docs/IMPLEMENTATION_PHASES.md) for detailed frontend implementation plan.

## Common API Use Cases

### Create a New User (Admin)

```bash
curl -X POST http://localhost:8000/users \
  -H "Authorization: Bearer YOUR_ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "username": "testuser",
    "email": "test@example.com",
    "password": "SecurePass123",
    "role": "user"
  }'
```

### Assign Device to User (Admin)

```bash
curl -X POST http://localhost:8000/devices/device_001/assign \
  -H "Authorization: Bearer YOUR_ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"user_id": 2}'
```

### Get Device Latest Reading

```bash
curl -X GET http://localhost:8000/devices/device_001/latest \
  -H "Authorization: Bearer YOUR_TOKEN"
```

### Get Historical Readings

```bash
curl -X GET "http://localhost:8000/devices/device_001/readings?start_time=2026-01-20T00:00:00Z&end_time=2026-01-21T00:00:00Z" \
  -H "Authorization: Bearer YOUR_TOKEN"
```

## Troubleshooting

### 401 Unauthorized
- Token expired (access tokens expire after 1 hour)
- Use refresh token to get new access token
- Or login again

### 403 Forbidden
- Insufficient permissions
- Admin endpoints require admin role
- Device access requires assignment

### 422 Validation Error
- Request body doesn't match expected schema
- Check required fields
- Check data types

### 500 Internal Server Error
- Check API logs: `docker-compose logs web-api`
- Check database connection
- Verify environment variables

## Documentation

- **Swagger UI:** http://localhost:8000/docs
- **ReDoc:** http://localhost:8000/redoc
- **Complete API Docs:** [docs/API.md](docs/API.md)
- **Security Guide:** [docs/SECURITY.md](docs/SECURITY.md)
