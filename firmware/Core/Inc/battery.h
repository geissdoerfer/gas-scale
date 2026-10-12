/**
 ******************************************************************************
 * @file    battery.h
 * @brief   Battery voltage sensing via ADC
 ******************************************************************************
 */

#ifndef __BATTERY_H__
#define __BATTERY_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read battery voltage using ADC
 * @note Enables battery sense circuit, reads ADC, then disables circuit
 * @retval Battery voltage in volts, or 0.0 if read failed
 */
float Battery_ReadVoltage(void);

/**
 * @brief Read bus voltage using ADC
 * @note Reads ADC1_IN18 (PB1); no sense-enable circuit to toggle
 * @retval Bus voltage in volts, or 0.0 if read failed
 */
float Battery_BusVoltage(void);

/**
 * @brief Block until the battery voltage reaches a usable level
 * @note Repeatedly enters STOP2 mode between voltage checks
 * @param min_voltage Minimum battery voltage required, in volts
 * @retval None
 */
void Battery_WaitUntilReady(float min_voltage);

#ifdef __cplusplus
}
#endif

#endif /* __BATTERY_H__ */
