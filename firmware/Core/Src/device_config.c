#include "device_config.h"
#include "stm32u0xx_hal.h"

// Must match the CONFIG memory region in STM32U083xx_FLASH.ld
#define DEVICE_CONFIG_BASE 0x0803F800U

#define FIRST_START_ADDR DEVICE_CONFIG_BASE
#define FIRST_START_SENTINEL 0xFEEDFACECAFEBEEFULL

// Placed at FIRST_START_ADDR by the .device_config section rule in
// STM32U083xx_FLASH.ld, so every flashed image reprograms this address
// back to the sentinel; re-testing first-start just means reflashing, no
// manual flash erase needed. That rule's KEEP() is what stops
// --gc-sections from discarding this otherwise-"unreferenced" section
// (nothing takes this symbol's address - the actual read goes through a
// raw volatile pointer below); the retain attribute would be the newer
// way to say the same thing, but this toolchain silently ignores it for
// this target (confirmed directly - not just assumed), so KEEP() it is.
__attribute__((section(".device_config"), used)) static const uint64_t
    first_start_sentinel = FIRST_START_SENTINEL;

// Never read first_start_sentinel directly: the compiler could constant-fold
// a read of this symbol to its initializer, which is exactly the one case
// that must actually hit flash instead. Going through a volatile pointer at
// its known address (FIRST_START_ADDR) is what makes the read real.

bool DeviceConfig_IsFirstStart(void) {
  return *(volatile uint64_t *)FIRST_START_ADDR == FIRST_START_SENTINEL;
}

void DeviceConfig_ClearFirstStart(void) {
  if (!DeviceConfig_IsFirstStart())
    return;

  // Flash programming can only clear bits, never set them, but it can
  // always clear *to* all-zero regardless of the current value (every bit
  // that needs to go 1->0 is allowed) - so this is safe to write over the
  // sentinel without an erase cycle, no matter what the sentinel's bits are.
  HAL_FLASH_Unlock();
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FIRST_START_ADDR, 0x0ULL);
  HAL_FLASH_Lock();
}
