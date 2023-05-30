#ifndef IOT_CONFIG_H_
#define IOT_CONFIG_H_

///<CONFIG THE LOG
/*if you need the iot log for the development , please enable it, else please comment it*/
#define CONFIG_LINKLOG_ENABLE   1

///< CONFIG THE WIFI
/* Please modify the ssid and pwd for the own */
#define CONFIG_AP_SSID  "Redmi Note 11T Pro"    //手机设置热点名称(此处名称和手机上的一致)
#define CONFIG_AP_PWD   "kisskiss"              //手机设置热点密码(此处密码和手机上的一致)

///< Configure the iot platform
/*Please modify the device id and pwd for your own */
#define CONFIG_DEVICE_ID  "63819d4c379b9f242471d605_Hi3861_1"   //云服务器的设备ID名称
#define CONFIG_DEVICE_PWD "12345678"                            //云服务器的设备密码
///< if you use the tls mode and x509mode, please modify the DEVICE CA AND PRIVATE KEY IN iot_main.c
// #define CONFIG_MQTT_SSL                  ///< which means use the tls
// #define CONFIG_MQTT_SSL_X509             ///< which means use the x509 mode, and must enable the SSL; if both disabled, it means use the tcp mode

/*app_demo_iot entery function */
int app_demo_iot(void);
#endif