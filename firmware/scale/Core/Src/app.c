#include "adc.h"
#include "at_commands.h"
#include "battery.h"
#include "bridge.h"
#include "gpio.h"
#include "hx711.h"
#include "main.h"
#include "mqtt.h"
#include "power.h"
#include "radio.h"
#include "rtc.h"
#include "stm32u0xx_hal.h"
#include "usart.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEBUG 1

void SystemClock_Config(void);

// Volatile flag to signal main loop that RTC wakeup occurred
volatile uint8_t rtc_wakeup_flag = 0;

/**
 * @brief RTC Wakeup Timer Event Callback
 * @param hrtc RTC handle pointer
 * @retval None
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
  rtc_wakeup_flag = 1;
}

// Redirect printf to UART1 by implementing __io_putchar
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

// Send AT and wait for OK, retrying a few times to wake up the module
static bool WakeupModule(void) {
  for (unsigned int i = 0; i < 3; i++) {
    if (AT_SendCommand("AT\r\n", "OK", 2000))
      return true;
    printf("Didn't receive OK from module (%u)\r\n", i);
  }
  return false;
}

// Format the sensor data into a JSON message
static void FormatMessage(char *buf, size_t buf_size, int value,
                          float battery_voltage) {
  snprintf(buf, buf_size, "{\"val\":%d,\"temp\":%.3f,\"bat\":%.3f,\"cnt\":%lu}",
           value, 23.5, battery_voltage, Radio_GetInitCount());
}

// Connect to the MQTT broker, retrying a few times on failure
static bool ConnectToBroker(void) {
  for (unsigned int i = 0; i < 3; i++) {
    if (MQTT_Connect())
      return true;
    printf("MQTT connect failed (%u)\r\n", i);
  }
  return false;
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_RTC_Init();

  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

  /* Allow time for a debug request */
  HAL_Delay(1000);

  /* Don't power up if battery voltage is low */
  Battery_WaitUntilReady(4.1f);

  // Initialize UART bridge
  UART_Bridge_Init();

  printf("Oh hello there!\r\n");
  // Initialize HX711
  HAL_GPIO_WritePin(CellsEnable_GPIO_Port, CellsEnable_Pin, GPIO_PIN_SET);
  HX711_Init();
  HX711_PowerDown();

  // Initialize radio module
  Radio_Init();
  MQTT_Init();

  while (1) {
    /* USER CODE END WHILE */

    // Put cellular module to sleep
    Radio_Sleep();

    // Turn off LED
    HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

    // Enter STOP2 mode until next RTC wakeup
    Power_EnterStop2Mode();

    // Check if RTC wakeup event occurred (every 60 seconds)
    if (rtc_wakeup_flag) {
      rtc_wakeup_flag = 0; // Clear the flag

      printf("RTC wakeup: Starting sensor read and MQTT publish\r\n");

      // Read battery voltage first thing after wakeup
      float battery_voltage = Battery_ReadVoltage();

      HX711_PowerUp();

      // Wake up the cellular module
      printf("Trying to wakeup module\r\n");

      if (!WakeupModule()) {
        HX711_PowerDown();
        Radio_Init();
        MQTT_Init();
        HAL_Delay(5000);
        continue;
      }

      // Turn on LED during sensor read and transmission
      HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);

      // Read sensor value
      int value = HX711_ReadAverage(HX711_GAIN_64, 32);
      HX711_PowerDown();

      if (!ConnectToBroker())
        continue;

      HAL_Delay(500);

      char message[128];
      FormatMessage(message, sizeof(message), value, battery_voltage);

      // Publish sensor data
      if (MQTT_Publish("sensors/c0ffee02/data", message)) {
        HAL_Delay(500);
        printf("Published weight: %d\r\n", value);
      } else {
        printf("MQTT publish failed\r\n");
      }

      // Disconnect from MQTT
      MQTT_Disconnect();
    }

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
