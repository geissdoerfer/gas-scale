package com.example.verolic.ui.pairing

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.verolic.data.model.PairingPayload
import com.example.verolic.di.LocalDeviceRepository
import com.example.verolic.di.viewModelFactory

@Composable
fun PairingScreen(onPaired: () -> Unit) {
    val deviceRepository = LocalDeviceRepository.current
    val viewModel: PairingViewModel =
        viewModel(factory = viewModelFactory { PairingViewModel(deviceRepository) })
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()

    var manualEntryMode by rememberSaveable { mutableStateOf(false) }
    var deviceIdInput by rememberSaveable { mutableStateOf("") }
    var secretInput by rememberSaveable { mutableStateOf("") }

    val context = LocalContext.current
    var hasCameraPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA)
                == PackageManager.PERMISSION_GRANTED
        )
    }
    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted -> hasCameraPermission = granted }

    LaunchedEffect(uiState) {
        if (uiState is PairingUiState.Success) onPaired()
    }

    LaunchedEffect(Unit) {
        if (!hasCameraPermission && !manualEntryMode) {
            permissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    Scaffold { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(24.dp)
                .verticalScroll(rememberScrollState()),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Spacer(Modifier.height(16.dp))
            Text("Pair your device", style = MaterialTheme.typography.headlineSmall)
            Text(
                "Scan the QR code on your device, or enter its ID and pairing secret manually.",
                style = MaterialTheme.typography.bodyMedium,
                textAlign = TextAlign.Center,
                modifier = Modifier.padding(top = 8.dp, bottom = 24.dp)
            )

            if (!manualEntryMode) {
                if (hasCameraPermission) {
                    QrScannerView(
                        modifier = Modifier
                            .fillMaxWidth()
                            .aspectRatio(1f)
                            .clip(RoundedCornerShape(16.dp)),
                        paused = uiState is PairingUiState.Pairing,
                        onQrCodeDetected = { raw ->
                            PairingPayload.parse(raw)?.let { viewModel.pair(it) }
                        }
                    )
                } else {
                    Text(
                        "Camera permission is needed to scan the QR code.",
                        textAlign = TextAlign.Center,
                        modifier = Modifier.padding(vertical = 24.dp)
                    )
                    Button(onClick = { permissionLauncher.launch(Manifest.permission.CAMERA) }) {
                        Text("Grant camera permission")
                    }
                }

                Spacer(Modifier.height(16.dp))
                TextButton(onClick = { manualEntryMode = true }) {
                    Text("Enter device ID and secret manually")
                }
            } else {
                OutlinedTextField(
                    value = deviceIdInput,
                    onValueChange = { deviceIdInput = it },
                    label = { Text("Device ID") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
                Spacer(Modifier.height(12.dp))
                OutlinedTextField(
                    value = secretInput,
                    onValueChange = { secretInput = it },
                    label = { Text("Pairing secret") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
                Spacer(Modifier.height(16.dp))
                Button(
                    onClick = {
                        viewModel.pair(PairingPayload(deviceIdInput.trim(), secretInput.trim()))
                    },
                    enabled = deviceIdInput.isNotBlank() && secretInput.isNotBlank() &&
                        uiState !is PairingUiState.Pairing,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Pair")
                }
                Spacer(Modifier.height(8.dp))
                TextButton(onClick = { manualEntryMode = false }) {
                    Text("Scan QR code instead")
                }
            }

            if (uiState is PairingUiState.Pairing) {
                Spacer(Modifier.height(16.dp))
                CircularProgressIndicator()
            }

            (uiState as? PairingUiState.Error)?.let { errorState ->
                Spacer(Modifier.height(16.dp))
                Text(
                    errorState.message,
                    color = MaterialTheme.colorScheme.error,
                    textAlign = TextAlign.Center
                )
            }
        }
    }
}

@Composable
private fun QrScannerView(
    modifier: Modifier = Modifier,
    paused: Boolean,
    onQrCodeDetected: (String) -> Unit
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val analyzer = remember { QrCodeAnalyzer(onQrCodeDetected) }
    val previewView = remember { PreviewView(context) }

    LaunchedEffect(paused) {
        if (paused) analyzer.pause() else analyzer.resume()
    }

    DisposableEffect(lifecycleOwner) {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(context)
        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()
            val preview = Preview.Builder().build().also {
                it.surfaceProvider = previewView.surfaceProvider
            }
            val imageAnalysis = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()
                .also { it.setAnalyzer(ContextCompat.getMainExecutor(context), analyzer) }

            runCatching {
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(
                    lifecycleOwner,
                    CameraSelector.DEFAULT_BACK_CAMERA,
                    preview,
                    imageAnalysis
                )
            }
        }, ContextCompat.getMainExecutor(context))

        onDispose {
            runCatching { cameraProviderFuture.get().unbindAll() }
        }
    }

    AndroidView(modifier = modifier, factory = { previewView })
}
