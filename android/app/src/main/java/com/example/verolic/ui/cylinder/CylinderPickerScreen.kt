package com.example.verolic.ui.cylinder

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.verolic.data.model.CylinderType
import com.example.verolic.di.LocalDeviceRepository
import com.example.verolic.di.viewModelFactory

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun CylinderPickerScreen(
    onDone: () -> Unit,
    onUnpaired: () -> Unit,
    onBack: (() -> Unit)? = null
) {
    val deviceRepository = LocalDeviceRepository.current
    val viewModel: CylinderPickerViewModel =
        viewModel(factory = viewModelFactory { CylinderPickerViewModel(deviceRepository) })
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()

    LaunchedEffect(uiState) {
        when (uiState) {
            is CylinderPickerUiState.Saved -> onDone()
            is CylinderPickerUiState.Unpaired -> onUnpaired()
            else -> Unit
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Cylinder type") },
                navigationIcon = {
                    onBack?.let { back -> TextButton(onClick = back) { Text("Back") } }
                }
            )
        }
    ) { padding ->
        when (val state = uiState) {
            is CylinderPickerUiState.Loading -> {
                Box(
                    modifier = Modifier.fillMaxSize().padding(padding),
                    contentAlignment = Alignment.Center
                ) { CircularProgressIndicator() }
            }

            is CylinderPickerUiState.Ready -> {
                Column(modifier = Modifier.fillMaxSize().padding(padding).padding(24.dp)) {
                    Text(
                        "What size gas cylinder is on this device?",
                        style = MaterialTheme.typography.titleMedium,
                        modifier = Modifier.padding(bottom = 16.dp)
                    )
                    CylinderType.entries.forEach { type ->
                        CylinderOptionCard(
                            type = type,
                            selected = state.current == type,
                            enabled = !state.saving,
                            onClick = { viewModel.select(type) }
                        )
                        Spacer(Modifier.height(12.dp))
                    }
                    if (state.saving) {
                        Spacer(Modifier.height(8.dp))
                        LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
                    }
                    state.error?.let {
                        Spacer(Modifier.height(8.dp))
                        Text(it, color = MaterialTheme.colorScheme.error)
                    }
                }
            }

            else -> Unit // Saved/Unpaired - navigating away via LaunchedEffect above
        }
    }
}

@Composable
private fun CylinderOptionCard(
    type: CylinderType,
    selected: Boolean,
    enabled: Boolean,
    onClick: () -> Unit
) {
    OutlinedCard(
        onClick = onClick,
        enabled = enabled,
        border = BorderStroke(
            width = if (selected) 2.dp else 1.dp,
            color = if (selected) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.outline
        ),
        modifier = Modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier.fillMaxWidth().padding(16.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(type.label, style = MaterialTheme.typography.titleMedium)
            if (selected) {
                Text("Selected", color = MaterialTheme.colorScheme.primary)
            }
        }
    }
}
