#include "mqtt_cloud.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include <queue.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>
#include <dtls_al.h>
#include <mqtt_al.h>
#include "cJSON.h"
#include "wifi_connect.h"
#include "Button.h"

#define CONFIG_APP_SERVERIP "385e33c7ca.st1.iotda-device.cn-east-3.myhuaweicloud.com" // 标准版平台对接地址

#define CONFIG_APP_SERVERPORT "1883"

#define CONFIG_APP_DEVICEID "672b21e47805a202afdf5b42_20241212" // deviceid

#define CONFIG_APP_DEVICEPWD "20030312" // 密钥

#define CONFIG_APP_LIFETIME 60 ///< seconds

#define CONFIG_QUEUE_TIMEOUT (5 * 1000)

#define MSGQUEUE_OBJECTS 16 // number of Message Queue Objects

typedef enum
{
    en_msg_cmd = 0,
    en_msg_report,
    en_msg_conn,
    en_msg_disconn,
} en_msg_type_t;

typedef struct
{
    char *request_id;
    char *payload;
} cmd_t;

typedef struct
{
    const char *motion;
    int temperature;
} report_t;

typedef struct
{
    en_msg_type_t msg_type;
    union
    {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct
{
    queue_t *app_msg;
    int connected;
    int led;
    int motor;
} app_cb_t;

osMessageQueueId_t mid_MsgQueue; // message queue id

static app_cb_t g_app_cb;

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t temperature;
    oc_mqtt_profile_kv_t motion;

    if (g_app_cb.connected != 1)
    {
        return;
    }

    service.event_time = NULL;
    service.service_id = "Sensor";
    service.service_property = &temperature;
    service.nxt = NULL;

    temperature.key = "温度";
    temperature.value = &report->temperature;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temperature.nxt = &motion;

    motion.key = "动作类型";
    motion.value = report->motion;
    motion.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    motion.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL, &service);
    return;
}

// use this function to push all the message to the buffer
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int ret = 0;
    char *buf;
    int buf_len;
    app_msg_t *app_msg;

    if ((NULL == msg) || (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS))
    {
        return ret;
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    if (NULL == buf)
    {
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);

    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.request_id = buf;
    buf_len = strlen(msg->request_id);
    buf += buf_len + 1;
    memcpy(app_msg->msg.cmd.request_id, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';

    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy(app_msg->msg.cmd.payload, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = queue_push(g_app_cb.app_msg, app_msg, 10);
    if (ret != 0)
    {
        printf("push rcv mess failed!\r\n");
        free(app_msg);
    }
    free(buf);
    return ret;
}

static void deal_cmd_msg(cmd_t *cmd)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;
    cJSON *obj_paras;
    cJSON *obj_para;

    int cmdret = 1;
    oc_mqtt_profile_cmdresp_t cmdresp;
    obj_root = cJSON_Parse(cmd->payload);
    if (NULL == obj_root)
    {
        goto EXIT_JSONPARSE;
    }

    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (NULL == obj_cmdname)
    {
        goto EXIT_CMDOBJ;
    }
    if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Control_light"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "D2");
        if (NULL == obj_para)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the LED here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            g_app_cb.led = 1;
            GpioSetOutputVal(GPIO_D2, 1);
            printf("Light On!\r\n");
        }
        else
        {
            g_app_cb.led = 0;
            GpioSetOutputVal(GPIO_D2, 0);
            printf("Light Off!\r\n");
        }
        free(obj_para);
        cmdret = 0;
    }

EXIT_OBJPARA:
    free(obj_paras);
EXIT_OBJPARAS:
EXIT_CMDOBJ:
    free(obj_root);
EXIT_JSONPARSE:
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL, &cmdresp);
    return;
}

// MQTT主函数
int Mqtt_Main_Task(void)
{
    app_msg_t *app_msg;
    uint32_t ret;

    WifiConnect(SELECT_WIFI_SSID, SELECT_WIFI_PASSWORD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();

    g_app_cb.app_msg = queue_create("queue_rcvmsg", 10, 1);
    if (NULL == g_app_cb.app_msg)
    {
        printf("Create receive msg queue failed");
    }
    oc_mqtt_profile_connect_t connect_para;
    (void)memset(&connect_para, 0, sizeof(connect_para));

    connect_para.boostrap = 0;
    connect_para.device_id = CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr = CONFIG_APP_SERVERIP;
    connect_para.server_port = CONFIG_APP_SERVERPORT;
    connect_para.life_time = CONFIG_APP_LIFETIME;
    connect_para.rcvfunc = msg_rcv_callback;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if ((ret == (int)en_oc_mqtt_err_ok))
    {
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
        GpioSetOutputVal(GPIO_LED, 1);
    }
    else
    {
        printf("oc_mqtt_profile_connect faild!\r\n");
        GpioSetOutputVal(GPIO_LED, 0);
    }

    while (1)
    {
        app_msg = NULL;
        (void)queue_pop(g_app_cb.app_msg, (void **)&app_msg, 0xFFFFFFFF);
        if (NULL != app_msg)
        {
            switch (app_msg->msg_type)
            {
            case en_msg_cmd:
                deal_cmd_msg(&app_msg->msg.cmd);
                break;
            case en_msg_report:
                deal_report_msg(&app_msg->msg.report);
                break;
            default:
                break;
            }
            free(app_msg);
        }
    }
    return 0;
}

int update_cloud(const char *motion, short temperature)
{
    app_msg_t *app_msg;
    app_msg = malloc(sizeof(app_msg_t));
    printf("Motion: %s, Temperature: %d\r\n", motion, temperature);
    if (NULL != app_msg)
    {
        app_msg->msg_type = en_msg_report;
        app_msg->msg.report.motion = motion;
        app_msg->msg.report.temperature = (int)temperature;
        if (0 != queue_push(g_app_cb.app_msg, app_msg, CONFIG_QUEUE_TIMEOUT))
        {
            free(app_msg);
        }
    }
    return 0;
}
