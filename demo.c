#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sakura_mqtt_client.h"


/* net buffer length */
#define DEMO_BUFFER_LEN                     4096
#define DEMO_MESSAGE_MAX_LEN                1024
#define DEMO_LOGD(fmt, ...)                 printf("[DEMO][D][----]" fmt, ##__VA_ARGS__)
#define DEMO_LOGE(fmt, ...)                 printf("[DEMO][E][----]" fmt, ##__VA_ARGS__)
#define DEMO_TEST_HOST                      "test.mosquitto.org"
#define DEMO_TEST_IP                        "91.121.93.94"
#define DEMO_TEST_PORT                      1883

#ifndef CONFIG_SYS_UNIX
/**
 * @brief Request Memory Space.
 *
 * @param[in] size memory size
 * @return success: Pointer to managed memory    fail:NULL
 */
sakura_void_t *sakura_malloc(sakura_size_t size)
{
    /* Use the malloc function of the standard C library. */
    return malloc(size);
}

/**
 * @brief Free memory.
 *
 * @param[out] ptr The pointer that manages this memory.
 * @return sakura_void_t
 */
sakura_void_t sakura_free(sakura_void_t *ptr)
{
    /* Use the free function of the standard C library. */
    free(ptr);
}
#endif


/* function declaration */
static sakura_void_t demo_work(sakura_void_t);
static sakura_void_t demo_connect(sakura_int32_t index, mqtt_cbs_t* mqtt_sess_cbs);

/* client_A callbacks */
static sakura_void_t client_A_set_opt(sakura_int32_t index);
static sakura_void_t client_A_on_connect(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_A_on_disconnect(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_A_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num);
static sakura_void_t client_A_on_unsubscribe(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_A_on_status(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_A_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len);
static sakura_void_t client_A_subscribe(sakura_int32_t index);
static sakura_void_t client_A_publish(sakura_int32_t index);

/* client_B callbacks */
static sakura_void_t client_B_set_opt(sakura_int32_t index);
static sakura_void_t client_B_on_connect(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_B_on_disconnect(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_B_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num);
static sakura_void_t client_B_on_unsubscribe(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_B_on_status(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_B_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len);
static sakura_void_t client_B_subscribe(sakura_int32_t index);
static sakura_void_t client_B_publish(sakura_int32_t index);
static sakura_int32_t demo_log_print(const sakura_char_t* log_str);


/* global variable */
static sakura_int32_t         client_A_sub_flag               = 0;
static sakura_int32_t         client_B_sub_flag               = 0;
static sakura_uint32_t        client_A_publish_count          = 1;
static sakura_uint32_t        client_B_publish_count          = 1;
static sakura_int32_t         client_A_disconnect_flag        = 0;
static sakura_int32_t         client_B_disconnect_flag        = 0;
static mqtt_cbs_t           client_A_cbs                    = {
                            client_A_on_connect,
                            client_A_on_disconnect,
                            client_A_on_subscribe,
                            client_A_on_unsubscribe,
                            client_A_on_status,
                            client_A_on_message
};
static mqtt_cbs_t           client_B_cbs                    = {
                            client_B_on_connect,
                            client_B_on_disconnect,
                            client_B_on_subscribe,
                            client_B_on_unsubscribe,
                            client_B_on_status,
                            client_B_on_message
};

static sakura_void_t client_A_on_connect(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("============= on_A_connect =============\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
}

static sakura_void_t client_A_on_disconnect(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("============= on_A_disconnect =============\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
    client_A_disconnect_flag ++;
}

static sakura_void_t client_A_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num)
{
    sakura_uint32_t i;
    DEMO_LOGD("============ on_A_subscribe ============\n");
    for (i = 0; i < code_num; i++){
        DEMO_LOGD("===  client [%d] topic_%d code = %d\n", index, i, suback_code[i]);
    }
    client_A_sub_flag = 1;
}

static sakura_void_t client_A_on_unsubscribe(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("=========== on_A_unsubscribe ===========\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
}

static sakura_void_t client_A_on_status(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("============= on_A_status  =============\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
}

static sakura_void_t client_A_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len)
{
    sakura_uint8_t out_buf[DEMO_BUFFER_LEN] = {0};
    DEMO_LOGD("============= on_A_message =============\n");
    DEMO_LOGD("index = %d, topic = %s, len = %d\n", index, topic, msg_len);
    memcpy(out_buf, msg, msg_len);
    DEMO_LOGD("message : %s\n", out_buf);
}

static sakura_void_t client_A_subscribe(sakura_int32_t index)
{
    sakura_mqtt_topic_t sub_topic_list[2] = {0};
    sakura_char_t* topic_1 = "sakura/B/message";
    sakura_char_t* topic_2 = "sakura/B/err";
    sub_topic_list[0].qos = QOS1;
    sub_topic_list[0].topic = topic_1;
    sub_topic_list[1].qos = QOS2;
    sub_topic_list[1].topic = topic_2;
    sakura_mqtt_subscribe(index, sub_topic_list, 2);    
}

static sakura_void_t client_B_on_connect(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("============= on_B_connect =============\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
}

static sakura_void_t client_B_on_disconnect(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("============= on_B_disconnect =============\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
    client_B_disconnect_flag ++;
}

static sakura_void_t client_B_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num)
{
    sakura_uint32_t i;
    DEMO_LOGD("============ on_B_subscribe ============\n");
    for (i = 0; i < code_num; i++){
        DEMO_LOGD("===  client [%d] topic_%d code = %d\n", index, i, suback_code[i]);
    }
    client_B_sub_flag = 1;
}

static sakura_void_t client_B_on_unsubscribe(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("=========== on_B_unsubscribe ===========\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
}

static sakura_void_t client_B_on_status(sakura_int32_t index, sakura_int32_t code)
{
    DEMO_LOGD("============= on_B_status  =============\n");
    DEMO_LOGD("====   client index[%2d], code = %2d  ====\n", index, code);
}

static sakura_void_t client_B_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len)
{
    sakura_uint8_t out_buf[DEMO_BUFFER_LEN] = {0};
    DEMO_LOGD("============= on_B_message =============\n");
    DEMO_LOGD("index = %d, topic = %s, len = %d\n", index, topic, msg_len);
    memcpy(out_buf, msg, msg_len);
    DEMO_LOGD("message : %s\n", out_buf);
}

static sakura_void_t client_B_subscribe(sakura_int32_t index)
{
    sakura_mqtt_topic_t sub_topic_list;
    sakura_char_t* topic = "sakura/A/message";
    sub_topic_list.qos = QOS2;
    sub_topic_list.topic = topic;
    sakura_mqtt_subscribe(index, &sub_topic_list, 1);    
}

static sakura_void_t client_A_set_opt(sakura_int32_t index)
{
    sakura_int32_t cleansession = 0;
    mqtt_will_t will_A = {0};
    /* will */
    will_A.qos = QOS0;
    will_A.topic = "sakura/A/err";
    will_A.payload = (sakura_uint8_t*)"sakura_A network failure, exited.";
    will_A.retained = 0;
    will_A.payloadlen = strlen((const sakura_char_t*)will_A.payload);
    /*set will */
    (sakura_void_t)sakura_mqtt_set_option(index, SAKURA_MQTT_SET_WILL, &will_A);
    /* set cleansession */
    (sakura_void_t)sakura_mqtt_set_option(index, SAKURA_MQTT_SET_CLEANSESSION, &cleansession);
}

static sakura_void_t client_B_set_opt(sakura_int32_t index)
{
    mqtt_will_t will_B = {0};
    /* will */
    will_B.qos = QOS0;
    will_B.topic = "sakura/B/err";
    will_B.payload = (sakura_uint8_t*)"sakura_B network failure, exited.";
    will_B.retained = (sakura_uint8_t)0;
    will_B.payloadlen = strlen((const sakura_char_t*)will_B.payload);
    /* set will */
    (sakura_void_t)sakura_mqtt_set_option(index, SAKURA_MQTT_SET_WILL, &will_B);
}

static sakura_void_t demo_connect(sakura_int32_t index, mqtt_cbs_t* mqtt_sess_cbs)
{
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    broker.hostname = DEMO_TEST_IP;
    broker.port = DEMO_TEST_PORT;
    account.broker = &broker;
    account.username = NULL;
    account.password = NULL;
    if(mqtt_sess_cbs != NULL){
        (sakura_void_t)sakura_mqtt_connect(index, &account, &(*mqtt_sess_cbs));
    } else {
        (sakura_void_t)sakura_mqtt_connect(index, &account, NULL);
    }
}


static sakura_void_t client_A_publish(sakura_int32_t index)
{
    sakura_char_t* topic = "sakura/A/message";
    sakura_uint8_t payload[DEMO_MESSAGE_MAX_LEN] = {0};
    mqtt_message_t message = {0};
    message.qos = QOS1;
    message.retained = 0;
    message.id = index;

    message.payloadlen = snprintf((sakura_char_t*)payload, DEMO_MESSAGE_MAX_LEN, "<message_%d> from clientA", client_A_publish_count ++);
    message.payload = (sakura_uint8_t*)(&payload);
    sakura_mqtt_publish(index, topic, &message, NULL);

    message.payloadlen = snprintf((sakura_char_t*)payload, DEMO_MESSAGE_MAX_LEN, "<message_%d> from clientA", client_A_publish_count ++);
    message.payload = (sakura_uint8_t*)(&payload);
    sakura_mqtt_publish(index, topic, &message, NULL);
}

static sakura_void_t client_B_publish(sakura_int32_t index)
{
    sakura_char_t* topic = "sakura/B/message";
    sakura_uint8_t payload[DEMO_MESSAGE_MAX_LEN] = {0};
    mqtt_message_t message = {0};
    message.qos = QOS2;
    message.retained = 0;
    message.id = index;

    message.payloadlen = snprintf((sakura_char_t*)payload, DEMO_MESSAGE_MAX_LEN, "<message_%d> from clientB", client_B_publish_count ++);
    message.payload = (sakura_uint8_t*)(&payload);
    sakura_mqtt_publish(index, topic, &message, NULL);
}

static sakura_int32_t demo_log_print(const sakura_char_t* log_str)
{
    return printf("%s", log_str);
}

/* primary service */
static sakura_void_t demo_work(sakura_void_t)
{
    sakura_int32_t index_A = 0;
    sakura_int32_t index_B = 0;
    sakura_uint32_t tick = 0;
    sakura_uint32_t right_of_speech_flag = 0;

    do
    {
        /* init */
        sakura_mqtt_init(4);

        /* query properties */
        sakura_mqtt_query_properties();

#ifndef CONFIG_SYS_UNIX
        sakura_mqtt_set_async_tick_rate(10);
#endif

        /* open */
        index_A = sakura_mqtt_open("client_A");
        index_B = sakura_mqtt_open("client_B");

        /* set client A options */
        client_A_set_opt(index_A);
        client_B_set_opt(index_B);

        /* connect */
        demo_connect(index_A, &client_A_cbs);
        demo_connect(index_B, &client_B_cbs);

        /* tick */
        while(1){
            /* tick */
#ifndef CONFIG_SYS_UNIX
            (sakura_void_t)sakura_mqtt_tick(tick);
#endif
            /* delay 100 ms */
            usleep(100 * 1000);
            tick ++;

            /* subscribe */
            if(tick == 20){
                client_A_subscribe(index_A);
                client_B_subscribe(index_B);
            }

            if(tick % 20 == 0){
                /* publish */
                if(right_of_speech_flag == 0 && client_A_sub_flag == 1){
                    if(client_A_disconnect_flag == 0){
                        client_A_publish(index_A);
                    }
                    right_of_speech_flag = 1;
                }
                if(right_of_speech_flag == 1 && client_B_sub_flag == 1){
                    if(client_B_disconnect_flag == 0){
                        client_B_publish(index_B);
                    }
                    right_of_speech_flag = 0;
                }
            }
        }
    } while (SAKURA_FALSE);

    /* cleanup */
    sakura_mqtt_cleanup();
    DEMO_LOGD("[DEMO]end of operation,clients is closed!\n");
}





/**
 * Demo is used to demonstrate the publish/subscribe operation on both ends.
 * The message level of online/offline status is QOS1,
 * The message level of error/online/offline status is QOS0,
 * the message level of topic A is QOS2,
 * and the message level of topic B is QOS1.
 */
sakura_int32_t main(sakura_void_t)
{
    sakura_sdk_log_conf_t log_conf = {0};

    log_conf.level = LOG_LEVEL_DEBUG;
    log_conf.is_color_console = SAKURA_TRUE;
    log_conf.print_function = demo_log_print;
    sakura_mqtt_set_log_config(&log_conf);

    demo_work();
    return 0;
}