<template>
  <div>
    <NavigationBar />

    <div class="container dashboard-container">
      <div class="dashboard-header">
        <h2 class="dashboard-title">My Devices</h2>
        <button @click="refreshDashboard" class="btn btn-secondary" :disabled="loading">
          <span v-if="loading" class="loading"></span>
          <span v-else>Refresh</span>
        </button>
      </div>

      <div v-if="error" class="error-banner">
        {{ error }}
      </div>

      <div v-if="loading && !dashboard" class="loading-container">
        <div class="loading"></div>
        <p>Loading devices...</p>
      </div>

      <div v-else-if="dashboard?.devices?.length > 0" class="grid grid-cols-3">
        <DeviceCard
          v-for="device in dashboard.devices"
          :key="device.id"
          :device="device"
          @click="goToDevice(device.device_id)"
        />
      </div>

      <div v-else class="empty-state">
        <p>No devices assigned to you yet.</p>
        <p v-if="!authStore.isAdmin" class="empty-subtitle">
          Contact your administrator to assign devices.
        </p>
        <router-link v-else to="/admin/devices" class="btn btn-primary">
          Manage Devices
        </router-link>
      </div>

      <div v-if="dashboard" class="dashboard-footer">
        <p>Total devices: {{ dashboard.total }}</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { dashboardAPI } from '@/services/api'
import NavigationBar from '@/components/NavigationBar.vue'
import DeviceCard from '@/components/DeviceCard.vue'

const router = useRouter()
const authStore = useAuthStore()

const dashboard = ref(null)
const loading = ref(false)
const error = ref('')

const fetchDashboard = async () => {
  loading.value = true
  error.value = ''

  try {
    const response = await dashboardAPI.get()
    dashboard.value = response.data
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to load dashboard'
  } finally {
    loading.value = false
  }
}

const refreshDashboard = () => {
  fetchDashboard()
}

const goToDevice = (deviceId) => {
  router.push(`/device/${deviceId}`)
}

onMounted(() => {
  fetchDashboard()
})
</script>

<style scoped>
.dashboard-container {
  padding: 32px 20px;
}

.dashboard-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 32px;
}

.dashboard-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.error-banner {
  background-color: #fee2e2;
  color: #991b1b;
  padding: 16px;
  border-radius: 8px;
  margin-bottom: 24px;
}

.loading-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px;
  color: #6b7280;
}

.empty-state {
  text-align: center;
  padding: 60px 20px;
  background: white;
  border-radius: 8px;
}

.empty-state p {
  font-size: 16px;
  color: #6b7280;
  margin-bottom: 8px;
}

.empty-subtitle {
  font-size: 14px;
  color: #9ca3af;
  margin-bottom: 24px;
}

.dashboard-footer {
  margin-top: 32px;
  padding-top: 16px;
  border-top: 1px solid #e5e7eb;
  text-align: center;
  color: #6b7280;
  font-size: 14px;
}
</style>
