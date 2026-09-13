package com.example.verolic.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.example.verolic.data.model.DeviceHealth

@Composable
fun StatusBadge(health: DeviceHealth, modifier: Modifier = Modifier) {
    val (label, color) = when (health) {
        DeviceHealth.OK -> "OK" to Color(0xFF059669)
        DeviceHealth.LOW_BATTERY -> "Low battery" to Color(0xFFD97706)
        DeviceHealth.NOT_CONFIGURED -> "Not configured" to Color(0xFF6B7280)
        DeviceHealth.NO_DATA -> "No data yet" to Color(0xFF6B7280)
        DeviceHealth.UNKNOWN -> "Unknown" to Color(0xFF6B7280)
    }
    Text(
        text = label,
        color = Color.White,
        style = MaterialTheme.typography.labelMedium,
        modifier = modifier
            .background(color, RoundedCornerShape(12.dp))
            .padding(horizontal = 12.dp, vertical = 4.dp)
    )
}
