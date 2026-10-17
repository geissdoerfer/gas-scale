"""
Cylinder capacity reference table and percentage calculations for the Android app's
account-less pairing flow (see android/app_plan.md).

The tare/full weights below are PLACEHOLDER estimates for common LPG cylinder sizes and
have NOT been verified against real hardware or a specific manufacturer's cylinders.
Confirm real values before trusting percentage_remaining in production.
"""
from typing import NamedTuple, Optional


class CylinderSpec(NamedTuple):
    tare_kg: float  # empty cylinder weight
    full_kg: float  # gross weight (tare + nominal gas fill) when full


# Keyed by schemas.CylinderType value.
CYLINDER_SPECS: dict[str, CylinderSpec] = {
    "3kg": CylinderSpec(tare_kg=5.5, full_kg=8.5),
    "6kg": CylinderSpec(tare_kg=8.0, full_kg=14.0),
    "12kg": CylinderSpec(tare_kg=14.0, full_kg=26.0),
    "45kg": CylinderSpec(tare_kg=25.0, full_kg=70.0),
}

# Placeholder linear voltage -> charge mapping; replace with a real discharge curve for
# the device's battery chemistry once available.
BATTERY_VOLTAGE_EMPTY = 10.5
BATTERY_VOLTAGE_FULL = 12.6

# Below this, a device's status is reported as "low_battery" (devices.py, readings.py).
LOW_BATTERY_VOLTAGE = 11.5


def gas_percentage_remaining(weight_kg: float, cylinder_type: Optional[str]) -> Optional[int]:
    """Percentage of usable gas remaining, clamped to [0, 100]. None if type is unknown."""
    spec = CYLINDER_SPECS.get(cylinder_type) if cylinder_type else None
    if spec is None:
        return None

    span = spec.full_kg - spec.tare_kg
    if span <= 0:
        return None

    pct = (weight_kg - spec.tare_kg) / span * 100.0
    return round(max(0.0, min(100.0, pct)))


def battery_percentage(voltage: float) -> int:
    """Battery charge percentage estimated from voltage, clamped to [0, 100]."""
    span = BATTERY_VOLTAGE_FULL - BATTERY_VOLTAGE_EMPTY
    pct = (voltage - BATTERY_VOLTAGE_EMPTY) / span * 100.0
    return round(max(0.0, min(100.0, pct)))
