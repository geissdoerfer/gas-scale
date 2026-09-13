@file:Suppress("DEPRECATION") // EncryptedSharedPreferences/MasterKey have no drop-in replacement yet.

package com.example.verolic.data.local

import android.content.Context
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

/** App-level state: unpaired, or paired to one device. Never more than one device at a time. */
sealed interface PairState {
    data object Unpaired : PairState
    data class Paired(val deviceId: String) : PairState
}

/**
 * Persists the paired device's id and device-scoped bearer token in EncryptedSharedPreferences.
 * There is no user account in this app - this token is the only credential, so it gets the
 * same encrypted-at-rest treatment a user JWT would.
 */
class TokenStore(context: Context) {

    private val prefs = run {
        val masterKey = MasterKey.Builder(context)
            .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
            .build()

        EncryptedSharedPreferences.create(
            context,
            PREFS_NAME,
            masterKey,
            EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
            EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
        )
    }

    private val _pairState = MutableStateFlow(loadPairState())
    val pairState: StateFlow<PairState> = _pairState.asStateFlow()

    val accessToken: String?
        get() = prefs.getString(KEY_ACCESS_TOKEN, null)

    fun save(deviceId: String, accessToken: String) {
        prefs.edit()
            .putString(KEY_DEVICE_ID, deviceId)
            .putString(KEY_ACCESS_TOKEN, accessToken)
            .apply()
        _pairState.value = PairState.Paired(deviceId)
    }

    fun clear() {
        prefs.edit().clear().apply()
        _pairState.value = PairState.Unpaired
    }

    private fun loadPairState(): PairState {
        val deviceId = prefs.getString(KEY_DEVICE_ID, null)
        val token = prefs.getString(KEY_ACCESS_TOKEN, null)
        return if (deviceId != null && token != null) PairState.Paired(deviceId) else PairState.Unpaired
    }

    private companion object {
        const val PREFS_NAME = "verolic_secure_prefs"
        const val KEY_DEVICE_ID = "device_id"
        const val KEY_ACCESS_TOKEN = "access_token"
    }
}
