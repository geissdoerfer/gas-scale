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
 * @note An erased flash cell reads as all 1s, so an unprogrammed device
 *       reads as "first start" with no explicit initialization needed
 * @retval true if this is the first start since the device was flashed
 */
bool DeviceConfig_IsFirstStart(void);

/**
 * @brief Permanently clear the first-start flag
 * @note Only clears bits (no erase needed) since the flag starts erased
 * @retval None
 */
void DeviceConfig_ClearFirstStart(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_CONFIG_H__ */
