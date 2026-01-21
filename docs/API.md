# API Documentation

## Overview

The DuoClean Energy API is built with FastAPI and provides RESTful endpoints for authentication, user management, device management, and sensor data queries.

**Base URL:** `http://localhost:8000` (development)

**Interactive Documentation:** `http://localhost:8000/docs` (Swagger UI)

## Authentication

All endpoints except `/auth/login` require JWT authentication.

### Authentication Flow

1. **Login:** POST credentials to `/auth/login`
2. **Receive tokens:** Access token (1 hour) + Refresh token (7 days)
3. **Use access token:** Include in `Authorization` header for all requests
4. **Refresh:** When access token expires, use refresh token to get new access token

### Headers

```http
Authorization: Bearer <access_token>
Content-Type: application/json
```

## Response Format

### Success Response

```json
{
  "status": "success",
  "data": { ... }
}
```

### Error Response

```json
{
  "detail": "Error message here"
}
```

**HTTP Status Codes:**
- `200` - Success
- `201` - Created
- `400` - Bad Request (validation error)
- `401` - Unauthorized (invalid/missing token)
- `403` - Forbidden (insufficient permissions)
- `404` - Not Found
- `500` - Internal Server Error

---

## Endpoints

## Authentication Endpoints

### POST /auth/login

Authenticate user and receive JWT tokens.

**Request:**
```json
{
  "username": "admin",
  "password": "admin123"
}
```

**Response (200):**
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "token_type": "bearer",
  "user": {
    "id": 1,
    "username": "admin",
    "email": "admin@duocleanenergy.com",
    "role": "admin"
  }
}
```

**Errors:**
- `401` - Invalid credentials

**Example:**
```bash
curl -X POST http://localhost:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

---

### POST /auth/refresh

Get new access token using refresh token.

**Request:**
```json
{
  "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Response (200):**
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "token_type": "bearer"
}
```

**Errors:**
- `401` - Invalid or expired refresh token

---

### GET /auth/me

Get current user information (requires authentication).

**Headers:**
```http
Authorization: Bearer <access_token>
```

**Response (200):**
```json
{
  "id": 1,
  "username": "admin",
  "email": "admin@duocleanenergy.com",
  "role": "admin",
  "created_at": "2026-01-20T10:00:00Z"
}
```

---

## User Management Endpoints (Admin Only)

### GET /users

List all users.

**Permissions:** Admin only

**Query Parameters:**
- `skip` (int, optional): Number of records to skip (default: 0)
- `limit` (int, optional): Maximum records to return (default: 100)

**Response (200):**
```json
{
  "users": [
    {
      "id": 1,
      "username": "admin",
      "email": "admin@duocleanenergy.com",
      "role": "admin",
      "created_at": "2026-01-20T10:00:00Z"
    },
    {
      "id": 2,
      "username": "john_doe",
      "email": "john@example.com",
      "role": "user",
      "created_at": "2026-01-21T09:30:00Z"
    }
  ],
  "total": 2
}
```

**Example:**
```bash
curl -X GET http://localhost:8000/users \
  -H "Authorization: Bearer <access_token>"
```

---

### POST /users

Create a new user.

**Permissions:** Admin only

**Request:**
```json
{
  "username": "jane_doe",
  "email": "jane@example.com",
  "password": "SecurePass123!",
  "role": "user"
}
```

**Response (201):**
```json
{
  "id": 3,
  "username": "jane_doe",
  "email": "jane@example.com",
  "role": "user",
  "created_at": "2026-01-21T11:00:00Z"
}
```

**Errors:**
- `400` - Username or email already exists
- `400` - Validation error (weak password, invalid email, etc.)

**Validation Rules:**
- Username: 3-50 characters, alphanumeric and underscore only
- Email: Valid email format
- Password: Minimum 8 characters
- Role: Must be "admin" or "user"

---

### GET /users/{user_id}

Get user by ID.

**Permissions:** Admin only

**Response (200):**
```json
{
  "id": 2,
  "username": "john_doe",
  "email": "john@example.com",
  "role": "user",
  "created_at": "2026-01-21T09:30:00Z",
  "updated_at": "2026-01-21T09:30:00Z",
  "assigned_devices": [
    {
      "id": 1,
      "device_id": "device_001",
      "name": "Sensor 001"
    }
  ]
}
```

**Errors:**
- `404` - User not found

---

### PUT /users/{user_id}

Update user information.

**Permissions:** Admin only

**Request:**
```json
{
  "username": "john_updated",
  "email": "john.new@example.com",
  "role": "admin",
  "password": "NewPassword123!"
}
```

**Note:** All fields are optional. Only include fields you want to update.

**Response (200):**
```json
{
  "id": 2,
  "username": "john_updated",
  "email": "john.new@example.com",
  "role": "admin",
  "updated_at": "2026-01-21T12:00:00Z"
}
```

**Errors:**
- `404` - User not found
- `400` - Username/email already taken

---

### DELETE /users/{user_id}

Delete a user.

**Permissions:** Admin only

**Response (200):**
```json
{
  "message": "User deleted successfully"
}
```

**Note:** Deleting a user automatically removes their device assignments (CASCADE).

**Errors:**
- `404` - User not found
- `400` - Cannot delete yourself (admin deleting their own account)

---

## Device Management Endpoints

### GET /devices

List all devices (filtered by user permissions).

**Permissions:**
- Admin: Returns all devices
- User: Returns only assigned devices

**Query Parameters:**
- `skip` (int, optional): Pagination offset (default: 0)
- `limit` (int, optional): Maximum records (default: 100)

**Response (200):**
```json
{
  "devices": [
    {
      "id": 1,
      "device_id": "device_001",
      "name": "Sensor 001",
      "description": "Temperature sensor in Building A",
      "created_at": "2026-01-20T08:00:00Z",
      "latest_reading": {
        "time": "2026-01-21T11:30:00Z",
        "load": 45.2,
        "battery_voltage": 12.6,
        "temperature": 23.5
      }
    }
  ],
  "total": 1
}
```

---

### POST /devices

Create a new device.

**Permissions:** Admin only

**Request:**
```json
{
  "device_id": "device_003",
  "name": "Sensor 003",
  "description": "New sensor in Building C"
}
```

**Response (201):**
```json
{
  "id": 3,
  "device_id": "device_003",
  "name": "Sensor 003",
  "description": "New sensor in Building C",
  "created_at": "2026-01-21T12:00:00Z"
}
```

**Errors:**
- `400` - Device ID already exists

---

### GET /devices/{device_id}

Get device details (includes latest reading).

**Permissions:**
- Admin: Can access any device
- User: Can only access assigned devices

**Path Parameter:**
- `device_id` (string): Device identifier (e.g., "device_001")

**Response (200):**
```json
{
  "id": 1,
  "device_id": "device_001",
  "name": "Sensor 001",
  "description": "Temperature sensor in Building A",
  "created_at": "2026-01-20T08:00:00Z",
  "updated_at": "2026-01-21T10:00:00Z",
  "latest_reading": {
    "time": "2026-01-21T11:30:00Z",
    "load": 45.2,
    "battery_voltage": 12.6,
    "temperature": 23.5
  },
  "assigned_users": [
    {
      "id": 2,
      "username": "john_doe"
    }
  ]
}
```

**Errors:**
- `404` - Device not found
- `403` - User doesn't have access to this device

---

### PUT /devices/{device_id}

Update device information.

**Permissions:** Admin only

**Request:**
```json
{
  "name": "Updated Sensor Name",
  "description": "Updated description"
}
```

**Response (200):**
```json
{
  "id": 1,
  "device_id": "device_001",
  "name": "Updated Sensor Name",
  "description": "Updated description",
  "updated_at": "2026-01-21T12:30:00Z"
}
```

---

### DELETE /devices/{device_id}

Delete a device.

**Permissions:** Admin only

**Response (200):**
```json
{
  "message": "Device deleted successfully"
}
```

**Note:** Deleting a device also deletes:
- All sensor readings (CASCADE)
- All user assignments (CASCADE)

**Errors:**
- `404` - Device not found

---

### POST /devices/{device_id}/assign

Assign device to a user.

**Permissions:** Admin only

**Request:**
```json
{
  "user_id": 2
}
```

**Response (200):**
```json
{
  "message": "Device assigned to user successfully",
  "device_id": "device_001",
  "user_id": 2
}
```

**Errors:**
- `404` - Device or user not found
- `400` - Device already assigned to this user

---

### DELETE /devices/{device_id}/unassign/{user_id}

Unassign device from a user.

**Permissions:** Admin only

**Response (200):**
```json
{
  "message": "Device unassigned from user successfully"
}
```

**Errors:**
- `404` - Assignment not found

---

### GET /devices/{device_id}/users

List all users assigned to a device.

**Permissions:** Admin only

**Response (200):**
```json
{
  "users": [
    {
      "id": 2,
      "username": "john_doe",
      "email": "john@example.com",
      "assigned_at": "2026-01-21T10:00:00Z"
    }
  ]
}
```

---

## Sensor Reading Endpoints

### GET /devices/{device_id}/latest

Get the latest sensor reading for a device.

**Permissions:**
- Admin: Any device
- User: Assigned devices only

**Response (200):**
```json
{
  "device_id": "device_001",
  "time": "2026-01-21T11:45:00Z",
  "load": 45.2,
  "battery_voltage": 12.6,
  "temperature": 23.5
}
```

**Errors:**
- `404` - Device not found or no readings available
- `403` - User doesn't have access to this device

---

### GET /devices/{device_id}/readings

Get historical sensor readings for a device.

**Permissions:**
- Admin: Any device
- User: Assigned devices only

**Query Parameters:**
- `start_time` (ISO 8601, optional): Start of time range (default: 24 hours ago)
- `end_time` (ISO 8601, optional): End of time range (default: now)
- `limit` (int, optional): Maximum records (default: 1000, max: 10000)

**Response (200):**
```json
{
  "device_id": "device_001",
  "readings": [
    {
      "time": "2026-01-21T11:45:00Z",
      "load": 45.2,
      "battery_voltage": 12.6,
      "temperature": 23.5
    },
    {
      "time": "2026-01-21T11:30:00Z",
      "load": 44.8,
      "battery_voltage": 12.5,
      "temperature": 23.3
    }
  ],
  "count": 2,
  "start_time": "2026-01-20T11:45:00Z",
  "end_time": "2026-01-21T11:45:00Z"
}
```

**Example:**
```bash
curl -X GET "http://localhost:8000/devices/device_001/readings?start_time=2026-01-20T00:00:00Z&end_time=2026-01-21T00:00:00Z" \
  -H "Authorization: Bearer <access_token>"
```

---

### GET /devices/{device_id}/aggregates

Get aggregated sensor data (hourly or daily statistics).

**Permissions:**
- Admin: Any device
- User: Assigned devices only

**Query Parameters:**
- `start_time` (ISO 8601, required): Start of time range
- `end_time` (ISO 8601, required): End of time range
- `interval` (string, optional): Aggregation interval - "1h" (hourly) or "1d" (daily), default: "1h"

**Response (200):**
```json
{
  "device_id": "device_001",
  "interval": "1h",
  "aggregates": [
    {
      "bucket": "2026-01-21T11:00:00Z",
      "avg_load": 45.1,
      "min_load": 44.5,
      "max_load": 45.8,
      "avg_battery_voltage": 12.55,
      "min_battery_voltage": 12.5,
      "max_battery_voltage": 12.6,
      "avg_temperature": 23.4,
      "min_temperature": 23.0,
      "max_temperature": 23.8,
      "reading_count": 15
    }
  ],
  "count": 1
}
```

**Note:** Uses TimescaleDB continuous aggregates for fast queries.

**Example:**
```bash
curl -X GET "http://localhost:8000/devices/device_001/aggregates?start_time=2026-01-14T00:00:00Z&end_time=2026-01-21T00:00:00Z&interval=1d" \
  -H "Authorization: Bearer <access_token>"
```

---

### GET /dashboard

Get dashboard summary: all accessible devices with latest readings.

**Permissions:** All authenticated users

**Response (200):**
```json
{
  "devices": [
    {
      "device_id": "device_001",
      "name": "Sensor 001",
      "description": "Temperature sensor in Building A",
      "latest_reading": {
        "time": "2026-01-21T11:45:00Z",
        "load": 45.2,
        "battery_voltage": 12.6,
        "temperature": 23.5
      },
      "status": "ok"
    },
    {
      "device_id": "device_002",
      "name": "Sensor 002",
      "description": "Sensor in Building B",
      "latest_reading": {
        "time": "2026-01-21T11:40:00Z",
        "load": 38.5,
        "battery_voltage": 11.2,
        "temperature": 25.1
      },
      "status": "low_battery"
    }
  ],
  "total": 2,
  "user": {
    "id": 2,
    "username": "john_doe",
    "role": "user"
  }
}
```

**Status values:**
- `ok` - All readings normal
- `low_battery` - Battery voltage < 11.5V
- `no_data` - No readings in last 24 hours
- `error` - Other issues

---

## Data Models

### User

```typescript
{
  id: number;
  username: string;
  email: string;
  role: "admin" | "user";
  created_at: string; // ISO 8601
  updated_at: string; // ISO 8601
}
```

### Device

```typescript
{
  id: number;
  device_id: string;
  name: string | null;
  description: string | null;
  created_at: string; // ISO 8601
  updated_at: string; // ISO 8601
}
```

### SensorReading

```typescript
{
  time: string; // ISO 8601
  device_id: string;
  load: number | null;
  battery_voltage: number | null;
  temperature: number | null;
}
```

### Aggregate

```typescript
{
  bucket: string; // ISO 8601 - start of time bucket
  device_id: string;
  avg_load: number | null;
  min_load: number | null;
  max_load: number | null;
  avg_battery_voltage: number | null;
  min_battery_voltage: number | null;
  max_battery_voltage: number | null;
  avg_temperature: number | null;
  min_temperature: number | null;
  max_temperature: number | null;
  reading_count: number;
}
```

---

## Error Handling

### Validation Errors (400)

```json
{
  "detail": [
    {
      "loc": ["body", "username"],
      "msg": "field required",
      "type": "value_error.missing"
    }
  ]
}
```

### Authentication Errors (401)

```json
{
  "detail": "Could not validate credentials"
}
```

### Permission Errors (403)

```json
{
  "detail": "Not enough permissions"
}
```

### Not Found Errors (404)

```json
{
  "detail": "Device not found"
}
```

---

## Rate Limiting

**MVP:** No rate limiting implemented.

**Production Recommendation:**
- 100 requests per minute per IP for login endpoint
- 1000 requests per minute per user for other endpoints
- Use libraries like `slowapi` or implement in reverse proxy

---

## CORS Configuration

**Allowed Origins (configured in .env):**
- http://localhost:3000 (frontend)
- http://localhost:80 (alternate frontend port)

**Allowed Methods:**
- GET, POST, PUT, DELETE, OPTIONS

**Allowed Headers:**
- Authorization, Content-Type

---

## Testing the API

### Using cURL

```bash
# Login
TOKEN=$(curl -X POST http://localhost:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  | jq -r '.access_token')

# Get dashboard
curl -X GET http://localhost:8000/dashboard \
  -H "Authorization: Bearer $TOKEN"

# Get device readings
curl -X GET "http://localhost:8000/devices/device_001/readings?limit=10" \
  -H "Authorization: Bearer $TOKEN"
```

### Using Python

```python
import requests

# Login
response = requests.post(
    "http://localhost:8000/auth/login",
    json={"username": "admin", "password": "admin123"}
)
token = response.json()["access_token"]

# Get dashboard
headers = {"Authorization": f"Bearer {token}"}
response = requests.get("http://localhost:8000/dashboard", headers=headers)
print(response.json())
```

### Using Swagger UI

1. Navigate to http://localhost:8000/docs
2. Click "Authorize" button
3. Enter token (with "Bearer " prefix)
4. Try out any endpoint interactively

---

## API Performance

**Expected Response Times (under normal load):**
- Authentication: < 200ms (bcrypt hashing)
- Latest reading: < 50ms
- Historical readings (24h): < 100ms
- Aggregates (30d): < 200ms (using continuous aggregates)
- Dashboard: < 150ms

**Optimization Tips:**
- Use aggregates for long time periods
- Limit result sets appropriately
- Cache dashboard data on frontend (30s refresh)
- Use connection pooling (configured in SQLAlchemy)

---

## Versioning

**Current Version:** v1 (implicit in MVP)

**Future:** API versioning via URL prefix
- `/api/v1/devices`
- `/api/v2/devices`

This allows breaking changes without affecting existing clients.

---

## Webhooks (Future Enhancement)

Not implemented in MVP. Future feature to push data to external systems:

- POST to external URL on new reading
- Configurable per device or threshold
- Retry logic with exponential backoff
