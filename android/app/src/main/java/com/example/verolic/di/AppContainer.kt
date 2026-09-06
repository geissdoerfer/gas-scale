package com.example.verolic.di

import androidx.compose.runtime.staticCompositionLocalOf
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.example.verolic.data.repository.AuthRepository
import com.example.verolic.data.repository.DeviceRepository
import com.example.verolic.data.repository.FakeAuthRepository
import com.example.verolic.data.repository.FakeDeviceRepository

/**
 * Hand-rolled service locator. A real DI framework (Hilt, per android/blueprint.md) is
 * overkill for two repositories — swap this in when the graph grows.
 */
class AppContainer {
    val authRepository: AuthRepository by lazy { FakeAuthRepository() }
    val deviceRepository: DeviceRepository by lazy { FakeDeviceRepository() }
}

val LocalAppContainer = staticCompositionLocalOf<AppContainer> {
    error("No AppContainer provided — wrap the app in CompositionLocalProvider(LocalAppContainer provides ...)")
}

/** One-off [ViewModelProvider.Factory] for a ViewModel with a non-trivial constructor. */
inline fun <VM : ViewModel> viewModelFactory(crossinline creator: () -> VM): ViewModelProvider.Factory =
    object : ViewModelProvider.Factory {
        @Suppress("UNCHECKED_CAST")
        override fun <T : ViewModel> create(modelClass: Class<T>): T = creator() as T
    }
