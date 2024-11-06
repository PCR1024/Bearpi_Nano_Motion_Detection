#ifndef __MQTT_CLOUD_H__
#define __MQTT_CLOUD_H__

int Mqtt_Main_Task(void);
int update_cloud(const char *motion, short temperature);

#endif