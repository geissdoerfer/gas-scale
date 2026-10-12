/**
 ******************************************************************************
 * @file    at_commands.h
 * @brief   AT command utilities for cellular module communication
 ******************************************************************************
 */

#ifndef __AT_COMMANDS_H__
#define __AT_COMMANDS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Response buffer size
#define AT_RESPONSE_BUFFER_SIZE 512

/**
 * @brief Send AT command and wait for response
 * @param cmd AT command string (with \r\n)
 * @param expected_response Expected string in response (NULL to skip check)
 * @param timeout_ms Timeout in milliseconds
 * @return true if successful (or expected response found), false otherwise
 */
bool AT_SendCommand(const char *cmd, const char *expected_response, uint32_t timeout_ms);

/**
 * @brief Get the last AT response
 * @return Pointer to response buffer
 */
const char* AT_GetLastResponse(void);

/**
 * @brief Clear the response buffer
 */
void AT_ClearResponse(void);

/**
 * @brief Send raw data (not an AT command)
 * @param data Data buffer to send
 * @param length Number of bytes to send
 * @param expected_response Expected string in response (NULL to skip check)
 * @param timeout_ms Timeout in milliseconds
 * @return true if successful (or expected response found), false otherwise
 */
bool AT_SendData(const uint8_t *data, uint16_t length, const char *expected_response, uint32_t timeout_ms);

/**
 * @brief Wait for response from module with timeout
 * @param timeout_ms Timeout in milliseconds
 * @param expected Expected string in response (NULL to just timeout)
 * @return true if expected string found (or if expected is NULL after timeout), false otherwise
 */
bool AT_WaitForResponse(uint32_t timeout_ms, const char *expected);

#ifdef __cplusplus
}
#endif

#endif /* __AT_COMMANDS_H__ */
