package com.example.verolic.ui.pairing

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.verolic.data.model.BottleType
import com.example.verolic.di.LocalAppContainer
import com.example.verolic.di.viewModelFactory

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun BottleSetupScreen(
    deviceId: String,
    onSaved: () -> Unit
) {
    val container = LocalAppContainer.current
    val viewModel: BottleSetupViewModel = viewModel(
        factory = viewModelFactory {
            BottleSetupViewModel(container.authRepository, container.deviceRepository, deviceId)
        }
    )
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()

    var name by rememberSaveable { mutableStateOf("") }
    var tareWeightText by rememberSaveable { mutableStateOf("") }
    var selectedType by remember { mutableStateOf<BottleType?>(null) }

    LaunchedEffect(uiState) {
        if (uiState is BottleSetupUiState.Saved) onSaved()
    }

    Scaffold(
        topBar = { TopAppBar(title = { Text("Set up your device") }) }
    ) { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(24.dp)
        ) {
            Text(
                "Device paired! Give it a name, then tell us which bottle is currently on it so we can work out how much gas is left.",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(Modifier.height(20.dp))

            Text("Device", style = MaterialTheme.typography.titleSmall, fontWeight = FontWeight.SemiBold)
            Spacer(Modifier.height(8.dp))
            OutlinedTextField(
                value = name,
                onValueChange = { name = it },
                label = { Text("Device name (e.g. Kitchen)") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth()
            )

            Spacer(Modifier.height(24.dp))
            Text("Bottle on this device", style = MaterialTheme.typography.titleSmall, fontWeight = FontWeight.SemiBold)
            Spacer(Modifier.height(8.dp))
            OutlinedTextField(
                value = tareWeightText,
                onValueChange = { tareWeightText = it },
                label = { Text("Tare (empty) weight, kg") },
                singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal),
                modifier = Modifier.fillMaxWidth()
            )
            Spacer(Modifier.height(12.dp))

            viewModel.bottleTypes.forEach { type ->
                BottleTypeRow(
                    type = type,
                    selected = type == selectedType,
                    onSelect = { selectedType = type }
                )
            }

            if (uiState is BottleSetupUiState.Error) {
                Spacer(Modifier.height(8.dp))
                Text(
                    (uiState as BottleSetupUiState.Error).message,
                    color = MaterialTheme.colorScheme.error,
                    style = MaterialTheme.typography.bodySmall
                )
            }

            Spacer(Modifier.height(24.dp))
            Button(
                onClick = { viewModel.save(name, tareWeightText, selectedType) },
                enabled = uiState !is BottleSetupUiState.Saving,
                modifier = Modifier.fillMaxWidth()
            ) {
                if (uiState is BottleSetupUiState.Saving) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(18.dp),
                        color = MaterialTheme.colorScheme.onPrimary,
                        strokeWidth = 2.dp
                    )
                } else {
                    Text("Save")
                }
            }
        }
    }
}

@Composable
private fun BottleTypeRow(type: BottleType, selected: Boolean, onSelect: () -> Unit) {
    Card(
        onClick = onSelect,
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        colors = CardDefaults.cardColors(
            containerColor = if (selected) {
                MaterialTheme.colorScheme.primaryContainer
            } else {
                MaterialTheme.colorScheme.surfaceVariant
            }
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .selectable(selected = selected, onClick = onSelect)
                .padding(horizontal = 12.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(type.label)
            RadioButton(selected = selected, onClick = onSelect)
        }
    }
}
