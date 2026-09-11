#include "radio.h"
#include "at_commands.h"
#include "bridge.h"
#include "cert_upload.h"
#include "main.h"
#include "mqtt.h"
#include "stm32u0xx_hal.h"
#include <stdio.h>
#include <string.h>

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

bool Radio_Init(void) {
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

uint32_t Radio_GetInitCount(void) { return radio_init_count; }

void Radio_Sleep(void) { AT_SendCommand("AT+CSCLK=2\r\n", "OK", 2000); }
