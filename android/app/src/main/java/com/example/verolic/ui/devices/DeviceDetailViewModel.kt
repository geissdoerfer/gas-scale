package com.example.verolic.ui.devices

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.verolic.data.model.AuthState
import com.example.verolic.data.model.Device
import com.example.verolic.data.repository.AuthRepository
import com.example.verolic.data.repository.DeviceRepository
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.flow.stateIn

@OptIn(ExperimentalCoroutinesApi::class)
class DeviceDetailViewModel(
    authRepository: AuthRepository,
    deviceRepository: DeviceRepository,
    deviceId: String
) : ViewModel() {

    val device: StateFlow<Device?> = authRepository.authState
        .flatMapLatest { state ->
            when (state) {
                is AuthState.LoggedIn -> deviceRepository.observeDevice(state.user.id, deviceId)
                AuthState.LoggedOut -> flowOf(null)
            }
        }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), null)
}
