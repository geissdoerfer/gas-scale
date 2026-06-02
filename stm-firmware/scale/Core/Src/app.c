#include "adc.h"
#include "at_commands.h"
#include "bridge.h"
#include "cert_upload.h"
#include "gpio.h"
#include "hx711.h"
#include "main.h"
#include "mqtt.h"
#include "rtc.h"
#include "stm32u0xx_hal.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void SystemClock_Config(void);

/**
 * @brief Configure and enter STOP2 low power mode
 * @note The MCU will wake up from RTC wakeup timer interrupt
 * @retval None
 */
static void Enter_STOP2_Mode(void) {
  printf("Entering STOP2 mode...\r\n");
  HAL_Delay(100); // Allow printf to complete transmission

  // Suspend SysTick interrupt to prevent wakeup
  HAL_SuspendTick();

  // Enter STOP2 mode
  // - Regulator in low-power mode
  // - Wake up on RTC wakeup timer interrupt (EXTI line 20)
  HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

  // --- MCU wakes up here when RTC interrupt fires ---

  // System clock is now running on MSI (4 MHz default after wakeup)
  // Need to reconfigure to HSI (16 MHz)
  SystemClock_Config();

  // Resume SysTick
  HAL_ResumeTick();

  printf("Woke up from STOP2 mode\r\n");
}

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
  MX_RTC_Init();

  // Initialize UART bridge
  UART_Bridge_Init();

  printf("Oh hello there!\r\n");
  // Initialize HX711
  HAL_GPIO_WritePin(CellsEnable_GPIO_Port, CellsEnable_Pin, GPIO_PIN_SET);
  HX711_Init();

  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_SET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_RESET);

  // Wait for module to be ready
  printf("Waiting for *ATREADY...\r\n");
  if (AT_WaitForResponse(20000, "*ATREADY")) {
    printf("Module is ready!\r\n");
  } else {
    printf("Timeout waiting for *ATREADY\r\n");
  }

#if 0
  if (CertUpload_UploadISRGRootX1()) {
    printf("Certificate uploaded successfully!\r\n");
  } else {
    printf("Certificate upload failed!\r\n");
  }
#endif
  AT_SendCommand("AT\r\n", "OK", 2000);
  HAL_Delay(1000);

  AT_SendCommand("AT+CGDCONT=1,\"IP\",\"hologram\"\r\n", "OK", 2000);

  // Wait for LTE service
  printf("Waiting for LTE service...\r\n");
  while (!has_lte_service()) {
    printf("No service yet, retrying...\r\n");
    HAL_Delay(5000);
  }
  printf("Service info: %s\r\n", AT_GetLastResponse());

  MQTT_Config_t config = {
      .broker_url =
          "tcp://f85c8d608a674db6881108d544f12896.s1.eu.hivemq.cloud:8883",
      .client_id = "ClientID",
      .username = "nessie",
      .password = "D-sub729",
      .keepalive = 60,
      .ca_cert = "isrgrootx1.pem"};

  AT_SendCommand("AT+CNTP\r\n", "OK", 2000);
  HAL_Delay(2000);
  send2module("AT+CCLK?\r\n");

  MQTT_Init(&config);

  printf(
      "RTC-based sensor reading initialized. Waiting for wakeup events...\r\n");

  // Enter STOP2 mode immediately - RTC will wake us up
  Enter_STOP2_Mode();

  while (1) {
    /* USER CODE END WHILE */

    // Check if RTC wakeup event occurred (every 60 seconds)
    if (rtc_wakeup_flag) {
      rtc_wakeup_flag = 0; // Clear the flag

      printf("RTC wakeup: Starting sensor read and MQTT publish\r\n");
      HX711_PowerUp();

      // Wake up the cellular module
      printf("Trying to wakeup module\r\n");
      for (unsigned int i = 0; i < 3; i++) {
        if (AT_SendCommand("AT\r\n", "OK", 2000))
          break;
        else
          printf("Didn't receive OK from module\r\n");
      }

      // Turn on LED during sensor read and transmission
      HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);

      // Read sensor value
      int value = HX711_ReadAverage(HX711_GAIN_64, 32);

      // Read battery voltage
      float battery_voltage = 0.0f;

      // Enable battery sense circuit
      HAL_GPIO_WritePin(BatSenseEnable_GPIO_Port, BatSenseEnable_Pin,
                        GPIO_PIN_SET);
      HAL_Delay(10); // Allow circuit to settle

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

        printf("Battery ADC: %lu, Voltage: %.3fV\r\n", adc_raw,
               battery_voltage);
      } else {
        printf("Battery voltage ADC read failed\r\n");
      }

      // Disable battery sense circuit
      HAL_GPIO_WritePin(BatSenseEnable_GPIO_Port, BatSenseEnable_Pin,
                        GPIO_PIN_RESET);
      HAL_ADC_Stop(&hadc1);

      // Format the sensor data into a JSON message
      char message[128];
      snprintf(message, sizeof(message),
               "{\"val\":%d,\"temp\":%.3f,\"bat\":%.3f}", value, 23.5,
               battery_voltage);

      // Connect to MQTT broker
      MQTT_Connect();
      HAL_Delay(500);

      // Publish sensor data
      MQTT_Publish("sensors/a46fb35d/data", message);
      HAL_Delay(500);

      // Disconnect from MQTT
      MQTT_Disconnect();

      printf("Published weight: %d\r\n", value);

      // Put cellular module to sleep
      AT_SendCommand("AT+CSCLK=2\r\n", "OK", 2000);

      // Turn off LED
      HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);
      HX711_PowerDown();

      // Enter STOP2 mode until next RTC wakeup
      Enter_STOP2_Mode();
    }
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
