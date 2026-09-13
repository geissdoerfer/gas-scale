package com.example.verolic.ui.cylinder

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.verolic.data.local.PairState
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.CylinderType
import com.example.verolic.data.repository.DeviceRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface CylinderPickerUiState {
    data object Loading : CylinderPickerUiState
    data class Ready(val current: CylinderType?, val saving: Boolean = false, val error: String? = null) :
        CylinderPickerUiState
    data object Saved : CylinderPickerUiState
    data object Unpaired : CylinderPickerUiState
}

class CylinderPickerViewModel(private val deviceRepository: DeviceRepository) : ViewModel() {

    private val _uiState = MutableStateFlow<CylinderPickerUiState>(CylinderPickerUiState.Loading)
    val uiState: StateFlow<CylinderPickerUiState> = _uiState.asStateFlow()

    init {
        viewModelScope.launch {
            when (val result = deviceRepository.getStatus()) {
                is ApiResult.Success -> _uiState.value = CylinderPickerUiState.Ready(result.data.cylinderType)
                is ApiResult.Error -> {
                    // Not fatal for onboarding - the picker still works with nothing
                    // pre-selected even if the current-value lookup failed, unless the
                    // failure was because the device got unpaired (see DeviceRepository).
                    _uiState.value = if (deviceRepository.pairState.value is PairState.Unpaired) {
                        CylinderPickerUiState.Unpaired
                    } else {
                        CylinderPickerUiState.Ready(current = null)
                    }
                }
            }
        }
    }

    fun select(type: CylinderType) {
        val current = _uiState.value as? CylinderPickerUiState.Ready ?: return
        _uiState.value = current.copy(saving = true, error = null)
        viewModelScope.launch {
            when (val result = deviceRepository.setCylinderType(type)) {
                is ApiResult.Success -> _uiState.value = CylinderPickerUiState.Saved
                is ApiResult.Error -> {
                    _uiState.value = if (deviceRepository.pairState.value is PairState.Unpaired) {
                        CylinderPickerUiState.Unpaired
                    } else {
                        current.copy(saving = false, error = result.message)
                    }
                }
            }
        }
    }
}
