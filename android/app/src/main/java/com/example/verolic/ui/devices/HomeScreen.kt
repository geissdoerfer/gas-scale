package com.example.verolic.ui.devices

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.Logout
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.LocalGasStation
import androidx.compose.material.icons.filled.QrCodeScanner
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
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
import com.example.verolic.di.LocalAppContainer
import com.example.verolic.di.viewModelFactory
import com.example.verolic.ui.components.GasGauge
import com.example.verolic.ui.components.StatusBadge

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HomeScreen(
    onAddDevice: () -> Unit,
    onOpenDevice: (String) -> Unit
) {
    val container = LocalAppContainer.current
    val viewModel: DevicesViewModel = viewModel(
        factory = viewModelFactory { DevicesViewModel(container.authRepository, container.deviceRepository) }
    )
    val devices by viewModel.devices.collectAsStateWithLifecycle()
    val username by viewModel.username.collectAsStateWithLifecycle()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(username?.let { "Hi, $it" } ?: "Your devices") },
                actions = {
                    IconButton(onClick = { viewModel.logout() }) {
                        Icon(Icons.AutoMirrored.Filled.Logout, contentDescription = "Log out")
                    }
                }
            )
        },
        floatingActionButton = {
            ExtendedFloatingActionButton(
                onClick = onAddDevice,
                icon = { Icon(Icons.Filled.QrCodeScanner, contentDescription = null) },
                text = { Text("Add device") }
            )
        }
    ) { padding ->
        if (devices.isEmpty()) {
            EmptyState(modifier = Modifier.padding(padding), onAddDevice = onAddDevice)
        } else {
            LazyColumn(
                modifier = Modifier
                    .padding(padding)
                    .fillMaxSize(),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                items(devices, key = { it.deviceId }) { device ->
                    DeviceCard(device = device, onClick = { onOpenDevice(device.deviceId) })
                }
            }
        }
    }
}

@Composable
private fun EmptyState(modifier: Modifier = Modifier, onAddDevice: () -> Unit) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(32.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Icon(
            imageVector = Icons.Filled.LocalGasStation,
            contentDescription = null,
            tint = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.size(56.dp)
        )
        Spacer(Modifier.height(16.dp))
        Text(
            "No devices yet",
            style = MaterialTheme.typography.titleMedium,
            fontWeight = FontWeight.Bold
        )
        Spacer(Modifier.height(4.dp))
        Text(
            "Scan the QR code printed on your gas bottle monitor to add the device, then tell us which bottle is on it.",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            textAlign = androidx.compose.ui.text.style.TextAlign.Center
        )
        Spacer(Modifier.height(20.dp))
        androidx.compose.material3.Button(onClick = onAddDevice) {
            Icon(Icons.Filled.Add, contentDescription = null, modifier = Modifier.size(18.dp))
            Spacer(Modifier.width(8.dp))
            Text("Add your first device")
        }
    }
}

@Composable
private fun DeviceCard(device: Device, onClick: () -> Unit) {
    Card(
        onClick = onClick,
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 1.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            GasGauge(
                percentage = device.percentageRemaining,
                status = device.status,
                diameter = 72.dp
            )
            Spacer(Modifier.width(16.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(device.name, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold)
                Spacer(Modifier.height(4.dp))
                Text(
                    device.currentBottle?.bottleType?.label ?: device.deviceId,
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Spacer(Modifier.height(8.dp))
                StatusBadge(status = device.status)
            }
        }
    }
}
