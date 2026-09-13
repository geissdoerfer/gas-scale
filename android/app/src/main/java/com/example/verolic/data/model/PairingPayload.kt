package com.example.verolic.data.model

import android.net.Uri

/**
 * What's encoded in the QR code printed on a device's label (see register_device.py and
 * web-api's POST /devices/pair). Also what a manual-entry form produces directly.
 */
data class PairingPayload(val deviceId: String, val secret: String) {
    companion object {
        private const val SCHEME = "verolic"
        private const val HOST = "pair"

        fun parse(raw: String): PairingPayload? {
            val uri = runCatching { Uri.parse(raw) }.getOrNull() ?: return null
            if (uri.scheme != SCHEME || uri.host != HOST) return null
            val deviceId = uri.getQueryParameter("device_id")?.takeIf { it.isNotBlank() } ?: return null
            val secret = uri.getQueryParameter("secret")?.takeIf { it.isNotBlank() } ?: return null
            return PairingPayload(deviceId, secret)
        }
    }
}
