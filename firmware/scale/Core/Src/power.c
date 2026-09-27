#include "power.h"
#include "stm32u0xx_hal.h"
#include <stdio.h>

void SystemClock_Config(void);

void Power_EnterStop2Mode(void) {
  printf("Entering STOP2 mode...\r\n");
  HAL_Delay(100); // Allow printf to complete transmission

  // Suspend SysTick interrupt to prevent wakeup
  HAL_SuspendTick();

  // Enter STOP2 mode
  // - Regulator in low-power mode
  // - Wakes up on any enabled interrupt (RTC wakeup timer on EXTI line 20,
  //   or ChargerDetect_Pin on EXTI line 1 while armed)
  HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

  // --- MCU wakes up here when RTC interrupt fires ---

  // System clock is now running on MSI (4 MHz default after wakeup)
  // Need to reconfigure to HSI (16 MHz)
  SystemClock_Config();

  // Resume SysTick
  HAL_ResumeTick();

  printf("Woke up from STOP2 mode\r\n");
}
