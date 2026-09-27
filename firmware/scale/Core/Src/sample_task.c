#include "sample_task.h"
#include "battery.h"
#include "hx711.h"
#include "main.h"
#include "send_task.h"
#include <stdio.h>

#define SAMPLE_QUEUE_LENGTH 8
#define SAMPLE_TASK_FLAG_RTC_TICK 0x01u

// Once the backlog exceeds this many samples, SendTask is worth waking
#define SEND_BACKLOG_THRESHOLD 2

osMessageQueueId_t sample_queue;
static osThreadId_t sample_task_handle;

// Blocks on the RTC tick notification, samples the sensor and battery
// voltage, and pushes the result onto sample_queue
static void SampleTask(void *argument) {
  (void)argument;

  // Initialize HX711
  HAL_GPIO_WritePin(CellsEnable_GPIO_Port, CellsEnable_Pin, GPIO_PIN_SET);
  HX711_Init();

  printf("Sample task started\r\n");

  for (;;) {
    osDelay(1 * 30 * 1000);

    HX711_PowerUp();
    Sample_t sample = {
        .battery_voltage = Battery_ReadVoltage(),
        .sensor_value = HX711_ReadAverage(HX711_GAIN_64, 32),
        .timestamp = osKernelGetTickCount(),
    };
    HX711_PowerDown();
    printf("Sample taken(@%lu): %.3f %ld\r\n", sample.timestamp,
           sample.battery_voltage, sample.sensor_value);
    if (osMessageQueuePut(sample_queue, &sample, 0, 0) != osOK) {
      printf("Sample queue full, dropping sample\r\n");
    }
  }
}

void StartSampleTask(void) {
  sample_queue = osMessageQueueNew(SAMPLE_QUEUE_LENGTH, sizeof(Sample_t), NULL);

  const osThreadAttr_t sample_task_attributes = {
      .name = "SampleTask",
      .priority = (osPriority_t)osPriorityNormal,
      .stack_size = 4096,
  };
  sample_task_handle = osThreadNew(SampleTask, NULL, &sample_task_attributes);
}

void SampleTask_NotifyTick(void) {
  if (sample_task_handle != NULL)
    osThreadFlagsSet(sample_task_handle, SAMPLE_TASK_FLAG_RTC_TICK);
}
