package com.example.verolic.ui.main

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.verolic.data.model.DeviceHealth
import com.example.verolic.di.LocalDeviceRepository
import com.example.verolic.di.viewModelFactory
import com.example.verolic.ui.components.BatteryIndicator
import com.example.verolic.ui.components.GasGauge
import com.example.verolic.ui.components.StatusBadge

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    onChangeCylinderType: () -> Unit,
    onUnpaired: () -> Unit
) {
    val deviceRepository = LocalDeviceRepository.current
    val viewModel: MainViewModel = viewModel(factory = viewModelFactory { MainViewModel(deviceRepository) })
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()
    var menuExpanded by remember { mutableStateOf(false) }
    var showUnpairConfirm by remember { mutableStateOf(false) }

    LaunchedEffect(uiState) {
        if (uiState is MainUiState.Unpaired) onUnpaired()
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Gas bottle") },
                actions = {
                    Box {
                        TextButton(onClick = { menuExpanded = true }) { Text("Menu") }
                        DropdownMenu(expanded = menuExpanded, onDismissRequest = { menuExpanded = false }) {
                            DropdownMenuItem(
                                text = { Text("Change cylinder type") },
                                onClick = {
                                    menuExpanded = false
                                    onChangeCylinderType()
                                }
                            )
                            DropdownMenuItem(
                                text = { Text("Unpair") },
                                onClick = {
                                    menuExpanded = false
                                    showUnpairConfirm = true
                                }
                            )
                        }
                    }
                }
            )
        }
    ) { padding ->
        Box(
            modifier = Modifier.fillMaxSize().padding(padding),
            contentAlignment = Alignment.Center
        ) {
            when (val state = uiState) {
                is MainUiState.Loading -> CircularProgressIndicator()

                is MainUiState.Error -> Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(state.message, color = MaterialTheme.colorScheme.error, textAlign = TextAlign.Center)
                    Spacer(Modifier.height(12.dp))
                    Button(onClick = { viewModel.refresh() }) { Text("Retry") }
                }

                is MainUiState.Loaded -> MainContent(state, onChangeCylinderType)

                is MainUiState.Unpaired -> Unit // navigating away via LaunchedEffect above
            }
        }
    }

    if (showUnpairConfirm) {
        AlertDialog(
            onDismissRequest = { showUnpairConfirm = false },
            title = { Text("Unpair this device?") },
            text = { Text("You'll need to scan its QR code (or enter its ID and secret) again to reconnect.") },
            confirmButton = {
                TextButton(onClick = {
                    showUnpairConfirm = false
                    viewModel.unpair()
                }) { Text("Unpair") }
            },
            dismissButton = {
                TextButton(onClick = { showUnpairConfirm = false }) { Text("Cancel") }
            }
        )
    }
}

@Composable
private fun MainContent(state: MainUiState.Loaded, onChangeCylinderType: () -> Unit) {
    val status = state.status
    Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.padding(24.dp)) {
        when (status.health) {
            DeviceHealth.NOT_CONFIGURED -> {
                Text(
                    "Set up your cylinder type to see the gas level.",
                    textAlign = TextAlign.Center
                )
                Spacer(Modifier.height(16.dp))
                Button(onClick = onChangeCylinderType) { Text("Choose cylinder type") }
            }

            DeviceHealth.NO_DATA -> {
                GasGauge(percentage = null, modifier = Modifier.size(220.dp))
                Spacer(Modifier.height(16.dp))
                Text("Waiting for the first reading from your device...", textAlign = TextAlign.Center)
            }

            else -> {
                GasGauge(percentage = status.percentageRemaining, modifier = Modifier.size(220.dp))
                Spacer(Modifier.height(24.dp))
                BatteryIndicator(percentage = status.batteryPercentage)
                Spacer(Modifier.height(12.dp))
                StatusBadge(health = status.health)
            }
        }

        if (state.refreshing) {
            Spacer(Modifier.height(16.dp))
            LinearProgressIndicator(modifier = Modifier.width(120.dp))
        }
    }
}
