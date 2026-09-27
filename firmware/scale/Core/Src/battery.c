#include "battery.h"
#include "adc.h"
#include "main.h"
#include "power.h"
#include "stm32u0xx_hal.h"
#include <stdio.h>

float Battery_ReadVoltage(void) {
  float battery_voltage = 0.0f;

  // Enable battery sense circuit
  HAL_GPIO_WritePin(BatSenseEnable_GPIO_Port, BatSenseEnable_Pin, GPIO_PIN_SET);
  HAL_Delay(25); // Allow circuit to settle

  // Perform ADC conversion on PA0 (ADC_CHANNEL_4)
  // The sequencer is a channel bitmask (needed for CHANNEL_18, see
  // Battery_BusVoltage), so explicitly drop CHANNEL_18 to keep only one
  // channel active at a time.
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_18;
  sConfig.Rank = ADC_RANK_NONE;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    printf("Battery voltage ADC channel config failed\r\n");
    HAL_GPIO_WritePin(BatSenseEnable_GPIO_Port, BatSenseEnable_Pin,
                      GPIO_PIN_RESET);
    return battery_voltage;
  }

  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
    uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);

    // Convert ADC value to actual battery voltage
    // ADC is 12-bit (0-4095), Vref = 1.8V (VDDA)
    // Voltage divider: 20k upper, 10k lower -> Vbat = Vadc * (20k+10k)/10k
    // = Vadc * 3
    float vadc = (adc_raw / 4095.0f) * 1.8f;
    battery_voltage = vadc * 3.0f; // Apply voltage divider scaling

  } else {
    printf("Battery voltage ADC read failed\r\n");
  }

  // Disable battery sense circuit
  HAL_GPIO_WritePin(BatSenseEnable_GPIO_Port, BatSenseEnable_Pin,
                    GPIO_PIN_RESET);

  HAL_ADC_Stop(&hadc1);

  return battery_voltage;
}

float Battery_BusVoltage(void) {
  float bus_voltage = 0.0f;

  // Perform ADC conversion on PB1 (ADC_CHANNEL_18)
  // Explicitly drop CHANNEL_4 first, see comment in Battery_ReadVoltage.
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_RANK_NONE;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_18;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    printf("Bus voltage ADC channel config failed\r\n");
    return bus_voltage;
  }

  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
    uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);

    // Convert ADC value to actual bus voltage
    // ADC is 12-bit (0-4095), Vref = 1.8V (VDDA)
    // Voltage divider: 20k upper, 10k lower -> Vbus = Vadc * (20k+10k)/10k
    // = Vadc * 3
    float vadc = (adc_raw / 4095.0f) * 1.8f;
    bus_voltage = vadc * 3.0f; // Apply voltage divider scaling

  } else {
    printf("Bus voltage ADC read failed\r\n");
  }

  HAL_ADC_Stop(&hadc1);

  return bus_voltage;
}

void Battery_WaitUntilReady(float min_voltage) {
  float v_bat;
  while ((v_bat = Battery_ReadVoltage()) < min_voltage) {
    printf("Battery voltage low: %.2f\r\n", v_bat);
    Power_EnterStop2Mode();
  }
}
