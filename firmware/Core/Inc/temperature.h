/**
 ******************************************************************************
 * @file    temperature.h
 * @brief   MCU internal temperature sensing via ADC
 ******************************************************************************
 */

#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read the MCU's internal temperature sensor
 * @retval Temperature in degrees Celsius, or 0.0 if the read failed
 */
float Temperature_Read(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEMPERATURE_H__ */
