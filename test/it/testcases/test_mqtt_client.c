#include "test.h"


static sakura_uint32_t glob_connect_flag = 0;
static sakura_uint32_t glob_disconnect_flag = 0;
static sakura_uint32_t glob_subscribe_flag = 0;
static sakura_uint32_t glob_unsubscribe_flag = 0;
static sakura_uint32_t glob_publish_flag = 0;
static mqtt_will_t *will_A = NULL;

static sakura_void_t test_client_clear_glob_vars(sakura_void_t)
{
    glob_connect_flag = 0;
    glob_disconnect_flag = 0;
    glob_subscribe_flag = 0;
    glob_unsubscribe_flag = 0;
    glob_publish_flag = 0;
    if(will_A != NULL){
        sakura_free(will_A);
        will_A = NULL;
    }
}

static sakura_void_t testcase_setup(sakura_void_t)
{
    will_A = NULL;
    test_network_up();
    test_client_clear_glob_vars();
}
static sakura_void_t testcase_teardown(sakura_void_t)
{
    if(will_A != NULL){
        sakura_free(will_A);
    }
    test_cleanup();
}

/* callbacks */
static sakura_void_t test_on_connect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_connect =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == SAKURA_MQTT_CONNECT_ACCEPTED){
        glob_connect_flag = 1;
    }
}

static sakura_void_t test_on_disconnect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_disconn =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == SAKURA_MQTT_CONNECT_DISCONNECTED){
        glob_disconnect_flag = 1;
    }
}

static sakura_void_t test_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num)
{
    sakura_uint32_t i = 0;
    printf("============ on_A_subscribe ============\n");
    for (i; i < code_num; i++){
        printf("===  client [%d] topic_%d code = %d\n", index, i, (sakura_int32_t)suback_code[i]);
    }
    glob_subscribe_flag = 1;
}

static sakura_void_t test_on_unsubscribe(sakura_int32_t index, sakura_int32_t code)
{
    printf("=========== client_on_unsubscribe ===========\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == SAKURA_MQTT_UNSUBSCRIBE_SUCCESS){
        glob_unsubscribe_flag = 1;
    }
}

static sakura_void_t test_on_status(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_status  =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
}

static sakura_void_t test_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len)
{
    sakura_uint8_t out_buf[512] = {0};
    printf("============= client_on_message =============\n");
    printf("index = %d, topic = %s, len = %d\n", index, topic, msg_len);
    memcpy(out_buf, msg, msg_len);
    printf("message : %s\n", out_buf);
}

static sakura_void_t test_on_publish(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_publish  ============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    glob_publish_flag = 1;
}


/* wrapper */
static sakura_void_t set_options_wrapper(sakura_int32_t index, sakura_uint32_t module)
{
    if(index < 0){
        return;
    }
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_uint8_t cleansession = 1;
    sakura_uint32_t keepalive_interval = 30;
    sakura_uint32_t timeout_interval = 30;
    will_A = (mqtt_will_t*)sakura_malloc(sizeof(mqtt_will_t));


    /* will */
    will_A->qos = QOS0;
    will_A->topic = TEST_TOPIC_A;
    will_A->payload = TEST_PAYLOAD;
    will_A->retained = 0;
    will_A->payloadlen = strlen(will_A->payload);

    switch (module)
    {
    case SAKURA_MQTT_SET_WILL:
        /*set will */
        ret = sakura_mqtt_set_option(index, SAKURA_MQTT_SET_WILL, will_A);
        CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);        
        break;
    case SAKURA_MQTT_SET_CLEANSESSION:
        /* set cleansession */
        ret = sakura_mqtt_set_option(index, SAKURA_MQTT_SET_CLEANSESSION, &cleansession);
        CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);        
        break;
    case SAKURA_MQTT_SET_KEEPALIVE:
        /* set keepalive */
        ret = sakura_mqtt_set_option(index, SAKURA_MQTT_SET_KEEPALIVE, &keepalive_interval);
        CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);        
        break;
    case SAKURA_MQTT_SET_TIMEOUT:
        /* set timeout */
        ret = sakura_mqtt_set_option(index, SAKURA_MQTT_SET_TIMEOUT, &timeout_interval);
        CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);        
        break;
    default:
        printf("no support module!\n");
        break;
    }

}

static sakura_void_t connect_wraaper(sakura_uint32_t index)
{
    if(index < 0){
        return;
    }
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    mqtt_cbs_t test_cbs = {
        test_on_connect,
        test_on_disconnect,
        test_on_subscribe,
        test_on_unsubscribe,
        test_on_status,
        test_on_message
    };
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    sakura_char_t *hostname = TEST_HOST;

    broker.hostname = hostname;
    broker.port = TEST_PORT;
    account.broker = &broker;
    account.username = NULL;
    account.password = NULL;
    ret = sakura_mqtt_connect(index, &account, &test_cbs);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
}

static sakura_void_t dissconnect_wrapper(sakura_int32_t index, sakura_uint32_t tick)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    tick = 0;
    if(index < 0){
        return;
    }
    ret = sakura_mqtt_disconnect(index);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
    while(1){  
        tick ++;
        usleep(100 * 1000);
        if(glob_disconnect_flag == 1 || tick > 200){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_disconnect_flag, 1);
}

static sakura_void_t publish_wrapper(sakura_int32_t index, QoS qos)
{
    if(index < 0){
        return;
    }
    sakura_char_t* topic = TEST_TOPIC_A;
    sakura_char_t* payload = TEST_PAYLOAD;
    mqtt_message_t message = {0};
    message.qos = qos;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.id = index;
    message.payload = payload;
    sakura_mqtt_publish(index, topic, &message, test_on_publish);
}


static sakura_void_t subscribe_wrapper(sakura_uint32_t module)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;
    sakura_mqtt_topic_t sub_topic_list[2] = {0};
    sakura_char_t* topic_a = TEST_TOPIC_A;
    sakura_char_t* topic_b = TEST_TOPIC_B;
    sub_topic_list[0].qos = QOS2;
    sub_topic_list[0].topic = topic_a;
    sub_topic_list[1].qos = QOS1;
    sub_topic_list[1].topic = topic_b;    

    /* init */
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);

    while(1){
        /* tick */
        tick ++;
        usleep(200 * 1000);
        if(tick > 200){
            break;
        }

        if(glob_connect_flag == 1){
            ret = sakura_mqtt_subscribe(client_index, sub_topic_list, module);
            CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
            glob_connect_flag = 0;
        }

        if(glob_subscribe_flag == 1){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_subscribe_flag, 1);

    /* disconnect */
    dissconnect_wrapper(client_index, tick);

}

static sakura_void_t unsubscribe_wrapper(sakura_int32_t module)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;
    sakura_mqtt_topic_t sub_topic_list[2] = {0};
    sakura_char_t* topic_a = TEST_TOPIC_A;
    sakura_char_t* topic_b = TEST_TOPIC_B;
    sakura_char_t* unsub_topic[2] = {0};
    unsub_topic[0] = TEST_TOPIC_A;
    unsub_topic[1] = TEST_TOPIC_B;
    sub_topic_list[0].qos = QOS2;
    sub_topic_list[0].topic = topic_a;
    sub_topic_list[1].qos = QOS1;
    sub_topic_list[1].topic = topic_b;  

    /* init */
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);

    while(1){
        /* tick */
        tick ++;
        usleep(200 * 1000);
        if(tick > 200){
            break;
        }

        if(glob_connect_flag == 1){
            ret = sakura_mqtt_subscribe(client_index, sub_topic_list, module);
            CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
            glob_connect_flag = 0;
        }

        if(glob_subscribe_flag == 1){
            ret = sakura_mqtt_unsubscribe(client_index, unsub_topic, module);
            CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
            glob_subscribe_flag = 0;
        }

        if(glob_unsubscribe_flag == 1){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_unsubscribe_flag, 1);

    /* disconnect */
    dissconnect_wrapper(client_index, tick);
}

/**
 * testcases
 */
static sakura_void_t test_mqtt_init(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
}
#ifndef CONFIG_SYS_UNIX
static sakura_void_t test_set_tick_rate(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    ret = sakura_mqtt_set_async_tick_rate(10);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
}
#endif

static sakura_void_t test_open(sakura_void_t)
{
    sakura_int32_t client_index = -1;
    sakura_int32_t ret =SAKURA_MQTT_ERROR;
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);
}

static sakura_void_t test_close(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    ret = sakura_mqtt_close(client_index);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
}

static sakura_void_t test_set_options(sakura_void_t)
{
    sakura_int32_t client_index = -1;
    sakura_int32_t ret = SAKURA_MQTT_ERROR;

    /* init */
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /*set will */
    set_options_wrapper(client_index, SAKURA_MQTT_SET_WILL);
    if(will_A != NULL){
        sakura_free(will_A);
        will_A = NULL;
    }

    /* set cleansession */
    set_options_wrapper(client_index, SAKURA_MQTT_SET_CLEANSESSION);
    if(will_A != NULL){
        sakura_free(will_A);
        will_A = NULL;
    }

    /* set keepalive */
    set_options_wrapper(client_index, SAKURA_MQTT_SET_KEEPALIVE);
    if(will_A != NULL){
        sakura_free(will_A);
        will_A = NULL;
    }

    /* set timeout */
    set_options_wrapper(client_index, SAKURA_MQTT_SET_TIMEOUT);
    if(will_A != NULL){
        sakura_free(will_A);
        will_A = NULL;
    }
}


static sakura_void_t test_connect(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    /* init */
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);

    while(1){
        /* tick */
        tick ++;
        usleep(200 * 1000);
        if(glob_connect_flag == 1 || tick > 50){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_connect_flag, 1);

    /* disconnect */
    dissconnect_wrapper(client_index, tick);
}

static sakura_void_t test_disconnect(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    /* init */
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);

    while(1){
        /* tick */
        tick++;
        usleep(200 * 1000);
        if(tick > 200){
            break;
        }

        if(glob_connect_flag == 1){
            sakura_mqtt_disconnect(client_index);
        }

        if(glob_disconnect_flag == 1){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_disconnect_flag, 1);
}

static sakura_void_t test_subscribe_single(sakura_void_t)
{
    subscribe_wrapper(1);
}

static sakura_void_t test_subscribe_double(sakura_void_t)
{
    subscribe_wrapper(2);
}

static sakura_void_t test_unsubscribe_single(sakura_void_t)
{
    unsubscribe_wrapper(1);
}

static sakura_void_t test_unsubscribe_double(sakura_void_t)
{
    unsubscribe_wrapper(2);
}

static sakura_void_t test_publish_wrapper(QoS qos)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    /* init */
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);

    while(1){
        /* tick */
        tick++;
        usleep(200 * 1000);
        if(tick > 200){
            break;
        }
        if(glob_connect_flag == 1){
            publish_wrapper(client_index, qos);
            glob_connect_flag = 0;
        }
        if(glob_publish_flag == 1){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_publish_flag, 1);

    /* disconnect */
    dissconnect_wrapper(client_index, tick);
}

static sakura_void_t test_publish_QOS0(sakura_void_t)
{
    test_publish_wrapper(QOS0);
}

static sakura_void_t test_publish_QOS1(sakura_void_t)
{
    test_publish_wrapper(QOS1);
}

static sakura_void_t test_publish_QOS2(sakura_void_t)
{
    test_publish_wrapper(QOS2);
}

static sakura_void_t test_mqtt_cleanup(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    /* init */
    ret = test_init();
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
    ret = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
    test_cleanup();
}


sakura_int32_t test_mqtt_client_suite()
{
    CU_TestInfo mqtt_testcases[] = {
        {"MQTT_001  init\n",                                    test_mqtt_init},
#ifndef CONFIG_SYS_UNIX
        {"MQTT_002  set tick rate\n",                           test_set_tick_rate},
#endif
        {"MQTT_003  open\n",                                    test_open},
        {"MQTT_004  close\n",                                   test_close},
        {"MQTT_005  set options\n",                             test_set_options},
        {"MQTT_006  connect\n",                                 test_connect},
        {"MQTT_007  disconnect\n",                              test_disconnect},
        {"MQTT_008  subscribe single\n",                        test_subscribe_single},
        {"MQTT_009  subscribe double\n",                        test_subscribe_double},
        {"MQTT_010  unsubscribe single\n",                      test_unsubscribe_single},
        {"MQTT_011  unsubscribe double\n",                      test_unsubscribe_double},
        {"MQTT_012  publish QOS0\n",                            test_publish_QOS0},
        {"MQTT_013  publish QOS1\n",                            test_publish_QOS1},
        {"MQTT_014  publish QOS2\n",                            test_publish_QOS2},
        {"MQTT_015  cleanup\n",                                 test_mqtt_cleanup},
        CU_TEST_INFO_NULL
    };

    CU_SuiteInfo mqtt_suite[] = {
        {"MQTT public interface", NULL, NULL, testcase_setup, testcase_teardown, mqtt_testcases},
        CU_SUITE_INFO_NULL
    };

    if (CUE_SUCCESS != CU_register_suites(mqtt_suite)) {
        printf("Failed to client suite\n");
        return -1;
    }

    return 0;
}
