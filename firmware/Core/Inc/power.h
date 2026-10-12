/**
 ******************************************************************************
 * @file    power.h
 * @brief   Low-power mode management
 ******************************************************************************
 */

#ifndef __POWER_H__
#define __POWER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure and enter STOP2 low power mode
 * @note The MCU wakes up on any enabled interrupt (e.g. the RTC wakeup
 *       timer, or the ChargerDetect_Pin EXTI line while it is armed)
 * @retval None
 */
void Power_EnterStop2Mode(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_H__ */
