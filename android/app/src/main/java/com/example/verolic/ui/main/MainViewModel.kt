package com.example.verolic.ui.main

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.verolic.data.local.PairState
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.DeviceStatus
import com.example.verolic.data.repository.DeviceRepository
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface MainUiState {
    data object Loading : MainUiState
    data class Loaded(val status: DeviceStatus, val refreshing: Boolean = false) : MainUiState
    data class Error(val message: String) : MainUiState
    data object Unpaired : MainUiState
}

class MainViewModel(private val deviceRepository: DeviceRepository) : ViewModel() {

    private val _uiState = MutableStateFlow<MainUiState>(MainUiState.Loading)
    val uiState: StateFlow<MainUiState> = _uiState.asStateFlow()

    init {
        // Polls while this screen (and therefore this ViewModel) is alive - simple by
        // design for the MVP, see android/app_plan.md §3.5.
        viewModelScope.launch {
            while (true) {
                loadStatus()
                delay(POLL_INTERVAL_MILLIS)
            }
        }
    }

    fun refresh() {
        viewModelScope.launch { loadStatus() }
    }

    fun unpair() {
        deviceRepository.unpair()
        _uiState.value = MainUiState.Unpaired
    }

    private suspend fun loadStatus() {
        (_uiState.value as? MainUiState.Loaded)?.let { _uiState.value = it.copy(refreshing = true) }

        when (val result = deviceRepository.getStatus()) {
            is ApiResult.Success -> _uiState.value = MainUiState.Loaded(result.data)
            is ApiResult.Error -> {
                _uiState.value = if (deviceRepository.pairState.value is PairState.Unpaired) {
                    MainUiState.Unpaired
                } else {
                    MainUiState.Error(result.message)
                }
            }
        }
    }

    private companion object {
        const val POLL_INTERVAL_MILLIS = 30_000L
    }
}
