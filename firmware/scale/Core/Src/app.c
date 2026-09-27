#include "FreeRTOS.h"
#include "adc.h"
#include "battery.h"
#include "bridge.h"
#include "cmsis_os2.h"
#include "device_config.h"
#include "gpio.h"
#include "iwdg.h"
#include "main.h"
#include "power.h"
#include "sample_task.h"
#include "send_task.h"
#include "stm32u0xx_hal.h"
#include "task.h"
#include "usart.h"
#include "watchdog_task.h"
#include <stdbool.h>
#include <stdio.h>

#define DEBUG 1

// Device identity; also used by send_task.c to build the MQTT topic
const char dev_name[] = "eagleXX";

void SystemClock_Config(void);

// Volatile flag set when a rising edge is seen on ChargerDetect_Pin
static volatile uint8_t charger_connect_flag = 0;

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

  printf("Oh hello there!\r\n");

  /* Allow time for a debug request */
  HAL_Delay(1000);

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

  // Initialize UART bridge
  UART_Bridge_Init();

  /* Don't power up if battery voltage is low */
  // Battery_WaitUntilReady(4.1f);

  osKernelInitialize();
  printf("Kernel initialized\r\n");
  StartSampleTask();
  StartSendTask();

  // IWDG starts counting down the instant it's initialized and can never
  // be stopped again, so this must come last - after every bare-metal gate
  // above that could otherwise block long enough to trip it unrefreshed.
  MX_IWDG_Init();
  StartWatchdogTask();

  osKernelStart();

  // Unreachable: osKernelStart() only returns on failure
  while (1) {
  }
}
