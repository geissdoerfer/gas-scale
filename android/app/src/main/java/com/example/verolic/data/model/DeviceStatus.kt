package com.example.verolic.data.model

/** Mirrors the `status` field of GET /devices/me/status. */
enum class DeviceHealth { OK, LOW_BATTERY, NOT_CONFIGURED, NO_DATA, UNKNOWN }

/** Domain-level view of a device's current status - percentage is always server-computed. */
data class DeviceStatus(
    val cylinderType: CylinderType?,
    val percentageRemaining: Int?,
    val batteryPercentage: Int?,
    val batteryVoltage: Double?,
    val lastUpdated: String?,
    val health: DeviceHealth
)
