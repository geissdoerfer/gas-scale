package com.example.verolic.ui.pairing

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.AuthState
import com.example.verolic.data.model.BottleType
import com.example.verolic.data.repository.AuthRepository
import com.example.verolic.data.repository.DeviceRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface BottleSetupUiState {
    data object Editing : BottleSetupUiState
    data object Saving : BottleSetupUiState
    data class Error(val message: String) : BottleSetupUiState
    data object Saved : BottleSetupUiState
}

class BottleSetupViewModel(
    private val authRepository: AuthRepository,
    private val deviceRepository: DeviceRepository,
    private val deviceId: String
) : ViewModel() {

    val bottleTypes: List<BottleType> = deviceRepository.bottleTypes

    private val _uiState = MutableStateFlow<BottleSetupUiState>(BottleSetupUiState.Editing)
    val uiState: StateFlow<BottleSetupUiState> = _uiState.asStateFlow()

    fun save(name: String, tareWeightKgText: String, bottleType: BottleType?) {
        val userId = (authRepository.authState.value as? AuthState.LoggedIn)?.user?.id ?: return

        val tareWeightKg = tareWeightKgText.replace(',', '.').toDoubleOrNull()
        if (tareWeightKg == null || tareWeightKg <= 0.0) {
            _uiState.value = BottleSetupUiState.Error("Enter the empty (tare) weight of your bottle in kg.")
            return
        }
        if (bottleType == null) {
            _uiState.value = BottleSetupUiState.Error("Pick your bottle type.")
            return
        }

        viewModelScope.launch {
            _uiState.value = BottleSetupUiState.Saving
            when (
                val result = deviceRepository.updateBottleConfig(
                    userId = userId,
                    deviceId = deviceId,
                    name = name,
                    tareWeightKg = tareWeightKg,
                    bottleTypeId = bottleType.id
                )
            ) {
                is ApiResult.Success -> _uiState.value = BottleSetupUiState.Saved
                is ApiResult.Error -> _uiState.value = BottleSetupUiState.Error(result.message)
            }
        }
    }
}
