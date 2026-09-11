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
 * @note The MCU will wake up from RTC wakeup timer interrupt
 * @retval None
 */
void Power_EnterStop2Mode(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_H__ */
