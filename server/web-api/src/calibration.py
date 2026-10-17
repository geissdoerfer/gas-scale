"""
Single source of truth for the raw-value -> weight conversion.

Every place a SensorReading's weight is needed - device detail, readings history, the
dashboard, and (via cylinder_specs.gas_percentage_remaining) the Android app's gas
percentage - goes through calculate_weight()/reading_to_dict() so the formula lives in
exactly one place instead of being re-typed at each call site.
"""
from typing import Optional

from src import models


def calculate_weight(raw_value: Optional[float], offset: float, gain: float) -> Optional[float]:
    """weight = (raw_value + offset) * gain. None if there's no raw reading yet."""
    if raw_value is None:
        return None
    return (raw_value + offset) * gain


def reading_to_dict(reading: models.SensorReading, offset: float, gain: float) -> dict:
    """The calibrated-reading shape used by schemas.SensorReadingResponse everywhere it's built."""
    return {
        "time": reading.time,
        "device_id": reading.device_id,
        "raw_value": reading.raw_value,
        "weight": calculate_weight(reading.raw_value, offset, gain),
        "battery_voltage": reading.battery_voltage,
        "temperature": reading.temperature,
    }
