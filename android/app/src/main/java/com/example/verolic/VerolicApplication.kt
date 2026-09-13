package com.example.verolic

import android.app.Application
import com.example.verolic.data.local.TokenStore
import com.example.verolic.data.network.NetworkModule
import com.example.verolic.data.repository.DeviceRepository

/** Hand-rolled service locator - hosts the one repository this app needs across Activity
 * recreation (rotation etc). A real DI framework would be overkill at this size. */
class VerolicApplication : Application() {

    lateinit var deviceRepository: DeviceRepository
        private set

    override fun onCreate() {
        super.onCreate()
        val tokenStore = TokenStore(this)
        val apiService = NetworkModule.createApiService(tokenStore)
        deviceRepository = DeviceRepository(apiService, tokenStore)
    }
}
