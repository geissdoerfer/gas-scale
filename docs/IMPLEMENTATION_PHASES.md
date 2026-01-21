# Implementation Phases

## Overview

This document provides a detailed, step-by-step implementation plan for building the DuoClean Energy IoT monitoring platform MVP. Follow these phases sequentially to ensure a smooth development process.

**Estimated Total Time:** 4-6 weeks (depending on experience level)

---

## Phase 1: Database Setup

**Goal:** Set up PostgreSQL with TimescaleDB and initialize schema

**Duration:** 2-3 days

### Tasks

1. **Create database directory structure**
   ```bash
   mkdir -p database
   ```

2. **Create init.sql**
   - Copy complete SQL from [DATABASE.md](./DATABASE.md)
   - Create users table
   - Create devices table
   - Create user_device_access table
   - Create sensor_readings hypertable
   - Create continuous aggregate (sensor_readings_hourly)
   - Add retention policy
   - Insert default admin user
   - Add indexes

3. **Create Dockerfile for postgres (optional)**
   ```dockerfile
   FROM timescale/timescaledb:latest-pg15
   # Add any custom configuration if needed
   ```

4. **Test locally with Docker**
   ```bash
   docker run -d \
     -e POSTGRES_DB=duoclean \
     -e POSTGRES_USER=duoclean_user \
     -e POSTGRES_PASSWORD=testpass \
     -v $(pwd)/database/init.sql:/docker-entrypoint-initdb.d/init.sql \
     -p 5432:5432 \
     timescale/timescaledb:latest-pg15
   ```

5. **Verify database initialization**
   ```bash
   docker exec -it <container_id> psql -U duoclean_user -d duoclean
   \dt  # List tables
   \d sensor_readings  # Describe hypertable
   SELECT * FROM users;  # Verify admin user
   ```

6. **Test continuous aggregate**
   ```sql
   INSERT INTO sensor_readings VALUES (NOW(), 'test_device', 50.0, 12.5, 22.0);
   SELECT * FROM sensor_readings;
   SELECT * FROM sensor_readings_hourly;
   ```

### Deliverable
- ✅ Working PostgreSQL + TimescaleDB container
- ✅ Complete schema with hypertables, aggregates, and retention
- ✅ Default admin user created
- ✅ Verified database queries work

---

## Phase 2: MQTT Ingestor

**Goal:** Build service that ingests MQTT messages and writes to database

**Duration:** 3-5 days

### Tasks

1. **Create project structure**
   ```bash
   mkdir -p mqtt-ingestor/src
   cd mqtt-ingestor
   ```

2. **Create requirements.txt**
   ```txt
   paho-mqtt==1.6.1
   psycopg2-binary==2.9.9
   ```

3. **Create config.py**
   - Load environment variables
   - Database connection params
   - MQTT broker params
   - See [MQTT.md](./MQTT.md) for details

4. **Create db_writer.py**
   - Initialize connection pool
   - Implement `_ensure_device_exists()` method
   - Implement `insert_reading()` method
   - Handle errors gracefully
   - Use ON CONFLICT for idempotency

5. **Create mqtt_client.py**
   - Initialize paho-mqtt client
   - Implement `_on_connect()` callback
   - Implement `_on_disconnect()` callback
   - Implement `_on_message()` callback
   - Implement `_validate_message()` method
   - Implement `_process_message()` method
   - Parse JSON payloads
   - Extract device_id from topic

6. **Create main.py**
   - Entry point
   - Setup logging
   - Signal handlers for graceful shutdown
   - Initialize DatabaseWriter
   - Initialize MQTTIngestor
   - Start loop

7. **Create Dockerfile**
   ```dockerfile
   FROM python:3.11-slim
   WORKDIR /app
   COPY requirements.txt .
   RUN pip install --no-cache-dir -r requirements.txt
   COPY src/ ./src/
   CMD ["python", "-u", "src/main.py"]
   ```

8. **Test locally**
   - Start PostgreSQL container
   - Start local Mosquitto broker
   - Run ingestor: `python src/main.py`
   - Publish test message:
     ```bash
     mosquitto_pub -t sensors/test_device/data -m '{"device_id":"test_device","load":50.0,"battery_voltage":12.5,"temperature":22.0}'
     ```
   - Check database for reading
   - Check logs for errors

9. **Test error cases**
   - Invalid JSON → should log warning and skip
   - Missing device_id → should log error and skip
   - Device auto-registration → verify new device created
   - Duplicate message → should be ignored (ON CONFLICT)

### Deliverable
- ✅ Working MQTT ingestor service
- ✅ Messages written to database
- ✅ Devices auto-registered
- ✅ Error handling and logging functional
- ✅ Dockerized service

---

## Phase 3: Authentication & User Management API

**Goal:** Build FastAPI backend with JWT authentication and user CRUD

**Duration:** 4-6 days

### Tasks

1. **Create project structure**
   ```bash
   mkdir -p web-api/src/routers
   cd web-api
   ```

2. **Create requirements.txt**
   ```txt
   fastapi==0.109.0
   uvicorn[standard]==0.27.0
   sqlalchemy==2.0.25
   psycopg2-binary==2.9.9
   python-jose[cryptography]==3.3.0
   passlib[bcrypt]==1.7.4
   python-multipart==0.0.6
   pydantic==2.5.3
   pydantic-settings==2.1.0
   ```

3. **Create config.py**
   - Load environment variables using Pydantic Settings
   - Database URL
   - JWT settings
   - CORS origins

4. **Create database.py**
   - SQLAlchemy engine setup
   - Session factory
   - `get_db()` dependency for FastAPI

5. **Create models.py** (SQLAlchemy models)
   - User model
   - Device model
   - UserDeviceAccess model
   - SensorReading model (for querying only, writes via ingestor)

6. **Create schemas.py** (Pydantic models)
   - UserCreate, UserUpdate, UserResponse
   - DeviceCreate, DeviceUpdate, DeviceResponse
   - SensorReadingResponse
   - Token, TokenData
   - LoginRequest

7. **Create auth.py**
   - `verify_password()` - bcrypt comparison
   - `hash_password()` - bcrypt hashing
   - `create_access_token()` - JWT generation
   - `create_refresh_token()` - JWT generation (longer expiry)
   - `decode_token()` - JWT verification

8. **Create dependencies.py**
   - `get_current_user()` - Extract user from JWT
   - `get_current_active_user()` - Verify user is active
   - `require_admin()` - Check user is admin
   - `check_device_access()` - Verify user can access device

9. **Create routers/auth.py**
   - `POST /auth/login` - Login endpoint
     - Validate credentials
     - Generate tokens
     - Return user info
   - `POST /auth/refresh` - Refresh token endpoint
   - `GET /auth/me` - Get current user info

10. **Create routers/users.py** (Admin only)
    - `GET /users` - List users
    - `POST /users` - Create user
    - `GET /users/{user_id}` - Get user
    - `PUT /users/{user_id}` - Update user
    - `DELETE /users/{user_id}` - Delete user
    - All endpoints use `require_admin` dependency

11. **Create main.py**
    - Initialize FastAPI app
    - Configure CORS middleware
    - Include routers
    - Add `/health` endpoint
    - Configure uvicorn

12. **Create Dockerfile**
    ```dockerfile
    FROM python:3.11-slim
    WORKDIR /app
    COPY requirements.txt .
    RUN pip install --no-cache-dir -r requirements.txt
    COPY src/ ./src/
    CMD ["uvicorn", "src.main:app", "--host", "0.0.0.0", "--port", "8000"]
    ```

13. **Test locally**
    - Start API: `uvicorn src.main:app --reload`
    - Access Swagger UI: http://localhost:8000/docs
    - Test login with default admin
    - Test creating a user
    - Test updating a user
    - Test deleting a user
    - Verify JWT token works
    - Test 401 errors with invalid token
    - Test 403 errors for non-admin users

### Deliverable
- ✅ Working FastAPI application
- ✅ JWT authentication functional
- ✅ User CRUD operations working
- ✅ Admin-only permissions enforced
- ✅ API documentation auto-generated
- ✅ Dockerized service

---

## Phase 4: Device & Reading API

**Goal:** Complete API with device management and sensor data queries

**Duration:** 4-6 days

### Tasks

1. **Create routers/devices.py**
   - `GET /devices` - List devices (filtered by user permissions)
     - Admin: return all devices
     - User: join with user_device_access table
   - `POST /devices` - Create device (admin only)
   - `GET /devices/{device_id}` - Get device with latest reading
   - `PUT /devices/{device_id}` - Update device (admin only)
   - `DELETE /devices/{device_id}` - Delete device (admin only)
   - `POST /devices/{device_id}/assign` - Assign device to user (admin only)
   - `DELETE /devices/{device_id}/unassign/{user_id}` - Unassign (admin only)
   - `GET /devices/{device_id}/users` - List users with access (admin only)

2. **Create routers/readings.py**
   - `GET /devices/{device_id}/latest` - Latest reading
     - Query sensor_readings, ORDER BY time DESC LIMIT 1
     - Check user has access to device
   - `GET /devices/{device_id}/readings` - Historical readings
     - Query params: start_time, end_time, limit
     - Query sensor_readings with time range
     - Check user has access
   - `GET /devices/{device_id}/aggregates` - Aggregated data
     - Query params: start_time, end_time, interval (1h or 1d)
     - Query sensor_readings_hourly continuous aggregate
     - Check user has access
   - `GET /dashboard` - Dashboard summary
     - Get all accessible devices for user
     - Join with latest reading (DISTINCT ON trick)
     - Return device list with status

3. **Implement permission checks**
   - Create `check_device_access()` dependency
   - Admin: always allow
   - User: check user_device_access table
   - Return 403 if no access

4. **Optimize queries**
   - Use indexes effectively
   - LIMIT results appropriately
   - Use continuous aggregates for long time periods
   - Test query performance with EXPLAIN ANALYZE

5. **Add device status logic**
   - OK: all readings normal
   - low_battery: battery_voltage < 11.5V
   - no_data: no reading in last 24 hours

6. **Update main.py**
   - Include devices and readings routers

7. **Test endpoints**
   - Test as admin (can access all devices)
   - Create test user and assign devices
   - Test as user (can only access assigned devices)
   - Test permission errors (403)
   - Test latest reading endpoint
   - Test historical readings with time ranges
   - Test aggregates with different intervals
   - Test dashboard endpoint
   - Verify query performance

8. **Test with real data**
   - Use MQTT ingestor to populate data
   - Publish messages for multiple devices
   - Query via API
   - Verify data matches database

### Deliverable
- ✅ Complete REST API with all endpoints
- ✅ Device management working
- ✅ Sensor data queries functional
- ✅ Permission system enforced correctly
- ✅ Dashboard endpoint returns correct data
- ✅ Query performance acceptable

---

## Phase 5: Web Frontend - Authentication

**Goal:** Login page and authentication utilities

**Duration:** 2-3 days

### Tasks

1. **Create project structure**
   ```bash
   mkdir -p web-frontend/public/{css,js}
   cd web-frontend
   ```

2. **Create index.html** (Login page)
   - Simple centered login form
   - Username and password inputs
   - Login button
   - Error message display
   - Load Vue.js from CDN
   - Load auth.js and api.js

3. **Create css/style.css**
   - Basic styling
   - Responsive layout
   - Form styles
   - Button styles
   - Color scheme

4. **Create js/auth.js**
   - Implement all auth utility functions (see [FRONTEND.md](./FRONTEND.md))
   - `storeTokens()`
   - `getAccessToken()`
   - `isAuthenticated()`
   - `getCurrentUser()`
   - `logout()`
   - `refreshAccessToken()`
   - `requireAuth()`
   - `redirectIfAuthenticated()`

5. **Create js/api.js**
   - Implement API client wrapper
   - `apiRequest()` with auto JWT injection
   - Handle 401 errors (refresh token)
   - Convenience methods for all API endpoints

6. **Create Vue app for login page**
   - Handle form submission
   - Call API login endpoint
   - Store tokens on success
   - Redirect to dashboard
   - Show error messages

7. **Test login flow**
   - Start frontend (nginx or python -m http.server)
   - Access login page
   - Test with correct credentials → should redirect
   - Test with wrong credentials → should show error
   - Check tokens in localStorage
   - Verify redirect to dashboard works

### Deliverable
- ✅ Working login page
- ✅ Tokens stored correctly
- ✅ Authentication utilities functional
- ✅ API client working with JWT

---

## Phase 6: Web Frontend - Dashboard & Device Views

**Goal:** Main user-facing pages for viewing devices

**Duration:** 4-6 days

### Tasks

1. **Create dashboard.html**
   - Header with username, role, logout button
   - Admin button (conditional)
   - Device grid/cards container
   - Loading state
   - Error message display
   - Load Vue.js, Chart.js, auth.js, api.js

2. **Create js/dashboard.js** (Vue app)
   - Implement dashboard component (see [FRONTEND.md](./FRONTEND.md))
   - Load devices from /dashboard API
   - Display device cards
   - Auto-refresh every 30 seconds
   - Click device → navigate to detail
   - Logout button
   - Admin button (if admin)

3. **Style device cards**
   - Grid layout (responsive)
   - Card styling
   - Status indicators (colored borders)
   - Latest reading values (large text)
   - Time since last update
   - Hover effects

4. **Create device-detail.html**
   - Header with device name, back button
   - Latest readings (large numbers)
   - Time range selector buttons
   - Three canvas elements for charts
   - Loading state

5. **Create js/device-detail.js** (Vue app)
   - Extract device_id from URL params
   - Load device info
   - Load latest reading
   - Load historical data or aggregates
   - Render Chart.js charts
   - Time range selector logic
   - Update charts when range changes

6. **Implement Chart.js charts**
   - Load chart (line chart)
   - Battery voltage chart (line chart)
   - Temperature chart (line chart)
   - Responsive sizing
   - Tooltips
   - Time-based x-axis
   - Auto-scaling y-axis

7. **Test dashboard**
   - Login and access dashboard
   - Verify devices displayed
   - Check latest readings shown
   - Verify auto-refresh works
   - Click device card → should navigate to detail

8. **Test device detail**
   - Access device detail page
   - Verify device info displayed
   - Verify charts render
   - Change time range → charts update
   - Test different time ranges (1h, 6h, 24h, 7d, 30d)
   - Back button returns to dashboard

9. **Test with multiple devices**
   - Create multiple devices via API
   - Assign some to test user
   - Login as test user → should only see assigned devices
   - Login as admin → should see all devices

### Deliverable
- ✅ Working dashboard page
- ✅ Device cards display correctly
- ✅ Auto-refresh functional
- ✅ Device detail page with charts working
- ✅ Time range selection working
- ✅ Navigation between pages functional

---

## Phase 7: Web Frontend - Admin Panel

**Goal:** Admin-only page for user and device management

**Duration:** 4-6 days

### Tasks

1. **Create admin.html**
   - Header with back to dashboard button
   - Tabbed interface (Users, Devices, Assignments)
   - Each tab has its own section
   - Load Vue.js, auth.js, api.js

2. **Implement Users Tab**
   - List all users in table
   - Create user form (username, email, password, role)
   - Edit user button → populate form
   - Delete user button → confirm and delete
   - Form validation

3. **Implement Devices Tab**
   - List all devices in table
   - Create device form (device_id, name, description)
   - Edit device button → populate form
   - Delete device button → confirm and delete
   - Form validation

4. **Implement Assignments Tab**
   - User selector dropdown
   - List of assigned devices for selected user
   - List of unassigned devices
   - Assign button → assign device to user
   - Unassign button → unassign device from user

5. **Create js/admin.js** (Vue app)
   - Tab switching logic
   - Users tab component
     - Load users from API
     - Create user via API
     - Update user via API
     - Delete user via API
   - Devices tab component
     - Load devices from API
     - Create device via API
     - Update device via API
     - Delete device via API
   - Assignments tab component
     - Load users and devices
     - Assign device via API
     - Unassign device via API

6. **Add permission check**
   - On page load, verify user is admin
   - If not admin, redirect to dashboard
   - Use `getCurrentUser()` and check role

7. **Style admin panel**
   - Table styles
   - Form styles
   - Button styles
   - Tabs styling
   - Responsive layout

8. **Test admin panel**
   - Login as admin
   - Navigate to admin panel
   - Test creating user
   - Test editing user
   - Test deleting user
   - Test creating device
   - Test editing device
   - Test deleting device
   - Test assigning device to user
   - Test unassigning device
   - Verify changes reflected in database

9. **Test permissions**
   - Try accessing admin panel as non-admin user
   - Should redirect to dashboard
   - Verify admin button only visible to admin

### Deliverable
- ✅ Working admin panel
- ✅ User management functional
- ✅ Device management functional
- ✅ Device assignment system working
- ✅ Permission checks enforced
- ✅ All CRUD operations working

---

## Phase 8: Frontend Containerization

**Goal:** Dockerize frontend with Nginx

**Duration:** 1-2 days

### Tasks

1. **Create nginx.conf**
   - Configure server block
   - Serve static files from /usr/share/nginx/html
   - Add security headers
   - Enable gzip compression
   - Optional: proxy /api/ to web-api service

2. **Create Dockerfile**
   ```dockerfile
   FROM nginx:alpine
   COPY nginx.conf /etc/nginx/conf.d/default.conf
   COPY public/ /usr/share/nginx/html/
   EXPOSE 80
   CMD ["nginx", "-g", "daemon off;"]
   ```

3. **Create .dockerignore**
   ```
   node_modules
   *.md
   .git
   ```

4. **Build and test**
   ```bash
   docker build -t duoclean-frontend .
   docker run -p 3000:80 duoclean-frontend
   ```

5. **Test in browser**
   - Access http://localhost:3000
   - Verify static files served
   - Check all pages load
   - Verify CSS and JS loaded

### Deliverable
- ✅ Dockerized frontend
- ✅ Nginx serving static files
- ✅ All pages accessible

---

## Phase 9: Docker Compose Integration

**Goal:** Orchestrate all services with Docker Compose

**Duration:** 2-3 days

### Tasks

1. **Create docker-compose.yml**
   - Define postgres service
   - Define mqtt-ingestor service
   - Define web-api service
   - Define web-frontend service
   - Configure networks
   - Configure volumes
   - Set up service dependencies
   - Add health checks
   - See [DEPLOYMENT.md](./DEPLOYMENT.md) for complete config

2. **Create .env.example**
   - Template with all required variables
   - Documentation for each variable

3. **Create .env**
   - Copy from .env.example
   - Fill in actual values
   - Generate secure secrets

4. **Test Docker Compose**
   ```bash
   docker-compose build
   docker-compose up -d
   docker-compose ps
   docker-compose logs -f
   ```

5. **Verify all services**
   - Check postgres is healthy
   - Check mqtt-ingestor is running
   - Check web-api is accessible
   - Check web-frontend is accessible

6. **Test end-to-end flow**
   - Send MQTT message
   - Verify in database
   - Login to web UI
   - View device and data
   - Test admin panel

7. **Test service dependencies**
   - Stop postgres → other services should fail gracefully
   - Restart postgres → services should reconnect
   - Test with docker-compose down and up

8. **Document deployment**
   - Update README.md with quick start
   - Document environment variables
   - Add troubleshooting section

### Deliverable
- ✅ Complete docker-compose.yml
- ✅ All services orchestrated
- ✅ End-to-end system working
- ✅ Services restart correctly
- ✅ Documentation updated

---

## Phase 10: Testing, Documentation & Polish

**Goal:** Comprehensive testing and documentation

**Duration:** 3-5 days

### Tasks

1. **Integration testing**
   - Test complete data flow: MQTT → Database → API → Frontend
   - Test with multiple devices
   - Test with multiple users
   - Test all API endpoints
   - Test all frontend pages
   - Test error scenarios

2. **Performance testing**
   - Send burst of MQTT messages → verify no data loss
   - Query large time ranges → verify performance
   - Test dashboard with many devices
   - Monitor resource usage (docker stats)

3. **Security testing**
   - Verify JWT expiration works
   - Test token refresh
   - Test permission boundaries
   - Verify SQL injection protection
   - Test XSS prevention

4. **Write README.md**
   - Project overview
   - Quick start guide
   - Prerequisites
   - Installation steps
   - Configuration
   - Usage examples
   - Troubleshooting

5. **Document API**
   - Already auto-generated by FastAPI
   - Add examples to README
   - Document MQTT message format

6. **Add code comments**
   - Document complex logic
   - Add docstrings to functions
   - Explain non-obvious decisions

7. **Create troubleshooting guide**
   - Common issues and solutions
   - How to check logs
   - How to reset system

8. **Polish UI**
   - Fix any visual bugs
   - Improve responsive design
   - Add loading indicators
   - Improve error messages
   - Test on mobile

9. **Create sample data script** (optional)
   - Script to generate test MQTT messages
   - Script to create sample users and devices
   - Useful for demos

10. **Final testing**
    - Fresh deployment from scratch
    - Follow README.md step-by-step
    - Verify everything works
    - Fix any issues found

### Deliverable
- ✅ Fully tested system
- ✅ Complete documentation
- ✅ Polished UI
- ✅ Ready for MVP deployment
- ✅ Troubleshooting guide

---

## Post-MVP Enhancements (Future Phases)

### Phase 11: PWA Features (Optional)
- Service worker for offline support
- Web app manifest for installation
- Push notifications (Android)
- Improved mobile UX

### Phase 12: Advanced Features (Optional)
- Alert system with email/SMS
- Data export (CSV, Excel)
- Advanced analytics and charts
- User preferences and settings
- Audit log

### Phase 13: Security Hardening (Production)
- MQTT TLS and authentication
- HTTPS with Let's Encrypt
- Rate limiting
- Enhanced password requirements
- Two-factor authentication

### Phase 14: Scaling (Production)
- Multiple API instances
- Multiple ingestor instances
- Database read replicas
- Redis caching
- Load balancer
- Monitoring and alerting

### Phase 15: Native Mobile App (Optional)
- Flutter or React Native
- Same API backend
- Native UI components
- Push notifications
- Offline capability

---

## Development Tips

### Best Practices

1. **Version control:** Commit after each subtask
2. **Testing:** Test each component before moving to next phase
3. **Logging:** Add comprehensive logging from the start
4. **Documentation:** Document as you build, not at the end
5. **Incremental:** Build features incrementally, test frequently

### Common Pitfalls

1. **Don't skip database indexes** - Will cause performance issues
2. **Don't hardcode values** - Use environment variables
3. **Don't skip error handling** - Add try/catch blocks
4. **Don't skip validation** - Validate all inputs
5. **Don't test only happy paths** - Test error scenarios

### Tools

- **API Testing:** Swagger UI (http://localhost:8000/docs)
- **MQTT Testing:** mosquitto_pub, MQTT Explorer
- **Database Testing:** psql, pgAdmin, DBeaver
- **Frontend Testing:** Browser DevTools, Vue DevTools
- **Container Testing:** docker logs, docker exec

### Getting Help

- FastAPI docs: https://fastapi.tiangolo.com/
- Vue.js docs: https://vuejs.org/
- TimescaleDB docs: https://docs.timescale.com/
- paho-mqtt docs: https://www.eclipse.org/paho/index.php?page=clients/python/docs/index.php

---

## Success Criteria

By the end of Phase 10, you should have:

- ✅ Thousands of IoT devices can send sensor data via MQTT
- ✅ Data is stored efficiently in TimescaleDB
- ✅ Users can log in and view their assigned devices
- ✅ Historical charts show sensor trends over time
- ✅ Admin can manage users, devices, and assignments
- ✅ System runs in Docker containers
- ✅ All services restart automatically
- ✅ Comprehensive documentation

**Congratulations!** You've built a production-ready MVP IoT monitoring platform.
