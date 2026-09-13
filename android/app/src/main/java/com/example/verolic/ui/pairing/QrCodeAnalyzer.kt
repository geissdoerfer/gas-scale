package com.example.verolic.ui.pairing

import android.annotation.SuppressLint
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.common.InputImage

/**
 * Feeds camera frames to ML Kit's on-device barcode scanner and reports the first decoded
 * value. Stops reporting after a hit until [resume] is called, so a code held in frame
 * doesn't fire the callback repeatedly while a pairing request is in flight.
 */
class QrCodeAnalyzer(private val onQrCodeDetected: (String) -> Unit) : ImageAnalysis.Analyzer {

    private val scanner = BarcodeScanning.getClient()

    @Volatile
    private var isPaused = false

    fun pause() {
        isPaused = true
    }

    fun resume() {
        isPaused = false
    }

    @SuppressLint("UnsafeOptInUsageError")
    @ExperimentalGetImage
    override fun analyze(imageProxy: ImageProxy) {
        val mediaImage = imageProxy.image
        if (isPaused || mediaImage == null) {
            imageProxy.close()
            return
        }

        val image = InputImage.fromMediaImage(mediaImage, imageProxy.imageInfo.rotationDegrees)
        scanner.process(image)
            .addOnSuccessListener { barcodes ->
                if (!isPaused) {
                    barcodes.firstNotNullOfOrNull { it.rawValue }?.let { value ->
                        isPaused = true
                        onQrCodeDetected(value)
                    }
                }
            }
            .addOnCompleteListener {
                imageProxy.close()
            }
    }
}
