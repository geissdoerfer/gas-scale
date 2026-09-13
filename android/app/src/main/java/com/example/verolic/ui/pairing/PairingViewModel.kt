package com.example.verolic.ui.pairing

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.PairingPayload
import com.example.verolic.data.repository.DeviceRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface PairingUiState {
    data object Idle : PairingUiState
    data object Pairing : PairingUiState
    data class Error(val message: String) : PairingUiState
    data object Success : PairingUiState
}

class PairingViewModel(private val deviceRepository: DeviceRepository) : ViewModel() {

    private val _uiState = MutableStateFlow<PairingUiState>(PairingUiState.Idle)
    val uiState: StateFlow<PairingUiState> = _uiState.asStateFlow()

    fun pair(payload: PairingPayload) {
        if (_uiState.value is PairingUiState.Pairing) return
        _uiState.value = PairingUiState.Pairing
        viewModelScope.launch {
            _uiState.value = when (val result = deviceRepository.pair(payload)) {
                is ApiResult.Success -> PairingUiState.Success
                is ApiResult.Error -> PairingUiState.Error(result.message)
            }
        }
    }
}
