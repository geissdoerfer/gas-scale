/**
 ******************************************************************************
 * @file    at_commands.h
 * @brief   AT command utilities for cellular module communication
 ******************************************************************************
 */

#ifndef __AT_COMMANDS_H__
#define __AT_COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

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

#ifdef __cplusplus
}
#endif

#endif /* __AT_COMMANDS_H__ */
