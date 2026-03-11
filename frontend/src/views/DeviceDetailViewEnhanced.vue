<template>
  <div>
    <NavigationBar />

    <div class="container device-detail-container">
      <div class="detail-header">
        <button @click="goBack" class="btn btn-secondary">← Back</button>
        <h2 class="detail-title">{{ device?.name || device?.device_id }}</h2>
        <div class="live-indicator" :class="{ 'live-active': isLiveUpdating }">
          <span class="live-dot"></span>
          {{ isLiveUpdating ? 'LIVE' : 'PAUSED' }}
        </div>
      </div>

      <div v-if="error" class="error-banner">
        {{ error }}
      </div>

      <div v-if="loading && !device" class="loading-container">
        <div class="loading"></div>
        <p>Loading device details...</p>
      </div>

      <div v-else-if="device">
        <!-- Device Info Card with Calibration -->
        <div class="card">
          <div class="card-header">
            <h3>Device Information</h3>
            <div class="refresh-info" v-if="lastUpdate">
              Updated {{ formatRelativeTime(lastUpdate) }}
            </div>
          </div>
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
            <div class="info-item">
              <span class="info-label">Calibration Offset:</span>
              <span class="info-value">{{ device.offset?.toFixed(2) || '0.00' }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">Calibration Gain:</span>
              <span class="info-value">{{ device.gain?.toFixed(4) || '1.0000' }}</span>
            </div>
          </div>
        </div>

        <!-- Latest Reading Card with Animation -->
        <div v-if="device.latest_reading" class="card">
          <div class="card-header">
            <h3>Latest Reading</h3>
            <button
              @click="toggleLiveUpdates"
              :class="['btn', isLiveUpdating ? 'btn-danger' : 'btn-primary']"
            >
              {{ isLiveUpdating ? 'Stop Live' : 'Start Live' }}
            </button>
          </div>
          <div class="readings-grid">
            <div class="reading-box" :class="{ 'reading-updated': recentlyUpdated.weight }">
              <div class="reading-label">Weight</div>
              <div class="reading-value">{{ formatValue(device.latest_reading.weight) }}</div>
              <div class="reading-unit">g</div>
              <div class="reading-raw" v-if="device.latest_reading.raw_value !== undefined">
                Raw: {{ formatValue(device.latest_reading.raw_value) }}
              </div>
            </div>
            <div class="reading-box" :class="{ 'reading-updated': recentlyUpdated.battery }">
              <div class="reading-label">Battery Voltage</div>
              <div class="reading-value">{{ formatValue(device.latest_reading.battery_voltage) }}</div>
              <div class="reading-unit">V</div>
              <div :class="['battery-indicator', batteryLevelClass]"></div>
            </div>
            <div class="reading-box" :class="{ 'reading-updated': recentlyUpdated.temperature }">
              <div class="reading-label">Temperature</div>
              <div class="reading-value">{{ formatValue(device.latest_reading.temperature) }}</div>
              <div class="reading-unit">°C</div>
            </div>
          </div>
          <div class="reading-time">
            Last updated: {{ formatDateTime(device.latest_reading.time) }}
            <span v-if="device.latest_reading.time" class="time-ago">
              ({{ formatRelativeTime(device.latest_reading.time) }})
            </span>
          </div>
        </div>

        <!-- Chart View Selector -->
        <div class="card">
          <div class="card-header">
            <h3>Historical Data</h3>
            <div class="view-controls">
              <div class="view-mode">
                <button
                  @click="chartView = 'separate'"
                  :class="['btn-view', { active: chartView === 'separate' }]"
                >
                  Separate
                </button>
                <button
                  @click="chartView = 'combined'"
                  :class="['btn-view', { active: chartView === 'combined' }]"
                >
                  Combined
                </button>
              </div>
            </div>
          </div>
          <div class="time-range-selector">
            <button
              v-for="range in timeRanges"
              :key="range.value"
              @click="selectTimeRange(range.value)"
              :class="['btn-time', selectedRange === range.value ? 'active' : '']"
            >
              {{ range.label }}
            </button>
            <div class="auto-refresh">
              <label>
                <input type="checkbox" v-model="autoRefreshCharts" />
                Auto-refresh charts
              </label>
            </div>
          </div>
        </div>

        <!-- Separate Charts View -->
        <template v-if="chartView === 'separate' && chartData">
          <div class="card">
            <InteractiveChart
              :data="chartData.weight"
              title="Weight"
              yAxisLabel="Weight (g)"
              unit="g"
            />
          </div>

          <div class="card">
            <BarChart
              :data="chartData.consumption"
              title="Consumption"
              yAxisLabel="Consumption (g/min)"
              unit="g/min"
            />
          </div>
        </template>

        <!-- Combined Chart View -->
        <div v-if="chartView === 'combined' && combinedChartData" class="card">
          <InteractiveChart
            :data="combinedChartData"
            title="Weight & Consumption"
            :show-legend="true"
          />
        </div>

        <!-- Stats Summary -->
        <div v-if="readings.length > 0" class="card">
          <h3>Statistics ({{ selectedRange }} hours)</h3>
          <div class="stats-grid">
            <div class="stat-box">
              <div class="stat-label">Avg Weight</div>
              <div class="stat-value">{{ calculateAvg('weight') }} g</div>
            </div>
            <div class="stat-box">
              <div class="stat-label">Min Weight</div>
              <div class="stat-value">{{ calculateMin('weight') }} g</div>
            </div>
            <div class="stat-box">
              <div class="stat-label">Max Weight</div>
              <div class="stat-value">{{ calculateMax('weight') }} g</div>
            </div>
            <div class="stat-box">
              <div class="stat-label">Data Points</div>
              <div class="stat-value">{{ readings.length }}</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { devicesAPI } from '@/services/api'
import { format, formatDistanceToNow, subHours } from 'date-fns'
import NavigationBar from '@/components/NavigationBar.vue'
import InteractiveChart from '@/components/InteractiveChart.vue'
import BarChart from '@/components/BarChart.vue'

const route = useRoute()
const router = useRouter()

const device = ref(null)
const readings = ref([])
const loading = ref(false)
const error = ref('')
const selectedRange = ref(24)
const chartView = ref('separate') // 'separate' or 'combined'
const isLiveUpdating = ref(false)
const autoRefreshCharts = ref(true)
const lastUpdate = ref(null)
const recentlyUpdated = ref({ weight: false, battery: false, temperature: false })

let liveUpdateInterval = null
let chartRefreshInterval = null

const timeRanges = [
  { label: '1 Hour', value: 1 },
  { label: '6 Hours', value: 6 },
  { label: '24 Hours', value: 24 },
  { label: '3 Days', value: 72 },
  { label: '7 Days', value: 168 },
]

const status = computed(() => {
  if (!device.value?.latest_reading) return 'Unknown'

  const lastUpdateTime = new Date(device.value.latest_reading.time)
  const now = new Date()
  const diffMinutes = (now - lastUpdateTime) / 1000 / 60

  if (diffMinutes < 10) return 'Online'
  if (diffMinutes < 60) return 'Recent'
  return 'Offline'
})

const statusClass = computed(() => {
  return `status-${status.value.toLowerCase()}`
})

const batteryLevelClass = computed(() => {
  const voltage = device.value?.latest_reading?.battery_voltage
  if (!voltage) return 'battery-unknown'
  if (voltage > 12.5) return 'battery-good'
  if (voltage > 11.5) return 'battery-ok'
  return 'battery-low'
})

const chartData = computed(() => {
  if (!readings.value || readings.value.length === 0) return null

  const labels = readings.value.map(r => new Date(r.time))

  // Calculate consumption: sum of weight deltas per 1-minute window
  // Group readings by minute, then sum all negative deltas (consumption)
  const minuteMap = new Map()

  for (let i = 0; i < readings.value.length; i++) {
    if (i === 0) continue // Skip first reading as it has no previous point

    const currentWeight = readings.value[i].weight
    const previousWeight = readings.value[i - 1].weight
    const currentTime = new Date(readings.value[i].time)

    // Round down to the minute - use the reading's timestamp
    const minuteKey = new Date(Date.UTC(
      currentTime.getUTCFullYear(),
      currentTime.getUTCMonth(),
      currentTime.getUTCDate(),
      currentTime.getUTCHours(),
      currentTime.getUTCMinutes(),
      0, 0
    ))

    // Calculate weight delta
    const weightDelta = currentWeight - previousWeight

    // Sum up deltas for this minute
    if (!minuteMap.has(minuteKey.getTime())) {
      minuteMap.set(minuteKey.getTime(), { time: minuteKey, totalDelta: 0 })
    }
    minuteMap.get(minuteKey.getTime()).totalDelta += weightDelta
  }

  // Convert map to arrays for chart
  const consumptionLabels = []
  const consumptionData = []

  // Sort by time
  const sortedMinutes = Array.from(minuteMap.values()).sort((a, b) => a.time - b.time)

  for (const minute of sortedMinutes) {
    consumptionLabels.push(minute.time)
    // Positive bar for negative delta (consumption), zero for positive delta
    const consumption = minute.totalDelta > 25 ? minute.totalDelta : 0
    consumptionData.push(consumption)
  }

  return {
    weight: {
      labels,
      datasets: [{
        label: 'Weight (g)',
        data: readings.value.map(r => r.weight),
        borderColor: 'rgb(37, 99, 235)',
        backgroundColor: 'rgba(37, 99, 235, 0.1)',
        tension: 0.4,
      }],
    },
    consumption: {
      labels: consumptionLabels,
      datasets: [{
        label: 'Consumption (g/min)',
        data: consumptionData,
        borderColor: 'rgb(37, 99, 235)',
        backgroundColor: 'rgba(37, 99, 235, 0.6)',
      }],
    },
  }
})

const combinedChartData = computed(() => {
  if (!readings.value || readings.value.length === 0) return null

  const labels = readings.value.map(r => new Date(r.time))

  // Calculate consumption data: sum of weight deltas per 1-minute window
  const minuteMap = new Map()

  for (let i = 0; i < readings.value.length; i++) {
    if (i === 0) continue

    const currentWeight = readings.value[i].weight
    const previousWeight = readings.value[i - 1].weight
    const currentTime = new Date(readings.value[i].time)

    // Round down to the minute - use the reading's timestamp
    const minuteKey = new Date(Date.UTC(
      currentTime.getUTCFullYear(),
      currentTime.getUTCMonth(),
      currentTime.getUTCDate(),
      currentTime.getUTCHours(),
      currentTime.getUTCMinutes(),
      0, 0
    ))

    // Calculate weight delta
    const weightDelta = currentWeight - previousWeight

    // Sum up deltas for this minute
    if (!minuteMap.has(minuteKey.getTime())) {
      minuteMap.set(minuteKey.getTime(), { time: minuteKey, totalDelta: 0 })
    }
    minuteMap.get(minuteKey.getTime()).totalDelta += weightDelta
  }

  // Create consumption array matching weight data timestamps
  // For combined view, we'll interpolate consumption data to match weight timestamps
  const consumptionByMinute = new Map()
  for (const [timestamp, data] of minuteMap.entries()) {
    const consumption = data.totalDelta < 0 ? -data.totalDelta : 0
    consumptionByMinute.set(timestamp, consumption)
  }

  // Map consumption to weight timestamps
  const consumptionData = readings.value.map(r => {
    const time = new Date(r.time)
    const minuteKey = new Date(Date.UTC(
      time.getUTCFullYear(),
      time.getUTCMonth(),
      time.getUTCDate(),
      time.getUTCHours(),
      time.getUTCMinutes(),
      0, 0
    )).getTime()
    return consumptionByMinute.get(minuteKey) || 0
  })

  // Normalize data to 0-100 scale for combined view
  const weightValues = readings.value.map(r => r.weight).filter(v => v != null)
  const consumptionValues = consumptionData.filter(v => v != null && v > 0)

  const weightMin = Math.min(...weightValues)
  const weightMax = Math.max(...weightValues)
  const consumptionMin = 0
  const consumptionMax = consumptionValues.length > 0 ? Math.max(...consumptionValues) : 1

  const normalize = (value, min, max) => {
    if (max === min) return 50
    return ((value - min) / (max - min)) * 100
  }

  return {
    labels,
    datasets: [
      {
        label: 'Weight',
        data: readings.value.map(r => r.weight ? normalize(r.weight, weightMin, weightMax) : null),
        borderColor: 'rgb(37, 99, 235)',
        backgroundColor: 'rgba(37, 99, 235, 0.1)',
        tension: 0.4,
        yAxisID: 'y',
      },
      {
        label: 'Consumption',
        data: consumptionData.map(c => c > 0 ? normalize(c, consumptionMin, consumptionMax) : 0),
        borderColor: 'rgb(16, 185, 129)',
        backgroundColor: 'rgba(16, 185, 129, 0.1)',
        tension: 0.4,
        yAxisID: 'y',
      },
    ],
  }
})

const fetchDevice = async () => {
  loading.value = true
  error.value = ''

  try {
    const response = await devicesAPI.get(route.params.id)

    // Detect changes for animation
    if (device.value?.latest_reading) {
      const old = device.value.latest_reading
      const newReading = response.data.latest_reading

      if (old.weight !== newReading?.weight) flashUpdate('weight')
      if (old.battery_voltage !== newReading?.battery_voltage) flashUpdate('battery')
      if (old.temperature !== newReading?.temperature) flashUpdate('temperature')
    }

    device.value = response.data
    lastUpdate.value = new Date()
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

const flashUpdate = (field) => {
  recentlyUpdated.value[field] = true
  setTimeout(() => {
    recentlyUpdated.value[field] = false
  }, 1000)
}

const toggleLiveUpdates = () => {
  isLiveUpdating.value = !isLiveUpdating.value

  if (isLiveUpdating.value) {
    // Fetch immediately
    fetchDevice()
    // Then every 3 seconds
    liveUpdateInterval = setInterval(() => {
      fetchDevice()
    }, 3000)
  } else {
    if (liveUpdateInterval) {
      clearInterval(liveUpdateInterval)
      liveUpdateInterval = null
    }
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

const formatRelativeTime = (time) => {
  try {
    return formatDistanceToNow(new Date(time), { addSuffix: true })
  } catch {
    return 'Unknown'
  }
}

const calculateAvg = (field) => {
  const values = readings.value.map(r => r[field]).filter(v => v != null)
  if (values.length === 0) return 'N/A'
  const avg = values.reduce((a, b) => a + b, 0) / values.length
  return avg.toFixed(2)
}

const calculateMin = (field) => {
  const values = readings.value.map(r => r[field]).filter(v => v != null)
  if (values.length === 0) return 'N/A'
  return Math.min(...values).toFixed(2)
}

const calculateMax = (field) => {
  const values = readings.value.map(r => r[field]).filter(v => v != null)
  if (values.length === 0) return 'N/A'
  return Math.max(...values).toFixed(2)
}

const goBack = () => {
  router.push('/')
}

onMounted(() => {
  fetchDevice()
  fetchReadings(selectedRange.value)

  // Auto-refresh charts every 30 seconds if enabled
  chartRefreshInterval = setInterval(() => {
    if (autoRefreshCharts.value) {
      fetchReadings(selectedRange.value)
    }
  }, 30000)
})

onBeforeUnmount(() => {
  if (liveUpdateInterval) {
    clearInterval(liveUpdateInterval)
  }
  if (chartRefreshInterval) {
    clearInterval(chartRefreshInterval)
  }
})
</script>

<style scoped>
.device-detail-container {
  padding: 32px 20px;
  max-width: 1400px;
  margin: 0 auto;
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
  flex: 1;
}

.live-indicator {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  border-radius: 6px;
  font-size: 12px;
  font-weight: 700;
  background: #f3f4f6;
  color: #6b7280;
  transition: all 0.3s;
}

.live-indicator.live-active {
  background: #dcfce7;
  color: #15803d;
}

.live-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: currentColor;
}

.live-indicator.live-active .live-dot {
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.3;
  }
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.card-header h3 {
  margin: 0;
}

.refresh-info {
  font-size: 13px;
  color: #6b7280;
}

.info-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 20px;
  margin-top: 16px;
}

.info-item {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.info-label {
  font-size: 13px;
  color: #6b7280;
  font-weight: 500;
}

.info-value {
  font-size: 15px;
  color: #1f2937;
  font-weight: 500;
}

.device-status {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
  display: inline-block;
  width: fit-content;
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
  padding: 24px;
  background: #f9fafb;
  border-radius: 12px;
  transition: all 0.3s;
  position: relative;
}

.reading-box.reading-updated {
  background: #dcfce7;
  transform: scale(1.02);
}

.reading-label {
  font-size: 13px;
  color: #6b7280;
  margin-bottom: 12px;
  font-weight: 500;
}

.reading-value {
  font-size: 36px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 4px;
}

.reading-unit {
  font-size: 14px;
  color: #9ca3af;
  font-weight: 500;
}

.reading-raw {
  font-size: 11px;
  color: #6b7280;
  margin-top: 8px;
  font-style: italic;
}

.battery-indicator {
  width: 40px;
  height: 8px;
  border-radius: 4px;
  margin: 8px auto 0;
}

.battery-good {
  background: linear-gradient(90deg, #10b981, #34d399);
}

.battery-ok {
  background: linear-gradient(90deg, #f59e0b, #fbbf24);
}

.battery-low {
  background: linear-gradient(90deg, #ef4444, #f87171);
}

.battery-unknown {
  background: #d1d5db;
}

.reading-time {
  margin-top: 20px;
  text-align: right;
  font-size: 13px;
  color: #6b7280;
}

.time-ago {
  color: #9ca3af;
  font-style: italic;
}

.view-controls {
  display: flex;
  gap: 16px;
  align-items: center;
}

.view-mode {
  display: flex;
  gap: 0;
  background: #f3f4f6;
  border-radius: 8px;
  padding: 2px;
}

.btn-view {
  padding: 6px 16px;
  border: none;
  background: transparent;
  color: #6b7280;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  border-radius: 6px;
  transition: all 0.2s;
}

.btn-view.active {
  background: white;
  color: #2563eb;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
}

.time-range-selector {
  display: flex;
  gap: 12px;
  margin-top: 16px;
  flex-wrap: wrap;
  align-items: center;
}

.btn-time {
  padding: 8px 16px;
  border: 1px solid #e5e7eb;
  background: white;
  color: #6b7280;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  border-radius: 8px;
  transition: all 0.2s;
}

.btn-time.active {
  background: #2563eb;
  color: white;
  border-color: #2563eb;
}

.btn-time:hover:not(.active) {
  border-color: #d1d5db;
  background: #f9fafb;
}

.auto-refresh {
  margin-left: auto;
  font-size: 13px;
  color: #6b7280;
}

.auto-refresh label {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
}

.auto-refresh input[type="checkbox"] {
  cursor: pointer;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 20px;
  margin-top: 16px;
}

.stat-box {
  text-align: center;
  padding: 16px;
  background: #f9fafb;
  border-radius: 8px;
}

.stat-label {
  font-size: 12px;
  color: #6b7280;
  margin-bottom: 8px;
  font-weight: 500;
}

.stat-value {
  font-size: 20px;
  font-weight: 700;
  color: #1f2937;
}

.btn-danger {
  background: #ef4444;
  color: white;
  border: none;
}

.btn-danger:hover {
  background: #dc2626;
}

@media (max-width: 1024px) {
  .info-grid {
    grid-template-columns: repeat(2, 1fr);
  }

  .stats-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}

@media (max-width: 768px) {
  .info-grid,
  .readings-grid,
  .stats-grid {
    grid-template-columns: 1fr;
  }

  .detail-header {
    flex-wrap: wrap;
  }

  .time-range-selector {
    flex-direction: column;
    align-items: stretch;
  }

  .auto-refresh {
    margin-left: 0;
  }
}
</style>
