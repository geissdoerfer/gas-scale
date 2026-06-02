/**
 ******************************************************************************
 * @file    at_commands.c
 * @brief   AT command utilities implementation
 ******************************************************************************
 */

#include "at_commands.h"
#include "bridge.h"
#include "usart.h"
#include "stm32u0xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Response buffer
static char response_buffer[AT_RESPONSE_BUFFER_SIZE];

/**
 * @brief Wait for response from module with timeout
 */
bool AT_WaitForResponse(uint32_t timeout_ms, const char *expected) {
  uint32_t start = HAL_GetTick();
  uint16_t idx = 0;

  memset(response_buffer, 0, AT_RESPONSE_BUFFER_SIZE);

  while (HAL_GetTick() - start < timeout_ms) {
    // Read from bridge buffer (which also forwards to UART1)
    uint8_t byte;
    if (UART_Bridge_ReadByte(&byte, 10)) {
      if (idx < AT_RESPONSE_BUFFER_SIZE - 1) {
        response_buffer[idx++] = byte;
        response_buffer[idx] = '\0';
      }

      // Check if we got the expected response
      if (expected && strstr(response_buffer, expected)) {
        return true;
      }
    }
  }

  return (expected == NULL); // If not expecting anything, just timeout
}

/**
 * @brief Send AT command and wait for response
 */
bool AT_SendCommand(const char *cmd, const char *expected_response, uint32_t timeout_ms) {
  // Send command
  HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);

  // Wait for response (reads from bridge buffer and forwards to UART1)
  return AT_WaitForResponse(timeout_ms, expected_response);
}

/**
 * @brief Get the last AT response
 */
const char* AT_GetLastResponse(void) {
  return response_buffer;
}

/**
 * @brief Clear the response buffer
 */
void AT_ClearResponse(void) {
  memset(response_buffer, 0, AT_RESPONSE_BUFFER_SIZE);
}

/**
 * @brief Send raw data (not an AT command)
 */
bool AT_SendData(const uint8_t *data, uint16_t length, const char *expected_response, uint32_t timeout_ms) {
  // Send raw data
  HAL_UART_Transmit(&huart3, (uint8_t*)data, length, 5000);

  // Wait for response (reads from bridge buffer and forwards to UART1)
  return AT_WaitForResponse(timeout_ms, expected_response);
}
