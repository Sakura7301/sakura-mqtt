#include "test.h"

static sakura_uint32_t glob_connect_flag = 0;
static sakura_uint32_t glob_neterror_flag = 0;
static sakura_uint32_t glob_disconnect_flag = 0;
static sakura_uint32_t glob_subscribe_flag = 0;
static sakura_uint32_t glob_unsubscribe_flag = 0;
static sakura_uint32_t glob_publish_flag = 0;
static sakura_uint32_t glob_on_message_flag = 0;
static sakura_uint32_t glob_malloc_mock_flag = 0;
static sakura_uint32_t glob_free_mock_flag = 0;
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
    if(will_A != NULL){
        sakura_free(will_A);
        will_A = NULL;
    }
}

static sakura_void_t testcase_setup(sakura_void_t)
{
    test_client_clear_glob_vars();
}
static sakura_void_t testcase_teardown(sakura_void_t)
{
    sakura_mqtt_cleanup();
}

/* callbacks */
static sakura_void_t test_on_connect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_connect =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == 0){
        glob_connect_flag ++;
    }
}

static sakura_void_t test_on_disconnect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_disconn =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == 0){
        glob_disconnect_flag = 1;
    }
}

static sakura_void_t test_on_subscribe(sakura_int32_t index, const sakura_uint8_t *bytes)
{
    sakura_uint32_t i = 0;
    sakura_uint32_t sub_failed = 0;
    printf("============ client_on_subscribe ============\n");
    for (i; i < strlen(bytes); i++)
    {
        printf("======  client [%d] topic_%d code = %d\n", index, i, (sakura_int32_t)bytes[i]);
        if(bytes[i] != QOS0 && bytes[i] != QOS1 && bytes[i] != QOS2){
            sub_failed = 1;
        }
    }
    if(sub_failed == 0){
        glob_subscribe_flag = 1;
    }
}

static sakura_void_t test_on_unsubscribe(sakura_int32_t index, sakura_int32_t code)
{
    printf("=========== client_on_unsubscribe ===========\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == 0){
        glob_unsubscribe_flag = 1;
    }
}

static sakura_void_t test_on_status(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_status  =============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    if(code == -101){
        glob_neterror_flag = 1;
    }
}

static sakura_void_t test_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len)
{
    sakura_uint8_t out_buf[512] = {0};
    printf("============= client_on_message =============\n");
    printf("index = %d, topic = %s, len = %d\n", index, topic, msg_len);
    memcpy(out_buf, msg, msg_len);
    printf("message : %s\n", out_buf);
    glob_on_message_flag = 1;
}

static sakura_void_t test_on_publish(sakura_int32_t index, sakura_int32_t code)
{
    printf("============= client_on_publish  ============\n");
    printf("=======   client index[%2d], code = %2d\n", index, code);
    glob_publish_flag = 1;
}

/* mock api */
sakura_void_t *sakura_malloc_mock(sakura_size_t size)
{
    if(glob_malloc_mock_flag == 0){
        glob_malloc_mock_flag = 1;
        return sakura_malloc(size);
    } else {
        return NULL;
    }
}

sakura_void_t sakura_free_mock(sakura_void_t *ptr)
{
    if(glob_free_mock_flag == 0){
        glob_free_mock_flag = 1;
        sakura_free(ptr);
        ptr = NULL;
    } else {
        return;
    }
}

sakura_int32_t sakura_socket_init_mock(sakura_void_t)
{
    return -1;
}


/* testcases */
static sakura_void_t test_mqtt_init(sakura_void_t)
{
    sakura_int32_t ret = 0;
    ret = sakura_mqtt_init(0);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);

    ret = sakura_mqtt_init(10);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);

    glob_malloc_mock_flag = 1;
    set_mock(NULL, (sakura_char_t*)sakura_malloc, (sakura_char_t*)sakura_malloc_mock);
    ret = sakura_mqtt_init(4);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);
    reset_mock();

    glob_malloc_mock_flag = 0;
    set_mock(NULL, (sakura_char_t*)sakura_malloc, (sakura_char_t*)sakura_malloc_mock);
    ret = sakura_mqtt_init(4);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);
    reset_mock();

    set_mock(NULL, (sakura_char_t*)sakura_socket_init, (sakura_char_t*)sakura_socket_init_mock);
    ret = sakura_mqtt_init(4);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);
    reset_mock();

    glob_free_mock_flag = 1;
    set_mock(NULL, (sakura_char_t*)sakura_free, (sakura_char_t*)sakura_free_mock);
    set_mock(NULL, (sakura_char_t*)sakura_socket_init, (sakura_char_t*)sakura_socket_init_mock);
    ret = sakura_mqtt_init(4);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);
    reset_mock2();

    glob_free_mock_flag = 0;
    set_mock(NULL, (sakura_char_t*)sakura_free, (sakura_char_t*)sakura_free_mock);
    set_mock(NULL, (sakura_char_t*)sakura_socket_init, (sakura_char_t*)sakura_socket_init_mock);
    ret = sakura_mqtt_init(4);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);
    reset_mock2();

    ret = sakura_mqtt_init(2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
}

static sakura_void_t test_mqtt_open(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_int32_t index = -1;

    ret = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);

    ret = sakura_mqtt_init(1);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    ret = sakura_mqtt_open(NULL);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);

    ret = sakura_mqtt_open("qwertyuioooopasdfghklzxcvbnmqweqweqwqweqdsfxvxckjvnisuhuajflaskmiuawfnapjnhbivknxv");
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);

    index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(index, -1);

    ret = sakura_mqtt_open(TEST_CLIENT_ID_2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);
}

static sakura_void_t test_mqtt_close(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_int32_t index = -1;
    mqtt_client_t *client = NULL;
    
    ret = sakura_mqtt_close(1);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);

    ret = sakura_mqtt_init(2);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    ret = sakura_mqtt_close(0);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_ERROR);

    index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(index, -1);
    ret = sakura_mqtt_close(0);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(index, -1);
    client = mqtt_get_client_by_index(index);
    client->net.sock = sakura_sock_create();
    ret = sakura_mqtt_close(0);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);

    index = sakura_mqtt_open(TEST_CLIENT_ID);
    CU_ASSERT_NOT_EQUAL(index, -1);
    client = mqtt_get_client_by_index(index);
    client->account_info = (mqtt_account_t*)sakura_malloc(sizeof(mqtt_account_t));
    ret = sakura_mqtt_close(0);
    CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
}

static sakura_void_t test_set_options(sakura_void_t)
{
    // sakura_int32_t ret = 0;
    // sakura_int32_t index = -1;
    // mqtt_client_t *client = NULL;
    // ret =sakura_mqtt_init(4);
    // CU_ASSERT_EQUAL(ret, SAKURA_MQTT_STAT_OK);
    // index = sakura_mqtt_open(TEST_CLIENT_ID);
    // CU_ASSERT_NOT_EQUAL(index, -1);
}

sakura_int32_t test_mqtt_client_suite()
{
    CU_TestInfo mqtt_testcases[] = {
        {"MQTT_001  init\n",                                   test_mqtt_init},
        {"MQTT_002  open\n",                                   test_mqtt_open},
        {"MQTT_003  close\n",                                  test_mqtt_close},
        {"MQTT_004  set options\n",                            test_set_options},
        CU_TEST_INFO_NULL
    };

    CU_SuiteInfo mqtt_suite[] = {
        {"MQTT client", NULL, NULL, testcase_setup, testcase_teardown, mqtt_testcases},
        CU_SUITE_INFO_NULL
    };

    if (CUE_SUCCESS != CU_register_suites(mqtt_suite)) {
        printf("Failed to add mqtt suite\n");
        return -1;
    }

    return 0;
}
