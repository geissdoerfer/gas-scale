/**
 ******************************************************************************
 * @file    mqtt.c
 * @brief   MQTT client implementation
 ******************************************************************************
 */

#include "mqtt.h"
#include "at_commands.h"
#include "stm32u0xx_hal.h"

#include <stdio.h>
#include <string.h>

// Store current configuration
static MQTT_Config_t current_config;
static bool mqtt_initialized = false;
static bool mqtt_connected = false;

/**
 * @brief Initialize MQTT service and configure SSL
 */
bool MQTT_Init(const MQTT_Config_t *config) {
  printf("Initializing MQTT...\r\n");

  // Store configuration
  memcpy(&current_config, config, sizeof(MQTT_Config_t));

  // Configure SSL BEFORE starting MQTT service if enabled
  char cmd[128];

  // Set SSL version (4 = all versions: SSL3.0, TLS1.0, TLS1.1, TLS1.2)
  if (!AT_SendCommand("AT+CSSLCFG=\"sslversion\",0,4\r\n", "OK", 2000)) {
    printf("Failed to set SSL version\r\n");
    return false;
  }

  HAL_Delay(500);

  // Set authentication mode (1 = verify server)
  if (!AT_SendCommand("AT+CSSLCFG=\"authmode\",0,1\r\n", "OK", 2000)) {
    printf("Failed to set auth mode\r\n");
    return false;
  }

  HAL_Delay(500);

  // Set CA certificate
  if (config->ca_cert) {
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"cacert\",0,\"%s\"\r\n",
             config->ca_cert);
    if (!AT_SendCommand(cmd, "OK", 2000)) {
      printf("Failed to set CA cert\r\n");
      return false;
    }
  }

  HAL_Delay(500);

  // Enable SNI (Server Name Indication)
  if (!AT_SendCommand("AT+CSSLCFG=\"enableSNI\",0,1\r\n", "OK", 2000)) {
    printf("Failed to enable SNI\r\n");
    return false;
  }

  HAL_Delay(500);

  // Start MQTT service
  if (!AT_SendCommand("AT+CMQTTSTART\r\n", "OK", 5000)) {
    printf("Failed to start MQTT service\r\n");
    return false;
  }

  mqtt_initialized = true;
  printf("MQTT initialized successfully\r\n");
  return true;
}

/**
 * @brief Connect to MQTT broker
 */
bool MQTT_Connect(void) {
  // Build connect command
  char cmd[256];
  if (!mqtt_initialized) {
    printf("MQTT not initialized\r\n");
    return false;
  }

  // Acquire MQTT client (index 0, with or without SSL)
  snprintf(cmd, sizeof(cmd), "AT+CMQTTACCQ=0,\"%s\",1\r\n", "ClientID");
  printf("\r\nAcquiring MQTT client...\r\n");
  if (!AT_SendCommand(cmd, "OK", 5000)) {
    printf("Failed to acquire MQTT client\r\n");
    return false;
  }

  HAL_Delay(500);

  // Set SSL context for MQTT (after acquiring client)
  if (!AT_SendCommand("AT+CMQTTSSLCFG=0,0\r\n", "OK", 2000)) {
    printf("Failed to set MQTT SSL config\r\n");
    return false;
  }
  HAL_Delay(500);

  // Enable MQTT EX interface (allows topic in publish command)
  if (!AT_SendCommand("AT+CMQTTCFG=\"argtopic\",0,1,1\r\n", "OK", 2000)) {
    printf("Failed to enable argtopic\r\n");
    return false;
  }

  HAL_Delay(500);

  if (current_config.username && current_config.password) {
    snprintf(cmd, sizeof(cmd),
             "AT+CMQTTCONNECT=0,\"%s\",%d,1,\"%s\",\"%s\"\r\n",
             current_config.broker_url, current_config.keepalive,
             current_config.username, current_config.password);
  } else {
    snprintf(cmd, sizeof(cmd), "AT+CMQTTCONNECT=0,\"%s\",%d,0\r\n",
             current_config.broker_url, current_config.keepalive);
  }

  if (!AT_SendCommand(cmd, "OK", 5000)) {
    printf("Failed to connect to MQTT broker\r\n");
    return false;
  }

  mqtt_connected = true;
  printf("Connected to MQTT broker\r\n");
  return true;
}

/**
 * @brief Publish a message to a topic
 */
bool MQTT_Publish(const char *topic, const char *message) {
  char cmd[256];

  if (!mqtt_connected) {
    printf("MQTT not connected\r\n");
    return false;
  }

  uint16_t msg_len = strlen(message);

  snprintf(cmd, sizeof(cmd), "AT+CMQTTPUB=0,\"%s\",0,%u,0\r\n", topic,
           strlen(message));

  // Send the publish command (expects ">" prompt)
  if (!AT_SendCommand(cmd, ">", 5000)) {
    printf("Failed to get prompt for message data\r\n");
    return false;
  }

  // Send the actual message data
  if (!AT_SendData((const uint8_t *)message, strlen(message), "OK", 5000)) {
    printf("Failed to send message data\r\n");
    return false;
  }

  printf("Message published successfully\r\n");
  return true;
}

/**
 * @brief Disconnect from MQTT broker
 */
bool MQTT_Disconnect(void) {
  if (!mqtt_connected) {
    return true; // Already disconnected
  }

  printf("Disconnecting from MQTT broker...\r\n");

  if (!AT_SendCommand("AT+CMQTTDISC=0,60\r\n", "OK", 10000)) {
    printf("Failed to disconnect from MQTT broker\r\n");
    return false;
  }

  mqtt_connected = false;
  printf("Disconnected from MQTT broker\r\n");
  return true;
}

/**
 * @brief Upload string to MQTT server (convenience function)
 */
bool MQTT_UploadString(const char *broker_url, const char *client_id,
                       const char *username, const char *password,
                       const char *topic, const char *message,
                       const char *ca_cert) {
  bool success = false;

  printf("=== MQTT Upload String ===\r\n");
  printf("Broker: %s\r\n", broker_url);
  printf("Topic: %s\r\n", topic);
  printf("Message length: %d bytes\r\n", strlen(message));

  // Configure MQTT
  MQTT_Config_t config = {.broker_url = broker_url,
                          .client_id = client_id,
                          .username = username,
                          .password = password,
                          .keepalive = 60,
                          .ca_cert = ca_cert};

  // Initialize
  if (!MQTT_Init(&config)) {
    printf("MQTT init failed\r\n");
    goto cleanup;
  }

  // Connect
  if (!MQTT_Connect()) {
    printf("MQTT connect failed\r\n");
    goto cleanup;
  }

  // Publish
  if (!MQTT_Publish(topic, message)) {
    printf("MQTT publish failed\r\n");
    goto cleanup;
  }

  success = true;
  printf("=== Upload Successful ===\r\n");

cleanup:
  // Always try to disconnect
  MQTT_Disconnect();
  return success;
}
