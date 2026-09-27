#include "send_task.h"
#include "FreeRTOS.h"
#include "at_commands.h"
#include "cmsis_os2.h"
#include "main.h"
#include "mqtt.h"
#include "queue.h"
#include "radio.h"
#include "sample_task.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

// Format the sensor data into a JSON message
static void FormatMessage(char *buf, size_t buf_size, int value,
                          float battery_voltage) {
  snprintf(buf, buf_size, "{\"val\":%d,\"temp\":%.3f,\"bat\":%.3f,\"cnt\":%lu}",
           value, 23.5, battery_voltage, Radio_GetInitCount());
}

// Publish one sample over MQTT
static bool PublishSample(const Sample_t *sample) {
  char message[128];
  FormatMessage(message, sizeof(message), sample->sensor_value,
                sample->battery_voltage);

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

// Wakes the cellular module and connects once, then drains sample_queue
// sample by sample: peek, publish, and only remove from the queue on a
// successful publish. On any error, stops draining (without losing the
// unsent sample) and waits for the next backlog notification.
static void SendTask(void *argument) {
  (void)argument;

  Radio_Init();
  MQTT_Init();

  printf("Send task started\r\n");

  for (;;) {
    Radio_Sleep();
    HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);
    osThreadFlagsWait(SEND_TASK_FLAG_PENDING, osFlagsWaitAny, osWaitForever);

    printf("Trying to wakeup module\r\n");
    if (!WakeupModule()) {
      Radio_Init();
      MQTT_Init();
      HAL_Delay(5000);
      continue;
    }
    HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);

    if (!ConnectToBroker()) {
      Radio_Sleep();
      continue;
    }

    HAL_Delay(500);

    Sample_t sample;
    while (xQueuePeek((QueueHandle_t)sample_queue, &sample, 0) == pdTRUE) {
      if (!PublishSample(&sample))
        break;

      // Only remove the sample now that it's been sent successfully
      osMessageQueueGet(sample_queue, &sample, NULL, 0);
    }

    MQTT_Disconnect();
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
