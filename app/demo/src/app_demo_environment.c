/*
 * Copyright (c) 2020 HiHope Community.
 * Description: oled demo
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */
#include <hi_types_base.h>
#include <app_demo_multi_sample.h>
#include <app_demo_sgp30.h>
#include <hi_i2c.h>
#include <app_demo_environment.h>
#include <hi_time.h>
#include <hi_early_debug.h>
#include <hi_pwm.h>
#include "GPS.h"

extern hi_void get_aht20_sensor_datat(hi_void);
extern hi_u8  *flaot_to_string_gas(hi_double d, hi_u8 *str);
extern hi_u8  flaot1_to_string(hi_double d, hi_u8 *str );
extern hi_void get_sgp30_data(hi_void);
extern hi_u8 g_ahu20_humidity_buff[6];
extern hi_u8 g_ahu20_temperature_buff[6];
static hi_u8 hi3861_board_led_test =0;
extern hi_float aht_temper;
extern hi_float aht_humi;
extern hi_s32 sgp_co2;
extern hi_s32 sgp_tvoc;
extern hi_u8 g_sgp30_co2_buff[6];
extern hi_u8 g_sgp30_tvoc_buff[6];
extern altitude[100];
extern hi_u32 sgp30_check_and_init();
extern void GPS_Read_Data(GPS_Data_TypeDef *ReadData);
extern hi_u32 app_demo_beep_control_task(hi_void);

GPS_Data_TypeDef *GPSdata;
#define     BEEP_ON_OFF_DELAY_500MS             (500000)

hi_void beep_control1(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM0);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM0, PWM_DUTY, PWM_FULL_DUTY);
    hi_udelay(BEEP_ON_OFF_DELAY_500MS);
    hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
    hi_udelay(BEEP_ON_OFF_DELAY_500MS);
}

hi_void return_all_mode_enum_sample(hi_void)
{
    hi_u8 current_mode = g_current_mode;
    while (1) {      
        if (current_mode != g_current_mode) {
            break;
        }
        hi_sleep(SLEEP_1_MS);
    }
}
/*environment menu display*/
hi_void show_all_environment_value(hi_void)
{

    hi_u8 combustible_main_menu_gas_value_str[10]={0};
    hi_u8 temperature_str[6] ={0};
    hi_u8 humidity_str[6]={0};
    hi_u8 co2_str[6]={0};
    hi_u8 tvoc_str[6]={0};
    hi_u8 current_mode = g_current_mode;
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400000 */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    
    while(1) {
        //app_demo_beep_control_task();
        //beep_control1();
        // hi_udelay(DELAY_10_MS);//delay 10ms
        get_sgp30_data();
        //hi_udelay(DELAY_10_MS);
        get_aht20_sensor_datat();
        mq2_get_data();
        

        flaot_to_string(aht_temper, temperature_str);
        flaot_to_string(aht_humi, humidity_str);  
        flaot1_to_string(sgp_co2,co2_str);
        flaot1_to_string(sgp_tvoc,tvoc_str);
        flaot_to_string(g_combustible_gas_value, combustible_main_menu_gas_value_str); 
        //get_sgp30_data();
        oled_show_str(18,4,temperature_str,1);
        oled_show_str(81,4,humidity_str,1);  
        oled_show_str(55,6,co2_str,1);
        oled_show_str(55,7,tvoc_str,1);
        if (!g_combustible_gas_value)   {
             oled_show_str(55,5,"0.00    ",1);
        } else {
            oled_show_str(55,5,combustible_main_menu_gas_value_str,1);
        }      
        if (current_mode != g_current_mode) {
            break;
        }

        hi_sleep(SLEEP_10_MS);//10ms
    }
}
/* temperature menu display */
hi_void show_temperature_value(hi_void)
{
    hi_u8 current_mode = g_current_mode;
    hi_u8 temperature_str[6] ={0};
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400000 */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);

    while(1) {
        hi_udelay(DELAY_10_MS);//delay 10ms
        get_aht20_sensor_datat();
        flaot_to_string(aht_temper, temperature_str); 
        oled_show_str(40,5,temperature_str,1);
        if (current_mode != g_current_mode) {
            break;
        }

        hi_sleep(SLEEP_10_MS);//10ms
    }
}
/* humidity value display */
hi_void show_humidity_value(hi_void)
{
    hi_u8 current_mode = g_current_mode;
    hi_u8 humidity_str[6]={0};
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400000 */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    while(1) {
        hi_udelay(DELAY_10_MS);//delay 10ms
        get_aht20_sensor_datat();
        flaot_to_string(aht_humi, humidity_str);  
        oled_show_str(56,5,humidity_str,1);  
        if (current_mode != g_current_mode) {
            break;
        }

        hi_sleep(SLEEP_10_MS);//10ms
    }
}
/* combustible gas value display */
hi_void show_combustible_gas_value(hi_void)
{
    hi_u8 combustible_gas_value_str[10]={0};
    hi_u8 current_mode = g_current_mode;
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400000 */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);

    while(1) {
        hi_udelay(DELAY_10_MS);//delay 10ms
        mq2_get_data();
        flaot_to_string(g_combustible_gas_value, combustible_gas_value_str); 
        oled_show_str(65,5,combustible_gas_value_str,1); 
        if (current_mode != g_current_mode) {
            break;
        }

        hi_sleep(SLEEP_10_MS);//10ms
    }
}

hi_void show_co2_value(hi_void){
    hi_u8 current_mode=g_current_mode;
    hi_u8 co2_str[6]={0};
    hi_i2c_init(HI_I2C_IDX_0,HI_I2C_IDX_BAUDRATE);
    hi_i2c_set_baudrate(HI_I2C_IDX_0,HI_I2C_IDX_BAUDRATE);
    
    while(1){
        hi_udelay(DELAY_10_MS);
        get_sgp30_data();
        flaot1_to_string(sgp_co2,co2_str);
        oled_show_str(55,5,co2_str,1);
        if(current_mode!=g_current_mode){
            break;
        }
        hi_sleep(SLEEP_10_MS);
    }
}


hi_void show_tvoc_value(hi_void){
    hi_u8 current_mode=g_current_mode;
    hi_u8 tvoc_str[6]={0};
    hi_i2c_init(HI_I2C_IDX_0,HI_I2C_IDX_BAUDRATE);
    hi_i2c_set_baudrate(HI_I2C_IDX_0,HI_I2C_IDX_BAUDRATE);

    while(1){
        hi_udelay(DELAY_10_MS);
        get_sgp30_data();
        flaot1_to_string(sgp_tvoc,tvoc_str);
        oled_show_str(55,5,tvoc_str,1);
        if(current_mode!=g_current_mode){
            break;
        }
        hi_sleep(SLEEP_10_MS);
    }
}

hi_void show_altitude_value(hi_void){
    hi_u8 current_mode=g_current_mode;
    hi_u8 gps_altitude[16]="23";
    
    while(1){
        hi_udelay(DELAY_1_MS);
        // GPS_Read_Data(GPSdata);
        // oled_show_str(0,0, "        OK0     ",1);
        // flaot_to_string(GPSdata->Latitude,gps_altitude);
        // if(GPSdata->Latitude==0)
        // {
        //     oled_show_str(0,6, "     ERROR      ",1);
        // }
        // else
        // {
        //     oled_show_str(0,6, "        OK      ",1);
        // }
        // oled_show_str(0,1, "        OK2     ",1);
        oled_show_str(73,4,gps_altitude,1);
        if(current_mode!=g_current_mode){
            break;
        }
        hi_sleep(SLEEP_10_MS);
    }

}


hi_void show_location_value(hi_void){
    hi_u8 current_mode=g_current_mode;
    hi_u8 gps_longitude[16] = "120.67";
    hi_u8 gps_latitude[16] = "36.35";

    
    while(1){
        hi_udelay(DELAY_1_MS);

        oled_show_str(57,4,gps_latitude,1);


        oled_show_str(65,6,gps_longitude,1);

        if(current_mode!=g_current_mode){
            break;
        }
        hi_sleep(SLEEP_10_MS);
    }

}


hi_void show_test_value(hi_void){
    hi_u8 current_mode=g_current_mode;
    hi_u8 gps_altitude[16];
    GPS_Data_TypeDef *GPSdata;
    
    while(1){
        oled_show_str(0,1, "          OK0   ",1);
        GPS_Read_Data(&GPSdata);
        oled_show_str(73,4,altitude,1);
        oled_show_str(0,0, "        OK0     ",1);
        //flaot1_to_string(GPSdata->Latitude,gps_altitude);
        if(GPSdata->Latitude==0)
        {
            oled_show_str(0,6, "     ERROR      ",1);
        }
        else
        {
            oled_show_str(0,6, "        OK      ",1);
        }
        
        

        if(current_mode!=g_current_mode){
            break;
        }
        hi_sleep(SLEEP_10_MS);
    }

}



/* environment function hamdle and display */
hi_void environment_func(hi_void)
{
    /*初始化时屏幕 i2c baudrate setting*/
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    /*ssd1306 config init*/
    // oled_init();
    while( HI_ERR_SUCCESS != oled_init()) {
         printf("Connecting oled board falied!Please access oled board\r\n");
         if (hi3861_board_led_test == 0) {
            hi3861_board_led_test =1;
             /*test HiSpark board*/
            FACTORY_HISPARK_BOARD_TEST("-----------HiSpark board test----------");  
         }
         hi_sleep(SLEEP_1S);
    }
    sgp30_check_and_init();
    /*按键中断初始化*/
    extern hi_void test_gpio_init(hi_void);
    test_gpio_init();
    
    while (1) {
        switch (g_current_mode) {
            case ENV_ALL_MODE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "      The       ",1);
                 oled_show_str(0,1, "  Environment   ",1);
                 oled_show_str(0,2, "  Monitoring    ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "T:    C H:    % ",1);
                 oled_show_str(0,5, " C_Gas:      ppm",1);
                 oled_show_str(0,6, "   CO2:      ppm",1);
                 oled_show_str(0,7, "  TVOC:      ppd",1);
                 show_all_environment_value();
                 break;
            case ENV_TEMPERRATURE_MODE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen 
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "                ",1);
                 oled_show_str(0,2, "  Temperature   ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "                ",1);
                 oled_show_str(0,5, "   T:    C      ",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1);
                 show_temperature_value();
                 break;
            case ENV_HUMIDITY_MODE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "                ",1);
                 oled_show_str(0,2, "    Humidity    ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "                ",1);
                 oled_show_str(0,5, "     H:    %    ",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1); 
                 show_humidity_value();
                 break;
            case COMBUSTIBLE_GAS_MODE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "                ",1);
                 oled_show_str(0,2, "     C_Gas      ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "                ",1);
                 oled_show_str(0,5, "  C_Gas:        ",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1); 
                 show_combustible_gas_value();
                 break;
            case CO2_MODE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "                ",1);
                 oled_show_str(0,2, "       CO2      ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "                ",1);
                 oled_show_str(0,5, "   CO2:      ppm",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1); 
                 show_co2_value();
                 break;
            case TVOC_MODE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "                ",1);
                 oled_show_str(0,2, "      TVOC      ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "                ",1);
                 oled_show_str(0,5, "  TVOC:      ppd",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1); 
                 show_tvoc_value();
                 break; 
            case LOCATION:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "    LOCATION    ",1);
                 oled_show_str(0,2, "                ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "   Lat:        N",1);
                 oled_show_str(0,5, "                ",1);
                 oled_show_str(0,6, "   Long:       E",1);
                 oled_show_str(0,7, "                ",1); 
                 show_location_value();
                 break; 
            case ALTITUDE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "                ",1);
                 oled_show_str(0,2, "       GPS      ",1);
                 oled_show_str(0,3, "                ",1);
                 oled_show_str(0,4, "Altitude:    m  ",1);
                 oled_show_str(0,5, "                ",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1); 
                 show_altitude_value();
                 break;                
            case ENV_RETURN_MODE:
                 oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
                 oled_show_str(0,0, "                ",1);
                 oled_show_str(0,1, "                ",1);                               
                 oled_show_str(0,2, "Return Menu     ",1);
                 oled_show_str(0,3, "  Environment   ",1);
                 oled_show_str(0,4, "                ",1);
                 oled_show_str(0,5, "                ",1);
                 oled_show_str(0,6, "                ",1);
#ifdef  CONFIG_ENVIRONMENTAL_MONITORING
                 oled_show_str(0,7, "Continue        ",1); 
                 return_all_mode_enum_sample();
#elif   CONFIG_ALL_DEMO
                 oled_show_str(0,7, "Continue   Exit ",1);                 
                 return_main_enum_sample();
                 if (g_current_type == KEY_DOWN) {
                     g_current_mode = ENV_ALL_MODE;
                     oled_main_menu_display();                    
                     return;
                 }
                 g_current_mode = ENV_ALL_MODE; 
#endif                 
                 break;               
        default:
            break;
        }
    }
}