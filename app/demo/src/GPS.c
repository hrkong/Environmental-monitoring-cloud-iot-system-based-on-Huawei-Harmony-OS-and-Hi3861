/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hi_types_base.h>
#include <math.h>
// #include "ohos_init.h"
#include "cmsis_os2.h"

#include "GPS.h"
// #include "wifiiot_errno.h"
// #include "wifiiot_gpio.h"
// #include "wifiiot_gpio_ex.h"
// #include "wifiiot_pwm.h"
// #include "wifiiot_uart.h"
#include "hi_uart.h"

gps_msg             gpsmsg;
static unsigned char gps_uart[2048];
hi_u8 gps_uart1[2048];
hi_u8 altitude[100] = {0};
extern hi_u8  *flaot_to_string(hi_double d, hi_u8 *str);
extern hi_void oled_show_str(hi_u8 x, hi_u8 y, hi_u8 *chr, hi_u8 char_size);


/***************************************************************
* 函数名称: GPS_Init
* 说    明: 初始化GPS模块
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void GPS_Init(void)
{

    uint32_t ret;

    // WifiIotUartAttribute uart_attr = {

    //     //baud_rate: 9600
    //     // .baudRate = 38400,

	// 	.baudRate = 921600,
    //     //data_bits: 8bits
    //     .dataBits = 8,
    //     .stopBits = 1,
    //     .parity = 0,
    // };
	hi_uart_attribute uart_attr = {

        //baud_rate: 9600
        // .baudRate = 38400,

		.baud_rate = 38400,
        //data_bits: 8bits
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
    };


    //Initialize uart driver
    // ret = UartInit(WIFI_IOT_UART_IDX_2, &uart_attr, NULL);
	ret=hi_uart_init(HI_UART_IDX_0,&uart_attr, NULL);
	
    // if (ret != WIFI_IOT_SUCCESS)
	if (ret != HI_ERR_SUCCESS)
    {
        printf("Failed to init uart! Err code = %d\n", ret);
        return;
    }
    
    return;
}

/***************************************************************
* 函数名称: Init_GPS
* 说    明: 初始化GPS扩展板
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void Init_GPS(void)
{
    GpioInit(); 
	GPS_Init();
}

/***************************************************\
* 函数名称: NMEA_Comma_Pos
*	函数功能：从buf里面得到第cx个逗号所在的位置
*	输入值：
*	返回值：0~0xFE，代表逗号所在位置的便宜
*				 	0xFF，代表不存在第cx个逗号
\***************************************************/

uint8_t NMEA_Comma_Pos(uint8_t *buf,uint8_t cx)
{
	uint8_t *p = buf;
	while(cx)
	{
		if(*buf=='*'||*buf<' '||*buf>'z')return 0xFF;
		if(*buf==',')cx--;
		buf++;
	}
	return buf-p;
}
/***************************************************\
* 函数名称: NMEA_Pow
*	函数功能：返回m的n次方值
*	输入值：底数m和指数n
*	返回值：m^n
\***************************************************/
uint32_t NMEA_Pow(uint8_t m,uint8_t n)
{
	uint32_t result = 1;
	while(n--)result *= m;
	return result;
}
/***************************************************\
* 函数名称: NMEA_Str2num
*	函数功能：str数字转换为（int）数字，以','或者'*'结束
*	输入值：buf，数字存储区
*				 	dx，小数点位数，返回给调用函数
*	返回值：转换后的数值
\***************************************************/
int NMEA_Str2num(uint8_t *buf,uint8_t*dx)
{
	uint8_t *p = buf;
	uint32_t ires = 0,fres = 0;
	uint8_t ilen = 0,flen = 0,i;
	uint8_t mask = 0;
	int res;
	while(1)
	{
		if(*p=='-'){mask |= 0x02;p++;}//说明有负数
		if(*p==','||*p=='*')break;//遇到结束符
		if(*p=='.'){mask |= 0x01;p++;}//遇到小数点
		else if(*p>'9'||(*p<'0'))//数字不在0和9之内，说明有非法字符
		{
			ilen = 0;
			flen = 0;
			break;
		}
		if(mask&0x01)flen++;//小数点的位数
		else ilen++;//str长度加一
		p++;//下一个字符
	}
	if(mask&0x02)buf++;//移到下一位，除去负号
	for(i=0;i<ilen;i++)//得到整数部分数据
	{
		ires += NMEA_Pow(10,ilen-1-i)*(buf[i]-'0');
	}
	if(flen>5)flen=5;//最多取五位小数
	*dx = flen;
	for(i=0;i<flen;i++)//得到小数部分数据
	{
		fres +=NMEA_Pow(10,flen-1-i)*(buf[ilen+1+i]-'0');
	}
	res = ires*NMEA_Pow(10,flen)+fres;
	if(mask&0x02)res = -res;
	return res;
}
/***************************************************\
* 函数名称: NMEA_BDS_GNRMC_Analysis
*	函数功能：解析GNRMC信息
*	输入值：gpsx,NMEA信息结构体
*				 buf：接收到的GPS数据缓冲区首地址
\***************************************************/
void NMEA_BDS_GNRMC_Analysis(gps_msg *gpsmsg,uint8_t *buf)
{
	uint8_t *p4,dx;			 
	uint8_t posx;     
	uint32_t temp;	   
	float rs;  
	p4=(uint8_t*)strstr((const char *)buf,"$GNRMC");//"$GNRMC",经常有&和GNRMC分开的情况,故只判断GNRMC.
	if(p4 != NULL){
		posx=NMEA_Comma_Pos(p4,3);								//得到纬度
		if(posx!=0XFF)
		{
			temp=NMEA_Str2num(p4+posx,&dx);		 	 
			gpsmsg->latitude_bd=temp/NMEA_Pow(10,dx+2);	//得到°
			rs=temp%NMEA_Pow(10,dx+2);				//得到'		 
			gpsmsg->latitude_bd=gpsmsg->latitude_bd*NMEA_Pow(10,5)+(rs*NMEA_Pow(10,5-dx))/60;//转换为° 
		}
		posx=NMEA_Comma_Pos(p4,4);								//南纬还是北纬 
		if(posx!=0XFF)gpsmsg->nshemi_bd=*(p4+posx);					 
		posx=NMEA_Comma_Pos(p4,5);								//得到经度
		if(posx!=0XFF)
		{												  
			temp=NMEA_Str2num(p4+posx,&dx);		 	 
			gpsmsg->longitude_bd=temp/NMEA_Pow(10,dx+2);	//得到°
			rs=temp%NMEA_Pow(10,dx+2);				//得到'		 
			gpsmsg->longitude_bd=gpsmsg->longitude_bd*NMEA_Pow(10,5)+(rs*NMEA_Pow(10,5-dx))/60;//转换为° 
		}
		posx=NMEA_Comma_Pos(p4,6);								//东经还是西经
		if(posx!=0XFF)gpsmsg->ewhemi_bd=*(p4+posx);	
	}
	  
}
/***************************************************\
* 函数名称: NMEA_BDS_GNGGA_Analysis
*	函数功能：解析GNGGA信息
*	输入值：gpsx,NMEA信息结构体
*				 buf：接收到的GPS数据缓冲区首地址

$GNGGA （GPS 定位信息，Global Positioning System Fix Data ）
$GNGGA 语句的基本格式如下（其中 M 指单位 M，hh 指校验和，CR 和 LF 代表回车
换行，下同）：
$GNGGA,(1),       (2),     (3),(4),     (5),(6),(7),(8),(9),M,(10),M,(11),(12)*hh(CR)(LF)
$GNGGA,022044.000,3603.3857,N,10344.3356,E,1,05,6.9,1669.4,M,-41.1,M,,0000*53
(1) UTC 时间，格式为 hhmmss.ss；
(2) 纬度，格式为 ddmm.mmmmm（度分格式）；
(3) 纬度半球，N 或 S（北纬或南纬）；
(4) 经度，格式为 dddmm.mmmmm（度分格式）；
(5) 经度半球，E 或 W（东经或西经）；
(6) GPS 状态，0=未定位，1=非差分定位，2=差分定位；
(7) 正在使用的用于定位的卫星数量（00~12）
(8) HDOP 水平精确度因子（0.5~99.9）
(9) 海拔高度（-9999.9 到 9999.9 米）
(10) 大地水准面高度（-9999.9 到 9999.9 米）
(11) 差分时间（从最近一次接收到差分信号开始的秒数，非差分定位，此项为空）
(12) 差分参考基站标号（0000 到 1023，首位 0 也将传送，非差分定位，此项为空)
\***************************************************/
void NMEA_BDS_GNGGA_Analysis(gps_msg *gpsmsg,uint8_t *buf)
{
	uint8_t *p4,dx;			 
	uint8_t posx;     
	uint32_t temp;	   
	float rs;  
	p4=(uint8_t*)strstr((const char *)buf,"$GNGGA");//"$GNGGA",经常有&和GNGGA分开的情况,故只判断GNGGA.
	if(p4 != NULL){
		posx=NMEA_Comma_Pos(p4,9);								//得到海拔高度
		if(posx!=0XFF)gpsmsg->altitude=NMEA_Str2num(p4+posx,&dx); 
	}
}

/***************************************************************
* 函数名称: GPS_Read_Data
* 说    明: 获取GPS经纬度信息
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void GPS_Read_Data(GPS_Data_TypeDef *ReadData)
{	
	//hi_u8 buf1[10];
	Init_GPS();
    //通过串口1接收数据
	// int i;
	// i=hi_uart_read(HI_UART_IDX_0,gps_uart1,2048);
	// flaot1_to_string(i,buf1);
	// oled_show_str(0,2, buf1,1);

    // if(-1 != UartRead(WIFI_IOT_UART_IDX_0, gps_uart, 2048))
	// {
		
	// 	NMEA_BDS_GNGGA_Analysis(&gpsmsg,(uint8_t*)gps_uart);	//分析字符串
	// 	NMEA_BDS_GNRMC_Analysis(&gpsmsg,(uint8_t*)gps_uart);
	// 	ReadData->Longitude=(float)((float)gpsmsg.longitude_bd/100000);	
	// 	ReadData->Latitude=(float)((float)gpsmsg.latitude_bd/100000);
	// 	ReadData->Altitude=(float)((float)gpsmsg.altitude/10);
	// 	oled_show_str(0,3, "        OK      ",1);
	// }
	// else
	// {
	// 	ReadData->Longitude=0;	
	// 	ReadData->Latitude=0;
	// 	ReadData->Altitude=0;
	// 	oled_show_str(0,4, "        OK      ",1);
	// }
	//flaot_to_string(ReadData->Latitude,altitude);
	// i=UartRead(WIFI_IOT_UART_IDX_0, gps_uart, 2048);
	// oled_show_str(0,2, "        OK      ",1);

	NMEA_BDS_GNGGA_Analysis(&gpsmsg,(uint8_t*)gps_uart);	//分析字符串
	NMEA_BDS_GNRMC_Analysis(&gpsmsg,(uint8_t*)gps_uart);
	ReadData->Longitude=(float)((float)gpsmsg.longitude_bd/100000);	
	ReadData->Latitude=(float)((float)gpsmsg.latitude_bd/100000);	
	ReadData->Altitude=(float)((float)gpsmsg.altitude/10);
	flaot_to_string(ReadData->Altitude,altitude);
	if((ReadData->Latitude)==0.0)
	{
		oled_show_str(0,3, "        OK      ",1);
	}
	



}

