#include "adc.h"
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

  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_SET);

  // HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // Toggle LED every 500ms (1s period)
    HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);
    HAL_Delay(1000);
    printf("Hello there!\r\n");
  }
  /* USER CODE END 3 */
}
