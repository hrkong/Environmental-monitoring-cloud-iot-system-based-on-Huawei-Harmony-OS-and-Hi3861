#ifndef __APP_SGP30_H__
#define __APP_SGP30_H__

#define SGP_SLEEP_20MS              (20)//20ms
#define SGP_SLEEP_50MS              (50)//5ms
#define SGP_SLEEP_1S                (1000)//1s
#define SGP_DELAY_10MS              (10000)//10ms
#define SGP_DELAY_50MS              (50000) //50ms
#define SGP_DELAY_500MS             (500000) //100ms
#define SGP_DELAY_1S                (1000000) //100ms
#define SGP_DELAY_10S               (10000000) //100ms
#define SGP_DEVICE_ADDR             (0x58) //SGP30的I2C通信固定地址为0x58
#define SGP_DEVICE_TEST_CMD         (0x58<<1) // 测试指令为
#define SGP_DEVICE_PARAM_HIGH_BYTE  (0x20)
#define SGP_DEVICE_PARAM_LOW_BYTE   (0x08)
#define SGP_TASK_SLEEP_TIME         (20) //thread sleep 20ms
#define BAUDRATE_INIT               (400000) 


typedef enum{
    SGP_TVOC=1,
    SGP_CO2=2,
}sgp_sensor_type;
#endif