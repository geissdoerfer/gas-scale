package com.example.verolic.ui.components

import androidx.compose.runtime.Composable
import androidx.compose.runtime.ReadOnlyComposable
import androidx.compose.ui.graphics.Color
import com.example.verolic.data.model.DeviceStatus
import com.example.verolic.ui.theme.GasCritical
import com.example.verolic.ui.theme.GasLow
import com.example.verolic.ui.theme.GasOk
import com.example.verolic.ui.theme.GasUnknown

@Composable
@ReadOnlyComposable
fun DeviceStatus.color(): Color = when (this) {
    DeviceStatus.OK -> GasOk
    DeviceStatus.LOW -> GasLow
    DeviceStatus.CRITICAL -> GasCritical
    DeviceStatus.NOT_CONFIGURED, DeviceStatus.NO_DATA -> GasUnknown
}

fun DeviceStatus.label(): String = when (this) {
    DeviceStatus.OK -> "OK"
    DeviceStatus.LOW -> "Low"
    DeviceStatus.CRITICAL -> "Critical"
    DeviceStatus.NOT_CONFIGURED -> "Setup needed"
    DeviceStatus.NO_DATA -> "No data"
}
