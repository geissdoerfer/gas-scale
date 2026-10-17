<template>
  <div class="login-container">
    <div class="login-card">
      <h1 class="login-title">DuoClean Energy</h1>
      <p class="login-subtitle">IoT Platform</p>

      <form @submit.prevent="handleLogin" class="login-form">
        <div class="form-group">
          <label for="username" class="form-label">Username</label>
          <input
            id="username"
            v-model="credentials.username"
            type="text"
            class="form-input"
            placeholder="Enter your username"
            required
            :disabled="loading"
          />
        </div>

        <div class="form-group">
          <label for="password" class="form-label">Password</label>
          <input
            id="password"
            v-model="credentials.password"
            type="password"
            class="form-input"
            placeholder="Enter your password"
            required
            :disabled="loading"
          />
        </div>

        <div v-if="error" class="error-message">
          {{ error }}
        </div>

        <button type="submit" class="btn btn-primary btn-block" :disabled="loading">
          <span v-if="loading" class="loading"></span>
          <span v-else>Sign In</span>
        </button>
      </form>

      <div class="login-footer">
        <p>Default admin: <strong>admin</strong> / <strong>admin123</strong></p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const authStore = useAuthStore()

const credentials = ref({
  username: '',
  password: '',
})

const loading = ref(false)
const error = ref('')

const handleLogin = async () => {
  loading.value = true
  error.value = ''

  try {
    await authStore.login(credentials.value)
    router.push('/')
  } catch (err) {
    error.value = err.response?.data?.detail || 'Login failed. Please check your credentials.'
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-container {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 100vh;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.login-card {
  background: white;
  border-radius: 12px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
  padding: 40px;
  width: 100%;
  max-width: 400px;
}

.login-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
  text-align: center;
}

.login-subtitle {
  font-size: 16px;
  color: #6b7280;
  margin-bottom: 32px;
  text-align: center;
}

.login-form {
  margin-bottom: 24px;
}

.btn-block {
  width: 100%;
  margin-top: 8px;
}

.login-footer {
  text-align: center;
  padding-top: 20px;
  border-top: 1px solid #e5e7eb;
}

.login-footer p {
  font-size: 14px;
  color: #6b7280;
}

.login-footer strong {
  color: #374151;
}
</style>
