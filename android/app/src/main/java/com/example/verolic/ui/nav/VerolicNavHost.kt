package com.example.verolic.ui.nav

import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.navigation.NavController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.verolic.data.local.PairState
import com.example.verolic.data.repository.DeviceRepository
import com.example.verolic.ui.cylinder.CylinderPickerScreen
import com.example.verolic.ui.main.MainScreen
import com.example.verolic.ui.pairing.PairingScreen

@Composable
fun VerolicNavHost(deviceRepository: DeviceRepository) {
    val navController = rememberNavController()

    // Read once, at cold start, rather than observed reactively: NavHost rebuilds its graph
    // (resetting the back stack) whenever this argument changes across recomposition, which
    // raced with PairingScreen's own explicit navigate() call the moment pairState flipped
    // mid-pair() - see PairState/DeviceRepository. All state transitions after this initial
    // read are driven by explicit navigate() calls from onPaired/onUnpaired callbacks below.
    val startDestination = remember {
        if (deviceRepository.pairState.value is PairState.Paired) Routes.MAIN else Routes.PAIRING
    }

    NavHost(navController = navController, startDestination = startDestination) {
        composable(Routes.PAIRING) {
            PairingScreen(
                onPaired = {
                    navController.navigate(Routes.CYLINDER_PICKER) {
                        popUpTo(Routes.PAIRING) { inclusive = true }
                    }
                }
            )
        }
        composable(Routes.CYLINDER_PICKER) {
            // This screen is reached two ways: right after pairing (no MAIN on the back
            // stack yet), or from MAIN's "change cylinder type" menu (MAIN already there).
            val cameFromMain = navController.previousBackStackEntry?.destination?.route == Routes.MAIN

            CylinderPickerScreen(
                onDone = {
                    if (cameFromMain) {
                        navController.popBackStack()
                    } else {
                        navController.navigate(Routes.MAIN) {
                            popUpTo(Routes.PAIRING) { inclusive = true }
                        }
                    }
                },
                onUnpaired = { navController.navigateToPairingFresh() },
                onBack = if (cameFromMain) ({ navController.popBackStack() }) else null
            )
        }
        composable(Routes.MAIN) {
            MainScreen(
                onChangeCylinderType = { navController.navigate(Routes.CYLINDER_PICKER) },
                onUnpaired = { navController.navigateToPairingFresh() }
            )
        }
    }
}

private fun NavController.navigateToPairingFresh() {
    navigate(Routes.PAIRING) {
        popUpTo(0) { inclusive = true }
    }
}
