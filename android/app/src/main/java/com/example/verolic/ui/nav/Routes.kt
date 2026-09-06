package com.example.verolic.ui.nav

sealed class Routes(val route: String) {
    data object Login : Routes("login")
    data object Register : Routes("register")
    data object Home : Routes("home")
    data object QrScan : Routes("qr_scan")

    data object BottleSetup : Routes("bottle_setup/{deviceId}") {
        const val ARG_DEVICE_ID = "deviceId"
        fun create(deviceId: String) = "bottle_setup/$deviceId"
    }

    data object DeviceDetail : Routes("device_detail/{deviceId}") {
        const val ARG_DEVICE_ID = "deviceId"
        fun create(deviceId: String) = "device_detail/$deviceId"
    }
}
