/**
 ******************************************************************************
 * @file    bridge.c
 * @brief   UART3 -> UART1 unidirectional bridge
 * @note    UART1->UART3 is handled manually via send2module()
 ******************************************************************************
 */

#include "bridge.h"
#include "cmsis_os2.h"
#include "usart.h"
#include "stm32u0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

static uint8_t uart3_rx_buffer;

// Bridge buffer: fed by the RX ISR, drained only by UART_Bridge_ReadByte()
// for AT response parsing. This is its own buffer (rather than shared with
// the debug buffer below) so nothing else can ever steal bytes it needs.
#define BRIDGE_BUFFER_SIZE 512
static uint8_t bridge_buffer[BRIDGE_BUFFER_SIZE];
static volatile uint16_t bridge_head = 0;
static volatile uint16_t bridge_tail = 0;

// Debug buffer: fed by the same RX ISR, drained autonomously onto UART1 for
// passthrough visibility - entirely interrupt-driven, no app-side polling.
#define DEBUG_BUFFER_SIZE 512
static uint8_t debug_buffer[DEBUG_BUFFER_SIZE];
static volatile uint16_t debug_head = 0;
static volatile uint16_t debug_tail = 0;
static volatile uint16_t debug_inflight = 0;
static volatile bool debug_tx_busy = false;

#define DEBUG_TX_TASK_FLAG_PENDING 0x01u
static osThreadId_t debug_tx_task_handle;

// Kick off (or continue) transmitting whatever's pending in debug_buffer to
// UART1. Safe to call from ISR context; re-triggered from
// HAL_UART_TxCpltCallback so it keeps draining until it's caught up.
static void DebugTx_Start(void) {
  if (debug_tx_busy || debug_head == debug_tail) {
    return;
  }

  // Only the contiguous run up to the physical end of the array, or up to
  // debug_head if that comes first - HAL_UART_Transmit_IT needs one
  // contiguous pointer+length, so a wrapped run is sent in two calls.
  uint16_t len = (debug_head > debug_tail) ? (debug_head - debug_tail)
                                            : (DEBUG_BUFFER_SIZE - debug_tail);

  // huart1 is shared with printf()'s blocking HAL_UART_Transmit (see
  // __io_putchar). If a printf happens to be mid-transmit right now, this
  // returns HAL_BUSY without starting anything - don't mark ourselves busy
  // in that case, or we'd wait forever for a TxCplt callback that will
  // never come. The next incoming byte retries via HAL_UART_RxCpltCallback.
  debug_inflight = len;
  if (HAL_UART_Transmit_IT(&huart1, &debug_buffer[debug_tail], len) == HAL_OK) {
    debug_tx_busy = true;
  }
}

// Calls DebugTx_Start() outside of ISR context, notified from
// HAL_UART_RxCpltCallback. DebugTx_Start()'s common case (kicking off a new
// HAL_UART_Transmit_IT chunk) has real HAL overhead; on a busy link that
// otherwise runs on nearly every received byte, right inside the
// time-critical USART3 RX ISR, widening the window for it to overrun before
// it's ready for the next byte. Deferring it here keeps that ISR down to
// just the buffer writes and the re-arm.
static void DebugTxTask(void *argument) {
  (void)argument;

  for (;;) {
    osThreadFlagsWait(DEBUG_TX_TASK_FLAG_PENDING, osFlagsWaitAny, osWaitForever);
    DebugTx_Start();
  }
}

/**
 * @brief Start DebugTxTask
 * @note Call once during FreeRTOS bring-up, before osKernelStart()
 */
void StartDebugTxTask(void) {
  const osThreadAttr_t debug_tx_task_attributes = {
      .name = "DebugTxTask",
      .priority = (osPriority_t)osPriorityNormal,
      .stack_size = 256 * 4,
  };
  debug_tx_task_handle = osThreadNew(DebugTxTask, NULL, &debug_tx_task_attributes);
}

/**
 * @brief Initialize UART bridge - call this after UART init
 */
void UART_Bridge_Init(void) {
  // Only enable UART3 receive interrupt
  // UART1->UART3 forwarding disabled (done manually via send2module)
  HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);
}

/**
 * @brief UART error callback
 * @note HAL treats a UART overrun as a "blocking" error: it disables RX
 *       interrupts and aborts reception via the error path rather than the
 *       complete path, so HAL_UART_RxCpltCallback's re-arm never runs. An
 *       overrun is an ordinary occurrence on a busy link, not exceptional,
 *       so without this override the first one would silently and
 *       permanently stop all further forwarding from the module.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {
    HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);
  }
}

/**
 * @brief UART transmit complete callback
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    debug_tail = (debug_tail + debug_inflight) % DEBUG_BUFFER_SIZE;
    debug_inflight = 0;
    debug_tx_busy = false;
    DebugTx_Start(); // continue if more arrived while this chunk was sending
  }
}

/**
 * @brief UART receive complete callback
 * @note This overrides the weak callback from stm32u0xx_hal_uart.c
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {
    uint8_t byte = uart3_rx_buffer;

    // Re-enable UART3 receive interrupt as early as possible, before the
    // slower buffer bookkeeping below (particularly DebugTx_Start(), which
    // can call into the full HAL_UART_Transmit_IT path) - minimizes the
    // window in which a back-to-back incoming byte could overrun before
    // we're ready to receive it again. byte is already a local copy, so
    // this can't lose the byte just captured above.
    HAL_UART_Receive_IT(&huart3, &uart3_rx_buffer, 1);

    // Feed the app-facing bridge buffer (consumed by UART_Bridge_ReadByte)
    uint16_t next_bridge_head = (bridge_head + 1) % BRIDGE_BUFFER_SIZE;
    if (next_bridge_head != bridge_tail) {
      bridge_buffer[bridge_head] = byte;
      bridge_head = next_bridge_head;
    }

    // Feed the debug passthrough buffer (drained by DebugTxTask, deferred
    // out of this ISR - see DebugTxTask's comment)
    uint16_t next_debug_head = (debug_head + 1) % DEBUG_BUFFER_SIZE;
    if (next_debug_head != debug_tail) {
      debug_buffer[debug_head] = byte;
      debug_head = next_debug_head;
      if (debug_tx_task_handle != NULL) {
        osThreadFlagsSet(debug_tx_task_handle, DEBUG_TX_TASK_FLAG_PENDING);
      }
    }
  }
  // UART1->UART3 forwarding disabled (handled manually via send2module)
}

/**
 * @brief Read byte from bridge buffer
 */
bool UART_Bridge_ReadByte(uint8_t *byte, uint32_t timeout_ms) {
  uint32_t start = HAL_GetTick();

  while ((HAL_GetTick() - start) < timeout_ms) {
    if (bridge_head != bridge_tail) {
      *byte = bridge_buffer[bridge_tail];
      bridge_tail = (bridge_tail + 1) % BRIDGE_BUFFER_SIZE;
      return true;
    }
  }

  return false;
}

/**
 * @brief Clear the bridge buffer
 */
void UART_Bridge_ClearBuffer(void) {
  // Reset the circular buffer pointers
  bridge_head = 0;
  bridge_tail = 0;
}
