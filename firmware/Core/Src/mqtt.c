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
bool MQTT_Init(void) {
  printf("Initializing MQTT...\r\n");

  // Store configuration
  current_config = (MQTT_Config_t){
      .broker_url =
          "tcp://f85c8d608a674db6881108d544f12896.s1.eu.hivemq.cloud:8883",
      .client_id = "ClientID",
      .username = "nessie",
      .password = "D-sub729",
      .keepalive = 60,
      .ca_cert = "isrgrootx1.pem"};

  // Sync module clock (needed for TLS certificate validation)
  AT_SendCommand("AT+CNTP\r\n", "OK", 2000);
  HAL_Delay(2000);
  AT_SendCommand("AT+CCLK?\r\n", NULL, 0);

  // Configure SSL BEFORE starting MQTT service
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
  if (current_config.ca_cert) {
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"cacert\",0,\"%s\"\r\n",
             current_config.ca_cert);
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

  HAL_Delay(1000);

  // Acquire MQTT client (index 0, with or without SSL)
  snprintf(cmd, sizeof(cmd), "AT+CMQTTACCQ=0,\"%s\",1\r\n", "ClientID");
  if (!AT_SendCommand(cmd, "OK", 2000)) {
    printf("Failed to acquire MQTT client\r\n");
    HAL_Delay(2000);
    if (!AT_SendCommand("AT+CMQTTREL=0\r\n", "OK", 2000)) {
      printf("Failed to release MQTT client\r\n");
    }
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
    HAL_Delay(2000);
    if (!AT_SendCommand("AT+CMQTTREL=0\r\n", "OK", 2000)) {
      printf("Failed to release MQTT client\r\n");
    }
    return false;
  }

  printf("Connected to MQTT broker\r\n");
  mqtt_connected = true;
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
