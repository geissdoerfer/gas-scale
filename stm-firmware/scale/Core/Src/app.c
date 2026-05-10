#include "adc.h"
#include "at_commands.h"
#include "bridge.h"
#include "cert_upload.h"
#include "gpio.h"
#include "hx711.h"
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

// Check if module has LTE service
static bool has_lte_service(void) {
  if (AT_SendCommand("AT+CPSI?\r\n", "OK", 2000)) {
    if (strstr(AT_GetLastResponse(), "LTE")) {
      return true;
    }
  }
  printf("%s\r\n", AT_GetLastResponse());
  return false;
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

  printf("Oh hello there!\r\n");
  // Initialize HX711
  HX711_Init();

  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_SET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_RESET);
  HAL_Delay(10000);

  send2module("AT\r\n");
  HAL_Delay(2000);
#if 0
  if (CertUpload_UploadISRGRootX1()) {
    printf("Certificate uploaded successfully!\r\n");
  } else {
    printf("Certificate upload failed!\r\n");
  }
#endif
  send2module("AT\r\n");
  HAL_Delay(2000);
  // send2module("AT+CPIN=6436\r\n");
  send2module("AT+CGDCONT=1,\"IP\",\"hologram\"\r\n");
  HAL_Delay(10000);

  HAL_GPIO_WritePin(CellsEnable_GPIO_Port, CellsEnable_Pin, GPIO_PIN_SET);

  // Wait for LTE service
  printf("Waiting for LTE service...\r\n");
  while (!has_lte_service()) {
    printf("No service yet, retrying...\r\n");
    HAL_Delay(5000);
  }
  printf("LTE service acquired!\r\n");
  printf("Service info: %s\r\n", AT_GetLastResponse());

  unsigned int counter = 0;
  uint32_t last_led_toggle = 0;
  while (1) {
    /* USER CODE END WHILE */

    // Process UART bridge continuously (high priority)
    UART_Bridge_Process();

    // Toggle LED every 500ms without blocking
    if (HAL_GetTick() - last_led_toggle >= 1000) {
      HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
      last_led_toggle = HAL_GetTick();
      send2module("AT+CPSI?\r\n");

      // int value = HX711_ReadAverage(HX711_GAIN_64, 32);
      //  printf("Hello %u: %d\r\n", counter++, value);
    }
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
