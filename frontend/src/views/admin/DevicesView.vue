<template>
  <div>
    <NavigationBar />

    <div class="container admin-container">
      <div class="admin-header">
        <h2 class="admin-title">Device Management</h2>
        <button @click="showCreateModal = true" class="btn btn-primary">+ Create Device</button>
      </div>

      <div v-if="error" class="error-banner">
        {{ error }}
      </div>

      <div v-if="loading && !devices.length" class="loading-container">
        <div class="loading"></div>
        <p>Loading devices...</p>
      </div>

      <div v-else-if="devices.length" class="card">
        <table class="data-table">
          <thead>
            <tr>
              <th>Device ID</th>
              <th>Name</th>
              <th>Description</th>
              <th>Status</th>
              <th>Actions</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="device in devices" :key="device.id">
              <td>{{ device.device_id }}</td>
              <td>{{ device.name || 'N/A' }}</td>
              <td>{{ device.description || 'N/A' }}</td>
              <td>
                <span :class="['device-status', getStatusClass(device)]">{{ getStatus(device) }}</span>
              </td>
              <td>
                <button @click="assignDevice(device)" class="btn-icon" title="Assign to user">👤</button>
                <button @click="editDevice(device)" class="btn-icon" title="Edit">✏️</button>
                <button @click="deleteDevice(device)" class="btn-icon" title="Delete">🗑️</button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Create/Edit Device Modal -->
    <div v-if="showCreateModal || editingDevice" class="modal" @click.self="closeModal">
      <div class="modal-content">
        <h3>{{ editingDevice ? 'Edit Device' : 'Create Device' }}</h3>

        <form @submit.prevent="submitDevice">
          <div class="form-group">
            <label class="form-label">Device ID</label>
            <input v-model="deviceForm.device_id" type="text" class="form-input" required :disabled="!!editingDevice" />
          </div>

          <div class="form-group">
            <label class="form-label">Name</label>
            <input v-model="deviceForm.name" type="text" class="form-input" />
          </div>

          <div class="form-group">
            <label class="form-label">Description</label>
            <textarea v-model="deviceForm.description" class="form-input" rows="3"></textarea>
          </div>

          <div class="modal-actions">
            <button type="button" @click="closeModal" class="btn btn-secondary">Cancel</button>
            <button type="submit" class="btn btn-primary" :disabled="submitting">
              {{ submitting ? 'Saving...' : 'Save' }}
            </button>
          </div>
        </form>
      </div>
    </div>

    <!-- Assign Device Modal -->
    <div v-if="assigningDevice" class="modal" @click.self="closeAssignModal">
      <div class="modal-content">
        <h3>Assign Device: {{ assigningDevice.device_id }}</h3>

        <div v-if="!users.length" class="loading-container">
          <div class="loading"></div>
          <p>Loading users...</p>
        </div>

        <div v-else class="user-list">
          <div v-for="user in users" :key="user.id" class="user-item">
            <div>
              <div class="user-name">{{ user.username }}</div>
              <div class="user-email">{{ user.email }}</div>
            </div>
            <button @click="assignToUser(user.id)" class="btn btn-primary btn-sm">Assign</button>
          </div>
        </div>

        <div class="modal-actions">
          <button @click="closeAssignModal" class="btn btn-secondary">Close</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { devicesAPI, usersAPI } from '@/services/api'
import NavigationBar from '@/components/NavigationBar.vue'

const devices = ref([])
const users = ref([])
const loading = ref(false)
const error = ref('')
const showCreateModal = ref(false)
const editingDevice = ref(null)
const assigningDevice = ref(null)
const submitting = ref(false)

const deviceForm = ref({
  device_id: '',
  name: '',
  description: '',
})

const fetchDevices = async () => {
  loading.value = true
  error.value = ''

  try {
    const response = await devicesAPI.list()
    devices.value = response.data
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to load devices'
  } finally {
    loading.value = false
  }
}

const fetchUsers = async () => {
  try {
    const response = await usersAPI.list()
    users.value = response.data
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to load users'
  }
}

const getStatus = (device) => {
  if (!device.latest_reading) return 'Unknown'

  const lastUpdate = new Date(device.latest_reading.time)
  const now = new Date()
  const diffMinutes = (now - lastUpdate) / 1000 / 60

  if (diffMinutes < 10) return 'Online'
  if (diffMinutes < 60) return 'Recent'
  return 'Offline'
}

const getStatusClass = (device) => {
  const status = getStatus(device)
  return `status-${status.toLowerCase()}`
}

const editDevice = (device) => {
  editingDevice.value = device
  deviceForm.value = {
    device_id: device.device_id,
    name: device.name,
    description: device.description,
  }
}

const deleteDevice = async (device) => {
  if (!confirm(`Delete device "${device.device_id}"?`)) return

  try {
    await devicesAPI.delete(device.device_id)
    await fetchDevices()
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to delete device'
  }
}

const assignDevice = async (device) => {
  assigningDevice.value = device
  await fetchUsers()
}

const assignToUser = async (userId) => {
  try {
    await devicesAPI.assign(assigningDevice.value.device_id, userId)
    alert('Device assigned successfully')
    closeAssignModal()
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to assign device'
  }
}

const submitDevice = async () => {
  submitting.value = true
  error.value = ''

  try {
    if (editingDevice.value) {
      await devicesAPI.update(editingDevice.value.device_id, {
        name: deviceForm.value.name,
        description: deviceForm.value.description,
      })
    } else {
      await devicesAPI.create(deviceForm.value)
    }

    await fetchDevices()
    closeModal()
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to save device'
  } finally {
    submitting.value = false
  }
}

const closeModal = () => {
  showCreateModal.value = false
  editingDevice.value = null
  deviceForm.value = {
    device_id: '',
    name: '',
    description: '',
  }
}

const closeAssignModal = () => {
  assigningDevice.value = null
  users.value = []
}

onMounted(() => {
  fetchDevices()
})
</script>

<style scoped>
.admin-container {
  padding: 32px 20px;
}

.admin-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 32px;
}

.admin-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.data-table {
  width: 100%;
  border-collapse: collapse;
}

.data-table th {
  text-align: left;
  padding: 12px;
  background-color: #f9fafb;
  color: #6b7280;
  font-weight: 600;
  font-size: 14px;
  border-bottom: 2px solid #e5e7eb;
}

.data-table td {
  padding: 12px;
  border-bottom: 1px solid #e5e7eb;
  color: #374151;
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

.btn-icon {
  background: none;
  border: none;
  cursor: pointer;
  font-size: 18px;
  padding: 4px 8px;
  opacity: 0.7;
  transition: opacity 0.2s;
}

.btn-icon:hover {
  opacity: 1;
}

.modal {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}

.modal-content {
  background: white;
  border-radius: 8px;
  padding: 32px;
  width: 100%;
  max-width: 500px;
  max-height: 90vh;
  overflow-y: auto;
}

.modal-content h3 {
  margin: 0 0 24px 0;
  font-size: 24px;
  color: #1f2937;
}

.modal-actions {
  display: flex;
  gap: 12px;
  justify-content: flex-end;
  margin-top: 24px;
}

.user-list {
  max-height: 400px;
  overflow-y: auto;
}

.user-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  border-bottom: 1px solid #e5e7eb;
}

.user-name {
  font-weight: 600;
  color: #1f2937;
  margin-bottom: 4px;
}

.user-email {
  font-size: 14px;
  color: #6b7280;
}

.btn-sm {
  padding: 6px 12px;
  font-size: 13px;
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

textarea.form-input {
  resize: vertical;
  font-family: inherit;
}
</style>
