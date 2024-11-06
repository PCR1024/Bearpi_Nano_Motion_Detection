#ifndef __BUTTON_H__
#define __BUTTON_H__
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"

#define GPIO_F1 WIFI_IOT_IO_NAME_GPIO_11
#define GPIO_F2 WIFI_IOT_IO_NAME_GPIO_12
#define GPIO_LED WIFI_IOT_IO_NAME_GPIO_2
#define GPIO_D1 WIFI_IOT_IO_NAME_GPIO_7
#define GPIO_D2 WIFI_IOT_IO_NAME_GPIO_8


#define PRESS_LEVEL     WIFI_IOT_GPIO_VALUE0
#define RELEASE_LEVEL   WIFI_IOT_GPIO_VALUE1


void Button_Init();
void Wait_Button_Press_And_Release(WifiIotGpioIdx id);

#endif
