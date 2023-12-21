#include "test.h"



static sakura_uint32_t glob_connect_flag = 0;
static sakura_uint32_t glob_neterror_flag = 0;
static sakura_uint32_t glob_disconnect_flag = 0;
static sakura_uint32_t glob_subscribe_flag = 0;
static sakura_uint32_t glob_unsubscribe_flag = 0;
static sakura_uint32_t glob_publish_flag = 0;
static sakura_uint32_t glob_on_message_flag = 0;
static sakura_uint32_t glob_status_code = 0;
static sakura_uint32_t glob_pubrel_flag = 0;
static mqtt_will_t *will_A = NULL;

static sakura_void_t test_client_clear_glob_vars(sakura_void_t)
{
    glob_connect_flag = 0;
    glob_neterror_flag = 0;
    glob_disconnect_flag = 0;
    glob_subscribe_flag = 0;
    glob_unsubscribe_flag = 0;
    glob_publish_flag = 0;
    glob_on_message_flag = 0;
    glob_status_code = 0;
    glob_pubrel_flag = 0;
    if(will_A != NULL){
        sakura_free(will_A);
        will_A = NULL;
    }
}


static sakura_void_t subscribe_wrapper(sakura_int32_t index)
{
    sakura_mqtt_topic_t sub_topic_list = {0};
    sakura_char_t* topic = "test/B/message";
    sub_topic_list.qos = QOS1;
    sub_topic_list.topic = topic;
    sakura_mqtt_subscribe(index, &sub_topic_list, 1);    
}

static sakura_void_t testcase_setup(sakura_void_t)
{
    will_A = NULL;
    test_network_up();
    test_client_clear_glob_vars();
    (sakura_void_t)test_init();
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
        glob_connect_flag ++;
    }
    glob_status_code = code;
}

static sakura_void_t test_on_disconnect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_disconn =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == SAKURA_MQTT_CONNECT_DISCONNECTED){
        glob_disconnect_flag = 1;
    }
    glob_status_code = code;
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
    glob_status_code = code;
}

static sakura_void_t test_on_status(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_status  =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == SAKURA_MQTT_NETWORK_ERROR){
        glob_neterror_flag = 1;
    }
    glob_status_code = code;
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
    sakura_uint32_t keepalive_interval = 10;
    sakura_uint32_t timeout_interval = 5;
    will_A = (mqtt_will_t*)sakura_malloc(sizeof(mqtt_will_t));


    /* will */
    will_A->qos = QOS0;
    will_A->topic = TEST_WILL_TOPIC;
    will_A->payload = TEST_WILL_PAYLOAD;
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
    if(index < 0){
        return;
    }
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    ret = sakura_mqtt_disconnect(index);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
    while(1){  
        if(glob_disconnect_flag == 1){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_disconnect_flag, 1);
}

static sakura_int32_t publish_wrapper(sakura_int32_t index, QoS qos)
{
    if(index < 0){
        return;
    }
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_char_t* topic = TEST_TOPIC;
    sakura_char_t* payload = TEST_PAYLOAD;
    mqtt_message_t message = {0};
    message.qos = qos;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.id = index;
    message.payload = payload;
    ret = sakura_mqtt_publish(index, topic, &message, test_on_publish);
}

/**
 * testcases
 */
static sakura_void_t test_reconnect(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;
    static sakura_uint32_t net_state_flag = 0;
    test_network_up();

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);

    while(1){
        tick ++;
        usleep(100 * 1000);

        if(glob_connect_flag == 1 && net_state_flag == 0){
            /* network down */
            test_network_down();
            net_state_flag ++;
        }
        
        if(glob_neterror_flag == 1 && net_state_flag == 1){
            /* network up */
            test_network_up();
            net_state_flag ++;
        }

        if(glob_connect_flag == 2){
            /* reconnect success or timeout */
            break;
        }

        if(tick > 1000){
            break;
        }
    }
    CU_ASSERT_EQUAL(glob_connect_flag, 2);
    CU_ASSERT_EQUAL(glob_neterror_flag, 1);

    /* disconnect */
    dissconnect_wrapper(client_index, tick);
}

static sakura_void_t test_keepalive(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* set options */
    set_options_wrapper(client_index, SAKURA_MQTT_SET_KEEPALIVE);

    /* connect */
    connect_wraaper(client_index);
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(tick > 500){
            break;
        }
    }

    /* disconnect */
    dissconnect_wrapper(client_index, tick);
}

#if 0
static sakura_void_t test_redeliver(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;
    sakura_uint32_t timer = 0;
    test_network_up();

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);

    set_options_wrapper(client_index, SAKURA_MQTT_SET_TIMEOUT);


    while(1){
        tick ++;
        usleep(100 * 1000);

        if(tick%20 == 0){
            publish_wrapper(client_index, QOS1);
        }

        if(glob_connect_flag == 1){
            test_network_down();
            glob_connect_flag = 0;
        }

        if(timer == 160) {
            test_network_up();
        }

        if(timer > 200){
            break;
        }

        timer ++;
    }

    /* disconnect */
    dissconnect_wrapper(client_index, tick);
}
#endif

static sakura_void_t test_redeliver_timeout(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;
    test_network_up();

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);
    set_options_wrapper(client_index, SAKURA_MQTT_SET_TIMEOUT);

    while(1){
        tick ++;
        usleep(100 * 1000);

        if(glob_connect_flag == 1){
            publish_wrapper(client_index, QOS1);
            test_network_down();
            glob_connect_flag ++;
        }

        if(tick > 200){
            break;
        }
    }
    /* disconnect */
    dissconnect_wrapper(client_index, tick);
}

static sakura_void_t test_connect_timeout(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    test_network_down();
    connect_wraaper(client_index);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_neterror_flag == 1){
            break;
        }
        if(tick > 200){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_status_code, SAKURA_MQTT_NETWORK_ERROR);
}

static sakura_void_t test_subscribe_timeout(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    test_network_up();
    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 1){
            test_network_down();
            subscribe_wrapper(client_index);
            glob_connect_flag ++;
        }
        if(glob_neterror_flag == 1){
            break;
        }
        if(tick > 500){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_connect_flag, 2);
    CU_ASSERT_EQUAL(glob_status_code, SAKURA_MQTT_NETWORK_ERROR);
    test_network_up();
}

static sakura_void_t test_unsubscribe_timeout(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_char_t *topic = "test/B/message";
    sakura_uint32_t tick = 0;

    test_network_up();
    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 1){
            subscribe_wrapper(client_index);
            glob_connect_flag ++;
        }
        if(glob_subscribe_flag == 1){
            test_network_down();
            sakura_mqtt_unsubscribe(client_index, &topic, 1);
            glob_subscribe_flag ++;
        }
        if(glob_neterror_flag == 1){
            break;
        }
        if(tick > 500){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_connect_flag, 2);
    CU_ASSERT_EQUAL(glob_subscribe_flag, 2);
    CU_ASSERT_EQUAL(glob_status_code, SAKURA_MQTT_NETWORK_ERROR);
    test_network_up();
}

static sakura_void_t test_publish_timeout_wrapper(QoS qos)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    test_network_up();
    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 1){
            publish_wrapper(client_index, qos);
            glob_connect_flag ++;
            test_network_down();
        }
        if(glob_neterror_flag == 1){
            break;
        }
        if(tick > 500){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_status_code, SAKURA_MQTT_NETWORK_ERROR);
    test_network_up();
}


static sakura_void_t test_qos1_publish_timeout(sakura_void_t)
{
    test_publish_timeout_wrapper(QOS1);
}

static sakura_void_t test_qos2_publish_timeout(sakura_void_t)
{
    test_publish_timeout_wrapper(QOS2);
}

#if 0
static sakura_void_t test_pubrel_timeout(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;

    test_network_up();
    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 1){
            publish_wrapper(client_index, QOS2);
            glob_connect_flag ++;
        }
        if(glob_pubrel_flag == 1){
            test_network_down();
            glob_pubrel_flag ++;
        }
        if(glob_neterror_flag == 1){
            break;
        }
        if(tick > 500){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_status_code, SAKURA_MQTT_NETWORK_ERROR);
}
#endif

static sakura_void_t test_halfway_close(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;
    test_network_up();
    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 1 && tick%20 == 0){
            ret = publish_wrapper(client_index, QOS2);
            if(ret < 0){
                break;
            }
        }
        if(tick == 150){
            ret = sakura_mqtt_close(client_index);
            CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
        }
        if(tick > 300){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_connect_flag, 1);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_halfway_cleanup(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index = -1;
    sakura_uint32_t tick = 0;
    test_network_up();
    /* open */
    client_index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index, -1);

    /* connect */
    connect_wraaper(client_index);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 1 && tick%20 == 0){
            ret = publish_wrapper(client_index, QOS2);
            if(ret < 0){
                break;
            }
        }
        if(tick == 150){
            sakura_mqtt_cleanup();
        }
        if(tick > 300){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_connect_flag, 1);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
}

static sakura_void_t test_two_clients_halfway_close(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t ret2 = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index_1 = -1;
    sakura_int32_t client_index_2 = -1;
    sakura_uint32_t tick = 0;
    test_network_up();
    /* open */
    client_index_1 = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index_1, -1);

    client_index_2 = sakura_mqtt_open(TEST_CLIENT_ID_2);
    CU_ASSERT_NOT_EQUAL(client_index_2, -1);

    /* connect */
    connect_wraaper(client_index_1);
    connect_wraaper(client_index_2);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 2 && tick%20 == 0){
            ret = publish_wrapper(client_index_1, QOS2);
            ret2 = publish_wrapper(client_index_2, QOS2);
            if((ret < 0) && (ret2 < 0)){
                break;
            }
        }
        if(tick == 150){
            ret = sakura_mqtt_close(client_index_1);
            CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
            ret = sakura_mqtt_close(client_index_2);
            CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
        }
        if(tick > 300){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_connect_flag, 2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_INVALID_ARGS);
    CU_ASSERT_EQUAL(ret2, SAKURA_MQTT_ERR_INVALID_ARGS);
}

static sakura_void_t test_two_clients_halfway_cleanup(sakura_void_t)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t ret2 = SAKURA_MQTT_ERROR;
    sakura_int32_t client_index_1 = -1;
    sakura_int32_t client_index_2 = -1;
    sakura_uint32_t tick = 0;
    test_network_up();
    /* open */
    client_index_1 = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(client_index_1, -1);

    client_index_2 = sakura_mqtt_open(TEST_CLIENT_ID_2);
    CU_ASSERT_NOT_EQUAL(client_index_2, -1);

    /* connect */
    connect_wraaper(client_index_1);
    connect_wraaper(client_index_2);
    
    while(1){
        tick ++;
        usleep(100 * 1000);
        if(glob_connect_flag == 2 && tick%20 == 0){
            ret = publish_wrapper(client_index_1, QOS2);
            ret2 = publish_wrapper(client_index_2, QOS2);
            if((ret < 0) && (ret2 < 0)){
                break;
            }
        }
        if(tick == 150){
            sakura_mqtt_cleanup();
        }
        if(tick > 300){
            break;
        }
    }

    CU_ASSERT_EQUAL(glob_connect_flag, 2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERR_REQ_IGN);
    CU_ASSERT_EQUAL(ret2, SAKURA_MQTT_ERR_REQ_IGN);
}


sakura_int32_t test_mqtt_net_suite()
{
    CU_TestInfo mqtt_testcases[] = {
        {"MQTT_001  reconnect\n",                                   test_reconnect},
        {"MQTT_002  keepalive\n",                                   test_keepalive},
        // {"MQTT_003  message redeliver\n",                           test_redeliver},
        {"MQTT_003  redeliver timeout\n",                           test_redeliver_timeout},
        {"MQTT_004  connect timout\n",                              test_connect_timeout},
        {"MQTT_005  subscribe timeout\n",                           test_subscribe_timeout},
        {"MQTT_006  unsubscribe timeout\n",                         test_unsubscribe_timeout},
        {"MQTT_007  qos1 publish timeout\n",                        test_qos1_publish_timeout},
        {"MQTT_008  qos2 publish timeout\n",                        test_qos2_publish_timeout},
        // {"MQTT_010  pubrel timeout\n",                              test_pubrel_timeout},
        {"MQTT_009  halfway close\n",                               test_halfway_close},
        {"MQTT_010  halfway cleanup\n",                             test_halfway_cleanup},
        {"MQTT_011  two clients halfway close\n",                   test_two_clients_halfway_close},
        {"MQTT_012  two clients halfway cleanup\n",                 test_two_clients_halfway_cleanup},
        CU_TEST_INFO_NULL
    };

    CU_SuiteInfo mqtt_suite[] = {
        {"MQTT net", NULL, NULL, testcase_setup, testcase_teardown, mqtt_testcases},
        CU_SUITE_INFO_NULL
    };

    if (CUE_SUCCESS != CU_register_suites(mqtt_suite)) {
        printf("Failed to mqtt net suite\n");
        return -1;
    }

    return 0;
}
