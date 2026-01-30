<template>
  <div class="device-card" @click="$emit('click')">
    <div class="device-header">
      <h3 class="device-name">{{ device.name || device.device_id }}</h3>
      <span :class="['device-status', statusClass]">{{ status }}</span>
    </div>

    <p class="device-id">ID: {{ device.device_id }}</p>

    <div v-if="device.latest_reading" class="device-readings">
      <div class="reading-item">
        <span class="reading-label">Weight:</span>
        <span class="reading-value">{{ formatValue(device.latest_reading.weight) }} g</span>
      </div>
      <div class="reading-item">
        <span class="reading-label">Battery:</span>
        <span class="reading-value">{{ formatValue(device.latest_reading.battery_voltage) }} V</span>
      </div>
      <div class="reading-item">
        <span class="reading-label">Temp:</span>
        <span class="reading-value">{{ formatValue(device.latest_reading.temperature) }} °C</span>
      </div>
      <div class="reading-time">
        {{ formatTime(device.latest_reading.time) }}
      </div>
    </div>

    <div v-else class="no-data">
      No data available
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { formatDistanceToNow } from 'date-fns'

const props = defineProps({
  device: {
    type: Object,
    required: true,
  },
})

defineEmits(['click'])

const status = computed(() => {
  if (!props.device.latest_reading) return 'Unknown'

  const lastUpdate = new Date(props.device.latest_reading.time)
  const now = new Date()
  const diffMinutes = (now - lastUpdate) / 1000 / 60

  if (diffMinutes < 10) return 'Online'
  if (diffMinutes < 60) return 'Recent'
  return 'Offline'
})

const statusClass = computed(() => {
  return `status-${status.value.toLowerCase()}`
})

const formatValue = (value) => {
  return value != null ? value.toFixed(2) : 'N/A'
}

const formatTime = (time) => {
  try {
    return formatDistanceToNow(new Date(time), { addSuffix: true })
  } catch {
    return 'Unknown'
  }
}
</script>

<style scoped>
.device-card {
  background: white;
  border-radius: 8px;
  padding: 20px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  cursor: pointer;
  transition: all 0.2s ease;
}

.device-card:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  transform: translateY(-2px);
}

.device-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.device-name {
  font-size: 18px;
  font-weight: 600;
  color: #1f2937;
  margin: 0;
}

.device-status {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
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

.device-id {
  font-size: 14px;
  color: #6b7280;
  margin-bottom: 16px;
}

.device-readings {
  border-top: 1px solid #e5e7eb;
  padding-top: 16px;
}

.reading-item {
  display: flex;
  justify-content: space-between;
  margin-bottom: 8px;
}

.reading-label {
  font-size: 14px;
  color: #6b7280;
}

.reading-value {
  font-size: 14px;
  font-weight: 600;
  color: #1f2937;
}

.reading-time {
  margin-top: 12px;
  font-size: 12px;
  color: #9ca3af;
  text-align: right;
}

.no-data {
  padding: 20px;
  text-align: center;
  color: #9ca3af;
  font-size: 14px;
}
</style>
