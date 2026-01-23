import { defineStore } from 'pinia'
import { authAPI } from '@/services/api'

export const useAuthStore = defineStore('auth', {
  state: () => ({
    user: JSON.parse(localStorage.getItem('user') || 'null'),
    accessToken: localStorage.getItem('access_token'),
    refreshToken: localStorage.getItem('refresh_token'),
    isAuthenticated: !!localStorage.getItem('access_token'),
  }),

  getters: {
    isAdmin: (state) => state.user?.role === 'admin',
    currentUser: (state) => state.user,
  },

  actions: {
    async login(credentials) {
      try {
        const response = await authAPI.login(credentials)
        const { access_token, refresh_token, user } = response.data

        this.accessToken = access_token
        this.refreshToken = refresh_token
        this.user = user
        this.isAuthenticated = true

        localStorage.setItem('access_token', access_token)
        localStorage.setItem('refresh_token', refresh_token)
        localStorage.setItem('user', JSON.stringify(user))

        return response.data
      } catch (error) {
        this.logout()
        throw error
      }
    },

    async refreshAuth() {
      try {
        if (!this.refreshToken) {
          throw new Error('No refresh token available')
        }

        const response = await authAPI.refresh(this.refreshToken)
        const { access_token } = response.data

        this.accessToken = access_token
        localStorage.setItem('access_token', access_token)

        return response.data
      } catch (error) {
        this.logout()
        throw error
      }
    },

    async fetchUser() {
      try {
        const response = await authAPI.getMe()
        this.user = response.data
        localStorage.setItem('user', JSON.stringify(response.data))
        return response.data
      } catch (error) {
        this.logout()
        throw error
      }
    },

    logout() {
      this.user = null
      this.accessToken = null
      this.refreshToken = null
      this.isAuthenticated = false

      localStorage.removeItem('access_token')
      localStorage.removeItem('refresh_token')
      localStorage.removeItem('user')
    },
  },
})
