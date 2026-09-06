package com.example.verolic.ui.nav

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.example.verolic.data.model.AuthState
import com.example.verolic.di.AppContainer
import com.example.verolic.ui.auth.LoginScreen
import com.example.verolic.ui.auth.RegisterScreen
import com.example.verolic.ui.devices.DeviceDetailScreen
import com.example.verolic.ui.devices.HomeScreen
import com.example.verolic.ui.pairing.BottleSetupScreen
import com.example.verolic.ui.pairing.QrScanScreen

@Composable
fun VerolicNavHost(container: AppContainer) {
    val navController = rememberNavController()
    val authState by container.authRepository.authState.collectAsStateWithLifecycle()

    // Session changes (login, register, logout) drive navigation from a single place instead
    // of every screen having to know where to send the user next.
    LaunchedEffect(authState) {
        when (authState) {
            is AuthState.LoggedIn -> navController.navigate(Routes.Home.route) {
                popUpTo(0) { inclusive = true }
                launchSingleTop = true
            }

            AuthState.LoggedOut -> navController.navigate(Routes.Login.route) {
                popUpTo(0) { inclusive = true }
                launchSingleTop = true
            }
        }
    }

    NavHost(navController = navController, startDestination = Routes.Login.route) {
        composable(Routes.Login.route) {
            LoginScreen(onNavigateToRegister = { navController.navigate(Routes.Register.route) })
        }
        composable(Routes.Register.route) {
            RegisterScreen(onNavigateBack = { navController.popBackStack() })
        }
        composable(Routes.Home.route) {
            HomeScreen(
                onAddDevice = { navController.navigate(Routes.QrScan.route) },
                onOpenDevice = { deviceId -> navController.navigate(Routes.DeviceDetail.create(deviceId)) }
            )
        }
        composable(Routes.QrScan.route) {
            QrScanScreen(
                onClaimed = { deviceId ->
                    navController.navigate(Routes.BottleSetup.create(deviceId)) {
                        popUpTo(Routes.Home.route)
                    }
                },
                onCancel = { navController.popBackStack() }
            )
        }
        composable(
            route = Routes.BottleSetup.route,
            arguments = listOf(navArgument(Routes.BottleSetup.ARG_DEVICE_ID) { type = NavType.StringType })
        ) { backStackEntry ->
            val deviceId = backStackEntry.arguments?.getString(Routes.BottleSetup.ARG_DEVICE_ID).orEmpty()
            BottleSetupScreen(
                deviceId = deviceId,
                onSaved = {
                    navController.navigate(Routes.Home.route) {
                        popUpTo(Routes.Home.route) { inclusive = true }
                    }
                }
            )
        }
        composable(
            route = Routes.DeviceDetail.route,
            arguments = listOf(navArgument(Routes.DeviceDetail.ARG_DEVICE_ID) { type = NavType.StringType })
        ) { backStackEntry ->
            val deviceId = backStackEntry.arguments?.getString(Routes.DeviceDetail.ARG_DEVICE_ID).orEmpty()
            DeviceDetailScreen(
                deviceId = deviceId,
                onBack = { navController.popBackStack() },
                onEditBottleConfig = { navController.navigate(Routes.BottleSetup.create(deviceId)) }
            )
        }
    }
}
