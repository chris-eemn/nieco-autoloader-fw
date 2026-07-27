/* 
 * File:   caldata.h
 * Author: DevonShustarich
 *
 * Created on March 27, 2024, 2:52 PM
 */

#ifndef CALDATA_H
#define	CALDATA_H

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef enum {
  CAL_OVEN_HYST_LOW,
  CAL_OVEN_HYST_HIGH,
  CAL_PROOFER_HYST_LOW,
  CAL_PROOFER_HYST_HIGH,
  CAL_HUMIDITY_HYST_LOW,
  CAL_HUMIDITY_HYST_HIGH,
  CAL_DRYOUT_TIME_S,
  CAL_NUM_PARAMS                /**< Number of calibration parameters */
} cal_param_enum;

typedef enum {
    CAL_UINT,
    CAL_INT,
    CAL_F32,
    CAL_STRING,
}cal_param_type_enum;

typedef struct{
    char* name;
    cal_param_type_enum type;
    void *data;
}cal_param_t;

typedef struct{
    uint32_t version;
    uint32_t magic_number;
    float oven_hyst_low;
    float oven_hyst_high;
    float proofer_hyst_low;
    float proofer_hyst_high;
    float humidity_hyst_low;
    float humidity_hyst_high;
    uint32_t dryout_time_s; 
    int dryout_enabled;
    int cooldown_enabled;
    int thermocouple_offsets[2];
    int temp_sensor_offsets[8];
    int humidity_offset;
    int moister_on_time;
    int moister_limp_on_time;
}nuvu_caldata_t;



#define CAL_DATA_VERSION_VAL 10

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

nuvu_caldata_t *caldata_get(void);
int caldata_get_num_params(void);

void caldata_save(void);
void caldata_init(void);
void caldata_set_default_data(void);

//works in ISR context as well as non-ISR
bool caldata_take_semphr(void);
void caldata_give_semphr(void);

cal_param_t *get_cal_param(char *param_name);
cal_param_t *caldata_get_array(void);

void set_caldata_modified(void);
bool get_caldata_modified(void);

#endif /* CALDATA_H_ */
