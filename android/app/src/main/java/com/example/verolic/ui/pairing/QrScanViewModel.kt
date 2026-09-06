package com.example.verolic.ui.pairing

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.AuthState
import com.example.verolic.data.model.PairingPayload
import com.example.verolic.data.repository.AuthRepository
import com.example.verolic.data.repository.DeviceRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface QrScanUiState {
    data object Scanning : QrScanUiState
    data object Claiming : QrScanUiState
    data class Error(val message: String) : QrScanUiState
    data class Claimed(val deviceId: String) : QrScanUiState
}

class QrScanViewModel(
    private val authRepository: AuthRepository,
    private val deviceRepository: DeviceRepository
) : ViewModel() {

    private val _uiState = MutableStateFlow<QrScanUiState>(QrScanUiState.Scanning)
    val uiState: StateFlow<QrScanUiState> = _uiState.asStateFlow()

    private var claimInFlight = false

    /** Called with the raw text decoded from a scanned QR code. Ignored once a claim is in flight. */
    fun onQrCodeScanned(raw: String) {
        if (claimInFlight) return

        val payload = PairingPayload.parse(raw)
        if (payload == null) {
            _uiState.value = QrScanUiState.Error("That doesn't look like a Verolic pairing code.")
            return
        }
        val userId = (authRepository.authState.value as? AuthState.LoggedIn)?.user?.id
            ?: return

        claimInFlight = true
        viewModelScope.launch {
            _uiState.value = QrScanUiState.Claiming
            when (val result = deviceRepository.claimDevice(userId, payload)) {
                is ApiResult.Success -> _uiState.value = QrScanUiState.Claimed(result.data.deviceId)
                is ApiResult.Error -> {
                    _uiState.value = QrScanUiState.Error(result.message)
                    claimInFlight = false
                }
            }
        }
    }

    fun retry() {
        claimInFlight = false
        _uiState.value = QrScanUiState.Scanning
    }
}
