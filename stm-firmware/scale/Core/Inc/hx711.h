/**
 ******************************************************************************
 * @file    hx711.h
 * @brief   HX711 24-bit ADC driver for load cell applications
 ******************************************************************************
 */

#ifndef __HX711_H__
#define __HX711_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief HX711 gain settings
 */
typedef enum {
  HX711_GAIN_128 = 1,  // Channel A, gain 128
  HX711_GAIN_32  = 2,  // Channel B, gain 32
  HX711_GAIN_64  = 3   // Channel A, gain 64
} HX711_Gain_t;

/**
 * @brief Initialize HX711
 */
void HX711_Init(void);

/**
 * @brief Check if HX711 is ready (DOUT is low)
 * @return true if ready, false otherwise
 */
bool HX711_IsReady(void);

/**
 * @brief Read raw 24-bit value from HX711
 * @param gain Gain setting to use
 * @return 24-bit signed value (sign-extended to 32-bit)
 */
int32_t HX711_Read(HX711_Gain_t gain);

/**
 * @brief Read raw value and average multiple readings
 * @param gain Gain setting to use
 * @param times Number of readings to average
 * @return Averaged value
 */
int32_t HX711_ReadAverage(HX711_Gain_t gain, uint8_t times);

/**
 * @brief Power down the HX711 (SCK high for >60us)
 */
void HX711_PowerDown(void);

/**
 * @brief Power up the HX711 (SCK low)
 */
void HX711_PowerUp(void);

/**
 * @brief Tare the scale (set zero point)
 * @param gain Gain setting to use
 * @param times Number of readings to average for tare
 */
void HX711_Tare(HX711_Gain_t gain, uint8_t times);

/**
 * @brief Get the tare offset value
 * @return Tare offset
 */
int32_t HX711_GetOffset(void);

/**
 * @brief Set the tare offset value manually
 * @param offset Offset value
 */
void HX711_SetOffset(int32_t offset);

/**
 * @brief Set calibration scale factor
 * @param scale Scale factor (raw_value / scale = weight)
 */
void HX711_SetScale(float scale);

/**
 * @brief Get calibration scale factor
 * @return Scale factor
 */
float HX711_GetScale(void);

/**
 * @brief Get weight reading (tared and scaled)
 * @param gain Gain setting to use
 * @param times Number of readings to average
 * @return Weight value
 */
float HX711_GetUnits(HX711_Gain_t gain, uint8_t times);

#ifdef __cplusplus
}
#endif

#endif /* __HX711_H__ */
