#include "temperature.h"
#include "adc.h"
#include "main.h"
#include "stm32u0xx_hal.h"
#include <stdio.h>

// Assumed analog supply voltage in mV, matching the fixed VDDA assumption
// used elsewhere for this ADC (see battery.c) rather than measuring VREFINT
#define ADC_VREF_MV 1800

float Temperature_Read(void) {
  float temperature = 0.0f;

  // Explicitly drop the other channels sharing this ADC first, see comment
  // in Battery_ReadVoltage (battery.c).
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_RANK_NONE;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_18;
  sConfig.Rank = ADC_RANK_NONE;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  // HAL_ADC_ConfigChannel enables the temperature sensor and waits out its
  // stabilization time internally when passed ADC_CHANNEL_TEMPSENSOR.
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    printf("Temperature ADC channel config failed\r\n");
    return temperature;
  }

  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
    uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);
    temperature = (float)__HAL_ADC_CALC_TEMPERATURE(ADC_VREF_MV, adc_raw,
                                                     ADC_RESOLUTION_12B);
  } else {
    printf("Temperature ADC read failed\r\n");
  }

  HAL_ADC_Stop(&hadc1);

  return temperature;
}
