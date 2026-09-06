package com.example.verolic.data.repository

import com.example.verolic.data.fake.FakeBackend
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.AuthState
import com.example.verolic.data.model.User
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

interface AuthRepository {
    val authState: StateFlow<AuthState>
    suspend fun login(username: String, password: String): ApiResult<User>
    suspend fun register(username: String, email: String, password: String): ApiResult<User>
    fun logout()
}

/**
 * Talks to [FakeBackend] instead of a Retrofit service. Swap this for a real implementation
 * once `/auth/login`, `/auth/register` and `/auth/refresh` exist on the server (see
 * android/blueprint.md) — [AuthState] and the method signatures are designed to stay the same.
 */
class FakeAuthRepository : AuthRepository {

    private val _authState = MutableStateFlow<AuthState>(AuthState.LoggedOut)
    override val authState: StateFlow<AuthState> = _authState.asStateFlow()

    override suspend fun login(username: String, password: String): ApiResult<User> {
        delay(SIMULATED_NETWORK_DELAY_MILLIS)
        val result = FakeBackend.login(username, password)
        if (result is ApiResult.Success) _authState.value = AuthState.LoggedIn(result.data)
        return result
    }

    override suspend fun register(username: String, email: String, password: String): ApiResult<User> {
        delay(SIMULATED_NETWORK_DELAY_MILLIS)
        val result = FakeBackend.register(username, email, password)
        if (result is ApiResult.Success) _authState.value = AuthState.LoggedIn(result.data)
        return result
    }

    override fun logout() {
        _authState.value = AuthState.LoggedOut
    }

    private companion object {
        const val SIMULATED_NETWORK_DELAY_MILLIS = 550L
    }
}
