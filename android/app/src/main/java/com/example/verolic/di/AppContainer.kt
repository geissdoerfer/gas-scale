package com.example.verolic.di

import androidx.compose.runtime.staticCompositionLocalOf
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.example.verolic.data.repository.DeviceRepository

val LocalDeviceRepository = staticCompositionLocalOf<DeviceRepository> {
    error("No DeviceRepository provided - wrap the app in CompositionLocalProvider(LocalDeviceRepository provides ...)")
}

/** One-off [ViewModelProvider.Factory] for a ViewModel with a non-trivial constructor. */
inline fun <VM : ViewModel> viewModelFactory(crossinline creator: () -> VM): ViewModelProvider.Factory =
    object : ViewModelProvider.Factory {
        @Suppress("UNCHECKED_CAST")
        override fun <T : ViewModel> create(modelClass: Class<T>): T = creator() as T
    }
