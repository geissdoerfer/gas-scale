package com.example.verolic.data.repository

import com.example.verolic.data.local.PairState
import com.example.verolic.data.local.TokenStore
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.CylinderType
import com.example.verolic.data.model.DeviceHealth
import com.example.verolic.data.model.DeviceStatus
import com.example.verolic.data.model.PairingPayload
import com.example.verolic.data.network.ApiService
import com.example.verolic.data.network.dto.CylinderUpdateRequestDto
import com.example.verolic.data.network.dto.DevicePairRequestDto
import com.example.verolic.data.network.dto.DeviceStatusResponseDto
import com.example.verolic.data.network.dto.ErrorResponseDto
import kotlinx.coroutines.flow.StateFlow
import kotlinx.serialization.json.Json
import retrofit2.HttpException
import java.io.IOException

class DeviceRepository(
    private val apiService: ApiService,
    private val tokenStore: TokenStore
) {
    val pairState: StateFlow<PairState> = tokenStore.pairState

    suspend fun pair(payload: PairingPayload): ApiResult<Unit> = safeCall {
        val response = apiService.pairDevice(DevicePairRequestDto(payload.deviceId, payload.secret))
        tokenStore.save(response.deviceId, response.accessToken)
    }

    suspend fun setCylinderType(type: CylinderType): ApiResult<DeviceStatus> = safeCall {
        apiService.setCylinderType(CylinderUpdateRequestDto(type.apiValue)).toDomain()
    }

    suspend fun getStatus(): ApiResult<DeviceStatus> = safeCall {
        apiService.getStatus().toDomain()
    }

    /** Purely local: clears the stored token. See android/app_plan.md §3.3 for why no
     * server call is needed - the pairing secret is static and reusable. */
    fun unpair() {
        tokenStore.clear()
    }

    private suspend fun <T> safeCall(block: suspend () -> T): ApiResult<T> {
        return try {
            ApiResult.Success(block())
        } catch (e: HttpException) {
            if (e.code() == 401) {
                // No refresh flow for a device token - if it's rejected, treat as unpaired.
                tokenStore.clear()
            }
            ApiResult.Error(extractErrorMessage(e))
        } catch (e: IOException) {
            ApiResult.Error("Couldn't reach the server. Check your connection and try again.")
        }
    }

    private fun extractErrorMessage(e: HttpException): String {
        val body = e.response()?.errorBody()?.string()
        val detail = body?.let {
            runCatching { errorJson.decodeFromString(ErrorResponseDto.serializer(), it).detail }.getOrNull()
        }
        return detail ?: when (e.code()) {
            401 -> "Invalid device ID or pairing secret"
            404 -> "Device not found"
            else -> "Something went wrong (${e.code()})"
        }
    }

    private companion object {
        val errorJson = Json { ignoreUnknownKeys = true }
    }
}

private fun DeviceStatusResponseDto.toDomain(): DeviceStatus = DeviceStatus(
    cylinderType = CylinderType.fromApiValue(cylinderType),
    percentageRemaining = percentageRemaining,
    batteryPercentage = batteryPercentage,
    batteryVoltage = batteryVoltage,
    lastUpdated = lastUpdated,
    health = when (status) {
        "ok" -> DeviceHealth.OK
        "low_battery" -> DeviceHealth.LOW_BATTERY
        "not_configured" -> DeviceHealth.NOT_CONFIGURED
        "no_data" -> DeviceHealth.NO_DATA
        else -> DeviceHealth.UNKNOWN
    }
)
