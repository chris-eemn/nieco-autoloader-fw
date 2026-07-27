/**
 * @file caldata.c
 * @author aanderson
 * @brief caldata for storing control loop and transfer function parameters
 *
 * @copyright Copyright (c) 2024 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "caldata.h"
#include "w25q.h"
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "definitions.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define DATA_VALID_MAGIC_NUMBER (0xef5019ab)
#define CAL_DATA_START_ADDRESS (W25Q_SECTOR_SIZE*0x10)
#define CAL_DATA_SECTOR_AMOUNT ((sizeof(nuvu_caldata_t)/W25Q_SECTOR_SIZE)+1)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

static SemaphoreHandle_t  cal_buffer_semphr = 0;
static bool caldata_modified = false;

/**
 * @brief Default calibration data values.
 *
 * This array stores the default calibration data values for various parameters.
 * The values are initialized in the order of the enumeration `CAL_PARAM_ID`.
 * The array size is `CAL_NUM_PARAMS`.
 */

/**
 * @brief The cal_param_strings array is a static array of strings that represents the calibration parameter names.
 * Each string corresponds to a specific calibration parameter.
 * The array is initialized with the names of the calibration parameters in the order they are defined.
 * The last element of the array is set to NULL to indicate the end of the array.
 */

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
nuvu_caldata_t default_cals = {
    .version = CAL_DATA_VERSION_VAL,
    .magic_number = DATA_VALID_MAGIC_NUMBER,
    .oven_hyst_low = 0.9f,
    .oven_hyst_high = 0.0f,
    .proofer_hyst_low = 3.0f,
    .proofer_hyst_high = 0.0f,
    .humidity_hyst_low = 1.0f,
    .humidity_hyst_high = 1.0f,
    .dryout_time_s = 45*60,
    .dryout_enabled = true,
    .cooldown_enabled = true,
    .thermocouple_offsets = {0,0},
    .humidity_offset = 0,
    .moister_on_time = 200,
};

nuvu_caldata_t caldata;

static cal_param_t cal_params[] = {
    {.name="oven_hyst_low",.type=CAL_F32,.data=&( caldata.oven_hyst_low )},
    {.name="oven_hyst_high",.type=CAL_F32,.data=&( caldata.oven_hyst_high )},
    {.name="proofer_hyst_low",.type=CAL_F32,.data=&( caldata.proofer_hyst_low )},
    {.name="proofer_hyst_low",.type=CAL_F32,.data=&caldata.proofer_hyst_high},
    {.name="humidity_hyst_high",.type=CAL_F32,.data=&caldata.proofer_hyst_low},
    {.name="humidity_hyst_high",.type=CAL_F32,.data=&caldata.humidity_hyst_high},
    {.name="dryout_time_s",.type=CAL_UINT,.data=&caldata.dryout_time_s},
    {.name="dryout_enabled", .type=CAL_INT, .data=&caldata.dryout_enabled},
    {.name="cooldown_enabled", .type=CAL_INT, .data=&caldata.cooldown_enabled},
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void save_caldata(void);
static void read_caldata(void);
static bool caldata_is_valid(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
nuvu_caldata_t *caldata_get(void)
{
    return &caldata;
}

/**
 * Initializes the NV storage. If no storage is available or it is corrupt, default settings will be loaded
 */
void caldata_init(void)
{
    w25q_initialize();
    read_caldata();
    cal_buffer_semphr = xSemaphoreCreateMutex();
    if(!caldata_is_valid()) {
        caldata_set_default_data();
    }
}

void caldata_save(void)
{
    save_caldata();
    caldata_modified = false;
}

bool caldata_take_semphr(void)
{
    bool ret;
    if(0 == cal_buffer_semphr){
        ret = true;
    } else if(xPortIsInsideInterrupt()){
        ret = xSemaphoreTakeFromISR(cal_buffer_semphr, 0);
    } else {
        ret = xSemaphoreTake(cal_buffer_semphr, portMAX_DELAY);
    }
    return ret; 
}

void caldata_give_semphr(void)
{
    if(0 == cal_buffer_semphr){
        //do nothing
    } else if (xPortIsInsideInterrupt()){
       xSemaphoreGiveFromISR(cal_buffer_semphr, 0); 
    } else {
        xSemaphoreGive(cal_buffer_semphr);
    }
}

cal_param_t *get_cal_param(char *param_name)
{
    for(int i = 0; i<caldata_get_num_params(); i++){
        if(!strcmp(param_name, cal_params[i].name)){
            return &(cal_params[i]);
        }
    }
    return 0;
}

cal_param_t *caldata_get_array(void)
{
    return &(cal_params[0]);
}
static int num_params;
int caldata_get_num_params(void)
{
    num_params = sizeof(cal_params)/sizeof(cal_param_t);
    return sizeof(cal_params)/sizeof(cal_param_t);
}

void caldata_set_default_data(void)
{
    caldata = default_cals;
    caldata_save();
}

void set_caldata_modified(void)
{
    caldata_modified = true;
}

bool get_caldata_modified(void)
{
    return caldata_modified;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
/**
 * Checks NV for a specific magic number in index 0
 * @return Whether the magic number exists, indicating that the data is good
 */
static bool caldata_is_valid(void)
{
    bool magic_number_valid = (caldata.magic_number == DATA_VALID_MAGIC_NUMBER);
    bool version_valid = (caldata.version == CAL_DATA_VERSION_VAL);

    if(!version_valid){
        SYS_CONSOLE_PRINT("INFO ==> CAL data version mismatch, using factory defaults...\r\n");
    }
    if(!magic_number_valid){
        SYS_CONSOLE_PRINT("INFO ==> Invalid CAL data memory, using factory defaults...\r\n");
    }
    return (magic_number_valid && version_valid);
}

/**
 * Sets default values and saves them to NV memory
 */

/**
 * Saves the calibration parameters to memory
 */
static void save_caldata(void)
{
    uint32_t erase_address = CAL_DATA_START_ADDRESS;
    for(int i = 0; i<(int)CAL_DATA_SECTOR_AMOUNT; i++){
        w25q_sector_erase(erase_address);
        while(W25Q_TRANSFER_STATUS_BUSY ==  w25q_get_transfer_status());
        erase_address+=W25Q_SECTOR_SIZE;
    }
    w25q_write((uint8_t*)&caldata, sizeof(nuvu_caldata_t), CAL_DATA_START_ADDRESS);
    while(W25Q_TRANSFER_STATUS_BUSY ==  w25q_get_transfer_status());
}

static void read_caldata(void)
{
    w25q_read((uint8_t*)&caldata, sizeof(nuvu_caldata_t), CAL_DATA_START_ADDRESS);
    while(W25Q_TRANSFER_STATUS_BUSY ==  w25q_get_transfer_status());
}
