/**
 ******************************************************************************
 * @file    hx711.c
 * @brief   HX711 24-bit ADC driver implementation
 * @note    Bit-banged communication protocol
 ******************************************************************************
 */

#include "hx711.h"
#include "main.h"
#include "stm32u0xx_hal.h"

// Private variables
static int32_t offset = 0;
static float scale = 1.0f;

// Microsecond delay helper (approximate)
static void delay_us(uint32_t us) {
  uint32_t cycles = us * (SystemCoreClock / 1000000U) / 4;
  for (uint32_t i = 0; i < cycles; i++) {
    __NOP();
  }
}

/**
 * @brief Initialize HX711
 */
void HX711_Init(void) {
  // Set SCK low and wait for chip to be ready
  HAL_GPIO_WritePin(Sck_GPIO_Port, Sck_Pin, GPIO_PIN_RESET);
  offset = 0;
  scale = 1.0f;
  HAL_Delay(1);
}

/**
 * @brief Check if HX711 is ready (DOUT is low)
 */
bool HX711_IsReady(void) {
  return (HAL_GPIO_ReadPin(Dout_GPIO_Port, Dout_Pin) == GPIO_PIN_RESET);
}

/**
 * @brief Read raw 24-bit value from HX711
 */
int32_t HX711_Read(HX711_Gain_t gain) {
  uint32_t data = 0;

  // Wait for HX711 to be ready (DOUT goes low)
  uint32_t timeout = HAL_GetTick() + 1000;
  while (!HX711_IsReady()) {
    if (HAL_GetTick() > timeout) {
      return 0; // Timeout
    }
  }

  // Read 24 bits (MSB first)
  for (uint8_t i = 0; i < 24; i++) {
    HAL_GPIO_WritePin(Sck_GPIO_Port, Sck_Pin, GPIO_PIN_SET);
    delay_us(1);
    data = (data << 1);
    if (HAL_GPIO_ReadPin(Dout_GPIO_Port, Dout_Pin)) {
      data |= 1;
    }
    HAL_GPIO_WritePin(Sck_GPIO_Port, Sck_Pin, GPIO_PIN_RESET);
    delay_us(1);
  }

  // Set gain for next reading (1-3 additional pulses)
  for (uint8_t i = 0; i < (uint8_t)gain; i++) {
    HAL_GPIO_WritePin(Sck_GPIO_Port, Sck_Pin, GPIO_PIN_SET);
    delay_us(1);
    HAL_GPIO_WritePin(Sck_GPIO_Port, Sck_Pin, GPIO_PIN_RESET);
    delay_us(1);
  }

  // Sign extend from 24-bit to 32-bit
  if (data & 0x800000) {
    data |= 0xFF000000;
  }

  return (int32_t)data;
}

/**
 * @brief Read and average multiple readings
 */
int32_t HX711_ReadAverage(HX711_Gain_t gain, uint8_t times) {
  int64_t sum = 0;

  if (times == 0) {
    times = 1;
  }

  for (uint8_t i = 0; i < times; i++) {
    sum += HX711_Read(gain);
  }

  return (int32_t)(sum / times);
}

/**
 * @brief Power down the HX711
 */
void HX711_PowerDown(void) {
  HAL_GPIO_WritePin(Sck_GPIO_Port, Sck_Pin, GPIO_PIN_SET);
  delay_us(70); // >60us for power down
}

/**
 * @brief Power up the HX711
 */
void HX711_PowerUp(void) {
  HAL_GPIO_WritePin(Sck_GPIO_Port, Sck_Pin, GPIO_PIN_RESET);
  delay_us(1);
}

/**
 * @brief Tare the scale
 */
void HX711_Tare(HX711_Gain_t gain, uint8_t times) {
  offset = HX711_ReadAverage(gain, times);
}

/**
 * @brief Get tare offset
 */
int32_t HX711_GetOffset(void) {
  return offset;
}

/**
 * @brief Set tare offset
 */
void HX711_SetOffset(int32_t value) {
  offset = value;
}

/**
 * @brief Set scale factor
 */
void HX711_SetScale(float value) {
  scale = value;
}

/**
 * @brief Get scale factor
 */
float HX711_GetScale(void) {
  return scale;
}

/**
 * @brief Get calibrated weight reading
 */
float HX711_GetUnits(HX711_Gain_t gain, uint8_t times) {
  int32_t raw = HX711_ReadAverage(gain, times);
  return (float)(raw - offset) / scale;
}
