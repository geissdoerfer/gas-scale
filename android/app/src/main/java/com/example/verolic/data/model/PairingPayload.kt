package com.example.verolic.data.model

import android.net.Uri

/**
 * What's encoded in the QR code printed on a device.
 * Scheme proposed for the real backend too, see android/blueprint.md.
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

        fun encode(deviceId: String, secret: String): String =
            "$SCHEME://$HOST?device_id=$deviceId&secret=$secret"
    }
}
