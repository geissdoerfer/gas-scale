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
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn

@OptIn(ExperimentalCoroutinesApi::class)
class DevicesViewModel(
    private val authRepository: AuthRepository,
    deviceRepository: DeviceRepository
) : ViewModel() {

    val devices: StateFlow<List<Device>> = authRepository.authState
        .flatMapLatest { state ->
            when (state) {
                is AuthState.LoggedIn -> deviceRepository.observeDevices(state.user.id)
                AuthState.LoggedOut -> flowOf(emptyList())
            }
        }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    val username: StateFlow<String?> = authRepository.authState
        .map { state -> (state as? AuthState.LoggedIn)?.user?.username }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), null)

    fun logout() {
        authRepository.logout()
    }
}
