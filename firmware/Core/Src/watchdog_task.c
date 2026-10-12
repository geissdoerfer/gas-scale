#include "watchdog_task.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "iwdg.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>

// IWDG's real timeout, given its CubeMX configuration (prescaler /512,
// reload 4095): (512 / 32000) * 4096 = 65.536 seconds. Refresh well under
// that so a couple of missed periods still don't trip it.
#define WATCHDOG_PERIOD_MS (20u * 1000u)

// WATCHDOG_STALE_THRESHOLD periods of no progress before giving up: chosen
// so threshold * period ~= 1 hour, the point at which we consider the
// device stuck and let the IWDG reset it.
#define WATCHDOG_STALE_THRESHOLD (60u * 60u * 1000u / WATCHDOG_PERIOD_MS)

// Plain counter guarded by a critical section rather than a CMSIS-RTOS2
// semaphore: a semaphore's count can only be incremented/decremented, with
// no way to reset it straight to 0, which is exactly what
// Watchdog_ClearStaleCounter() needs to do.
static volatile uint32_t stale_periods = 0;

static void WatchdogTask(void *argument) {
  (void)argument;

  printf("Watchdog task started\r\n");

  for (;;) {
    osDelay(WATCHDOG_PERIOD_MS);

    taskENTER_CRITICAL();
    bool progressing = (stale_periods < WATCHDOG_STALE_THRESHOLD);
    if (progressing) {
      stale_periods++;
    }
    taskEXIT_CRITICAL();

    if (!progressing) {
      printf("Watchdog: no progress in ~1h, letting IWDG reset the "
             "device\r\n");
      continue; // Stop refreshing; IWDG will reset on its own from here
    }

    HAL_IWDG_Refresh(&hiwdg);
  }
}

void StartWatchdogTask(void) {
  const osThreadAttr_t watchdog_task_attributes = {
      .name = "WatchdogTask",
      .priority = (osPriority_t)osPriorityNormal,
      .stack_size = 256 * 4,
  };
  osThreadNew(WatchdogTask, NULL, &watchdog_task_attributes);
}

void Watchdog_ClearStaleCounter(void) {
  taskENTER_CRITICAL();
  stale_periods = 0;
  taskEXIT_CRITICAL();
}
