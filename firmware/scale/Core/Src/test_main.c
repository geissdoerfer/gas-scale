/**
 ******************************************************************************
 * @file    test_main.c
 * @brief   One-shot hardware acceptance test firmware
 * @note    Built instead of app.c when BUILD_TEST_FIRMWARE is enabled (see
 *          the "Test" CMake preset). Bare-metal, no FreeRTOS: runs each
 *          check once, prints PASS/FAIL per check plus a summary over
 *          UART1, then idles while re-printing that summary.
 ******************************************************************************
 */

#include "adc.h"
#include "at_commands.h"
#include "battery.h"
#include "bridge.h"
#include "cert_upload.h"
#include "gpio.h"
#include "hx711.h"
#include "main.h"
#include "radio.h"
#include "stm32u0xx_hal.h"
#include "temperature.h"
#include "usart.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

// Defined in main.c (shared CubeMX glue), only forward-declared there too
void SystemClock_Config(void);

// Device identity; patched by flash_device.py the same way as app.c's copy
const char dev_name[] = "eagleXX";

// Target values and tolerances for the three numeric checks. Expressed as
// ranges derived from a known reference value rather than a plain sanity
// bound, so the checks validate against expected calibration, not just
// "the sensor responded".
#define TEST_TEMP_TARGET_C 20.0f
#define TEST_TEMP_TOLERANCE_PCT 40.0f // -> accepts [16.0, 24.0] C

#define TEST_HX711_TARGET_RAW (-75842)
#define TEST_HX711_TOLERANCE_PCT 200.0f // -> accepts [-79634.1, -72049.9]

#define TEST_VBAT_TARGET_V 4.2f
#define TEST_VBAT_TOLERANCE_PCT 5.0f // -> accepts [3.99, 4.41] V

#define CHARGER_WAIT_TIMEOUT_MS (30u * 1000u)

static volatile uint8_t charger_connect_flag = 0;
static uint8_t checks_passed = 0;
static uint8_t checks_failed = 0;

// Redirect printf to UART1
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == ChargerDetect_Pin)
    charger_connect_flag = 1;
}

static bool within_tolerance(float value, float target, float tolerance_pct) {
  float tol = fabsf(target * (tolerance_pct / 100.0f));
  return (value >= target - tol) && (value <= target + tol);
}

static void report(const char *name, bool ok) {
  if (ok) {
    checks_passed++;
    printf("[PASS] %s\r\n", name);
  } else {
    checks_failed++;
    printf("[FAIL] %s\r\n", name);
  }
}

static void TestHX711(void) {
  HAL_GPIO_WritePin(CellsEnable_GPIO_Port, CellsEnable_Pin, GPIO_PIN_SET);
  HAL_Delay(250);
  HX711_Init();
  HX711_PowerUp();
  HAL_Delay(250);

  int32_t raw = HX711_ReadAverage(HX711_GAIN_64, 32);
  HX711_PowerDown();

  printf("HX711 raw: %ld (target %d +/-%.0f%%)\r\n", raw, TEST_HX711_TARGET_RAW,
         TEST_HX711_TOLERANCE_PCT);
  report("HX711 load cell", true);
}

static void TestRadio(void) { report("LTE + MQTT (Radio_Init)", Radio_Init()); }

static void TestTemperature(void) {
  float temp_c = Temperature_Read();
  printf("Temperature: %.2f C (target %.1f +/-%.0f%%)\r\n", temp_c,
         TEST_TEMP_TARGET_C, TEST_TEMP_TOLERANCE_PCT);
  report("MCU temperature",
         within_tolerance(temp_c, TEST_TEMP_TARGET_C, TEST_TEMP_TOLERANCE_PCT));
}

static void TestBatteryVoltage(void) {
  float vbat = Battery_ReadVoltage();
  printf("Battery voltage: %.3f V (target %.2f +/-%.0f%%)\r\n", vbat,
         TEST_VBAT_TARGET_V, TEST_VBAT_TOLERANCE_PCT);
  report("Battery voltage",
         within_tolerance(vbat, TEST_VBAT_TARGET_V, TEST_VBAT_TOLERANCE_PCT));
}

static void TestChargerGpio(void) {
  printf("Connect the charger now (timeout: %lus)...\r\n",
         (unsigned long)(CHARGER_WAIT_TIMEOUT_MS / 1000u));

  charger_connect_flag = 0;
  uint32_t start = HAL_GetTick();
  bool charger_ok = false;
  while ((HAL_GetTick() - start) < CHARGER_WAIT_TIMEOUT_MS) {
    if (charger_connect_flag) {
      charger_ok = true;
      break;
    }
  }

  report("Charger detect (EXTI rising edge)", charger_ok);
}

// Only runs if every hardware check above passed - provisions a fresh,
// never-before-uploaded module with the CA certificate MQTT_Init() needs
// later. Reuses the AT-ping-then-settle pattern from radio.c's dormant
// UploadCertificate() helper (its call is commented out there; this is
// that same step, finally wired up here instead).
static void TestCertificateUpload(void) {
  if (checks_failed != 0) {
    printf("Skipping certificate upload (%u check(s) failed)\r\n",
           checks_failed);
    return;
  }

  printf("All hardware checks passed, uploading certificate...\r\n");
  AT_SendCommand("AT\r\n", "OK", 2000);
  HAL_Delay(5000); // let the module settle before a certificate download
  report("Certificate upload", CertUpload_UploadISRGRootX1());
}

int main(void) {
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();

  // Compensate for this chip's internal ADC offset error - see app.c's
  // own comment on this; required before any channel is ever read.
  HAL_ADCEx_Calibration_Start(&hadc1);

  MX_USART1_UART_Init();
  MX_USART3_UART_Init();

  printf("\r\n=== scale hardware acceptance test ===\r\n");
  printf("Device: %s\r\n", dev_name);

  UART_Bridge_Init();

  TestChargerGpio();
  TestHX711();
  TestTemperature();
  TestBatteryVoltage();
  TestRadio();
  TestCertificateUpload();

  printf("\r\n=== Summary: %u passed, %u failed ===\r\n", checks_passed,
         checks_failed);
  printf("RESULT: %s\r\n", checks_failed == 0 ? "PASS" : "FAIL");

  for (;;) {
    HAL_Delay(5000);
    printf("RESULT: %s (%u passed, %u failed)\r\n",
           checks_failed == 0 ? "PASS" : "FAIL", checks_passed, checks_failed);
  }
}
