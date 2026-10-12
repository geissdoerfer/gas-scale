/**
 ******************************************************************************
 * @file    cert_upload.c
 * @brief   Certificate upload implementation
 ******************************************************************************
 */

#include "cert_upload.h"
#include "certificate.h"
#include "at_commands.h"
#include "usart.h"
#include "stm32u0xx_hal.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Upload certificate to module
 */
bool CertUpload_Upload(const char *cert_data, const char *cert_name, uint32_t timeout_ms) {
  char at_cmd[128];
  uint32_t cert_size = strlen(cert_data);

  printf("Uploading certificate '%s' (%lu bytes)...\r\n", cert_name, cert_size);

  // Step 1: Send AT+CCERTDOWN command
  snprintf(at_cmd, sizeof(at_cmd), "AT+CCERTDOWN=\"%s\",%lu\r\n", cert_name, cert_size);
  printf("Sending: %s", at_cmd);

  HAL_UART_Transmit(&huart3, (uint8_t*)at_cmd, strlen(at_cmd), 1000);

  // Wait for module response
  HAL_Delay(1000);
  AT_SendCommand("", NULL, 1000); // Just read response without sending anything
  printf("Module response: %s\r\n", AT_GetLastResponse());

  printf("Uploading certificate data...\r\n");

  // Step 2: Send certificate data byte by byte
  for (uint32_t i = 0; i < cert_size; i++) {
    uint8_t byte = cert_data[i];
    HAL_UART_Transmit(&huart3, &byte, 1, 100);

    // Small delay between bytes
    HAL_Delay(1);

    // Progress indicator every 100 bytes
    if ((i + 1) % 100 == 0) {
      printf("Sent %lu/%lu bytes...\r\n", i + 1, cert_size);
    }
  }

  printf("Sent all %lu bytes\r\n", cert_size);

  // Step 3: Wait for OK response
  if (!AT_SendCommand("", "OK", 10000)) {
    printf("Error: Did not receive OK after upload\r\n");
    printf("Response: %s\r\n", AT_GetLastResponse());
    return false;
  }

  printf("Certificate uploaded successfully!\r\n");
  return true;
}

/**
 * @brief Upload ISRG Root X1 certificate
 */
bool CertUpload_UploadISRGRootX1(void) {
  return CertUpload_Upload(ISRG_ROOT_X1_CERT, CERT_NAME_ISRG_ROOT_X1, 30000);
}
