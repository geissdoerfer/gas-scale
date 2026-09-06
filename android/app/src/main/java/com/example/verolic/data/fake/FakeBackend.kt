package com.example.verolic.data.fake

import com.example.verolic.data.model.ApiResult
import com.example.verolic.data.model.Bottle
import com.example.verolic.data.model.BottleType
import com.example.verolic.data.model.Device
import com.example.verolic.data.model.SensorReading
import com.example.verolic.data.model.User
import java.util.UUID
import kotlin.random.Random
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * Stands in for the real web-api while the Android app is built ahead of the backend work.
 * Holds all "server" state in memory and ticks device readings on a timer so the UI has
 * something live to react to. Nothing here talks to the network or survives process death —
 * see android/blueprint.md's "Backend API changes required" for what this needs to become.
 */
object FakeBackend {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
    private val lock = Any()

    data class UserRecord(
        val id: String,
        val username: String,
        val email: String,
        val password: String
    )

    data class DeviceRecord(
        val deviceId: String,
        val name: String,
        val pairingSecret: String,
        val ownerId: String?,
        val tareWeightKg: Double?,
        val bottleTypeId: String?,
        val currentWeightKg: Double,
        val batteryVoltage: Double,
        val temperatureC: Double,
        val lastUpdatedMillis: Long
    )

    val bottleTypes: List<BottleType> = listOf(
        BottleType("camping_2_75", "Camping gas cylinder – 2.75 kg", 2.75),
        BottleType("propane_5", "Propane bottle – 5 kg", 5.0),
        BottleType("propane_11", "Propane bottle – 11 kg", 11.0),
        BottleType("propane_33", "Propane bottle – 33 kg", 33.0)
    )

    private val _users = MutableStateFlow<Map<String, UserRecord>>(emptyMap())
    private val _devices = MutableStateFlow<Map<String, DeviceRecord>>(emptyMap())
    val devices: StateFlow<Map<String, DeviceRecord>> = _devices.asStateFlow()

    init {
        seed()
        startTicking()
    }

    // --- Auth -----------------------------------------------------------------------------

    fun login(username: String, password: String): ApiResult<User> = synchronized(lock) {
        val record = _users.value[username.trim().lowercase()]
            ?: return ApiResult.Error("No account found for that username.")
        if (record.password != password) return ApiResult.Error("Incorrect password.")
        ApiResult.Success(record.toUser())
    }

    fun register(username: String, email: String, password: String): ApiResult<User> = synchronized(lock) {
        val key = username.trim().lowercase()
        if (key.length < 3) return ApiResult.Error("Username must be at least 3 characters.")
        if (key.any { !(it.isLetterOrDigit() || it == '_') }) {
            return ApiResult.Error("Username can only contain letters, numbers and underscores.")
        }
        if (_users.value.containsKey(key)) return ApiResult.Error("That username is already taken.")
        if (!email.contains("@") || !email.contains(".")) return ApiResult.Error("Enter a valid email address.")
        if (password.length < 8) return ApiResult.Error("Password must be at least 8 characters.")

        val record = UserRecord(id = "u_${UUID.randomUUID()}", username = username.trim(), email = email.trim(), password = password)
        _users.update { it + (key to record) }
        ApiResult.Success(record.toUser())
    }

    // --- Devices ----------------------------------------------------------------------------

    fun claimDevice(userId: String, deviceId: String, secret: String): ApiResult<Device> = synchronized(lock) {
        val record = _devices.value[deviceId]
            ?: return ApiResult.Error("That QR code doesn't match any known device.")
        if (record.pairingSecret != secret) {
            return ApiResult.Error("That QR code isn't valid for this device.")
        }
        if (record.ownerId != null) {
            return if (record.ownerId == userId) {
                ApiResult.Error("You've already added this device.")
            } else {
                ApiResult.Error("This device is already linked to another account.")
            }
        }
        val updated = record.copy(ownerId = userId)
        _devices.update { it + (deviceId to updated) }
        ApiResult.Success(updated.toDevice())
    }

    fun updateBottleConfig(
        userId: String,
        deviceId: String,
        name: String,
        tareWeightKg: Double,
        bottleTypeId: String
    ): ApiResult<Device> = synchronized(lock) {
        val record = _devices.value[deviceId] ?: return ApiResult.Error("Device not found.")
        if (record.ownerId != userId) return ApiResult.Error("This device isn't linked to your account.")
        if (tareWeightKg <= 0.0) return ApiResult.Error("Enter a valid tare weight.")
        if (bottleTypes.none { it.id == bottleTypeId }) return ApiResult.Error("Pick a bottle type.")

        val updated = record.copy(
            name = name.trim().ifBlank { record.name },
            tareWeightKg = tareWeightKg,
            bottleTypeId = bottleTypeId
        )
        _devices.update { it + (deviceId to updated) }
        ApiResult.Success(updated.toDevice())
    }

    // --- Simulation -------------------------------------------------------------------------

    private fun startTicking() {
        scope.launch {
            while (isActive) {
                delay(TICK_INTERVAL_MILLIS)
                synchronized(lock) {
                    _devices.update { current ->
                        current.mapValues { (_, record) ->
                            if (record.ownerId == null) record else record.ticked()
                        }
                    }
                }
            }
        }
    }

    private fun DeviceRecord.ticked(): DeviceRecord {
        val tare = tareWeightKg ?: 0.0
        val drop = if (Random.nextInt(100) < 8) Random.nextDouble(0.015, 0.05) else Random.nextDouble(0.0, 0.0025)
        val newWeight = (currentWeightKg - drop).coerceAtLeast(tare)
        val newBattery = (batteryVoltage - Random.nextDouble(0.0, 0.0004)).coerceAtLeast(3.0)
        val newTemp = (temperatureC + Random.nextDouble(-0.3, 0.3)).coerceIn(15.0, 30.0)
        return copy(
            currentWeightKg = newWeight,
            batteryVoltage = newBattery,
            temperatureC = newTemp,
            lastUpdatedMillis = System.currentTimeMillis()
        )
    }

    private fun seed() {
        val now = System.currentTimeMillis()
        val demoUser = UserRecord(id = "u_demo", username = "demo", email = "demo@verolic.app", password = "demo1234")
        _users.value = mapOf(demoUser.username to demoUser)

        val kitchenTare = 12.5
        val kitchenFill = bottleTypeById("propane_11").nominalFillKg
        val garageTare = 4.6
        val garageFill = bottleTypeById("propane_5").nominalFillKg

        _devices.value = mapOf(
            "device_101" to DeviceRecord(
                deviceId = "device_101",
                name = "Kitchen",
                pairingSecret = "PAIR-KITCHEN-DEMO",
                ownerId = demoUser.id,
                tareWeightKg = kitchenTare,
                bottleTypeId = "propane_11",
                currentWeightKg = kitchenTare + kitchenFill * 0.62,
                batteryVoltage = 3.82,
                temperatureC = 21.0,
                lastUpdatedMillis = now
            ),
            "device_102" to DeviceRecord(
                deviceId = "device_102",
                name = "Garage",
                pairingSecret = "PAIR-GARAGE-DEMO",
                ownerId = demoUser.id,
                tareWeightKg = garageTare,
                bottleTypeId = "propane_5",
                currentWeightKg = garageTare + garageFill * 0.08,
                batteryVoltage = 3.31,
                temperatureC = 18.5,
                lastUpdatedMillis = now
            ),
            "device_201" to DeviceRecord(
                deviceId = "device_201",
                name = "New device",
                pairingSecret = "PAIR-7F3K9Q",
                ownerId = null,
                tareWeightKg = null,
                bottleTypeId = null,
                currentWeightKg = 13.0,
                batteryVoltage = 4.12,
                temperatureC = 20.0,
                lastUpdatedMillis = now
            ),
            "device_202" to DeviceRecord(
                deviceId = "device_202",
                name = "New device",
                pairingSecret = "PAIR-2M8LXZ",
                ownerId = null,
                tareWeightKg = null,
                bottleTypeId = null,
                currentWeightKg = 13.0,
                batteryVoltage = 4.15,
                temperatureC = 20.0,
                lastUpdatedMillis = now
            )
        )
    }

    private fun bottleTypeById(id: String) = bottleTypes.first { it.id == id }

    private fun UserRecord.toUser() = User(id = id, username = username, email = email)

    fun DeviceRecord.toDevice(): Device {
        val type = bottleTypeId?.let { id -> bottleTypes.find { it.id == id } }
        val bottle = if (tareWeightKg != null && type != null) Bottle(tareWeightKg, type) else null
        return Device(
            deviceId = deviceId,
            name = name,
            currentBottle = bottle,
            latestReading = SensorReading(
                timestampMillis = lastUpdatedMillis,
                weightKg = currentWeightKg,
                batteryVoltage = batteryVoltage,
                temperatureC = temperatureC
            )
        )
    }

    private const val TICK_INTERVAL_MILLIS = 4000L
}
