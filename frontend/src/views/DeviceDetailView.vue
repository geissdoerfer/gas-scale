<template>
  <div>
    <NavigationBar />

    <div class="container device-detail-container">
      <div class="detail-header">
        <button @click="goBack" class="btn btn-secondary">← Back</button>
        <h2 class="detail-title">{{ device?.name || device?.device_id }}</h2>
      </div>

      <div v-if="error" class="error-banner">
        {{ error }}
      </div>

      <div v-if="loading && !device" class="loading-container">
        <div class="loading"></div>
        <p>Loading device details...</p>
      </div>

      <div v-else-if="device">
        <!-- Device Info Card -->
        <div class="card">
          <h3>Device Information</h3>
          <div class="info-grid">
            <div class="info-item">
              <span class="info-label">Device ID:</span>
              <span class="info-value">{{ device.device_id }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">Name:</span>
              <span class="info-value">{{ device.name || 'N/A' }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">Status:</span>
              <span :class="['device-status', statusClass]">{{ status }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">Description:</span>
              <span class="info-value">{{ device.description || 'N/A' }}</span>
            </div>
          </div>
        </div>

        <!-- Latest Reading Card -->
        <div v-if="device.latest_reading" class="card">
          <h3>Latest Reading</h3>
          <div class="readings-grid">
            <div class="reading-box">
              <div class="reading-label">Load</div>
              <div class="reading-value">{{ formatValue(device.latest_reading.load) }}</div>
              <div class="reading-unit">kW</div>
            </div>
            <div class="reading-box">
              <div class="reading-label">Battery Voltage</div>
              <div class="reading-value">{{ formatValue(device.latest_reading.battery_voltage) }}</div>
              <div class="reading-unit">V</div>
            </div>
            <div class="reading-box">
              <div class="reading-label">Temperature</div>
              <div class="reading-value">{{ formatValue(device.latest_reading.temperature) }}</div>
              <div class="reading-unit">°C</div>
            </div>
          </div>
          <div class="reading-time">
            Last updated: {{ formatDateTime(device.latest_reading.time) }}
          </div>
        </div>

        <!-- Time Range Selector -->
        <div class="card">
          <h3>Historical Data</h3>
          <div class="time-range-selector">
            <button
              v-for="range in timeRanges"
              :key="range.value"
              @click="selectTimeRange(range.value)"
              :class="['btn', selectedRange === range.value ? 'btn-primary' : 'btn-secondary']"
            >
              {{ range.label }}
            </button>
          </div>
        </div>

        <!-- Charts -->
        <div v-if="chartData" class="card">
          <h3>Load (kW)</h3>
          <LineChart :data="chartData.load" />
        </div>

        <div v-if="chartData" class="card">
          <h3>Battery Voltage (V)</h3>
          <LineChart :data="chartData.battery" />
        </div>

        <div v-if="chartData" class="card">
          <h3>Temperature (°C)</h3>
          <LineChart :data="chartData.temperature" />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { devicesAPI } from '@/services/api'
import { format, formatDistanceToNow, subHours, subDays } from 'date-fns'
import NavigationBar from '@/components/NavigationBar.vue'
import LineChart from '@/components/LineChart.vue'

const route = useRoute()
const router = useRouter()

const device = ref(null)
const readings = ref([])
const loading = ref(false)
const error = ref('')
const selectedRange = ref(24)

const timeRanges = [
  { label: '6 Hours', value: 6 },
  { label: '24 Hours', value: 24 },
  { label: '3 Days', value: 72 },
  { label: '7 Days', value: 168 },
]

const status = computed(() => {
  if (!device.value?.latest_reading) return 'Unknown'

  const lastUpdate = new Date(device.value.latest_reading.time)
  const now = new Date()
  const diffMinutes = (now - lastUpdate) / 1000 / 60

  if (diffMinutes < 10) return 'Online'
  if (diffMinutes < 60) return 'Recent'
  return 'Offline'
})

const statusClass = computed(() => {
  return `status-${status.value.toLowerCase()}`
})

const chartData = computed(() => {
  if (!readings.value || readings.value.length === 0) return null

  const labels = readings.value.map(r => new Date(r.time))

  return {
    load: {
      labels,
      datasets: [{
        label: 'Load (kW)',
        data: readings.value.map(r => r.load),
        borderColor: 'rgb(37, 99, 235)',
        backgroundColor: 'rgba(37, 99, 235, 0.1)',
        tension: 0.4,
      }],
    },
    battery: {
      labels,
      datasets: [{
        label: 'Battery Voltage (V)',
        data: readings.value.map(r => r.battery_voltage),
        borderColor: 'rgb(16, 185, 129)',
        backgroundColor: 'rgba(16, 185, 129, 0.1)',
        tension: 0.4,
      }],
    },
    temperature: {
      labels,
      datasets: [{
        label: 'Temperature (°C)',
        data: readings.value.map(r => r.temperature),
        borderColor: 'rgb(239, 68, 68)',
        backgroundColor: 'rgba(239, 68, 68, 0.1)',
        tension: 0.4,
      }],
    },
  }
})

const fetchDevice = async () => {
  loading.value = true
  error.value = ''

  try {
    const response = await devicesAPI.get(route.params.id)
    device.value = response.data
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to load device'
  } finally {
    loading.value = false
  }
}

const fetchReadings = async (hours) => {
  loading.value = true
  error.value = ''

  try {
    const endTime = new Date()
    const startTime = subHours(endTime, hours)

    const response = await devicesAPI.getReadings(route.params.id, {
      start_time: startTime.toISOString(),
      end_time: endTime.toISOString(),
    })

    readings.value = response.data.readings
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to load readings'
  } finally {
    loading.value = false
  }
}

const selectTimeRange = (hours) => {
  selectedRange.value = hours
  fetchReadings(hours)
}

const formatValue = (value) => {
  return value != null ? value.toFixed(2) : 'N/A'
}

const formatDateTime = (time) => {
  try {
    return format(new Date(time), 'PPpp')
  } catch {
    return 'Unknown'
  }
}

const goBack = () => {
  router.push('/')
}

onMounted(() => {
  fetchDevice()
  fetchReadings(selectedRange.value)
})
</script>

<style scoped>
.device-detail-container {
  padding: 32px 20px;
}

.detail-header {
  display: flex;
  align-items: center;
  gap: 16px;
  margin-bottom: 32px;
}

.detail-title {
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

.info-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
  margin-top: 16px;
}

.info-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.info-label {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
}

.info-value {
  font-size: 16px;
  color: #1f2937;
}

.device-status {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
  display: inline-block;
}

.status-online {
  background-color: #d1fae5;
  color: #065f46;
}

.status-recent {
  background-color: #fed7aa;
  color: #92400e;
}

.status-offline {
  background-color: #fee2e2;
  color: #991b1b;
}

.status-unknown {
  background-color: #e5e7eb;
  color: #374151;
}

.readings-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 20px;
  margin-top: 16px;
}

.reading-box {
  text-align: center;
  padding: 20px;
  background: #f9fafb;
  border-radius: 8px;
}

.reading-label {
  font-size: 14px;
  color: #6b7280;
  margin-bottom: 8px;
}

.reading-value {
  font-size: 32px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 4px;
}

.reading-unit {
  font-size: 14px;
  color: #9ca3af;
}

.reading-time {
  margin-top: 16px;
  text-align: right;
  font-size: 14px;
  color: #6b7280;
}

.time-range-selector {
  display: flex;
  gap: 12px;
  margin-top: 16px;
}

@media (max-width: 768px) {
  .info-grid {
    grid-template-columns: 1fr;
  }

  .readings-grid {
    grid-template-columns: 1fr;
  }

  .time-range-selector {
    flex-wrap: wrap;
  }
}
</style>
