import axios from 'axios'

const API_URL = import.meta.env.VITE_API_URL || 'http://localhost:8000'

const api = axios.create({
  baseURL: API_URL,
  headers: {
    'Content-Type': 'application/json',
  },
})

// Request interceptor to add auth token
api.interceptors.request.use(
  (config) => {
    const token = localStorage.getItem('access_token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// Response interceptor to handle token refresh
api.interceptors.response.use(
  (response) => response,
  async (error) => {
    const originalRequest = error.config

    if (error.response?.status === 401 && !originalRequest._retry) {
      originalRequest._retry = true

      try {
        const refreshToken = localStorage.getItem('refresh_token')
        if (refreshToken) {
          const response = await axios.post(`${API_URL}/auth/refresh`, {
            refresh_token: refreshToken,
          })

          const { access_token } = response.data
          localStorage.setItem('access_token', access_token)

          originalRequest.headers.Authorization = `Bearer ${access_token}`
          return api(originalRequest)
        }
      } catch (refreshError) {
        localStorage.removeItem('access_token')
        localStorage.removeItem('refresh_token')
        localStorage.removeItem('user')
        window.location.href = '/login'
        return Promise.reject(refreshError)
      }
    }

    return Promise.reject(error)
  }
)

export default api

// Auth API
export const authAPI = {
  login: (credentials) => api.post('/auth/login', credentials),
  refresh: (refreshToken) => api.post('/auth/refresh', { refresh_token: refreshToken }),
  getMe: () => api.get('/auth/me'),
}

// Users API
export const usersAPI = {
  list: () => api.get('/users'),
  create: (userData) => api.post('/users', userData),
  update: (userId, userData) => api.put(`/users/${userId}`, userData),
  delete: (userId) => api.delete(`/users/${userId}`),
}

// Devices API
export const devicesAPI = {
  list: () => api.get('/devices'),
  get: (deviceId) => api.get(`/devices/${deviceId}`),
  create: (deviceData) => api.post('/devices', deviceData),
  update: (deviceId, deviceData) => api.put(`/devices/${deviceId}`, deviceData),
  delete: (deviceId) => api.delete(`/devices/${deviceId}`),
  assign: (deviceId, userId) => api.post(`/devices/${deviceId}/assign`, { user_id: userId }),
  unassign: (deviceId, userId) => api.delete(`/devices/${deviceId}/unassign/${userId}`),
  getLatest: (deviceId) => api.get(`/devices/${deviceId}/latest`),
  getReadings: (deviceId, params) => api.get(`/devices/${deviceId}/readings`, { params }),
}

// Dashboard API
export const dashboardAPI = {
  get: () => api.get('/dashboard'),
}
