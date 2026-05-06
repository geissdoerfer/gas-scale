#include "adc.h"
#include "bridge.h"
#include "gpio.h"
#include "main.h"
#include "stm32u0xx_hal.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void SystemClock_Config(void);

// Redirect printf to UART1 by implementing __io_putchar
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

int send2module(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  int len = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  if (len > 0) {
    HAL_UART_Transmit(&huart3, (uint8_t *)buffer, len, HAL_MAX_DELAY);
  }

  return len;
}
/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();

  // Initialize UART bridge
  UART_Bridge_Init();

  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_SET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_RESET);
  HAL_Delay(10000);

  send2module("AT\r\n");
  HAL_Delay(2000);

  send2module("AT\r\n");
  HAL_Delay(2000);
  send2module("AT+CPIN=6436\r\n");
  HAL_Delay(10000);
  send2module("AT+CPSI?\r\n");

  unsigned int counter = 0;
  uint32_t last_led_toggle = 0;
  while (1) {
    /* USER CODE END WHILE */

    // Process UART bridge continuously (high priority)
    UART_Bridge_Process();

    // Toggle LED every 500ms without blocking
    if (HAL_GetTick() - last_led_toggle >= 500) {
      HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
      last_led_toggle = HAL_GetTick();
      // printf("Hello %u\r\n", counter++);
    }
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
