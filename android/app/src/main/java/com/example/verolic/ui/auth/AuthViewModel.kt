package com.example.verolic.ui.auth

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.repository.AuthRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface AuthUiState {
    data object Idle : AuthUiState
    data object Loading : AuthUiState
    data class Error(val message: String) : AuthUiState
}

class AuthViewModel(private val authRepository: AuthRepository) : ViewModel() {

    private val _uiState = MutableStateFlow<AuthUiState>(AuthUiState.Idle)
    val uiState: StateFlow<AuthUiState> = _uiState.asStateFlow()

    fun login(username: String, password: String) {
        if (username.isBlank() || password.isBlank()) {
            _uiState.value = AuthUiState.Error("Enter your username and password.")
            return
        }
        viewModelScope.launch {
            _uiState.value = AuthUiState.Loading
            when (val result = authRepository.login(username, password)) {
                is ApiResult.Success -> _uiState.value = AuthUiState.Idle
                is ApiResult.Error -> _uiState.value = AuthUiState.Error(result.message)
            }
        }
    }

    fun register(username: String, email: String, password: String, confirmPassword: String) {
        if (username.isBlank() || email.isBlank() || password.isBlank()) {
            _uiState.value = AuthUiState.Error("Fill in all fields.")
            return
        }
        if (password != confirmPassword) {
            _uiState.value = AuthUiState.Error("Passwords don't match.")
            return
        }
        viewModelScope.launch {
            _uiState.value = AuthUiState.Loading
            when (val result = authRepository.register(username, email, password)) {
                is ApiResult.Success -> _uiState.value = AuthUiState.Idle
                is ApiResult.Error -> _uiState.value = AuthUiState.Error(result.message)
            }
        }
    }
}
