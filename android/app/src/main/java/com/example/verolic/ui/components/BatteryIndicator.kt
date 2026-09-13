package com.example.verolic.ui.components

import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.width
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

@Composable
fun BatteryIndicator(percentage: Int?, modifier: Modifier = Modifier) {
    Row(modifier = modifier, verticalAlignment = Alignment.CenterVertically) {
        Text("Battery", style = MaterialTheme.typography.labelLarge)
        Spacer(Modifier.width(12.dp))
        if (percentage != null) {
            LinearProgressIndicator(
                progress = { percentage.coerceIn(0, 100) / 100f },
                modifier = Modifier.width(100.dp)
            )
            Spacer(Modifier.width(8.dp))
            Text("$percentage%")
        } else {
            Text("Unknown", style = MaterialTheme.typography.bodyMedium)
        }
    }
}
