# Web Frontend Documentation

## Overview

The web frontend is a responsive single-page application built with Vue.js 3 (via CDN), vanilla JavaScript, and Chart.js for data visualization. It provides user authentication, device monitoring, and admin management capabilities.

## Technology Stack

- **Vue.js 3** - Reactive UI framework (loaded via CDN, no build step)
- **Chart.js** - Time-series data visualization
- **Vanilla JavaScript** - API client, auth utilities
- **HTML5/CSS3** - Markup and styling
- **Nginx** - Static file server

## Project Structure

```
web-frontend/
├── Dockerfile
├── nginx.conf
└── public/
    ├── index.html              # Login page
    ├── dashboard.html          # Main dashboard (device list)
    ├── device-detail.html      # Device detail with charts
    ├── admin.html              # Admin panel
    ├── css/
    │   └── style.css           # Main stylesheet
    └── js/
        ├── auth.js             # Authentication utilities
        ├── api.js              # API client wrapper
        ├── dashboard.js        # Dashboard Vue app
        ├── device-detail.js    # Device detail Vue app
        └── admin.js            # Admin panel Vue app
```

## Pages

### 1. Login Page (index.html)

**Purpose:** User authentication entry point

**Features:**
- Username/password form
- Login button with loading state
- Error message display
- Redirect to dashboard on success

**Flow:**
1. User enters credentials
2. Submit → Call `/auth/login` API
3. Store JWT tokens in localStorage
4. Store user info in sessionStorage
5. Redirect to dashboard.html

**Layout:**
```
┌────────────────────────────┐
│                            │
│    DuoClean Energy         │
│    IoT Monitoring          │
│                            │
│  ┌──────────────────────┐  │
│  │ Username             │  │
│  └──────────────────────┘  │
│  ┌──────────────────────┐  │
│  │ Password             │  │
│  └──────────────────────┘  │
│                            │
│      [  Login  ]           │
│                            │
│    Error message here      │
│                            │
└────────────────────────────┘
```

### 2. Dashboard (dashboard.html)

**Purpose:** Overview of all accessible devices with latest readings

**Features:**
- Device grid/cards with latest sensor values
- Auto-refresh every 30 seconds
- Click device → navigate to detail page
- Admin button (admin users only)
- Logout button
- Status indicators (OK, low battery, no data)

**Layout:**
```
┌─────────────────────────────────────────────┐
│ DuoClean Energy         [Admin] [Logout]    │
├─────────────────────────────────────────────┤
│                                             │
│  Welcome, username (role)                   │
│                                             │
│  ┌───────────┐  ┌───────────┐              │
│  │ Device 001│  │ Device 002│              │
│  │ Sensor A  │  │ Sensor B  │              │
│  │           │  │           │              │
│  │ Load: 45.2│  │ Load: 38.5│              │
│  │ Volt: 12.6│  │ Volt: 11.2│ ⚠️           │
│  │ Temp: 23.5│  │ Temp: 25.1│              │
│  │           │  │           │              │
│  │ Updated   │  │ Updated   │              │
│  │ 2 min ago │  │ 5 min ago │              │
│  └───────────┘  └───────────┘              │
│                                             │
└─────────────────────────────────────────────┘
```

**Device Card Status:**
- **Green border:** All readings normal
- **Yellow border:** Low battery (< 11.5V)
- **Red border:** No data in last 24 hours
- **Gray border:** Unknown/error

**Auto-refresh:**
- Polls `/dashboard` API every 30 seconds
- Updates device cards without page reload
- Shows "Updating..." indicator during fetch

### 3. Device Detail (device-detail.html)

**Purpose:** Historical data and charts for a single device

**Features:**
- Device info header
- Latest readings (large numbers)
- Time range selector (1h, 6h, 24h, 7d, 30d)
- Three line charts:
  - Load over time
  - Battery voltage over time
  - Temperature over time
- Back to dashboard button

**Layout:**
```
┌─────────────────────────────────────────────┐
│ ← Back to Dashboard              [Logout]   │
├─────────────────────────────────────────────┤
│                                             │
│  Device 001 - Sensor A                      │
│  Building A, Floor 2                        │
│                                             │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐       │
│  │ 45.2    │ │ 12.6V   │ │ 23.5°C  │       │
│  │ Load    │ │ Battery │ │ Temp    │       │
│  └─────────┘ └─────────┘ └─────────┘       │
│                                             │
│  Time Range: [1h][6h][24h][7d][30d]        │
│                                             │
│  Load Chart                                 │
│  ┌───────────────────────────────────────┐ │
│  │     /\    /\                          │ │
│  │    /  \  /  \  /\                     │ │
│  │   /    \/    \/  \                    │ │
│  └───────────────────────────────────────┘ │
│                                             │
│  Battery Voltage Chart                      │
│  ┌───────────────────────────────────────┐ │
│  │  ────────────────────────────          │ │
│  └───────────────────────────────────────┘ │
│                                             │
│  Temperature Chart                          │
│  ┌───────────────────────────────────────┐ │
│  │   ─────/\─────────                     │ │
│  └───────────────────────────────────────┘ │
│                                             │
└─────────────────────────────────────────────┘
```

**Chart Features:**
- Responsive canvas sizing
- Tooltips on hover
- Time-based x-axis
- Auto-scaling y-axis
- Legend
- Grid lines

### 4. Admin Panel (admin.html)

**Purpose:** User and device management (admin only)

**Features:**
- Tabbed interface:
  1. **Users Tab:** List, create, edit, delete users
  2. **Devices Tab:** List, create, edit, delete devices
  3. **Assignments Tab:** Assign/unassign devices to users

**Layout:**
```
┌─────────────────────────────────────────────┐
│ Admin Panel                     [Dashboard] │
├─────────────────────────────────────────────┤
│                                             │
│  [Users] [Devices] [Assignments]            │
│                                             │
│  Users                         [+ New User] │
│  ┌─────────────────────────────────────┐   │
│  │ ID │ Username  │ Email    │ Role │ │   │
│  ├────┼───────────┼──────────┼──────┤ │   │
│  │ 1  │ admin     │ admin@.. │ admin│ │   │
│  │ 2  │ john_doe  │ john@... │ user │ │   │
│  │    │           │          │ [Edit]│ │   │
│  │    │           │          │ [Del ]│ │   │
│  └─────────────────────────────────────┘   │
│                                             │
│  Create New User                            │
│  ┌─────────────────────────────────────┐   │
│  │ Username: [____________]            │   │
│  │ Email:    [____________]            │   │
│  │ Password: [____________]            │   │
│  │ Role:     [User ▼]                  │   │
│  │           [Create]                  │   │
│  └─────────────────────────────────────┘   │
│                                             │
└─────────────────────────────────────────────┘
```

**Permissions:**
- Only admin users can access admin.html
- Check role on page load → redirect if not admin
- All admin API endpoints require admin role

## JavaScript Architecture

### auth.js - Authentication Utilities

**Responsibilities:**
- Token storage and retrieval
- Login/logout functions
- Token refresh logic
- Check if user is authenticated
- Get current user info

**Key Functions:**

```javascript
// Store tokens after login
function storeTokens(accessToken, refreshToken, user) {
  localStorage.setItem('access_token', accessToken);
  localStorage.setItem('refresh_token', refreshToken);
  sessionStorage.setItem('user', JSON.stringify(user));
}

// Get access token
function getAccessToken() {
  return localStorage.getItem('access_token');
}

// Check if user is authenticated
function isAuthenticated() {
  return !!getAccessToken();
}

// Get current user info
function getCurrentUser() {
  const userStr = sessionStorage.getItem('user');
  return userStr ? JSON.parse(userStr) : null;
}

// Logout
function logout() {
  localStorage.removeItem('access_token');
  localStorage.removeItem('refresh_token');
  sessionStorage.removeItem('user');
  window.location.href = '/index.html';
}

// Refresh access token
async function refreshAccessToken() {
  const refreshToken = localStorage.getItem('refresh_token');
  if (!refreshToken) {
    logout();
    return null;
  }

  try {
    const response = await fetch('/auth/refresh', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ refresh_token: refreshToken })
    });

    if (response.ok) {
      const data = await response.json();
      localStorage.setItem('access_token', data.access_token);
      return data.access_token;
    } else {
      logout();
      return null;
    }
  } catch (error) {
    console.error('Failed to refresh token:', error);
    logout();
    return null;
  }
}

// Redirect to login if not authenticated
function requireAuth() {
  if (!isAuthenticated()) {
    window.location.href = '/index.html';
  }
}

// Redirect to dashboard if authenticated
function redirectIfAuthenticated() {
  if (isAuthenticated()) {
    window.location.href = '/dashboard.html';
  }
}
```

### api.js - API Client

**Responsibilities:**
- Centralized API calls
- Automatic JWT header injection
- Error handling (401 → refresh token or logout)
- Request/response formatting

**Key Functions:**

```javascript
const API_BASE_URL = 'http://localhost:8000';

async function apiRequest(endpoint, options = {}) {
  const token = getAccessToken();
  const headers = {
    'Content-Type': 'application/json',
    ...options.headers
  };

  if (token) {
    headers['Authorization'] = `Bearer ${token}`;
  }

  const url = `${API_BASE_URL}${endpoint}`;

  try {
    let response = await fetch(url, {
      ...options,
      headers
    });

    // If 401, try refreshing token
    if (response.status === 401) {
      const newToken = await refreshAccessToken();
      if (newToken) {
        // Retry request with new token
        headers['Authorization'] = `Bearer ${newToken}`;
        response = await fetch(url, {
          ...options,
          headers
        });
      } else {
        throw new Error('Authentication failed');
      }
    }

    if (!response.ok) {
      const error = await response.json();
      throw new Error(error.detail || 'Request failed');
    }

    return await response.json();
  } catch (error) {
    console.error('API request failed:', error);
    throw error;
  }
}

// Convenience methods
const api = {
  // Auth
  login: (username, password) =>
    apiRequest('/auth/login', {
      method: 'POST',
      body: JSON.stringify({ username, password })
    }),

  // Dashboard
  getDashboard: () => apiRequest('/dashboard'),

  // Devices
  getDevices: () => apiRequest('/devices'),
  getDevice: (deviceId) => apiRequest(`/devices/${deviceId}`),
  createDevice: (data) =>
    apiRequest('/devices', {
      method: 'POST',
      body: JSON.stringify(data)
    }),
  updateDevice: (deviceId, data) =>
    apiRequest(`/devices/${deviceId}`, {
      method: 'PUT',
      body: JSON.stringify(data)
    }),
  deleteDevice: (deviceId) =>
    apiRequest(`/devices/${deviceId}`, { method: 'DELETE' }),

  // Readings
  getLatestReading: (deviceId) =>
    apiRequest(`/devices/${deviceId}/latest`),
  getReadings: (deviceId, startTime, endTime, limit) =>
    apiRequest(
      `/devices/${deviceId}/readings?start_time=${startTime}&end_time=${endTime}&limit=${limit}`
    ),
  getAggregates: (deviceId, startTime, endTime, interval) =>
    apiRequest(
      `/devices/${deviceId}/aggregates?start_time=${startTime}&end_time=${endTime}&interval=${interval}`
    ),

  // Users (admin only)
  getUsers: () => apiRequest('/users'),
  createUser: (data) =>
    apiRequest('/users', {
      method: 'POST',
      body: JSON.stringify(data)
    }),
  updateUser: (userId, data) =>
    apiRequest(`/users/${userId}`, {
      method: 'PUT',
      body: JSON.stringify(data)
    }),
  deleteUser: (userId) =>
    apiRequest(`/users/${userId}`, { method: 'DELETE' }),

  // Device assignments
  assignDevice: (deviceId, userId) =>
    apiRequest(`/devices/${deviceId}/assign`, {
      method: 'POST',
      body: JSON.stringify({ user_id: userId })
    }),
  unassignDevice: (deviceId, userId) =>
    apiRequest(`/devices/${deviceId}/unassign/${userId}`, {
      method: 'DELETE'
    })
};
```

## Vue.js Components

### Dashboard App (dashboard.js)

```javascript
const { createApp } = Vue;

createApp({
  data() {
    return {
      devices: [],
      user: null,
      loading: true,
      error: null,
      autoRefreshInterval: null
    };
  },
  computed: {
    isAdmin() {
      return this.user && this.user.role === 'admin';
    }
  },
  methods: {
    async loadDashboard() {
      try {
        this.loading = true;
        const data = await api.getDashboard();
        this.devices = data.devices;
        this.user = data.user;
        this.error = null;
      } catch (error) {
        this.error = 'Failed to load dashboard: ' + error.message;
      } finally {
        this.loading = false;
      }
    },
    goToDevice(deviceId) {
      window.location.href = `/device-detail.html?device_id=${deviceId}`;
    },
    goToAdmin() {
      window.location.href = '/admin.html';
    },
    logout() {
      logout();
    },
    getDeviceStatus(device) {
      if (!device.latest_reading) return 'no_data';
      if (device.latest_reading.battery_voltage < 11.5) return 'low_battery';
      return 'ok';
    },
    formatTime(timestamp) {
      const date = new Date(timestamp);
      const now = new Date();
      const diffMs = now - date;
      const diffMins = Math.floor(diffMs / 60000);

      if (diffMins < 1) return 'Just now';
      if (diffMins < 60) return `${diffMins} min ago`;
      const diffHours = Math.floor(diffMins / 60);
      if (diffHours < 24) return `${diffHours} hour${diffHours > 1 ? 's' : ''} ago`;
      const diffDays = Math.floor(diffHours / 24);
      return `${diffDays} day${diffDays > 1 ? 's' : ''} ago`;
    },
    startAutoRefresh() {
      this.autoRefreshInterval = setInterval(() => {
        this.loadDashboard();
      }, 30000); // 30 seconds
    },
    stopAutoRefresh() {
      if (this.autoRefreshInterval) {
        clearInterval(this.autoRefreshInterval);
      }
    }
  },
  mounted() {
    requireAuth();
    this.user = getCurrentUser();
    this.loadDashboard();
    this.startAutoRefresh();
  },
  unmounted() {
    this.stopAutoRefresh();
  }
}).mount('#app');
```

### Device Detail App (device-detail.js)

```javascript
createApp({
  data() {
    return {
      deviceId: null,
      device: null,
      latestReading: null,
      timeRange: '24h',
      charts: {
        load: null,
        voltage: null,
        temperature: null
      },
      loading: true,
      error: null
    };
  },
  methods: {
    async loadDevice() {
      try {
        this.loading = true;
        this.device = await api.getDevice(this.deviceId);
        this.latestReading = this.device.latest_reading;
      } catch (error) {
        this.error = 'Failed to load device: ' + error.message;
      } finally {
        this.loading = false;
      }
    },
    async loadCharts() {
      const { startTime, endTime, useAggregates } = this.getTimeRange();

      try {
        let data;
        if (useAggregates) {
          data = await api.getAggregates(this.deviceId, startTime, endTime, '1h');
          this.renderAggregateCharts(data.aggregates);
        } else {
          data = await api.getReadings(this.deviceId, startTime, endTime, 1000);
          this.renderReadingCharts(data.readings);
        }
      } catch (error) {
        console.error('Failed to load charts:', error);
      }
    },
    getTimeRange() {
      const now = new Date();
      let startTime;
      let useAggregates = false;

      switch (this.timeRange) {
        case '1h':
          startTime = new Date(now - 1 * 60 * 60 * 1000);
          break;
        case '6h':
          startTime = new Date(now - 6 * 60 * 60 * 1000);
          break;
        case '24h':
          startTime = new Date(now - 24 * 60 * 60 * 1000);
          break;
        case '7d':
          startTime = new Date(now - 7 * 24 * 60 * 60 * 1000);
          useAggregates = true;
          break;
        case '30d':
          startTime = new Date(now - 30 * 24 * 60 * 60 * 1000);
          useAggregates = true;
          break;
      }

      return {
        startTime: startTime.toISOString(),
        endTime: now.toISOString(),
        useAggregates
      };
    },
    renderReadingCharts(readings) {
      // Render charts with raw readings
      // (Chart.js implementation)
    },
    renderAggregateCharts(aggregates) {
      // Render charts with aggregated data
      // (Chart.js implementation)
    },
    selectTimeRange(range) {
      this.timeRange = range;
      this.loadCharts();
    },
    goBack() {
      window.location.href = '/dashboard.html';
    }
  },
  mounted() {
    requireAuth();
    const params = new URLSearchParams(window.location.search);
    this.deviceId = params.get('device_id');
    if (!this.deviceId) {
      window.location.href = '/dashboard.html';
      return;
    }
    this.loadDevice();
    this.loadCharts();
  }
}).mount('#app');
```

## Chart.js Integration

### Example: Load Chart

```javascript
function createLoadChart(canvasId, readings) {
  const ctx = document.getElementById(canvasId).getContext('2d');

  const chart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: readings.map(r => new Date(r.time)),
      datasets: [{
        label: 'Load',
        data: readings.map(r => r.load),
        borderColor: 'rgb(75, 192, 192)',
        backgroundColor: 'rgba(75, 192, 192, 0.1)',
        tension: 0.1,
        fill: true
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: {
          type: 'time',
          time: {
            unit: 'hour'
          },
          title: {
            display: true,
            text: 'Time'
          }
        },
        y: {
          title: {
            display: true,
            text: 'Load'
          }
        }
      },
      plugins: {
        legend: {
          display: true,
          position: 'top'
        },
        tooltip: {
          mode: 'index',
          intersect: false
        }
      }
    }
  });

  return chart;
}
```

## Styling (CSS)

### Design System

**Colors:**
- Primary: #2563eb (blue)
- Success: #10b981 (green)
- Warning: #f59e0b (yellow/orange)
- Danger: #ef4444 (red)
- Background: #f9fafb (light gray)
- Text: #111827 (dark gray)

**Typography:**
- Font family: system-ui, -apple-system, sans-serif
- Heading: 24px, bold
- Subheading: 18px, semibold
- Body: 16px, normal
- Small: 14px

**Spacing:**
- Small: 8px
- Medium: 16px
- Large: 24px
- XLarge: 32px

### Responsive Design

**Breakpoints:**
- Mobile: < 640px
- Tablet: 640px - 1024px
- Desktop: > 1024px

**Mobile-first approach:**
```css
/* Mobile default */
.device-grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 16px;
}

/* Tablet */
@media (min-width: 640px) {
  .device-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}

/* Desktop */
@media (min-width: 1024px) {
  .device-grid {
    grid-template-columns: repeat(3, 1fr);
  }
}
```

## Nginx Configuration

```nginx
server {
  listen 80;
  server_name localhost;

  root /usr/share/nginx/html;
  index index.html;

  # Serve static files
  location / {
    try_files $uri $uri/ /index.html;
  }

  # Proxy API requests to backend
  location /api/ {
    proxy_pass http://web-api:8000/;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection 'upgrade';
    proxy_set_header Host $host;
    proxy_cache_bypass $http_upgrade;
  }

  # Security headers
  add_header X-Frame-Options "SAMEORIGIN" always;
  add_header X-Content-Type-Options "nosniff" always;
  add_header X-XSS-Protection "1; mode=block" always;

  # Gzip compression
  gzip on;
  gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;
}
```

## Dockerfile

```dockerfile
FROM nginx:alpine

# Copy nginx config
COPY nginx.conf /etc/nginx/conf.d/default.conf

# Copy static files
COPY public/ /usr/share/nginx/html/

# Expose port
EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
```

## Testing

### Manual Testing Checklist

**Login:**
- [ ] Valid credentials → redirects to dashboard
- [ ] Invalid credentials → shows error
- [ ] Already logged in → redirects to dashboard

**Dashboard:**
- [ ] Shows correct devices for user role
- [ ] Displays latest readings
- [ ] Auto-refreshes every 30 seconds
- [ ] Click device → navigates to detail page
- [ ] Admin button visible only for admin
- [ ] Logout button works

**Device Detail:**
- [ ] Shows device info and latest reading
- [ ] Charts render correctly
- [ ] Time range selector updates charts
- [ ] Back button returns to dashboard

**Admin Panel:**
- [ ] Only accessible by admin users
- [ ] Create user works
- [ ] Edit user works
- [ ] Delete user works
- [ ] Assign device to user works
- [ ] Unassign device works

### Browser Compatibility

**Tested on:**
- Chrome/Edge (latest)
- Firefox (latest)
- Safari (latest)

**Required features:**
- ES6+ JavaScript
- Fetch API
- localStorage
- Canvas (for Chart.js)

## Performance Optimization

### Best Practices

1. **Minimize API calls:**
   - Cache dashboard data (30s)
   - Debounce user actions

2. **Optimize charts:**
   - Limit data points (1000 max)
   - Use aggregates for long time periods
   - Destroy old charts before creating new ones

3. **Lazy loading:**
   - Load Chart.js only on pages that need it
   - Load Vue.js from CDN with caching

4. **Image optimization:**
   - Use SVG for icons
   - Compress images

## Security Considerations

### XSS Protection

- Escape user input
- Vue.js escapes by default
- CSP headers in Nginx

### CSRF Protection

- Not needed for JWT-based auth (stateless)
- Tokens in Authorization header (not cookies)

### Token Storage

**MVP:** localStorage
- Accessible by JavaScript
- Vulnerable to XSS
- Simple and functional

**Production:** HttpOnly cookies (alternative)
- Not accessible by JavaScript
- Immune to XSS
- Requires backend cookie management

## Accessibility

**MVP:** Basic accessibility
- Semantic HTML
- Alt text for images
- Keyboard navigation
- Focus indicators

**Future improvements:**
- ARIA labels
- Screen reader testing
- High contrast mode
- Keyboard shortcuts

## Internationalization (i18n)

Not implemented in MVP. Future enhancement:
- Vue I18n plugin
- Language selector
- Translation files (en, de, fr, etc.)
- Date/time formatting

## Deployment

See [DEPLOYMENT.md](./DEPLOYMENT.md) for Docker and nginx configuration details.
