package com.example.verolic

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.CompositionLocalProvider
import com.example.verolic.di.LocalDeviceRepository
import com.example.verolic.ui.nav.VerolicNavHost
import com.example.verolic.ui.theme.VerolicTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        val deviceRepository = (application as VerolicApplication).deviceRepository

        setContent {
            VerolicTheme {
                CompositionLocalProvider(LocalDeviceRepository provides deviceRepository) {
                    VerolicNavHost(deviceRepository)
                }
            }
        }
    }
}
