package com.example.verolic

import android.app.Application
import com.example.verolic.di.AppContainer

class VerolicApplication : Application() {
    val container by lazy { AppContainer() }
}
