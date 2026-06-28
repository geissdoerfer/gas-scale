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

/**
 * @brief Read battery voltage using ADC
 * @note Enables battery sense circuit, reads ADC, then disables circuit
 * @retval Battery voltage in volts, or 0.0 if read failed
 */
static float Read_Battery_Voltage(void) {
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

// Radio initialization counter
static uint32_t radio_init_count = 0;

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

/**
 * @brief Initialize the cellular radio module
 * @note Powers on the module, waits for LTE service, and initializes MQTT
 * @retval true if initialization successful, false otherwise
 */
static bool RadioInit(void) {
  radio_init_count++;
  printf("Initializing radio module (count: %lu)...\r\n", radio_init_count);

  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_RESET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_SET);
  HAL_Delay(1000);

  // Clear any old data from the bridge buffer before starting
  UART_Bridge_ClearBuffer();
  AT_ClearResponse();

  /* Power on module (>500ms, using 1500ms for reliability) */
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_SET);
  HAL_Delay(1500);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_RESET);

  // Wait for module to be ready
  printf("Waiting for *ATREADY...\r\n");
  if (AT_WaitForResponse(20000, "*ATREADY")) {
    printf("Module is ready!\r\n");
  } else {
    printf("Timeout waiting for *ATREADY\r\n");
    return false;
  }

  AT_SendCommand("AT\r\n", "OK", 2000);
  HAL_Delay(1000);
  AT_SendCommand("AT\r\n", "OK", 2000);

#if 0
  AT_SendCommand("AT\r\n", "OK", 2000);
  HAL_Delay(30000);
  if (CertUpload_UploadISRGRootX1()) {
    printf("Certificate uploaded successfully!\r\n");
  } else {
    printf("Certificate upload failed!\r\n");
  }
#endif
  AT_SendCommand("AT+CGDCONT=1,\"IP\",\"hologram\"\r\n", "OK", 2000);

  // Wait for LTE service
  printf("Waiting for LTE service...\r\n");
  int retry_count = 0;
  while (!has_lte_service()) {
    printf("No service yet, retrying...\r\n");
    HAL_Delay(5000);
    retry_count++;
    if (retry_count > 20) { // Timeout after ~100 seconds
      printf("Failed to get LTE service\r\n");
      return false;
    }
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
  AT_SendCommand("AT+CCLK?\r\n", NULL, 0);

  if (!MQTT_Init(&config)) {
    printf("MQTT initialization failed\r\n");
    return false;
  }

  printf("Radio initialization complete\r\n");
  return true;
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
  float v_bat;
  while ((v_bat = Read_Battery_Voltage()) < 4.1) {
    printf("Battery voltage low: %.2f\r\n", v_bat);
    if (rtc_wakeup_flag)
      rtc_wakeup_flag = 0; // Clear the software flag
    Enter_STOP2_Mode();
  }

  // Initialize UART bridge
  UART_Bridge_Init();

  printf("Oh hello there!\r\n");
  // Initialize HX711
  HAL_GPIO_WritePin(CellsEnable_GPIO_Port, CellsEnable_Pin, GPIO_PIN_SET);
  HX711_Init();
  HX711_PowerDown();

  // Initialize radio module
  RadioInit();

  while (1) {
    /* USER CODE END WHILE */

    // Put cellular module to sleep
    AT_SendCommand("AT+CSCLK=2\r\n", "OK", 2000);

    // Turn off LED
    HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

    // Enter STOP2 mode until next RTC wakeup
    Enter_STOP2_Mode();

    // Check if RTC wakeup event occurred (every 60 seconds)
    if (rtc_wakeup_flag) {
      rtc_wakeup_flag = 0; // Clear the flag

      printf("RTC wakeup: Starting sensor read and MQTT publish\r\n");

      // Read battery voltage first thing after wakeup
      float battery_voltage = Read_Battery_Voltage();

      HX711_PowerUp();

      // Wake up the cellular module
      printf("Trying to wakeup module\r\n");
      for (unsigned int i = 0; i < 3; i++) {
        if (AT_SendCommand("AT\r\n", "OK", 2000))
          break;
        else {
          printf("Didn't receive OK from module\r\n");
        }
      }
      // Turn on LED during sensor read and transmission
      HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);

      // Read sensor value
      int value = HX711_ReadAverage(HX711_GAIN_64, 32);
      HX711_PowerDown();

      // Format the sensor data into a JSON message
      char message[128];
      snprintf(message, sizeof(message),
               "{\"val\":%d,\"temp\":%.3f,\"bat\":%.3f,\"cnt\":%lu}", value,
               23.5, battery_voltage, radio_init_count);

      if (MQTT_Connect()) {
        HAL_Delay(500);

        // Publish sensor data
        if (MQTT_Publish("sensors/c0ffee00/data", message)) {
          HAL_Delay(500);
          printf("Published weight: %d\r\n", value);
        } else {
          printf("MQTT publish failed\r\n");
        }

        // Disconnect from MQTT
        MQTT_Disconnect();
      } else {
        printf("Reinitializing radio due to MQTT failure...\r\n");
        RadioInit();
      }
    }
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
