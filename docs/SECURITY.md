# Security Documentation

## Overview

This document outlines the security considerations, implementations, and best practices for the DuoClean Energy IoT monitoring platform. It covers both MVP security (basic but functional) and production recommendations.

## Security Model

### Threat Model

**Assets to Protect:**
1. User credentials and authentication tokens
2. Sensor data (confidentiality and integrity)
3. Database access
4. API endpoints
5. MQTT broker access

**Threat Actors:**
1. External attackers (internet-facing services)
2. Malicious insiders with user accounts
3. Compromised IoT devices
4. Man-in-the-middle attackers (network level)

**Attack Vectors:**
1. Credential theft or brute force
2. SQL injection
3. Cross-site scripting (XSS)
4. Cross-site request forgery (CSRF)
5. Unauthorized API access
6. MQTT message injection or tampering
7. Token theft or replay
8. Denial of service

## MVP Security Implementation

### Authentication

#### Password Security

**Implementation:**
- Passwords hashed with bcrypt (cost factor: 12)
- Salt automatically included in bcrypt hash
- Passwords never stored in plaintext
- No password recovery (admin resets in MVP)

**bcrypt Configuration:**
```python
from passlib.context import CryptContext

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")

# Hash password
password_hash = pwd_context.hash(plain_password)

# Verify password
is_valid = pwd_context.verify(plain_password, password_hash)
```

**Strength:**
- bcrypt is designed to be slow (prevents brute force)
- Cost factor of 12 means ~250ms to hash on modern CPU
- Adaptive: can increase cost factor as hardware improves

**Weaknesses (MVP):**
- No password complexity requirements
- No account lockout after failed attempts
- No password expiration

#### JWT Token Security

**Implementation:**
- Access tokens: 1 hour expiration
- Refresh tokens: 7 days expiration
- Signed with HS256 (HMAC-SHA256)
- Secret key from environment variable

**Token Structure:**
```json
{
  "sub": "user_id",
  "username": "john_doe",
  "role": "user",
  "exp": 1674567890,
  "iat": 1674564290
}
```

**Token Generation:**
```python
from jose import jwt
from datetime import datetime, timedelta

def create_access_token(data: dict):
    to_encode = data.copy()
    expire = datetime.utcnow() + timedelta(minutes=60)
    to_encode.update({"exp": expire})
    encoded_jwt = jwt.encode(to_encode, JWT_SECRET, algorithm=JWT_ALGORITHM)
    return encoded_jwt
```

**Strength:**
- Short-lived access tokens limit exposure
- Stateless (no server-side session storage)
- Cryptographically signed (can't be tampered with)

**Weaknesses (MVP):**
- HS256 requires shared secret (less secure than RS256)
- No token revocation mechanism
- Tokens stored in localStorage (vulnerable to XSS)
- No token rotation

### Authorization

#### Role-Based Access Control (RBAC)

**Roles:**
- **admin:** Full access to all resources
- **user:** Limited access to assigned devices only

**Implementation:**
```python
def require_admin(current_user: User = Depends(get_current_user)):
    if current_user.role != "admin":
        raise HTTPException(status_code=403, detail="Not enough permissions")
    return current_user

def check_device_access(device_id: str, current_user: User = Depends(get_current_user)):
    # Admin has access to all devices
    if current_user.role == "admin":
        return True

    # Check if user has access to this device
    has_access = db.query(UserDeviceAccess).filter(
        UserDeviceAccess.user_id == current_user.id,
        UserDeviceAccess.device_id == device_id
    ).first()

    if not has_access:
        raise HTTPException(status_code=403, detail="Access denied")

    return True
```

**Strength:**
- Clear separation of privileges
- Row-level security via database joins
- Enforced at API level

**Weaknesses (MVP):**
- Only two roles (not granular)
- No attribute-based access control (ABAC)
- No audit logging of access attempts

### API Security

#### Input Validation

**Implementation:**
- Pydantic models for request validation
- Type checking
- String length limits
- Email format validation
- Enum validation for fixed values

**Example:**
```python
class UserCreate(BaseModel):
    username: str = Field(..., min_length=3, max_length=50, regex="^[a-zA-Z0-9_]+$")
    email: EmailStr
    password: str = Field(..., min_length=8)
    role: Literal["admin", "user"]
```

**Strength:**
- Automatic validation before reaching business logic
- Type safety
- Clear error messages

**Weaknesses (MVP):**
- No password complexity requirements
- Limited business logic validation

#### SQL Injection Protection

**Implementation:**
- SQLAlchemy ORM with parameterized queries
- Never use string concatenation for queries
- User input never directly in SQL

**Example (Safe):**
```python
# SQLAlchemy automatically parameterizes
user = db.query(User).filter(User.username == username).first()

# Manual parameterization if needed
db.execute("SELECT * FROM users WHERE username = :username", {"username": username})
```

**Example (UNSAFE - Never do this):**
```python
# DANGEROUS: SQL injection vulnerability
db.execute(f"SELECT * FROM users WHERE username = '{username}'")
```

**Strength:**
- Industry-standard protection
- ORM handles parameterization automatically

**Weaknesses:**
- None if used correctly

#### Cross-Site Scripting (XSS) Protection

**Implementation:**
- Vue.js escapes all variables by default
- No use of `v-html` with user input
- CSP headers in Nginx (recommended)

**Nginx CSP Header:**
```nginx
add_header Content-Security-Policy "default-src 'self'; script-src 'self' https://cdn.jsdelivr.net; style-src 'self' 'unsafe-inline';" always;
```

**Strength:**
- Vue.js provides automatic escaping
- CSP provides defense in depth

**Weaknesses (MVP):**
- Tokens in localStorage accessible by JavaScript
- CSP not configured by default

#### Cross-Site Request Forgery (CSRF) Protection

**Implementation:**
- Not needed for JWT-based stateless API
- Tokens in Authorization header (not cookies)
- No state-changing GET requests

**Why CSRF not a concern:**
- JWT tokens are not automatically sent by browser (unlike cookies)
- Attacker can't force victim's browser to send Authorization header
- All state-changing requests require POST/PUT/DELETE with JWT

**Note:** If switching to cookie-based auth, CSRF protection is required.

#### CORS Configuration

**Implementation:**
```python
from fastapi.middleware.cors import CORSMiddleware

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000"],  # Frontend URL
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
```

**Strength:**
- Restricts which origins can make requests
- Prevents unauthorized frontend from accessing API

**Weaknesses (MVP):**
- Wildcard for methods and headers (overly permissive)
- Should be more restrictive in production

### Database Security

#### Connection Security

**Implementation:**
- Database credentials in environment variables
- Connection pooling limits concurrent connections
- Database not exposed to internet (internal Docker network)

**Environment Variables:**
```env
POSTGRES_HOST=postgres  # Docker service name
POSTGRES_PASSWORD=secure_password_here
```

**Strength:**
- Credentials not in code
- Network isolation

**Weaknesses (MVP):**
- No connection encryption (TLS)
- Port 5432 exposed for debugging (remove in production)
- Shared database credentials (all services use same user)

#### Data Protection

**Implementation:**
- Password hashes (never plaintext)
- No encryption at rest (MVP)
- No column-level encryption

**Strength:**
- Passwords protected even if database compromised

**Weaknesses (MVP):**
- Sensor data not encrypted
- No encryption at rest
- No encryption in transit (within Docker network)

### MQTT Security

#### MVP Configuration

**Implementation:**
- No authentication required
- No TLS encryption
- No ACLs (access control lists)
- Devices can publish to any topic

**Acceptable for:**
- Local network deployment
- Trusted devices
- Development/testing
- Proof of concept

**Risks:**
- Anyone on network can publish fake data
- Anyone can subscribe to all topics
- Messages sent in plaintext
- Devices can impersonate each other

### Frontend Security

#### Token Storage

**Implementation:**
- Access token in localStorage
- Refresh token in localStorage
- User info in sessionStorage

**Strength:**
- Simple and functional
- Tokens persist across page reloads

**Weaknesses:**
- Vulnerable to XSS attacks
- JavaScript can access tokens
- Tokens not httpOnly

**Alternative (Production):**
- Store tokens in httpOnly cookies
- Backend sets cookies after login
- Browser automatically includes cookies
- JavaScript cannot access cookies (XSS protection)

#### Authentication Flow

**Implementation:**
1. User enters credentials
2. Frontend POSTs to /auth/login
3. Backend validates and returns tokens
4. Frontend stores tokens
5. Frontend includes token in Authorization header

**Token Refresh:**
- When API returns 401, attempt token refresh
- If refresh succeeds, retry original request
- If refresh fails, redirect to login

**Strength:**
- Automatic token refresh
- User doesn't need to re-login frequently

**Weaknesses:**
- No token rotation
- Refresh token long-lived (7 days)

## Production Security Recommendations

### Authentication Enhancements

#### Password Requirements

**Implement:**
- Minimum 12 characters
- Must include uppercase, lowercase, digit, special character
- Check against common password list (HIBP API)
- Prevent password reuse

```python
import re

def validate_password_strength(password: str) -> bool:
    if len(password) < 12:
        return False
    if not re.search(r"[A-Z]", password):
        return False
    if not re.search(r"[a-z]", password):
        return False
    if not re.search(r"\d", password):
        return False
    if not re.search(r"[!@#$%^&*(),.?\":{}|<>]", password):
        return False
    return True
```

#### Account Protection

**Implement:**
- Rate limiting on login endpoint (5 attempts per 15 minutes)
- Account lockout after failed attempts
- CAPTCHA after multiple failures
- Email alerts for suspicious login attempts
- Password expiration (optional)

```python
from slowapi import Limiter
from slowapi.util import get_remote_address

limiter = Limiter(key_func=get_remote_address)

@app.post("/auth/login")
@limiter.limit("5/15minutes")
async def login(request: Request, credentials: LoginRequest):
    # ... login logic
```

#### Two-Factor Authentication (2FA)

**Implement:**
- TOTP (Time-based One-Time Password) support
- Backup codes
- SMS fallback (optional)
- U2F/WebAuthn hardware keys (optional)

**Libraries:**
- PyOTP for TOTP generation/verification
- QR code generation for setup

#### Token Security Enhancements

**Implement:**
- Use RS256 (asymmetric) instead of HS256
- Token rotation on refresh
- Token revocation list (Redis)
- Shorter access token expiry (15 minutes)
- Fingerprint tokens to device/IP

```python
# Token revocation check
def is_token_revoked(jti: str) -> bool:
    return redis.exists(f"revoked_token:{jti}")

def revoke_token(jti: str, expiry: int):
    redis.setex(f"revoked_token:{jti}", expiry, "1")
```

### API Security Enhancements

#### Rate Limiting

**Implement:**
- Per-endpoint rate limits
- Per-user rate limits
- Adaptive rate limiting (stricter after suspicious activity)

```python
@app.get("/devices")
@limiter.limit("100/minute")
async def get_devices(request: Request):
    # ... logic
```

#### API Key Authentication

**For programmatic access:**
- Generate API keys for external integrations
- Store hashed (like passwords)
- Scope API keys to specific resources
- Allow revocation

#### Request Validation

**Implement:**
- Maximum request size limits
- Timeout for long-running requests
- Request ID for tracing
- User-Agent validation (optional)

#### Security Headers

**Nginx Configuration:**
```nginx
add_header X-Frame-Options "DENY" always;
add_header X-Content-Type-Options "nosniff" always;
add_header X-XSS-Protection "1; mode=block" always;
add_header Referrer-Policy "strict-origin-when-cross-origin" always;
add_header Content-Security-Policy "default-src 'self'" always;
add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;
```

### MQTT Security

#### TLS Encryption

**Mosquitto Configuration:**
```conf
listener 8883
cafile /etc/mosquitto/ca_certificates/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key
require_certificate true
```

**Client Configuration:**
```python
client.tls_set(
    ca_certs="ca.crt",
    certfile="client.crt",
    keyfile="client.key"
)
```

#### Authentication

**Username/Password per Device:**
```conf
# Mosquitto password file
device_001:$6$hashed_password_here
device_002:$6$hashed_password_here
```

**ACL (Access Control Lists):**
```conf
# Only allow device_001 to publish to its own topic
user device_001
topic write sensors/device_001/#

user device_002
topic write sensors/device_002/#
```

#### Message Validation

**Implement:**
- HMAC signature in message payload
- Verify signature before processing
- Prevents message tampering

```python
import hmac
import hashlib

def verify_message_signature(message: dict, signature: str, device_secret: str) -> bool:
    expected_signature = hmac.new(
        device_secret.encode(),
        json.dumps(message, sort_keys=True).encode(),
        hashlib.sha256
    ).hexdigest()
    return hmac.compare_digest(expected_signature, signature)
```

### Database Security

#### Connection Encryption

**Enable SSL/TLS:**
```python
engine = create_engine(
    DATABASE_URL,
    connect_args={
        "sslmode": "require",
        "sslrootcert": "ca.crt",
        "sslcert": "client.crt",
        "sslkey": "client.key"
    }
)
```

#### Principle of Least Privilege

**Create separate database users:**
- `ingestor_user`: INSERT only on sensor_readings, devices
- `api_user`: SELECT, UPDATE on all tables
- `admin_user`: Full access for migrations

```sql
CREATE USER ingestor_user WITH PASSWORD 'password';
GRANT INSERT ON sensor_readings TO ingestor_user;
GRANT INSERT, SELECT ON devices TO ingestor_user;
```

#### Data Encryption at Rest

**Enable PostgreSQL encryption:**
- Use encrypted filesystem (LUKS)
- Use managed database with encryption (AWS RDS, GCP Cloud SQL)
- Consider pgcrypto for column-level encryption

```sql
-- Column-level encryption (performance impact)
CREATE EXTENSION pgcrypto;

INSERT INTO sensitive_data (id, encrypted_field)
VALUES (1, pgp_sym_encrypt('sensitive value', 'encryption_key'));

SELECT pgp_sym_decrypt(encrypted_field, 'encryption_key') FROM sensitive_data;
```

#### Backup Security

**Implement:**
- Encrypt backups
- Store backups in separate location
- Test restore procedures
- Limit access to backups

```bash
# Encrypted backup
pg_dump -U postgres duoclean | gpg --encrypt --recipient admin@example.com > backup.sql.gpg

# Restore
gpg --decrypt backup.sql.gpg | psql -U postgres duoclean
```

### Network Security

#### HTTPS/TLS

**Use Let's Encrypt with Certbot:**
```bash
certbot --nginx -d app.duocleanenergy.com
```

**Nginx HTTPS Configuration:**
```nginx
server {
    listen 443 ssl http2;
    server_name app.duocleanenergy.com;

    ssl_certificate /etc/letsencrypt/live/app.duocleanenergy.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/app.duocleanenergy.com/privkey.pem;

    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    ssl_prefer_server_ciphers on;

    # HSTS
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;

    # ... rest of config
}

# Redirect HTTP to HTTPS
server {
    listen 80;
    server_name app.duocleanenergy.com;
    return 301 https://$server_name$request_uri;
}
```

#### Firewall Rules

**Only expose necessary ports:**
```bash
# ufw firewall
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow 22/tcp  # SSH
sudo ufw allow 80/tcp  # HTTP (redirects to HTTPS)
sudo ufw allow 443/tcp # HTTPS
sudo ufw enable
```

**Docker network isolation:**
- Use internal networks for inter-service communication
- Don't expose database port externally
- Use reverse proxy for all external access

#### VPN for Admin Access

**Implement:**
- WireGuard or OpenVPN for admin access
- Admin panel only accessible via VPN
- Database admin tools only via VPN
- Logs and monitoring only via VPN

### Monitoring and Auditing

#### Audit Logging

**Log security events:**
- Login attempts (success and failure)
- Password changes
- User creation/deletion
- Role changes
- Device assignment changes
- API key creation/revocation

**Implementation:**
```python
def log_security_event(event_type: str, user_id: int, details: dict):
    audit_log = AuditLog(
        event_type=event_type,
        user_id=user_id,
        details=json.dumps(details),
        ip_address=request.client.host,
        timestamp=datetime.utcnow()
    )
    db.add(audit_log)
    db.commit()

# Usage
log_security_event("user_login", user.id, {"username": user.username})
```

#### Intrusion Detection

**Monitor for suspicious activity:**
- Multiple failed login attempts
- Login from unusual location/IP
- Large number of API requests
- Database query anomalies
- Unusual MQTT traffic patterns

**Tools:**
- Fail2ban for blocking IPs
- Prometheus alerts
- ELK stack for log analysis

#### Security Scanning

**Regular scans:**
- Dependency vulnerability scanning (pip-audit, safety)
- Container image scanning (Trivy, Clair)
- Code security scanning (Bandit, Semgrep)
- Penetration testing

```bash
# Scan Python dependencies
pip-audit

# Scan Docker image
trivy image duoclean/web-api:latest

# Scan Python code
bandit -r src/
```

## Security Checklist

### Pre-Deployment

- [ ] Change default admin password
- [ ] Generate strong JWT secret
- [ ] Generate strong database password
- [ ] Review and restrict CORS origins
- [ ] Enable HTTPS/TLS
- [ ] Configure firewall rules
- [ ] Remove database port exposure
- [ ] Enable MQTT authentication (production)
- [ ] Enable MQTT TLS (production)
- [ ] Set up backup encryption
- [ ] Configure security headers
- [ ] Set up rate limiting
- [ ] Enable audit logging
- [ ] Set up monitoring and alerts
- [ ] Review all environment variables
- [ ] Scan for vulnerabilities
- [ ] Test authentication flows
- [ ] Test authorization boundaries
- [ ] Document security procedures

### Ongoing

- [ ] Rotate secrets quarterly
- [ ] Review audit logs weekly
- [ ] Update dependencies monthly
- [ ] Scan for vulnerabilities weekly
- [ ] Review access permissions monthly
- [ ] Test backups monthly
- [ ] Security training for team
- [ ] Incident response plan
- [ ] Penetration testing annually

## Incident Response

### Security Incident Procedure

1. **Detection:**
   - Alert triggered or suspicious activity reported
   - Identify scope of incident

2. **Containment:**
   - Revoke compromised credentials
   - Block malicious IPs
   - Isolate affected services if needed

3. **Investigation:**
   - Review audit logs
   - Identify attack vector
   - Assess data exposure

4. **Eradication:**
   - Fix vulnerability
   - Remove malicious code/access
   - Update dependencies

5. **Recovery:**
   - Restore from clean backups if needed
   - Verify system integrity
   - Resume normal operations

6. **Post-Mortem:**
   - Document incident
   - Update security procedures
   - Implement preventive measures

### Security Contacts

- Security lead: [contact]
- Infrastructure admin: [contact]
- Legal/compliance: [contact]
- External security consultant: [contact]

## Compliance Considerations

### Data Protection

**GDPR (if applicable):**
- User consent for data collection
- Right to access personal data
- Right to delete personal data
- Data portability
- Privacy policy

**HIPAA (if handling health data):**
- Additional encryption requirements
- Audit controls
- Access restrictions

### Industry Standards

**OWASP Top 10:**
- Address all OWASP vulnerabilities
- Regular security testing
- Security training

**ISO 27001 (optional):**
- Information security management system
- Risk assessment
- Security controls

## Resources

- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [OWASP API Security Top 10](https://owasp.org/www-project-api-security/)
- [FastAPI Security](https://fastapi.tiangolo.com/tutorial/security/)
- [Vue.js Security](https://vuejs.org/guide/best-practices/security.html)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
- [CIS Controls](https://www.cisecurity.org/controls)
