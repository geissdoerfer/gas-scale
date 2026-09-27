#include "device_config.h"
#include "stm32u0xx_hal.h"

// Must match the CONFIG memory region in STM32U083xx_FLASH.ld
#define DEVICE_CONFIG_BASE 0x0803F800U

#define FIRST_START_ADDR DEVICE_CONFIG_BASE
#define FIRST_START_ERASED_VALUE 0xFFFFFFFFFFFFFFFFULL

bool DeviceConfig_IsFirstStart(void) {
  return *(volatile uint64_t *)FIRST_START_ADDR == FIRST_START_ERASED_VALUE;
}

void DeviceConfig_ClearFirstStart(void) {
  if (!DeviceConfig_IsFirstStart())
    return;

  // Flash can only be cleared to 0 without an erase cycle when starting
  // from the erased (all 1s) state, which is guaranteed here.
  HAL_FLASH_Unlock();
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FIRST_START_ADDR, 0x0ULL);
  HAL_FLASH_Lock();
}
