/**
 ******************************************************************************
 * @file    bridge.h
 * @brief   UART bridge header file
 ******************************************************************************
 */

#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

void UART_Bridge_Init(void);

/**
 * @brief Start DebugTxTask
 * @note Call once during FreeRTOS bring-up, before osKernelStart(). Debug
 *       passthrough to UART1 won't actually run until this has been called,
 *       even though UART_Bridge_Init() (called earlier, pre-kernel) already
 *       starts buffering received bytes for it.
 */
void StartDebugTxTask(void);

/**
 * @brief Read byte from the bridge buffer
 * @note UART1 debug passthrough happens autonomously via interrupts; this
 *       only reads from the buffer AT_WaitForResponse() parses responses
 *       from and no longer needs to be paired with any polling call.
 * @param byte Pointer to store the byte
 * @param timeout_ms Timeout in milliseconds
 * @return true if byte read, false on timeout
 */
bool UART_Bridge_ReadByte(uint8_t *byte, uint32_t timeout_ms);

/**
 * @brief Clear the bridge buffer
 * @note Useful before starting operations that expect specific responses
 */
void UART_Bridge_ClearBuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* __BRIDGE_H__ */
