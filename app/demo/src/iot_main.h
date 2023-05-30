#ifndef IOR_MAIN_H_
#define IOT_MAIN_H_
#include <stdint.h>
#include <stddef.h>

typedef void  (*fnMsgCallBack)(int qos,const char *topic,const char *payload);

int IoTMain(void);

int IoTSetMsgCallback(fnMsgCallBack msgCallback);

/**
 * w
 * p
 * @param qos:the mqtt qos,:0,1,2
 * @param topic:the iot mqtt topic
 * @param payload:the mqtt payload
 * 
 * @return 0 success while others failed
 * 
 * @instruction:if success means
*/
int IotSendMsg(int qos,const char *topic,const char *payload);

#endif /* IOT_MAIN_H_ */