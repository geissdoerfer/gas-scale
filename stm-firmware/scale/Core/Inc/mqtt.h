/**
 ******************************************************************************
 * @file    mqtt.h
 * @brief   MQTT client for cellular module
 ******************************************************************************
 */

#ifndef __MQTT_H__
#define __MQTT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief MQTT configuration structure
 */
typedef struct {
  const char *broker_url; // e.g., "tcp://broker.hivemq.com:8883"
  const char *client_id;  // Client ID
  const char *username;   // Username (NULL if not required)
  const char *password;   // Password (NULL if not required)
  uint16_t keepalive;     // Keep-alive interval in seconds
  const char *ca_cert;    // CA certificate name (if using SSL)
} MQTT_Config_t;

/**
 * @brief Initialize MQTT service and configure SSL
 * @param config MQTT configuration
 * @return true if successful, false otherwise
 */
bool MQTT_Init(const MQTT_Config_t *config);

/**
 * @brief Connect to MQTT broker
 * @return true if connected, false otherwise
 */
bool MQTT_Connect(void);

/**
 * @brief Publish a message to a topic
 * @param topic Topic name
 * @param message Message string
 * @return true if published successfully, false otherwise
 */
bool MQTT_Publish(const char *topic, const char *message);

/**
 * @brief Disconnect from MQTT broker
 * @return true if disconnected, false otherwise
 */
bool MQTT_Disconnect(void);

/**
 * @brief Upload string to MQTT server (convenience function)
 * @param broker_url Broker URL with port (e.g., "tcp://broker:8883")
 * @param client_id Client ID
 * @param username Username (NULL if not required)
 * @param password Password (NULL if not required)
 * @param topic Topic to publish to
 * @param message Message to publish
 * @param ca_cert CA certificate name (e.g., "isrgrootx1.pem")
 * @return true if successful, false otherwise
 */
bool MQTT_UploadString(const char *broker_url, const char *client_id,
                       const char *username, const char *password,
                       const char *topic, const char *message,
                       const char *ca_cert);

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_H__ */
