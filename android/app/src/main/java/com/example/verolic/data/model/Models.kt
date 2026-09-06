package com.example.verolic.data.model

/** Mirrors what the server will eventually compute and return per device. */
enum class DeviceStatus {
    OK,
    LOW,
    CRITICAL,
    NOT_CONFIGURED,
    NO_DATA
}

data class BottleType(
    val id: String,
    val label: String,
    val nominalFillKg: Double
)

/**
 * The physical gas bottle currently sitting on a [Device]. Distinct from the device itself:
 * a device is hardware you pair once via its QR code, a bottle is what's on it right now and
 * can be swapped out (e.g. on a refill) without re-pairing anything.
 */
data class Bottle(
    val tareWeightKg: Double,
    val bottleType: BottleType
)

data class SensorReading(
    val timestampMillis: Long,
    val weightKg: Double,
    val batteryVoltage: Double,
    val temperatureC: Double
)

data class User(
    val id: String,
    val username: String,
    val email: String
)

/** A paired gas bottle monitor. [currentBottle] is null until the owner finishes setup. */
data class Device(
    val deviceId: String,
    val name: String,
    val currentBottle: Bottle?,
    val latestReading: SensorReading?
) {
    /** Server-computed in the real API; derived here from the same formula for the mock. */
    val percentageRemaining: Int?
        get() {
            val bottle = currentBottle ?: return null
            val reading = latestReading ?: return null
            val fill = bottle.bottleType.nominalFillKg
            if (fill <= 0.0) return null
            val pct = ((reading.weightKg - bottle.tareWeightKg) / fill) * 100.0
            return pct.coerceIn(0.0, 100.0).toInt()
        }

    val status: DeviceStatus
        get() {
            if (currentBottle == null) return DeviceStatus.NOT_CONFIGURED
            val reading = latestReading ?: return DeviceStatus.NO_DATA
            val isStale = System.currentTimeMillis() - reading.timestampMillis > STALE_READING_MILLIS
            val pct = percentageRemaining
            if (isStale || pct == null) return DeviceStatus.NO_DATA
            return when {
                pct <= 10 -> DeviceStatus.CRITICAL
                pct <= 25 -> DeviceStatus.LOW
                else -> DeviceStatus.OK
            }
        }

    private companion object {
        const val STALE_READING_MILLIS = 24L * 60 * 60 * 1000
    }
}

/** Result of a simulated network call — mirrors how the real Retrofit calls will be wrapped. */
sealed interface ApiResult<out T> {
    data class Success<T>(val data: T) : ApiResult<T>
    data class Error(val message: String) : ApiResult<Nothing>
}

sealed interface AuthState {
    data object LoggedOut : AuthState
    data class LoggedIn(val user: User) : AuthState
}
