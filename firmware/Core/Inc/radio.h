/**
 ******************************************************************************
 * @file    radio.h
 * @brief   Cellular radio module initialization and MQTT bring-up
 ******************************************************************************
 */

#ifndef __RADIO_H__
#define __RADIO_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power off the cellular radio module (RadioEnable low)
 */
void Radio_PowerOff(void);

/**
 * @brief Power on the cellular radio module (RadioEnable high)
 */
void Radio_PowerOn(void);

/**
 * @brief Initialize the cellular radio module
 * @note Powers on the module, waits for LTE service, and initializes MQTT
 * @retval true if initialization successful, false otherwise
 */
bool Radio_Init(void);

/**
 * @brief Get the number of times the radio module has been initialized
 * @retval Radio initialization count
 */
uint32_t Radio_GetInitCount(void);

/**
 * @brief Put the cellular radio module to sleep
 */
void Radio_Sleep(void);

#ifdef __cplusplus
}
#endif

#endif /* __RADIO_H__ */
