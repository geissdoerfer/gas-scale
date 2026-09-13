package com.example.verolic.data.model

/** Mirrors schemas.CylinderType in web-api - keep the apiValue set in sync with the server. */
enum class CylinderType(val apiValue: String, val label: String) {
    KG_3("3kg", "3 kg"),
    KG_6("6kg", "6 kg"),
    KG_12("12kg", "12 kg"),
    KG_45("45kg", "45 kg");

    companion object {
        fun fromApiValue(value: String?): CylinderType? = entries.find { it.apiValue == value }
    }
}
