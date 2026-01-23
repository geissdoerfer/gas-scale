<template>
  <nav class="navbar">
    <div class="container navbar-content">
      <div class="navbar-brand">
        <router-link to="/" class="brand-link">
          <h1 class="brand-title">DuoClean Energy</h1>
        </router-link>
      </div>

      <div class="navbar-menu">
        <router-link to="/" class="nav-link">Dashboard</router-link>

        <div v-if="authStore.isAdmin" class="nav-dropdown">
          <button class="nav-link">Admin</button>
          <div class="dropdown-content">
            <router-link to="/admin/users" class="dropdown-link">Users</router-link>
            <router-link to="/admin/devices" class="dropdown-link">Devices</router-link>
          </div>
        </div>

        <div class="nav-user">
          <span class="user-name">{{ authStore.currentUser?.username }}</span>
          <button @click="handleLogout" class="btn btn-secondary btn-sm">Logout</button>
        </div>
      </div>
    </div>
  </nav>
</template>

<script setup>
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const authStore = useAuthStore()

const handleLogout = () => {
  authStore.logout()
  router.push('/login')
}
</script>

<style scoped>
.navbar {
  background: white;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  position: sticky;
  top: 0;
  z-index: 100;
}

.navbar-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 20px;
}

.navbar-brand {
  flex-shrink: 0;
}

.brand-link {
  text-decoration: none;
}

.brand-title {
  font-size: 20px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.navbar-menu {
  display: flex;
  align-items: center;
  gap: 24px;
}

.nav-link {
  color: #374151;
  text-decoration: none;
  font-weight: 500;
  font-size: 14px;
  padding: 8px 12px;
  border-radius: 4px;
  transition: all 0.2s;
  background: none;
  border: none;
  cursor: pointer;
}

.nav-link:hover {
  color: #2563eb;
  background-color: #eff6ff;
}

.nav-dropdown {
  position: relative;
}

.dropdown-content {
  display: none;
  position: absolute;
  top: 100%;
  left: 0;
  background: white;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  border-radius: 4px;
  min-width: 150px;
  margin-top: 4px;
}

.nav-dropdown:hover .dropdown-content {
  display: block;
}

.dropdown-link {
  display: block;
  padding: 12px 16px;
  color: #374151;
  text-decoration: none;
  font-size: 14px;
  transition: all 0.2s;
}

.dropdown-link:hover {
  background-color: #f3f4f6;
  color: #2563eb;
}

.nav-user {
  display: flex;
  align-items: center;
  gap: 12px;
}

.user-name {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
}

.btn-sm {
  padding: 6px 12px;
  font-size: 13px;
}
</style>
