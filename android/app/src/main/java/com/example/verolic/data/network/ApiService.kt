package com.example.verolic.data.network

import com.example.verolic.data.network.dto.CylinderUpdateRequestDto
import com.example.verolic.data.network.dto.DevicePairRequestDto
import com.example.verolic.data.network.dto.DevicePairResponseDto
import com.example.verolic.data.network.dto.DeviceStatusResponseDto
import retrofit2.http.Body
import retrofit2.http.GET
import retrofit2.http.PATCH
import retrofit2.http.POST

interface ApiService {
    /** Public - no auth. device_id + secret is the credential (see web-api POST /devices/pair). */
    @POST("devices/pair")
    suspend fun pairDevice(@Body request: DevicePairRequestDto): DevicePairResponseDto

    @PATCH("devices/me/cylinder")
    suspend fun setCylinderType(@Body request: CylinderUpdateRequestDto): DeviceStatusResponseDto

    @GET("devices/me/status")
    suspend fun getStatus(): DeviceStatusResponseDto
}
