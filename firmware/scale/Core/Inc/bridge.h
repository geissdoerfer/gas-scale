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
void UART_Bridge_Process(void);

/**
 * @brief Read byte from bridge buffer and forward to UART1
 * @param byte Pointer to store the byte
 * @param timeout_ms Timeout in milliseconds
 * @return true if byte read, false on timeout
 */
bool UART_Bridge_ReadByte(uint8_t *byte, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __BRIDGE_H__ */
