/**
 ******************************************************************************
 * @file    bridge.c
 * @brief   UART3 -> UART1 unidirectional bridge
 * @note    UART1->UART3 is handled manually via send2module()
 ******************************************************************************
 */

#include "bridge.h"
#include "usart.h"
#include "stm32u0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

// Buffers for UART bridge
#define BRIDGE_BUFFER_SIZE 512
static uint8_t uart3_rx_buffer;
static uint8_t bridge_buffer[BRIDGE_BUFFER_SIZE];
static uint8_t tx_buffer[BRIDGE_BUFFER_SIZE];
static volatile uint16_t bridge_head = 0;
static volatile uint16_t bridge_tail = 0;
static volatile uint16_t tx_count = 0;
static volatile uint8_t tx_busy = 0;

/**
 * @brief Initialize UART bridge - call this after UART init
 */
void UART_Bridge_Init(void) {
  // Only enable UART3 receive interrupt
  // UART1->UART3 forwarding disabled (done manually via send2module)
  HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);
}

/**
 * @brief Process bridge - call from main loop
 */
void UART_Bridge_Process(void) {
  // Check if there's data to send and UART1 is ready
  if (bridge_head != bridge_tail && !tx_busy) {
    // Copy data from circular buffer to linear TX buffer
    tx_count = 0;
    while (bridge_tail != bridge_head && tx_count < BRIDGE_BUFFER_SIZE) {
      tx_buffer[tx_count++] = bridge_buffer[bridge_tail];
      bridge_tail = (bridge_tail + 1) % BRIDGE_BUFFER_SIZE;
    }

    // Start transmission
    if (tx_count > 0) {
      tx_busy = 1;
      HAL_UART_Transmit_IT(&huart1, tx_buffer, tx_count);
    }
  }
}

/**
 * @brief UART transmit complete callback
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    tx_busy = 0;
    tx_count = 0;
  }
}

/**
 * @brief UART receive complete callback
 * @note This overrides the weak callback from stm32u0xx_hal_uart.c
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {
    // Data received on UART3, add to circular buffer
    uint16_t next_head = (bridge_head + 1) % BRIDGE_BUFFER_SIZE;

    // Only add if buffer not full
    if (next_head != bridge_tail) {
      bridge_buffer[bridge_head] = uart3_rx_buffer;
      bridge_head = next_head;
    }

    // Re-enable UART3 receive interrupt
    HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);
  }
  // UART1->UART3 forwarding disabled (handled manually via send2module)
}

/**
 * @brief Read byte from bridge buffer and forward to UART1
 */
bool UART_Bridge_ReadByte(uint8_t *byte, uint32_t timeout_ms) {
  uint32_t start = HAL_GetTick();

  while ((HAL_GetTick() - start) < timeout_ms) {
    // Check if data available in bridge buffer
    if (bridge_head != bridge_tail) {
      *byte = bridge_buffer[bridge_tail];
      bridge_tail = (bridge_tail + 1) % BRIDGE_BUFFER_SIZE;

      // Forward to UART1 immediately
      HAL_UART_Transmit(&huart1, byte, 1, 10);

      return true;
    }
  }

  return false;
}
