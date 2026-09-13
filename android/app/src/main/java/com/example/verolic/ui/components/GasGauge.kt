package com.example.verolic.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke

/** Circular fill gauge for the remaining-gas percentage. Null renders an empty ring + "--". */
@Composable
fun GasGauge(percentage: Int?, modifier: Modifier = Modifier) {
    val trackColor = MaterialTheme.colorScheme.surfaceVariant
    val fillColor = when {
        percentage == null -> MaterialTheme.colorScheme.outline
        percentage <= 15 -> MaterialTheme.colorScheme.error
        percentage <= 30 -> Color(0xFFF59E0B) // amber - matches web-api's low_battery-style warning tone
        else -> MaterialTheme.colorScheme.primary
    }

    Box(modifier = modifier, contentAlignment = Alignment.Center) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val strokeWidth = size.minDimension * 0.12f
            val arcSize = Size(size.width - strokeWidth, size.height - strokeWidth)
            val topLeft = Offset(strokeWidth / 2, strokeWidth / 2)

            drawArc(
                color = trackColor,
                startAngle = 135f,
                sweepAngle = 270f,
                useCenter = false,
                topLeft = topLeft,
                size = arcSize,
                style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
            )

            if (percentage != null) {
                drawArc(
                    color = fillColor,
                    startAngle = 135f,
                    sweepAngle = 270f * (percentage.coerceIn(0, 100) / 100f),
                    useCenter = false,
                    topLeft = topLeft,
                    size = arcSize,
                    style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
                )
            }
        }

        Text(
            text = percentage?.let { "$it%" } ?: "--",
            style = MaterialTheme.typography.displayMedium
        )
    }
}
