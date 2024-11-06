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
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "E53_SC2.h"
#include "Button.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "weights_inception.h"
#include "mqtt_cloud.h"
#include "cJSON.h"

#define DEBUG 0

#define TASK_STACK_SIZE 1024 * 8
#define TASK_PRIO 10

#define SAMPLE_INTERVAL 10
#define SAMPLE_TIMES 200

// AI任务事件标志
osEventFlagsId_t AI_Done_Flag;
// 采集任务开始标志
osEventFlagsId_t Sample_Start_Flag;

// 数据数组
int16_t MPU6050_GYRO_Value[200][3]; // 定义保存MPU6050陀螺仪数据数组

nnom_model_t *model;
const char *const motion_name[] = {"Circle", "Letter_L", "Letter_M", "Letter_R", "Letter_W", "NoMotion", "Triangle", "Upanddown"};
const int motion_num = sizeof(motion_name) / sizeof(const char *);
#define QUANTIFICATION_SCALE (pow(2, INPUT_1_OUTPUT_DEC))

// 执行ai任务
static int model_predict()
{
    for (int i = 0; i < SAMPLE_TIMES; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            nnom_input_data[i * 3 + j] = (int8_t)round(MPU6050_GYRO_Value[i][j] * QUANTIFICATION_SCALE);
        }
    }
    model_run(model); // 运行模型
    int max_prb = 0, max_i = 0;
    for (int i = 0; i < motion_num; i++)
    {
        if (nnom_output_data[i] > max_prb)
        {
            max_prb = nnom_output_data[i];
            max_i = i;
        }
    }
    printf("motion_class:%s, acc: %f\r\n", motion_name[max_i], max_prb / 127.0 * 100);
    return max_i;
}

// MPU6050采集任务
static void Mpu6050_Task(void *argument)
{
    E53_SC2_Init();

    // 创建 JSON 对象
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", 1);
    // 添加键值对
    char Gyro_arr_name[10];
    for (int i = 0; i < 3; i++)
    {
        snprintf(Gyro_arr_name, sizeof(Gyro_arr_name), "Gyro[%d]", i);
        cJSON_AddNumberToObject(root, Gyro_arr_name, 0);
    }

    while (1)
    {
        // 等待F1按键按下
        osEventFlagsWait(Sample_Start_Flag, 1, osFlagsWaitAny, osWaitForever);
        // 点亮D1
        GpioSetOutputVal(GPIO_D1, 1);
        // 10ms采集一次, 采集2s, 共200次
        for (int i = 0; i < SAMPLE_TIMES; i++)
        {
            MPU6050_GetGyro_Value(MPU6050_GYRO_Value[i]);
            usleep(SAMPLE_INTERVAL * 1000);
        }
        // 熄灭D1
        GpioSetOutputVal(GPIO_D1, 0);

#if DEBUG
        for (int i = 0; i < SAMPLE_TIMES; i++)
        {
            cJSON *key = cJSON_GetObjectItemCaseSensitive(root, "id");
            if (key != NULL && cJSON_IsNumber(key))
            {
                cJSON_SetNumberValue(key, i);
                free(key);
            }
            for (int j = 0; j < 3; j++)
            {
                snprintf(Gyro_arr_name, sizeof(Gyro_arr_name), "Gyro[%d]", j);
                cJSON *key = cJSON_GetObjectItemCaseSensitive(root, Gyro_arr_name);
                if (key != NULL && cJSON_IsNumber(key))
                {
                    cJSON_SetNumberValue(key, MPU6050_GYRO_Value[i][j]);
                    free(key);
                }
            }
            char *cjson_print = cJSON_PrintUnformatted(root);
            printf("%s\r\n", cjson_print);
            // printf("send :\r\n");
            free(cjson_print);
        }
#else
        // 执行推理
        int motion_id = model_predict();
        short temp;
        MPU6050_ReturnTemp(&temp);
        update_cloud(motion_name[motion_id], temp);
#endif
        usleep(10 * 1000);
    }
}

static void Button_Task(void *argument)
{
    Button_Init();

    while (1)
    {
        Wait_Button_Press_And_Release(GPIO_F1);

        // 开始采集
        osEventFlagsSet(Sample_Start_Flag, 1);

        usleep(SAMPLE_TIMES * SAMPLE_INTERVAL * 1000);
    }
}

static void Main_Task(void)
{
    AI_Done_Flag = osEventFlagsNew(NULL);
    if (AI_Done_Flag == NULL)
        printf("Falied to create EventFlags!\n");

    Sample_Start_Flag = osEventFlagsNew(NULL);
    if (Sample_Start_Flag == NULL)
        printf("Falied to create EventFlags!\n");

    // WifiConnect(SELECT_WIFI_SSID, SELECT_WIFI_PASSWORD);

    model = nnom_model_create(); // 创建模型

    osThreadAttr_t mpu6050_attr;
    mpu6050_attr.name = "Mpu6050_Task";
    mpu6050_attr.attr_bits = 0U;
    mpu6050_attr.cb_mem = NULL;
    mpu6050_attr.cb_size = 0U;
    mpu6050_attr.stack_mem = NULL;
    mpu6050_attr.stack_size = TASK_STACK_SIZE;
    mpu6050_attr.priority = 12;

    if (osThreadNew((osThreadFunc_t)Mpu6050_Task, NULL, &mpu6050_attr) == NULL)
    {
        printf("Falied to create %s!\n", mpu6050_attr.name);
    }

    osThreadAttr_t button_attr;
    button_attr.name = "Button_Task";
    button_attr.attr_bits = 0U;
    button_attr.cb_mem = NULL;
    button_attr.cb_size = 0U;
    button_attr.stack_mem = NULL;
    button_attr.stack_size = TASK_STACK_SIZE;
    button_attr.priority = 11;

    if (osThreadNew((osThreadFunc_t)Button_Task, NULL, &button_attr) == NULL)
    {
        printf("Falied to create %s!\n", button_attr.name);
    }

    osThreadAttr_t mqtt_attr;

    mqtt_attr.name = "mqtt_main_task";
    mqtt_attr.attr_bits = 0U;
    mqtt_attr.cb_mem = NULL;
    mqtt_attr.cb_size = 0U;
    mqtt_attr.stack_mem = NULL;
    mqtt_attr.stack_size = 10240;
    mqtt_attr.priority = 24;

    if (osThreadNew((osThreadFunc_t)Mqtt_Main_Task, NULL, &mqtt_attr) == NULL)
    {
        printf("Falied to create %s!\n", mqtt_attr.name);
    }
}

APP_FEATURE_INIT(Main_Task);