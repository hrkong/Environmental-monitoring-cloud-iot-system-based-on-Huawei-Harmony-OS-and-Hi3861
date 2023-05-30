/*
 * Copyright (c) 2020 HiHope Community.
 * Description:IoT platform
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */
#ifndef IOT_PROFILE_H_
#define IOT_PROFILE_H_
#include <app_demo_multi_sample.h>

typedef hi_void (*oc_clfl_call_back_func) (hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
typedef hi_void (*oc_trfl_call_back_func) (hi_traffic_light_mode current_mode, hi_control_mode_type current_type);
typedef hi_void (*oc_env_call_back_func) (hi_environment_mode current_mode, hi_control_mode_type current_type);

#define OC_BEEP_STATUS_ON       ((hi_u8) 0x01)
#define OC_BEEP_STATUS_OFF      ((hi_u8) 0x00)

////< enum all the data type for the oc profile
typedef enum
{
    EN_IOT_DATATYPE_INT = 0,
    EN_IOT_DATATYPE_LONG,
    EN_IOT_DATATYPE_FLOAT,
    EN_IOT_DATATYPE_DOUBLE,
    EN_IOT_DATATYPE_STRING,           ///< must be ended with '\0'
    EN_IOT_DATATYPE_LAST,
}IoTDataType_t;

typedef enum {
    OC_LED_ON =1,
    OC_LED_OFF
}oc_led_value;

typedef struct {
    hi_colorful_light_mode          *colorful_light_module;
    void                            *colorful_device_online_time;
    void                            *clf_red_value;
    void                            *clf_green_value;
    void                            *clf_blue_value;
    void                            *human_test_module_value;
    void                            *human_test_module_time_count;
}oc_colorful_light_def;

typedef struct {
    hi_traffic_light_mode           *traffic_light_module;
    void                            *traffic_device_online_time;
    void                            *trf_red_value;
    void                            *trf_yellow_value;
    void                            *trf_green_value;
}oc_traffic_light_def;

typedef struct {
    hi_environment_mode             *environment_module;
    void                            *env_device_online_time;
    void                            *env_temperrature_value;
    void                            *env_humidity_value;
}oc_environment_def;

typedef struct
{
    void                            *nxt;   ///< ponit to the next key
    const char                      *key;
    const char                      *value;
    hi_u32                          i_value;
    hi_float                       oc_evvironment_value;
    IoTDataType_t                   type;
}IoTProfileKV_t;

typedef struct
{
   void *nxt;
   char *serviceID;                  ///< the service id in the profile, which could not be NULL
   char *eventTime;                  ///< eventtime, which could be NULL means use the platform time
   IoTProfileKV_t *serviceProperty; ///< the property in the profile, which could not be NULL
}IoTProfileService_t;

typedef struct
{
    int  retCode;           ///< response code, 0 success while others failed
    const char   *respName; ///< response name
    const char   *requestID;///< specified by the message command
    IoTProfileKV_t  *paras;  ///< the command paras
}IoTCmdResp_t;
/**
 * Use this function to make the command response here
 * and you must supplied the device id, and the payload defines as IoTCmdResp_t
 * 
*/
int IoTProfileCmdResp(const char *deviceid,IoTCmdResp_t *payload);

/**
 * use this function to report the property to the iot platform
 * 
*/
int IoTProfilePropertyReport(char *deviceID,IoTProfileService_t *payload);
/**/

hi_void setup_trfl_control_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_trfl_auto_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_trfl_human_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clean_trfl_status(hi_traffic_light_mode early_mode);
hi_void oc_traffic_light_status_report(hi_traffic_light_mode current_mode, oc_trfl_call_back_func msg_report);
hi_void oc_environment_status_report(hi_environment_mode current_mode, oc_env_call_back_func msg_report);
hi_void setup_env_combustible_gas_module(hi_environment_mode current_mode, hi_control_mode_type current_type);
hi_void setup_env_humidity_module(hi_environment_mode current_mode, hi_control_mode_type current_type);
hi_void setup_env_temperature_module(hi_environment_mode current_mode, hi_control_mode_type current_type);
hi_void setup_env_all_module(hi_environment_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clfl_control_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clfl_colorful_light_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clfl_pwm_control_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clfl_brightness_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clfl_human_detect_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clfl_light_detect_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
hi_void setup_clfl_union_detect_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type);
hi_void oc_colorful_light_status_report(hi_colorful_light_mode current_mode,oc_clfl_call_back_func msg_report);
hi_void report_led_light_time_count(hi_void);
#endif