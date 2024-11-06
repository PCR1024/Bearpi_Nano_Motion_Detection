#include "cmsis_os2.h"
#include "ohos_init.h"
#include <stdio.h>
#include "Button.h"

void Wait_Button_Press_And_Release(WifiIotGpioIdx id)
{
    WifiIotGpioValue val;
    while(1)
    {
        // 等待按键按下
        GpioGetInputVal(id, &val);
        if(val == PRESS_LEVEL)
        {
            while(1)
            {
                // 检测按键连续松开
                int Press_cnt = 0;
                for(int i = 0; i < 10; i++)
                {
                    usleep(10000);
                    GpioGetInputVal(id, &val);
                    if(val == PRESS_LEVEL)
                        Press_cnt++;
                }
                if(Press_cnt == 0)
                {
                    return;
                }
                usleep(10000);
            }
        }
        usleep(10000);
    }

}

void Button_Init()
{
    // 初始化LED
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_2, WIFI_IOT_IO_FUNC_GPIO_2_GPIO);

    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_2, WIFI_IOT_GPIO_DIR_OUT);

    // 初始化D1
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_FUNC_GPIO_7_GPIO);//设置GPIO_7的复用功能为普通GPIO
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_GPIO_DIR_OUT);//设置GPIO_7为输出模式
    
    // 初始化D2
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_8, WIFI_IOT_IO_FUNC_GPIO_8_GPIO);//设置GPIO_8的复用功能为普通GPIO
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_8, WIFI_IOT_GPIO_DIR_OUT);//设置GPIO_8为输出模式

    //初始化F1按键，设置为下降沿触发中断
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_GPIO);

    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_DIR_IN);
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_PULL_UP);

    //初始化F2按键，设置为下降沿触发中断
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_GPIO);

    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_DIR_IN);
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_PULL_UP);
    
}
