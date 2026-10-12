/**
 ******************************************************************************
 * @file    watchdog_task.h
 * @brief   FreeRTOS task that periodically refreshes the IWDG
 ******************************************************************************
 */

#ifndef __WATCHDOG_TASK_H__
#define __WATCHDOG_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start WatchdogTask
 * @note Call once during FreeRTOS bring-up, before osKernelStart(). Must be
 *       called only after MX_IWDG_Init(), and MX_IWDG_Init() must itself be
 *       called late (right before this) since IWDG starts counting down the
 *       instant it's initialized and can never be stopped again.
 * @retval None
 */
void StartWatchdogTask(void);

/**
 * @brief Signal that the application made real progress
 * @note Resets the stale-progress counter to 0. Safe to call from any task.
 *       If nothing calls this for about an hour, WatchdogTask stops
 *       refreshing the IWDG and lets it reset the device.
 * @retval None
 */
void Watchdog_ClearStaleCounter(void);

#ifdef __cplusplus
}
#endif

#endif /* __WATCHDOG_TASK_H__ */
