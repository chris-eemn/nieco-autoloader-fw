/**
 * @file ext_adc_i2c_service.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief MCP342x thermocouple temperature sensing service on I2C1.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "ext_adc_i2c_service.h"

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "app_console.h"
#include "app_task.h"
#include "autoloader_types.h"
#include "cal_data.h"
#include "mcp342x.h"
#include "task.h"
#include "temp_layer.h"
#include "thermocouple.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/* Fallback over-temp threshold in whole degrees F when cal-data is
 * unavailable. The live threshold comes from cal_data_params_t.temp_max_f. */
#define TEMP_MAX_F_DEFAULT (30)

/* Number of thermocouple channels wired to the MCP342x. */
#define TEMP_CHANNEL_COUNT (2U)

/* Largest magnitude (in hundredths of a degree) the fixed-point temp
 * printer will display. Guards against negating INT32_MIN. */
#define TEMP_PRINT_MAX_HUNDREDTHS (99999999L)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
/* Table 5-3 of the MCP342x datasheet. */
static mcp342x_t adc1 = {.address = MCP342x_DEVICE_CODE | 0b100};

static mcp342x_config_reg_t adc1_configs[TEMP_CHANNEL_COUNT] = {
    {
        .ch = MCP342x_CHANNEL_1,
        .oc = MCP342x_ONESHOT,
        .pga = MCP342x_PGA_1X,
        .res = MCP342x_18_BIT_SAMPLE,
    },
    {
        .ch = MCP342x_CHANNEL_2,
        .oc = MCP342x_ONESHOT,
        .pga = MCP342x_PGA_1X,
        .res = MCP342x_18_BIT_SAMPLE,
    },
};

/* Round-robin index into adc1_configs[]; also the index used for
 * temp_layer and s_over_limit. */
static uint8_t adc1_idx = 0;

/* Over-limit edge state per channel table index. The fault event is
 * posted only on the healthy -> over transition. */
static bool s_over_limit[TEMP_CHANNEL_COUNT];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void ext_adc_i2c_task(void *pvParams);
static void ext_adc_print_over_limit(uint8_t idx, double temp_c);
static double ext_adc_get_max_temp_c(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
void ext_adc_i2c_init(void) {
  xTaskCreate(ext_adc_i2c_task, "TempSense", 1024, NULL, 2, 0);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
/**
 * @brief Print an over-limit notice for one channel.
 *
 * The project links newlib-nano without float printf support, so the
 * value is converted to hundredths and printed with integer formats.
 *
 * @param idx    Channel table index (0-based).
 * @param temp_c Temperature in degrees Celsius.
 */
static void ext_adc_print_over_limit(uint8_t idx, double temp_c) {
  /* Clamp before negating: an out-of-range double->int32_t cast can yield
   * INT32_MIN, whose negation overflows. */
  int32_t h = (int32_t)(temp_c * 100.0);

  if (h > TEMP_PRINT_MAX_HUNDREDTHS) {
    h = TEMP_PRINT_MAX_HUNDREDTHS;
  }
  else if (h < -TEMP_PRINT_MAX_HUNDREDTHS) {
    h = -TEMP_PRINT_MAX_HUNDREDTHS;
  }

  bool    neg = (h < 0);
  int32_t ah  = (neg ? -h : h);

  app_console_print("[TEMP] CH%u over limit (%s%ld.%02ld C)\r\n", (unsigned int)(idx + 1U), (neg ? "-" : ""), (long)(ah / 100),
                    (long)(ah % 100));
}

/**
 * @brief Read the over-temp fault threshold from cal-data, in degrees C.
 *
 * The stored threshold is whole degrees F; it is converted to C here so the
 * comparison against the (Celsius) thermocouple reading stays in one unit.
 * cal_data_get() never returns NULL per its contract; the NULL check is a
 * guard that falls back to the compile-time default.
 *
 * @return double Threshold in degrees Celsius.
 */
static double ext_adc_get_max_temp_c(void) {
  const cal_data_params_t *params = cal_data_get();
  uint32_t max_f = TEMP_MAX_F_DEFAULT;

  if (params != NULL) {
    max_f = params->temp_max_f;
  }

  return (double)temp_layer_f_to_c((int16_t)max_f);
}

/**
 * @brief FreeRTOS task: polls the MCP342x round-robin and posts an over-temp
 * fault on each healthy -> over transition.
 * @param pvParams Unused.
 */
static void ext_adc_i2c_task(void *pvParams) {
  (void)pvParams;
  float reading_v;

  /* I2C1 boot quiet period: PCA9538A init in stepper_system_init() is a
   * one-shot during bringup; stay off the bus until it is long finished. */
  vTaskDelay(pdMS_TO_TICKS(5000U));

  /* Latch address pins on all MCP342x devices. */
  mcp342x_gen_call_latch(&adc1);
  /* Allow 10 ms for the latch to take effect. */
  vTaskDelay(pdMS_TO_TICKS(10U));

  mcp342x_init(&adc1, adc1_configs[0]);
  mcp342x_start_conversion(&adc1, adc1_configs[0].ch);
  vTaskDelay(pdMS_TO_TICKS(1000U));

  for (;;) {
    if (mcp342x_get_reading(&adc1, &reading_v)) {
      temp_layer_set_sense_v(adc1_idx, reading_v);

      /* Convert voltage to mV, apply cold-junction compensation, then
       * convert the corrected EMF to temperature. */
      double emf_mV = (double)reading_v * 1000.0;
      double cjc = (double)temp_layer_get_cjc_temp_c();
      double emf_corrected = emf_mV - thermocouple_get_emf(THERMO_TYPE_J, cjc);
      double temp_c = thermocouple_get_temp(THERMO_TYPE_J, emf_corrected);

      temp_layer_set_temp_c(adc1_idx, (float)temp_c);

      /* Edge-triggered over-temp: post the fault only on the healthy ->
       * over transition so the queue and console do not flood. The
       * threshold is re-read each cycle so CLI/modbus changes apply live. */
      bool over = (temp_c > ext_adc_get_max_temp_c());
      if ((over == true) && (s_over_limit[adc1_idx] == false)) {
        ext_adc_print_over_limit(adc1_idx, temp_c);

        app_event_t event = {
            .id = APP_EV_FAULT,
            .slot = APP_NO_SLOT,
            .value = APP_FAULT_CODE_TEMP_HIGH,
        };
        /* If the queue is full the event is lost; keep the edge state
         * clear so the next reading retries the post. */
        if (app_task_post(&event) == true) {
          s_over_limit[adc1_idx] = true;
        }
      }
      else {
        s_over_limit[adc1_idx] = over;
      }

      adc1_idx = (uint8_t)((adc1_idx + 1U) % TEMP_CHANNEL_COUNT);
      mcp342x_write_config(&adc1, adc1_configs[adc1_idx]);
      mcp342x_start_conversion(&adc1, adc1_configs[adc1_idx].ch);
    }
    vTaskDelay(pdMS_TO_TICKS(100U));
  }
}
