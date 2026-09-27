#include "adc.h"
#include "at_commands.h"
#include "battery.h"
#include "bridge.h"
#include "cmsis_os2.h"
#include "device_config.h"
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

const char dev_name[] = "eagleXX";

void SystemClock_Config(void);

// Volatile flag to signal main loop that RTC wakeup occurred
volatile uint8_t rtc_wakeup_flag = 0;

// Volatile flag set when a rising edge is seen on ChargerDetect_Pin
static volatile uint8_t charger_connect_flag = 0;

/**
 * @brief RTC Wakeup Timer Event Callback
 * @param hrtc RTC handle pointer
 * @retval None
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
  rtc_wakeup_flag = 1;
}

/**
 * @brief GPIO EXTI Rising Edge Callback
 * @param GPIO_Pin Pin that triggered the interrupt
 * @retval None
 */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == ChargerDetect_Pin)
    charger_connect_flag = 1;
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

// Block until a rising edge is seen on ChargerDetect_Pin (charger
// connected), asleep in STOP2 mode until the EXTI interrupt wakes it.
// The RTC wakeup timer must not be running yet, since it would otherwise
// also wake the MCU periodically with nothing to check for.
static void WaitForChargerFirstConnect(void) {
  while (!charger_connect_flag) {
    Power_EnterStop2Mode();
  }
  charger_connect_flag = 0;
}

// FreeRTOS bring-up: blinks the LED forever, independent of the existing
// bare-metal loop in main(). Not started automatically; call
// StartLedBlinkDemo() yourself (e.g. in place of the current int main(void)
// body) to verify the RTOS port before migrating the rest of the
// application into tasks.
static void LedBlinkTask(void *argument) {
  (void)argument;
  for (;;) {
    HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
    osDelay(500);
  }
}

void StartLedBlinkDemo(void) {
  osKernelInitialize();

  const osThreadAttr_t led_blink_task_attributes = {
      .name = "LedBlinkTask",
      .priority = (osPriority_t)osPriorityNormal,
      .stack_size = 128 * 4,
  };
  osThreadNew(LedBlinkTask, NULL, &led_blink_task_attributes);

  osKernelStart();
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

  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

  /* Allow time for a debug request */
  HAL_Delay(1000);
  StartLedBlinkDemo();

  // On first start, wait for the charger to be connected once before
  // proceeding, then permanently clear the first-start flag. RTC init is
  // deferred until after this so STOP2 mode isn't also woken periodically
  // by the RTC wakeup timer while we're only waiting for the charger EXTI.
  if (DeviceConfig_IsFirstStart()) {
    printf("First start: waiting for charger connection\r\n");
    WaitForChargerFirstConnect();
    DeviceConfig_ClearFirstStart();
    printf("Charger detected, first start complete\r\n");
  }

  MX_RTC_Init();

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
      char topic[64];
      snprintf(topic, sizeof(topic), "sensors/%s/data", dev_name);
      if (MQTT_Publish(topic, message)) {
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
