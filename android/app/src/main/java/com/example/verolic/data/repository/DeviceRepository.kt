package com.example.verolic.data.repository

import com.example.verolic.data.fake.FakeBackend
import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.BottleType
import com.example.verolic.data.model.Device
import com.example.verolic.data.model.PairingPayload
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

interface DeviceRepository {
    val bottleTypes: List<BottleType>
    fun observeDevices(userId: String): Flow<List<Device>>
    fun observeDevice(userId: String, deviceId: String): Flow<Device?>
    suspend fun claimDevice(userId: String, pairing: PairingPayload): ApiResult<Device>
    suspend fun updateBottleConfig(
        userId: String,
        deviceId: String,
        name: String,
        tareWeightKg: Double,
        bottleTypeId: String
    ): ApiResult<Device>
}

/**
 * Talks to [FakeBackend] instead of the real `/devices` endpoints. Swap for a Retrofit-backed
 * implementation once the claim/config endpoints from android/blueprint.md exist server-side.
 */
class FakeDeviceRepository : DeviceRepository {

    override val bottleTypes: List<BottleType> = FakeBackend.bottleTypes

    override fun observeDevices(userId: String): Flow<List<Device>> =
        FakeBackend.devices.map { all ->
            with(FakeBackend) {
                all.values
                    .filter { it.ownerId == userId }
                    .map { it.toDevice() }
                    .sortedBy { it.name }
            }
        }

    override fun observeDevice(userId: String, deviceId: String): Flow<Device?> =
        FakeBackend.devices.map { all ->
            with(FakeBackend) {
                all[deviceId]?.takeIf { it.ownerId == userId }?.toDevice()
            }
        }

    override suspend fun claimDevice(userId: String, pairing: PairingPayload): ApiResult<Device> {
        delay(SIMULATED_NETWORK_DELAY_MILLIS)
        return FakeBackend.claimDevice(userId, pairing.deviceId, pairing.secret)
    }

    override suspend fun updateBottleConfig(
        userId: String,
        deviceId: String,
        name: String,
        tareWeightKg: Double,
        bottleTypeId: String
    ): ApiResult<Device> {
        delay(SIMULATED_NETWORK_DELAY_MILLIS)
        return FakeBackend.updateBottleConfig(userId, deviceId, name, tareWeightKg, bottleTypeId)
    }

    private companion object {
        const val SIMULATED_NETWORK_DELAY_MILLIS = 450L
    }
}
