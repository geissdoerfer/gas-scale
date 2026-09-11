#include "battery.h"
#include "adc.h"
#include "main.h"
#include "power.h"
#include "rtc.h"
#include "stm32u0xx_hal.h"
#include <stdio.h>

float Battery_ReadVoltage(void) {
  float battery_voltage = 0.0f;

  // Enable battery sense circuit
  HAL_GPIO_WritePin(BatSenseEnable_GPIO_Port, BatSenseEnable_Pin, GPIO_PIN_SET);
  HAL_Delay(25); // Allow circuit to settle

  // Perform ADC conversion on PA0 (ADC_CHANNEL_4)
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
    uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);

    // Convert ADC value to actual battery voltage
    // ADC is 12-bit (0-4095), Vref = 1.8V (VDDA)
    // Voltage divider: 20k upper, 10k lower -> Vbat = Vadc * (20k+10k)/10k
    // = Vadc * 3
    float vadc = (adc_raw / 4095.0f) * 1.8f;
    battery_voltage = vadc * 3.0f; // Apply voltage divider scaling

    printf("Battery ADC: %lu, Voltage: %.3fV\r\n", adc_raw, battery_voltage);
  } else {
    printf("Battery voltage ADC read failed\r\n");
  }

  // Disable battery sense circuit
  HAL_GPIO_WritePin(BatSenseEnable_GPIO_Port, BatSenseEnable_Pin,
                    GPIO_PIN_RESET);

  HAL_ADC_Stop(&hadc1);

  return battery_voltage;
}

void Battery_WaitUntilReady(float min_voltage) {
  float v_bat;
  while ((v_bat = Battery_ReadVoltage()) < min_voltage) {
    printf("Battery voltage low: %.2f\r\n", v_bat);
    if (rtc_wakeup_flag)
      rtc_wakeup_flag = 0; // Clear the software flag
    Power_EnterStop2Mode();
  }
}
