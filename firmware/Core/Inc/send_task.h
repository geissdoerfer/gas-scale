/**
 ******************************************************************************
 * @file    send_task.h
 * @brief   FreeRTOS task that sends samples off the device
 ******************************************************************************
 */

#ifndef __SEND_TASK_H__
#define __SEND_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start SendTask
 * @note Call once during FreeRTOS bring-up, before osKernelStart()
 * @retval None
 */
void StartSendTask(void);

/**
 * @brief Notify SendTask that sample_queue has a backlog worth sending
 * @note Safe to call from SampleTask; see sample_task.c
 * @retval None
 */
void SendTask_NotifyPending(void);

#ifdef __cplusplus
}
#endif

#endif /* __SEND_TASK_H__ */
