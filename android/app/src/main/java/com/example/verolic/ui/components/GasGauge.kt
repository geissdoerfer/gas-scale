package com.example.verolic.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.size
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.example.verolic.data.model.DeviceStatus

/** A 270-degree arc gauge showing % gas remaining, color-coded by [DeviceStatus]. */
@Composable
fun GasGauge(
    percentage: Int?,
    status: DeviceStatus,
    modifier: Modifier = Modifier,
    diameter: Dp = 160.dp
) {
    val valueColor = status.color()
    val trackColor = MaterialTheme.colorScheme.surfaceVariant

    Box(modifier = modifier.size(diameter), contentAlignment = Alignment.Center) {
        Canvas(modifier = Modifier.size(diameter)) {
            val strokeWidth = size.minDimension * 0.12f
            val arcSize = Size(size.width - strokeWidth, size.height - strokeWidth)
            val topLeft = Offset(strokeWidth / 2f, strokeWidth / 2f)

            drawArc(
                color = trackColor,
                startAngle = 135f,
                sweepAngle = 270f,
                useCenter = false,
                topLeft = topLeft,
                size = arcSize,
                style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
            )

            val sweep = 270f * ((percentage ?: 0).coerceIn(0, 100) / 100f)
            if (sweep > 0f) {
                drawArc(
                    color = valueColor,
                    startAngle = 135f,
                    sweepAngle = sweep,
                    useCenter = false,
                    topLeft = topLeft,
                    size = arcSize,
                    style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
                )
            }
        }
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                text = percentage?.let { "$it%" } ?: "—",
                style = if (diameter > 100.dp) MaterialTheme.typography.headlineMedium else MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.Bold,
                color = valueColor
            )
            if (diameter > 100.dp) {
                Text(
                    text = status.label(),
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}
