# DuoClean Energy Frontend

Vue.js 3 frontend for the DuoClean Energy IoT Platform.

## Features

- **Authentication**: JWT-based login with token refresh
- **Dashboard**: View all assigned devices with latest readings
- **Device Details**: Historical data visualization with Chart.js
- **Admin Panel**: User and device management (admin only)
- **Responsive Design**: Works on desktop and mobile devices

## Tech Stack

- Vue 3 (Composition API)
- Vue Router 4
- Pinia (State Management)
- Axios (HTTP Client)
- Chart.js + vue-chartjs (Data Visualization)
- Vite (Build Tool)
- date-fns (Date Formatting)

## Project Structure

```
frontend/
├── public/              # Static assets
├── src/
│   ├── assets/         # CSS and static files
│   ├── components/     # Reusable Vue components
│   │   ├── DeviceCard.vue
│   │   ├── LineChart.vue
│   │   └── NavigationBar.vue
│   ├── router/         # Vue Router configuration
│   ├── services/       # API client and services
│   ├── stores/         # Pinia stores (auth, etc.)
│   ├── views/          # Page components
│   │   ├── admin/      # Admin views
│   │   ├── DashboardView.vue
│   │   ├── DeviceDetailView.vue
│   │   └── LoginView.vue
│   ├── App.vue         # Root component
│   └── main.js         # Application entry point
├── index.html          # HTML template
├── vite.config.js      # Vite configuration
├── package.json        # Dependencies
├── Dockerfile          # Docker configuration
└── README.md           # This file
```

## Development

### Prerequisites

- Node.js 20+
- npm or yarn

### Setup

1. Install dependencies:
```bash
npm install
```

2. Create `.env` file:
```bash
cp .env.example .env
```

3. Start development server:
```bash
npm run dev
```

The application will be available at http://localhost:3000

### Build for Production

```bash
npm run build
```

Built files will be in the `dist/` directory.

## Docker

Run with Docker Compose (from project root):

```bash
docker-compose up -d frontend
```

## API Integration

The frontend communicates with the backend API at `VITE_API_URL` (default: http://localhost:8000).

### API Endpoints Used

- `POST /auth/login` - User authentication
- `POST /auth/refresh` - Token refresh
- `GET /auth/me` - Get current user
- `GET /dashboard` - Dashboard data
- `GET /devices` - List devices
- `GET /devices/{id}` - Get device details
- `GET /devices/{id}/readings` - Get device readings
- `GET /users` - List users (admin)
- `POST /users` - Create user (admin)
- `POST /devices` - Create device (admin)
- `POST /devices/{id}/assign` - Assign device to user (admin)

## Authentication

The app uses JWT tokens stored in localStorage:
- `access_token` - Short-lived token (1 hour)
- `refresh_token` - Long-lived token (7 days)
- `user` - Current user information

Tokens are automatically refreshed when they expire.

## Routes

### Public Routes
- `/login` - Login page

### Protected Routes
- `/` - Dashboard (all users)
- `/device/:id` - Device detail page (all users)

### Admin Routes
- `/admin/users` - User management
- `/admin/devices` - Device management

## Components

### DeviceCard
Displays device information and latest readings with status indicator.

### LineChart
Wrapper for Chart.js line charts with time-series data.

### NavigationBar
Main navigation with user menu and admin dropdown.

## State Management

### Auth Store
- Manages user authentication state
- Handles login/logout
- Token refresh
- Role-based access control

## Default Credentials

- Username: `admin`
- Password: `admin123`

**Change these in production!**
