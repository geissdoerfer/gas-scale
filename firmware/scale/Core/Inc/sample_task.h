/**
 ******************************************************************************
 * @file    sample_task.h
 * @brief   FreeRTOS task that samples the HX711 and battery voltage
 ******************************************************************************
 */

#ifndef __SAMPLE_TASK_H__
#define __SAMPLE_TASK_H__

#include "cmsis_os2.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float battery_voltage;
  int32_t sensor_value;
  uint32_t timestamp; // HAL_GetTick() value at sample time, in ms
} Sample_t;

// Queue of Sample_t, filled by SampleTask on every RTC wakeup tick.
// Valid only after StartSampleTask() has run.
extern osMessageQueueId_t sample_queue;

/**
 * @brief Create the sample queue and start SampleTask
 * @note Call once during FreeRTOS bring-up, before osKernelStart()
 * @retval None
 */
void StartSampleTask(void);

/**
 * @brief Notify SampleTask that an RTC wakeup tick occurred
 * @note Safe to call from ISR context; see
 *       HAL_RTCEx_WakeUpTimerEventCallback in app.c
 * @retval None
 */
void SampleTask_NotifyTick(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAMPLE_TASK_H__ */
