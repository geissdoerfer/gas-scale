package com.example.verolic.data.network.dto

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/** Wire types for the account-less device-pairing endpoints - keep in sync with web-api/src/schemas.py. */

@Serializable
data class DevicePairRequestDto(
    @SerialName("device_id") val deviceId: String,
    val secret: String
)

@Serializable
data class DevicePairResponseDto(
    @SerialName("access_token") val accessToken: String,
    @SerialName("token_type") val tokenType: String,
    @SerialName("device_id") val deviceId: String
)

@Serializable
data class CylinderUpdateRequestDto(
    @SerialName("cylinder_type") val cylinderType: String
)

@Serializable
data class DeviceStatusResponseDto(
    @SerialName("device_id") val deviceId: String,
    @SerialName("cylinder_type") val cylinderType: String? = null,
    @SerialName("percentage_remaining") val percentageRemaining: Int? = null,
    @SerialName("battery_percentage") val batteryPercentage: Int? = null,
    @SerialName("battery_voltage") val batteryVoltage: Double? = null,
    @SerialName("last_updated") val lastUpdated: String? = null,
    val status: String
)

@Serializable
data class ErrorResponseDto(val detail: String? = null)
