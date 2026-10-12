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

// Upload the CA certificate needed for the MQTT TLS connection
static bool UploadCertificate(void) {
  AT_SendCommand("AT\r\n", "OK", 2000);
  HAL_Delay(60000);
  if (CertUpload_UploadISRGRootX1()) {
    printf("Certificate uploaded successfully!\r\n");
    return true;
  } else {
    printf("Certificate upload failed!\r\n");
    return false;
  }
}

void Radio_PowerOff(void) {
  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_RESET);
}

void Radio_PowerOn(void) {
  HAL_GPIO_WritePin(RadioEnable_GPIO_Port, RadioEnable_Pin, GPIO_PIN_SET);
}

// Toggle power to the radio module and pulse its power key to (re)boot it
static void Radio_Reset(void) {
  Radio_PowerOff();
  HAL_Delay(1000);
  Radio_PowerOn();
  HAL_Delay(1000);

  // Clear any old data from the bridge buffer before starting
  UART_Bridge_ClearBuffer();
  AT_ClearResponse();

  /* Power on module (>500ms, using 1500ms for reliability) */
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_SET);
  HAL_Delay(1500);
  HAL_GPIO_WritePin(RadioPwrKey_GPIO_Port, RadioPwrKey_Pin, GPIO_PIN_RESET);
}

bool Radio_Init(void) {
  radio_init_count++;
  printf("Initializing radio module (count: %lu)...\r\n", radio_init_count);

  // Retry the reset + power-on sequence a few times in case the module
  // doesn't come up cleanly on the first try
  bool ready = false;
  for (unsigned int attempt = 1; attempt <= 3; attempt++) {
    Radio_Reset();

    printf("Waiting for *ATREADY (attempt %u/3)...\r\n", attempt);
    if (AT_WaitForResponse(20000, "*ATREADY")) {
      printf("Module is ready!\r\n");
      ready = true;
      break;
    }
    printf("Timeout waiting for *ATREADY\r\n");
  }

  if (!ready) {
    return false;
  }

  AT_SendCommand("AT\r\n", "OK", 2000);
  HAL_Delay(1000);
  AT_SendCommand("AT\r\n", "OK", 2000);

  // UploadCertificate();

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

  printf("Radio initialization complete\r\n");
  return true;
}

uint32_t Radio_GetInitCount(void) { return radio_init_count; }

void Radio_Sleep(void) { AT_SendCommand("AT+CSCLK=2\r\n", "OK", 2000); }
