# Import Path Fix Applied

## Issues Fixed

### Issue 1: Module Import Errors
The web-api service was failing to start with:
```
ModuleNotFoundError: No module named 'config'
```

**Root Cause:**
Python imports were using relative imports without the `src.` prefix, but uvicorn was running from `/app`, making the imports fail.

### Issue 2: Missing email-validator Dependency
After fixing imports, encountered:
```
ImportError: email-validator is not installed, run `pip install pydantic[email]`
```

**Root Cause:**
Pydantic requires the `email-validator` package for EmailStr field validation, but it wasn't included in requirements.txt.

### Issue 3: Pydantic Forward Reference Error
After fixing the dependency, encountered:
```
pydantic.errors.PydanticUndefinedAnnotation: name 'SensorReadingResponse' is not defined
```

**Root Cause:**
Forward references in Pydantic v2 (like `Optional['SensorReadingResponse']`) require explicit model rebuilding after all classes are defined.

### Issue 4: bcrypt Compatibility Error
After fixing forward references, login endpoint failed with:
```
AttributeError: module 'bcrypt' has no attribute '__about__'
ValueError: password cannot be longer than 72 bytes
```

**Root Cause:**
The latest bcrypt library (v4.x) has breaking changes that are incompatible with passlib 1.7.4. Passlib tries to access `_bcrypt.__about__.__version__` which no longer exists in bcrypt 4.1+.

## Solutions Applied

### Fix 1: Updated Import Paths
Updated all import statements in the web-api to use the `src.` prefix:

### Files Fixed:

1. **src/main.py**
   - `from config import settings` → `from src.config import settings`
   - `from routers import ...` → `from src.routers import ...`
   - `import schemas` → `from src import schemas`

2. **src/database.py**
   - `from config import settings` → `from src.config import settings`

3. **src/models.py**
   - `from database import Base` → `from src.database import Base`

4. **src/auth.py**
   - `from config import settings` → `from src.config import settings`

5. **src/dependencies.py**
   - `from database import get_db` → `from src.database import get_db`
   - `from auth import decode_token` → `from src.auth import decode_token`
   - `import models` → `from src import models`
   - `import schemas` → `from src import schemas`

6. **src/routers/auth.py**
   - All imports updated to use `src.` prefix

7. **src/routers/users.py**
   - All imports updated to use `src.` prefix

8. **src/routers/devices.py**
   - All imports updated to use `src.` prefix

9. **src/routers/readings.py**
   - All imports updated to use `src.` prefix

### Fix 2: Added Missing Dependency
Added `email-validator==2.1.0` to [web-api/requirements.txt](web-api/requirements.txt) to support Pydantic EmailStr field validation.

### Fix 3: Fixed Pydantic Forward References
Added `model_rebuild()` calls at the end of [web-api/src/schemas.py](web-api/src/schemas.py) for all models that use forward references:
- `UserWithDevices.model_rebuild()`
- `DeviceWithLatestReading.model_rebuild()`
- `DashboardDevice.model_rebuild()`
- `DashboardResponse.model_rebuild()`

### Fix 4: Fixed bcrypt Version Compatibility
Pinned `bcrypt==4.0.1` in [web-api/requirements.txt](web-api/requirements.txt) to use a compatible version that works with passlib 1.7.4. Version 4.0.1 is the last version before breaking changes in 4.1+.

## Testing

After applying all four fixes, rebuild and restart the web-api service:

```bash
# If docker-compose works on your system:
docker-compose up -d --build web-api

# Or stop and start all services:
docker-compose down
docker-compose up -d --build

# Check logs:
docker-compose logs -f web-api
```

## Expected Result

The web-api service should now start successfully and show:
```
INFO:     Started server process [1]
INFO:     Waiting for application startup.
INFO:     ============================================================
INFO:     DuoClean Energy API starting up
INFO:     Environment: INFO
INFO:     Database: postgres:5432/duoclean
INFO:     CORS origins: ['http://localhost:3000', 'http://localhost']
INFO:     ============================================================
INFO:     Application startup complete.
INFO:     Uvicorn running on http://0.0.0.0:8000 (Press CTRL+C to quit)
```

Then you can access:
- **API Docs:** http://localhost:8000/docs
- **API Root:** http://localhost:8000/

## Manual Testing (if Docker issues persist)

If you continue to have Docker compatibility issues, you can test the API locally:

```bash
cd web-api

# Install dependencies
pip install -r requirements.txt

# Set environment variables
export POSTGRES_HOST=localhost
export POSTGRES_PORT=5432
export POSTGRES_DB=duoclean
export POSTGRES_USER=duoclean_user
export POSTGRES_PASSWORD=your_password
export JWT_SECRET=your_secret_key
export API_CORS_ORIGINS=http://localhost:3000

# Run the API
uvicorn src.main:app --reload --host 0.0.0.0 --port 8000
```

Then access http://localhost:8000/docs to test.

## All Fixes Complete ✅

The web-api codebase is now correctly configured with:
- ✅ Proper Python module imports with `src.` prefix
- ✅ All required dependencies including `email-validator`
- ✅ Pydantic forward references properly resolved
- ✅ Compatible bcrypt library version (4.0.1)
- ✅ Ready to run inside Docker containers
