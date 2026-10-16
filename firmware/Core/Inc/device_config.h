/**
 ******************************************************************************
 * @file    device_config.h
 * @brief   Non-volatile device configuration stored in flash
 ******************************************************************************
 */

#ifndef __DEVICE_CONFIG_H__
#define __DEVICE_CONFIG_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether the device has ever completed a first start
 * @note Checks for a sentinel value the linker places at this address (see
 *       device_config.c and the CONFIG region in the .ld script), so every
 *       newly flashed image reads as "first start" again - no separate
 *       flash erase needed to re-test it, only a reflash
 * @retval true if this is the first start since the device was last flashed
 */
bool DeviceConfig_IsFirstStart(void);

/**
 * @brief Permanently clear the first-start flag (until the next reflash)
 * @note Only clears bits (no erase needed): writes all-zero, which flash
 *       programming can always do regardless of the current value
 * @retval None
 */
void DeviceConfig_ClearFirstStart(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_CONFIG_H__ */
