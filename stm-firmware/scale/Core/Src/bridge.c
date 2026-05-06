/**
 ******************************************************************************
 * @file    bridge.c
 * @brief   UART1 <-> UART3 bidirectional bridge
 ******************************************************************************
 */

#include "bridge.h"
#include "usart.h"

// Buffers for UART bridge
static uint8_t uart1_rx_buffer;
static uint8_t uart3_rx_buffer;

/**
 * @brief Initialize UART bridge - call this after UART init
 */
void UART_Bridge_Init(void) {
  // Enable UART1 receive interrupt
  HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer, 1);

  // Enable UART3 receive interrupt
  HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);
}

/**
 * @brief UART receive complete callback
 * @note This overrides the weak callback from stm32u0xx_hal_uart.c
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    // Data received on UART1, forward to UART3
    HAL_UART_Transmit(&huart3, &uart1_rx_buffer, 1, HAL_MAX_DELAY);

    // Re-enable UART1 receive interrupt
    HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer, 1);
  } else if (huart->Instance == USART3) {
    // Data received on UART3, forward to UART1
    HAL_UART_Transmit(&huart1, &uart3_rx_buffer, 1, HAL_MAX_DELAY);

    // Re-enable UART3 receive interrupt
    HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);
  }
}
