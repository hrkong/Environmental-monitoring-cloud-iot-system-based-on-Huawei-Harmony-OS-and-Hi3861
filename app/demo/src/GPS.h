#ifndef __GPS_H
#define __GPS_H	 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hi_types_base.h>
#include <math.h>
// #include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"
#include "wifiiot_uart.h"
/***************************************************\
*GPS NMEA-0183协议重要参数结构体定义
*卫星信息
\***************************************************/
typedef struct
{
	uint32_t latitude_bd;					//纬度   分扩大100000倍，实际要除以100000
	uint8_t nshemi_bd;						//北纬/南纬,N:北纬;S:南纬	
	uint32_t longitude_bd;			        //经度 分扩大100000倍,实际要除以100000
	uint8_t ewhemi_bd;						//东经/西经,E:东经;W:西经
	int altitude;			 	            //海拔高度,放大了10倍,实际除以10.单位:0.1m	
}gps_msg;

/* GPS传感器数据类型定义 ------------------------------------------------------------*/
typedef struct
{
	float  Longitude;				//经度
	float  Latitude;                //纬度
	float  Altitude;			 	//海拔高度,放大了10倍,实际除以10.单位:0.1m	
} GPS_Data_TypeDef;


void Init_GPS(void);
void GPS_Read_Data(GPS_Data_TypeDef *ReadData);
#endif

