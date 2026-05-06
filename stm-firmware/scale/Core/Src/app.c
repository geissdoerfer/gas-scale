#include "adc.h"
#include "bridge.h"
#include "gpio.h"
#include "main.h"
#include "stm32u0xx_hal.h"
#include "usart.h"
#include <stdio.h>

void SystemClock_Config(void);

// Redirect printf to UART1 by implementing __io_putchar
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
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

  unsigned int counter = 0;
  while (1) {
    /* USER CODE END WHILE */
    HAL_Delay(500);
    HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
    // printf("Hello %u\r\n", counter++);
    /* USER CODE BEGIN 3 */
    // UART bridge runs in interrupts - main loop can be empty or do other tasks
  }
  /* USER CODE END 3 */
}
