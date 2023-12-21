#include "test.h"

#define TEST_DFILE_HOST                     "files.getquicker.net"
#define TEST_DFILE_IP                       "58.220.52.239"
#define TEST_DFILE_PATH                     "/_productfiles/202303/Quicker.x64.1.37.18.0.msi"
#define TEST_FILE_NAME                      "./dist/Quicker_x64_1_37_18_0.msi"
#define TEST_FILE_SIZE                      20611072
#define TEST_DFILE_PORT                     80
#define TEST_REGISTER_HOST                  "link.my.sakura.com"
#define TEST_REGISTER_IP                    "59.110.244.181"
#define TEST_REGISTER_PATH                  "/api/devices/register?registerCode=wcuYixU0tTrKT8bh"
#define TEST_REGISTER_PORT                  80
#define TEST_IOT_MQTT_HOST                  "linkx.sakura.com"
#define TEST_IOT_MQTT_IP                    "39.105.11.57"
#define TEST_IOT_MQTT_PORT                  1883
#define TEST_CHILD_NUM                      2
#define TEST_HOST                           "test.mosquitto.org"
#define TEST_IP                             "91.121.93.94"

static sakura_uint32_t glob_thread_end_flag = 0;
static sakura_uint32_t glob_connect_succ_flag = 0;

static sakura_void_t test_clear_glob_vars(sakura_void_t)
{
    glob_thread_end_flag = 0;
    glob_connect_succ_flag = 0;
}

/* callbacks */
static sakura_void_t test_on_connect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_connect =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == SAKURA_MQTT_CONNECT_ACCEPTED){
        glob_connect_succ_flag++;
    }
}

static sakura_void_t test_on_disconnect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_disconn =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
}

static sakura_void_t test_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num)
{
    sakura_uint32_t i = 0;
    printf("============ on_A_subscribe ============\n");
    for (i; i < code_num; i++){
        printf("===  client [%d] topic_%d code = %d\n", index, i, (sakura_int32_t)suback_code[i]);
    }
}

static sakura_void_t test_on_unsubscribe(sakura_int32_t index, sakura_int32_t code)
{
    printf("=========== client_on_unsubscribe ===========\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
}

static sakura_void_t test_on_status(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_status  =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == SAKURA_MQTT_NETWORK_ERROR){
        glob_connect_succ_flag = 0;
    }
}

static sakura_void_t test_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len)
{
    printf("============= client_on_message =============\n");
    printf("index = %d, topic = %s, len = %d\n", index, topic, msg_len);
    printf("message : %s\n", msg);
}

static sakura_void_t test_init_and_cleanup_loop(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    test_init();
    while (SAKURA_TRUE)
    {
        tick ++;
        usleep(100 * 1000);
        if(tick%2 == 0){
            test_init();
        } else {
            test_cleanup();
        }
        if(tick == 200){
            break;
        }
    }

    test_cleanup();
}

static sakura_void_t test_init_and_cleanup_open_close_loop(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = -1;
    sakura_int32_t ret = 0;
    sakura_uint32_t init_flag = 0;
    test_init();

    while (SAKURA_TRUE)
    {
        tick ++;
        usleep(100 * 1000);
        if(tick%2 == 0){
            test_init();
            init_flag = 1;
        } else {
            test_cleanup();
            init_flag = 0;
        }

        if(init_flag == 1){
            if(tick%3 == 0){
                index = sakura_mqtt_open("test");
                CU_ASSERT_NOT_EQUAL(index, SAKURA_MQTT_ERROR);
            }
            if(index != -1){
                ret = sakura_mqtt_close(index);
                CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
                index = -1;
            }            
        }

        if(tick == 200){
            break;
        }
    }
}

static sakura_void_t test_uninit_open(sakura_void_t)
{
    sakura_int32_t index = -1;
    index = sakura_mqtt_open("test_client");
    CU_ASSERT_EQUAL(index, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_uninit_close(sakura_void_t)
{
    sakura_int32_t ret = -1;
    ret = sakura_mqtt_close(1);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_uninit_set_options(sakura_void_t)
{
    sakura_int32_t ret = -1;
    ret = sakura_mqtt_set_option(1, SAKURA_MQTT_SET_KEEPALIVE, NULL);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_invaild_client_set_options(sakura_void_t)
{
    sakura_int32_t ret = -1;
    test_init();
    ret = sakura_mqtt_set_option(1, SAKURA_MQTT_SET_KEEPALIVE, NULL);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_uninit_tick(sakura_void_t)
{
    sakura_int32_t ret = -1;
    ret = sakura_mqtt_tick(50);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);
}

static sakura_void_t test_invaild_client_tick(sakura_void_t)
{
    sakura_int32_t ret = -1;
    test_init();
    ret = sakura_mqtt_tick(50);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ALL_CLIENTS_IDLE);
}

static sakura_void_t test_uninit_connect(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_mqtt_account_info_t account = {0};
    mqtt_cbs_t cbs = {
        test_on_connect,
        test_on_disconnect,
        test_on_subscribe,
        test_on_unsubscribe,
        test_on_status,
        test_on_message
    };
    ret = sakura_mqtt_connect(1, &account, &cbs);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_invaild_client_connect(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_mqtt_account_info_t account = {0};
    mqtt_cbs_t cbs = {
        test_on_connect,
        test_on_disconnect,
        test_on_subscribe,
        test_on_unsubscribe,
        test_on_status,
        test_on_message
    };
    test_init();
    ret = sakura_mqtt_connect(1, &account, &cbs);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_uninit_disconnect(sakura_void_t)
{
    sakura_int32_t ret = -1;
    ret = sakura_mqtt_disconnect(1);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_invaild_client_disconnect(sakura_void_t)
{
    sakura_int32_t ret = -1;
    test_init();
    ret = sakura_mqtt_disconnect(1);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_uninit_subscribe(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_mqtt_topic_t sub_topic_list[2] = {0};
    sakura_char_t* topic_a = TEST_TOPIC_A;
    sakura_char_t* topic_b = TEST_TOPIC_B;
    sub_topic_list[0].qos = QOS2;
    sub_topic_list[0].topic = topic_a;
    sub_topic_list[1].qos = QOS1;
    sub_topic_list[1].topic = topic_b;  
    ret = sakura_mqtt_subscribe(1, sub_topic_list, 2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_invaild_client_subscribe(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_mqtt_topic_t sub_topic_list[2] = {0};
    sakura_char_t* topic_a = TEST_TOPIC_A;
    sakura_char_t* topic_b = TEST_TOPIC_B;
    sub_topic_list[0].qos = QOS2;
    sub_topic_list[0].topic = topic_a;
    sub_topic_list[1].qos = QOS1;
    sub_topic_list[1].topic = topic_b;  
    test_init();
    ret = sakura_mqtt_subscribe(1, sub_topic_list, 2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_uninit_unsubscribe(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_char_t* unsub_topic[2] = {0};
    unsub_topic[0] = TEST_TOPIC_A;
    unsub_topic[1] = TEST_TOPIC_B;
    ret = sakura_mqtt_unsubscribe(1, unsub_topic, 2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_invaild_client_unsubscribe(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_char_t* unsub_topic[2] = {0};
    unsub_topic[0] = TEST_TOPIC_A;
    unsub_topic[1] = TEST_TOPIC_B;
    test_init();
    ret = sakura_mqtt_unsubscribe(1, unsub_topic, 2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_uninit_publish(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_char_t* topic = TEST_TOPIC_A;
    sakura_char_t* payload = TEST_PAYLOAD;
    mqtt_message_t message = {0};
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.id = 1;
    message.payload = payload;
    ret = sakura_mqtt_publish(1, topic, &message, NULL);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_invaild_client_publish(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_char_t* topic = TEST_TOPIC_A;
    sakura_char_t* payload = TEST_PAYLOAD;
    mqtt_message_t message = {0};
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.id = 1;
    message.payload = payload;
    test_init();
    ret = sakura_mqtt_publish(1, topic, &message, NULL);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_uninit_cleanup(sakura_void_t)
{
    sakura_int32_t ret = -1;
    sakura_mqtt_cleanup();
}

sakura_int32_t test_public_interface_suite(sakura_void_t)
{
    CU_TestInfo public_interface_testcases[] = {
        /* mqtt */
        {"public 001 init/cleanup loop\n",                                      test_init_and_cleanup_loop},
        {"public 002 init/cleanup/open/close loop\n",                           test_init_and_cleanup_open_close_loop},
        {"public 003 uninit open\n",                                            test_uninit_open},
        {"public 004 uninit close\n",                                           test_uninit_close},
        {"public 005 uninit set_option\n",                                      test_uninit_set_options},
        {"public 006 invalid client set options\n",                             test_invaild_client_set_options},
        {"public 007 uninit tick\n",                                            test_uninit_tick},
        {"public 008 invalid client tick\n",                                    test_invaild_client_tick},
        {"public 009 uninit connect\n",                                         test_uninit_connect},
        {"public 010 invalid client connect\n",                                 test_invaild_client_connect},
        {"public 011 uninit disconnect\n",                                      test_uninit_disconnect},
        {"public 012 invalid client disconnect\n",                              test_invaild_client_disconnect},
        {"public 013 uninit subscribe\n",                                       test_uninit_subscribe},
        {"public 014 invalid client subscribe\n",                               test_invaild_client_subscribe},
        {"public 015 uninit unsubscribe\n",                                     test_uninit_unsubscribe},
        {"public 016 invalid client unsubscribe\n",                             test_invaild_client_unsubscribe},
        {"public 017 uninit publish\n",                                         test_uninit_publish},
        {"public 018 invalid client publish\n",                                 test_invaild_client_publish},
        {"public 019 uninit cleanup\n",                                         test_uninit_cleanup},
        CU_TEST_INFO_NULL
    };

    CU_SuiteInfo public_interface_suite[] = {
        {"public interface cases", NULL, NULL, NULL, test_cleanup, public_interface_testcases},
        CU_SUITE_INFO_NULL
    };

    if (CUE_SUCCESS != CU_register_suites(public_interface_suite)) {
        printf("Failed to add public interface suite\n");
        return -1;
    }

    return 0;
}