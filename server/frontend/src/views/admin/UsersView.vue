<template>
  <div>
    <NavigationBar />

    <div class="container admin-container">
      <div class="admin-header">
        <h2 class="admin-title">User Management</h2>
        <button @click="showCreateModal = true" class="btn btn-primary">+ Create User</button>
      </div>

      <div v-if="error" class="error-banner">
        {{ error }}
      </div>

      <div v-if="loading && !users.length" class="loading-container">
        <div class="loading"></div>
        <p>Loading users...</p>
      </div>

      <div v-else-if="users.length" class="card">
        <table class="data-table">
          <thead>
            <tr>
              <th>Username</th>
              <th>Email</th>
              <th>Role</th>
              <th>Created</th>
              <th>Actions</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="user in users" :key="user.id">
              <td>{{ user.username }}</td>
              <td>{{ user.email }}</td>
              <td>
                <span :class="['role-badge', `role-${user.role}`]">{{ user.role }}</span>
              </td>
              <td>{{ formatDate(user.created_at) }}</td>
              <td>
                <button @click="editUser(user)" class="btn-icon">✏️</button>
                <button @click="deleteUser(user)" class="btn-icon" :disabled="user.id === authStore.currentUser?.id">🗑️</button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Create/Edit Modal -->
    <div v-if="showCreateModal || editingUser" class="modal" @click.self="closeModal">
      <div class="modal-content">
        <h3>{{ editingUser ? 'Edit User' : 'Create User' }}</h3>

        <form @submit.prevent="submitUser">
          <div class="form-group">
            <label class="form-label">Username</label>
            <input v-model="userForm.username" type="text" class="form-input" required />
          </div>

          <div class="form-group">
            <label class="form-label">Email</label>
            <input v-model="userForm.email" type="email" class="form-input" required />
          </div>

          <div v-if="!editingUser" class="form-group">
            <label class="form-label">Password</label>
            <input v-model="userForm.password" type="password" class="form-input" required minlength="8" />
          </div>

          <div class="form-group">
            <label class="form-label">Role</label>
            <select v-model="userForm.role" class="form-input" required>
              <option value="user">User</option>
              <option value="admin">Admin</option>
            </select>
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
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { usersAPI } from '@/services/api'
import { format } from 'date-fns'
import NavigationBar from '@/components/NavigationBar.vue'

const authStore = useAuthStore()

const users = ref([])
const loading = ref(false)
const error = ref('')
const showCreateModal = ref(false)
const editingUser = ref(null)
const submitting = ref(false)

const userForm = ref({
  username: '',
  email: '',
  password: '',
  role: 'user',
})

const fetchUsers = async () => {
  loading.value = true
  error.value = ''

  try {
    const response = await usersAPI.list()
    users.value = response.data
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to load users'
  } finally {
    loading.value = false
  }
}

const editUser = (user) => {
  editingUser.value = user
  userForm.value = {
    username: user.username,
    email: user.email,
    role: user.role,
  }
}

const deleteUser = async (user) => {
  if (!confirm(`Delete user "${user.username}"?`)) return

  try {
    await usersAPI.delete(user.id)
    await fetchUsers()
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to delete user'
  }
}

const submitUser = async () => {
  submitting.value = true
  error.value = ''

  try {
    if (editingUser.value) {
      await usersAPI.update(editingUser.value.id, userForm.value)
    } else {
      await usersAPI.create(userForm.value)
    }

    await fetchUsers()
    closeModal()
  } catch (err) {
    error.value = err.response?.data?.detail || 'Failed to save user'
  } finally {
    submitting.value = false
  }
}

const closeModal = () => {
  showCreateModal.value = false
  editingUser.value = null
  userForm.value = {
    username: '',
    email: '',
    password: '',
    role: 'user',
  }
}

const formatDate = (date) => {
  try {
    return format(new Date(date), 'PP')
  } catch {
    return 'N/A'
  }
}

onMounted(() => {
  fetchUsers()
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

.role-badge {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
}

.role-admin {
  background-color: #dbeafe;
  color: #1e40af;
}

.role-user {
  background-color: #f3f4f6;
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

.btn-icon:hover:not(:disabled) {
  opacity: 1;
}

.btn-icon:disabled {
  opacity: 0.3;
  cursor: not-allowed;
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
</style>
