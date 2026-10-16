#include "send_task.h"
#include "FreeRTOS.h"
#include "at_commands.h"
#include "bridge.h"
#include "cmsis_os2.h"
#include "main.h"
#include "mqtt.h"
#include "queue.h"
#include "radio.h"
#include "sample_task.h"
#include "watchdog_task.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SEND_PERIOD_SECONDS (10 * 60)

// Defined in app.c; also used to build the MQTT topic
extern const char dev_name[];

#define SEND_TASK_FLAG_PENDING 0x01u

static osThreadId_t send_task_handle;

// Send AT and wait for OK, retrying a few times to wake up the module
static bool WakeupModule(void) {
  for (unsigned int i = 0; i < 3; i++) {
    if (AT_SendCommand("AT\r\n", "OK", 2000))
      return true;
    printf("Didn't receive OK from module (%u)\r\n", i);
  }
  return false;
}

// Connect to the MQTT broker, retrying a few times on failure
static bool ConnectToBroker(void) {
  for (unsigned int i = 0; i < 3; i++) {
    if (MQTT_Connect())
      return true;
    printf("MQTT connect failed (%u)\r\n", i);
  }
  return false;
}

// Format the sensor data into a JSON message. age_ms is how long ago
// (relative to now, i.e. time of transmission) the sample was taken.
static void FormatMessage(char *buf, size_t buf_size, int value,
                          float battery_voltage, float temperature,
                          uint32_t age_ms) {
  snprintf(buf, buf_size,
           "{\"val\":%d,\"temp\":%.3f,\"bat\":%.3f,\"cnt\":%lu,\"age_ms\":%lu}",
           value, temperature, battery_voltage, Radio_GetInitCount(), age_ms);
}

// Publish one sample over MQTT
static bool PublishSample(const Sample_t *sample) {
  uint32_t age_ms = osKernelGetTickCount() - sample->timestamp;

  char message[128];
  FormatMessage(message, sizeof(message), sample->sensor_value,
                sample->battery_voltage, sample->temperature, age_ms);

  char topic[64];
  snprintf(topic, sizeof(topic), "sensors/%s/data", dev_name);

  if (!MQTT_Publish(topic, message)) {
    printf("MQTT publish failed\r\n");
    return false;
  }

  HAL_Delay(500);
  printf("Published weight: %ld\r\n", sample->sensor_value);
  return true;
}

static bool DrainSampleQueue(void) {
  Sample_t sample;
  while (xQueuePeek((QueueHandle_t)sample_queue, &sample, 0) == pdTRUE) {
    osDelay(250);
    if (!PublishSample(&sample)) {
      return false;
    }
    // Only remove the sample now that it's been sent successfully
    osMessageQueueGet(sample_queue, &sample, NULL, 0);
  }
  return true;
}

// Wakes the cellular module and connects once, then drains sample_queue.
// On any error, stops draining (without losing the unsent sample) and
// waits for the next backlog notification.
static void SendTask(void *argument) {
  (void)argument;

  // Arms USART3 RX for the modem link - only meaningful here, since this
  // task is the only thing that drives the radio. Owning it here (same
  // idea as SampleTask owning HX711_Init) means disabling this task also
  // disables the bridge, instead of needing both toggled in sync by hand.
  UART_Bridge_Init();

  printf("Send task started\r\n");
  while (1) {
    /* If Init fails, power off and wait 10 minutes */
    if (!Radio_Init()) {
      printf("Radio Init failed. Power off and restart in 10min\r\n");
      Radio_PowerOff();
      osDelay(10 * 60 * 1000);
      continue;
    }
    osDelay(1000);
    MQTT_Init();
    osDelay(1000);

    for (;;) {
      osDelay(500);
      Radio_Sleep();
      printf("Radio sleep mode activated\r\n");
      HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

      osDelay(SEND_PERIOD_SECONDS * 1000);

      printf("Trying to wakeup module\r\n");
      if (!WakeupModule()) {
        printf("Wakeup Radio failed. Power off and restart in 5min\r\n");

        /* On failure power off and wait five minutes */
        Radio_PowerOff();
        osDelay(5 * 60 * 1000);
        break;
      }
      osDelay(500);
      HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);

      if (!MQTT_Connect()) {
        printf("MQTT Connect failed. Power off and restart in 5min\r\n");
        /* On failure power off and wait five minutes */
        Radio_PowerOff();
        osDelay(5 * 60 * 1000);
        break;
      }

      if (!DrainSampleQueue()) {
        printf("Publish failed. Power off and restart in 5min\r\n");

        /* On failure power off and wait five minutes */
        Radio_PowerOff();
        osDelay(5 * 60 * 1000);
      }

      Watchdog_ClearStaleCounter();
      MQTT_Disconnect();
    }
  }
}

void StartSendTask(void) {
  const osThreadAttr_t send_task_attributes = {
      .name = "SendTask",
      .priority = (osPriority_t)osPriorityNormal,
      .stack_size = 4096,
  };
  send_task_handle = osThreadNew(SendTask, NULL, &send_task_attributes);
}

void SendTask_NotifyPending(void) {
  if (send_task_handle != NULL)
    osThreadFlagsSet(send_task_handle, SEND_TASK_FLAG_PENDING);
}
