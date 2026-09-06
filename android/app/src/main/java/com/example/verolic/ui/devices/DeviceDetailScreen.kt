package com.example.verolic.ui.devices

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.verolic.data.model.Device
import com.example.verolic.data.model.DeviceStatus
import com.example.verolic.di.LocalAppContainer
import com.example.verolic.di.viewModelFactory
import com.example.verolic.ui.components.GasGauge
import com.example.verolic.ui.components.StatusBadge
import java.util.concurrent.TimeUnit

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DeviceDetailScreen(
    deviceId: String,
    onBack: () -> Unit,
    onEditBottleConfig: (String) -> Unit
) {
    val container = LocalAppContainer.current
    val viewModel: DeviceDetailViewModel = viewModel(
        factory = viewModelFactory {
            DeviceDetailViewModel(container.authRepository, container.deviceRepository, deviceId)
        }
    )
    val device by viewModel.device.collectAsStateWithLifecycle()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(device?.name ?: "Device") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                }
            )
        }
    ) { padding ->
        val current = device
        if (current == null) {
            Box(modifier = Modifier.padding(padding).fillMaxSize())
            return@Scaffold
        }

        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            GasGauge(percentage = current.percentageRemaining, status = current.status, diameter = 220.dp)
            Spacer(Modifier.height(12.dp))
            StatusBadge(status = current.status)
            Spacer(Modifier.height(24.dp))

            if (current.status == DeviceStatus.NOT_CONFIGURED) {
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text("Finish setting up this device", fontWeight = FontWeight.SemiBold)
                        Spacer(Modifier.height(4.dp))
                        Text(
                            "Tell us which bottle is on this device — its tare weight and bottle type — so Verolic can work out how much gas is left.",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        Spacer(Modifier.height(12.dp))
                        OutlinedButton(onClick = { onEditBottleConfig(deviceId) }) {
                            Icon(Icons.Filled.Edit, contentDescription = null, modifier = Modifier.size(18.dp))
                            Spacer(Modifier.width(8.dp))
                            Text("Finish setup")
                        }
                    }
                }
            } else {
                DetailRow("Bottle type", current.currentBottle?.bottleType?.label ?: "—")
                DetailRow("Tare weight", current.currentBottle?.tareWeightKg?.let { "%.2f kg".format(it) } ?: "—")
                DetailRow("Current weight", current.latestReading?.weightKg?.let { "%.2f kg".format(it) } ?: "—")
                DetailRow("Battery", current.latestReading?.batteryVoltage?.let { "%.2f V".format(it) } ?: "—")
                DetailRow("Temperature", current.latestReading?.temperatureC?.let { "%.1f °C".format(it) } ?: "—")
                DetailRow("Last updated", current.latestReading?.timestampMillis?.let { relativeTime(it) } ?: "—")
                DetailRow("Device ID", current.deviceId)

                Spacer(Modifier.height(16.dp))
                OutlinedButton(onClick = { onEditBottleConfig(deviceId) }, modifier = Modifier.fillMaxWidth()) {
                    Icon(Icons.Filled.Edit, contentDescription = null, modifier = Modifier.size(18.dp))
                    Spacer(Modifier.width(8.dp))
                    Text("Edit tare weight / bottle type")
                }
            }
        }
    }
}

@Composable
private fun DetailRow(label: String, value: String) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 10.dp),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(label, color = MaterialTheme.colorScheme.onSurfaceVariant)
            Text(value, fontWeight = FontWeight.Medium)
        }
        HorizontalDivider()
    }
}

private fun relativeTime(timestampMillis: Long): String {
    val diffMillis = (System.currentTimeMillis() - timestampMillis).coerceAtLeast(0)
    val seconds = TimeUnit.MILLISECONDS.toSeconds(diffMillis)
    return when {
        seconds < 60 -> "Just now"
        seconds < 3600 -> "${TimeUnit.SECONDS.toMinutes(seconds)} min ago"
        seconds < 86400 -> "${TimeUnit.SECONDS.toHours(seconds)} h ago"
        else -> "${TimeUnit.SECONDS.toDays(seconds)} d ago"
    }
}
