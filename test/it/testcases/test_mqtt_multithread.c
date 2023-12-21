#include "test.h"


#define TEST_THREAD_1               "thread_1"
#define TEST_THREAD_2               "thread_2"
#define TEST_THREAD_3               "thread_3"
#define TEST_THREAD_4               "thread_4"
#define TEST_THREAD_5               "thread_5"
#define TEST_THREAD_6               "thread_6"
#define TEST_THREAD_7               "thread_7"
#define TEST_THREAD_8               "thread_8"
#define TEST_THREAD_9               "thread_9"
#define TEST_THREAD_10              "thread_10"
#define TEST_THREAD_11              "thread_11"
#define TEST_THREAD_12              "thread_12"
#define TEST_THREAD_13              "thread_13"
#define TEST_THREAD_14              "thread_14"
#define TEST_THREAD_15              "thread_15"
#define TEST_THREAD_16              "thread_16"
#define TEST_THREAD_17              "thread_17"
#define TEST_THREAD_18              "thread_18"
#define TEST_THREAD_19              "thread_19"
#define TEST_THREAD_20              "thread_20"
#define TEST_THREAD_1_PAYLOAD       "I'm thread_1!"
#define TEST_THREAD_2_PAYLOAD       "I'm thread_2!"
#define TEST_THREAD_3_PAYLOAD       "I'm thread_3!"
#define TEST_THREAD_4_PAYLOAD       "I'm thread_4!"
#define TEST_THREAD_5_PAYLOAD       "I'm thread_5!"
#define TEST_THREAD_6_PAYLOAD       "I'm thread_6!"
#define TEST_THREAD_7_PAYLOAD       "I'm thread_7!"
#define TEST_THREAD_8_PAYLOAD       "I'm thread_8!"
#define TEST_THREAD_9_PAYLOAD       "I'm thread_9!"
#define TEST_THREAD_10_PAYLOAD      "I'm thread_10!"
#define TEST_THREAD_11_PAYLOAD      "I'm thread_11!"
#define TEST_THREAD_12_PAYLOAD      "I'm thread_12!"
#define TEST_THREAD_13_PAYLOAD      "I'm thread_13!"
#define TEST_THREAD_14_PAYLOAD      "I'm thread_14!"
#define TEST_THREAD_15_PAYLOAD      "I'm thread_15!"
#define TEST_THREAD_16_PAYLOAD      "I'm thread_16!"
#define TEST_THREAD_17_PAYLOAD      "I'm thread_17!"
#define TEST_THREAD_18_PAYLOAD      "I'm thread_18!"
#define TEST_THREAD_19_PAYLOAD      "I'm thread_19!"
#define TEST_THREAD_20_PAYLOAD      "I'm thread_20!"




static sakura_void_t client_on_connect(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_on_disconnect(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num);
static sakura_void_t client_on_unsubscribe(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_on_status(sakura_int32_t index, sakura_int32_t code);
static sakura_void_t client_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len);
static sakura_void_t client_on_publish(sakura_int32_t index, sakura_int32_t code);


static sakura_int32_t glob_thread_end_flag = 0;
static sakura_int32_t glob_thread_publish_flag = 0;
static sakura_int32_t glob_network_flag = 0;
/* thread 1 */
static sakura_int32_t glob_thread_1_index = -1;
static sakura_uint32_t glob_thread_1_connect_flag = 0;
static sakura_uint32_t glob_thread_1_disconnect_flag = 0;
static sakura_uint32_t glob_thread_1_subscribe_flag = 0;
static sakura_uint32_t glob_thread_1_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_1_publish_flag = 0;
static sakura_int32_t glob_thread_1_status_code = 0;
/* thread 2 */
static sakura_int32_t glob_thread_2_index = -1;
static sakura_uint32_t glob_thread_2_connect_flag = 0;
static sakura_uint32_t glob_thread_2_disconnect_flag = 0;
static sakura_uint32_t glob_thread_2_subscribe_flag = 0;
static sakura_uint32_t glob_thread_2_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_2_publish_flag = 0;
static sakura_int32_t glob_thread_2_status_code = 0;
/* thread 3 */
static sakura_int32_t glob_thread_3_index = -1;
static sakura_uint32_t glob_thread_3_connect_flag = 0;
static sakura_uint32_t glob_thread_3_disconnect_flag = 0;
static sakura_uint32_t glob_thread_3_subscribe_flag = 0;
static sakura_uint32_t glob_thread_3_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_3_publish_flag = 0;
static sakura_int32_t glob_thread_3_status_code = 0;
/* thread 4 */
static sakura_int32_t glob_thread_4_index = -1;
static sakura_uint32_t glob_thread_4_connect_flag = 0;
static sakura_uint32_t glob_thread_4_disconnect_flag = 0;
static sakura_uint32_t glob_thread_4_subscribe_flag = 0;
static sakura_uint32_t glob_thread_4_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_4_publish_flag = 0;
static sakura_int32_t glob_thread_4_status_code = 0;
/* thread 5 */
static sakura_int32_t glob_thread_5_index = -1;
static sakura_uint32_t glob_thread_5_connect_flag = 0;
static sakura_uint32_t glob_thread_5_disconnect_flag = 0;
static sakura_uint32_t glob_thread_5_subscribe_flag = 0;
static sakura_uint32_t glob_thread_5_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_5_publish_flag = 0;
static sakura_int32_t glob_thread_5_status_code = 0;
/* thread 6 */
static sakura_int32_t glob_thread_6_index = -1;
static sakura_uint32_t glob_thread_6_connect_flag = 0;
static sakura_uint32_t glob_thread_6_disconnect_flag = 0;
static sakura_uint32_t glob_thread_6_subscribe_flag = 0;
static sakura_uint32_t glob_thread_6_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_6_publish_flag = 0;
static sakura_int32_t glob_thread_6_status_code = 0;
/* thread 7 */
static sakura_int32_t glob_thread_7_index = -1;
static sakura_uint32_t glob_thread_7_connect_flag = 0;
static sakura_uint32_t glob_thread_7_disconnect_flag = 0;
static sakura_uint32_t glob_thread_7_subscribe_flag = 0;
static sakura_uint32_t glob_thread_7_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_7_publish_flag = 0;
static sakura_int32_t glob_thread_7_status_code = 0;
/* thread 8 */
static sakura_int32_t glob_thread_8_index = -1;
static sakura_uint32_t glob_thread_8_connect_flag = 0;
static sakura_uint32_t glob_thread_8_disconnect_flag = 0;
static sakura_uint32_t glob_thread_8_subscribe_flag = 0;
static sakura_uint32_t glob_thread_8_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_8_publish_flag = 0;
static sakura_int32_t glob_thread_8_status_code = 0;
/* thread 9 */
static sakura_int32_t glob_thread_9_index = -1;
static sakura_uint32_t glob_thread_9_connect_flag = 0;
static sakura_uint32_t glob_thread_9_disconnect_flag = 0;
static sakura_uint32_t glob_thread_9_subscribe_flag = 0;
static sakura_uint32_t glob_thread_9_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_9_publish_flag = 0;
static sakura_int32_t glob_thread_9_status_code = 0;
/* thread 10 */
static sakura_int32_t glob_thread_10_index = -1;
static sakura_uint32_t glob_thread_10_connect_flag = 0;
static sakura_uint32_t glob_thread_10_disconnect_flag = 0;
static sakura_uint32_t glob_thread_10_subscribe_flag = 0;
static sakura_uint32_t glob_thread_10_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_10_publish_flag = 0;
static sakura_int32_t glob_thread_10_status_code = 0;
/* thread 11 */
static sakura_int32_t glob_thread_11_index = -1;
static sakura_uint32_t glob_thread_11_connect_flag = 0;
static sakura_uint32_t glob_thread_11_disconnect_flag = 0;
static sakura_uint32_t glob_thread_11_subscribe_flag = 0;
static sakura_uint32_t glob_thread_11_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_11_publish_flag = 0;
static sakura_int32_t glob_thread_11_status_code = 0;
/* thread 12 */
static sakura_int32_t glob_thread_12_index = -1;
static sakura_uint32_t glob_thread_12_connect_flag = 0;
static sakura_uint32_t glob_thread_12_disconnect_flag = 0;
static sakura_uint32_t glob_thread_12_subscribe_flag = 0;
static sakura_uint32_t glob_thread_12_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_12_publish_flag = 0;
static sakura_int32_t glob_thread_12_status_code = 0;
/* thread 13 */
static sakura_int32_t glob_thread_13_index = -1;
static sakura_uint32_t glob_thread_13_connect_flag = 0;
static sakura_uint32_t glob_thread_13_disconnect_flag = 0;
static sakura_uint32_t glob_thread_13_subscribe_flag = 0;
static sakura_uint32_t glob_thread_13_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_13_publish_flag = 0;
static sakura_int32_t glob_thread_13_status_code = 0;
/* thread 14 */
static sakura_int32_t glob_thread_14_index = -1;
static sakura_uint32_t glob_thread_14_connect_flag = 0;
static sakura_uint32_t glob_thread_14_disconnect_flag = 0;
static sakura_uint32_t glob_thread_14_subscribe_flag = 0;
static sakura_uint32_t glob_thread_14_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_14_publish_flag = 0;
static sakura_int32_t glob_thread_14_status_code = 0;
/* thread 15 */
static sakura_int32_t glob_thread_15_index = -1;
static sakura_uint32_t glob_thread_15_connect_flag = 0;
static sakura_uint32_t glob_thread_15_disconnect_flag = 0;
static sakura_uint32_t glob_thread_15_subscribe_flag = 0;
static sakura_uint32_t glob_thread_15_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_15_publish_flag = 0;
static sakura_int32_t glob_thread_15_status_code = 0;
/* thread 16 */
static sakura_int32_t glob_thread_16_index = -1;
static sakura_uint32_t glob_thread_16_connect_flag = 0;
static sakura_uint32_t glob_thread_16_disconnect_flag = 0;
static sakura_uint32_t glob_thread_16_subscribe_flag = 0;
static sakura_uint32_t glob_thread_16_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_16_publish_flag = 0;
static sakura_int32_t glob_thread_16_status_code = 0;
/* thread 17 */
static sakura_int32_t glob_thread_17_index = -1;
static sakura_uint32_t glob_thread_17_connect_flag = 0;
static sakura_uint32_t glob_thread_17_disconnect_flag = 0;
static sakura_uint32_t glob_thread_17_subscribe_flag = 0;
static sakura_uint32_t glob_thread_17_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_17_publish_flag = 0;
static sakura_int32_t glob_thread_17_status_code = 0;
/* thread 18 */
static sakura_int32_t glob_thread_18_index = -1;
static sakura_uint32_t glob_thread_18_connect_flag = 0;
static sakura_uint32_t glob_thread_18_disconnect_flag = 0;
static sakura_uint32_t glob_thread_18_subscribe_flag = 0;
static sakura_uint32_t glob_thread_18_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_18_publish_flag = 0;
static sakura_int32_t glob_thread_18_status_code = 0;
/* thread 19 */
static sakura_int32_t glob_thread_19_index = -1;
static sakura_uint32_t glob_thread_19_connect_flag = 0;
static sakura_uint32_t glob_thread_19_disconnect_flag = 0;
static sakura_uint32_t glob_thread_19_subscribe_flag = 0;
static sakura_uint32_t glob_thread_19_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_19_publish_flag = 0;
static sakura_int32_t glob_thread_19_status_code = 0;
/* thread 20 */
static sakura_int32_t glob_thread_20_index = -1;
static sakura_uint32_t glob_thread_20_connect_flag = 0;
static sakura_uint32_t glob_thread_20_disconnect_flag = 0;
static sakura_uint32_t glob_thread_20_subscribe_flag = 0;
static sakura_uint32_t glob_thread_20_unsubscribe_flag = 0;
static sakura_uint32_t glob_thread_20_publish_flag = 0;
static sakura_int32_t glob_thread_20_status_code = 0;
/* common callback */
static mqtt_cbs_t test_cbs = {
        client_on_connect,
        client_on_disconnect,
        client_on_subscribe,
        client_on_unsubscribe,
        client_on_status,
        client_on_message
};

static sakura_void_t test_clear_all_vars(sakura_void_t)
{
    glob_thread_publish_flag = 0;
    glob_thread_end_flag = 0;
    glob_network_flag = 0;

    glob_thread_1_index = -1;
    glob_thread_1_connect_flag = 0;
    glob_thread_1_disconnect_flag = 0;
    glob_thread_1_subscribe_flag = 0;
    glob_thread_1_unsubscribe_flag = 0;
    glob_thread_1_publish_flag = 0;
    glob_thread_1_status_code = 0;

    glob_thread_2_index = -1;
    glob_thread_2_connect_flag = 0;
    glob_thread_2_disconnect_flag = 0;
    glob_thread_2_subscribe_flag = 0;
    glob_thread_2_unsubscribe_flag = 0;
    glob_thread_2_publish_flag = 0;
    glob_thread_2_status_code = 0;

    glob_thread_3_index = -1;
    glob_thread_3_connect_flag = 0;
    glob_thread_3_disconnect_flag = 0;
    glob_thread_3_subscribe_flag = 0;
    glob_thread_3_unsubscribe_flag = 0;
    glob_thread_3_publish_flag = 0;
    glob_thread_3_status_code = 0;

    glob_thread_4_index = -1;
    glob_thread_4_connect_flag = 0;
    glob_thread_4_disconnect_flag = 0;
    glob_thread_4_subscribe_flag = 0;
    glob_thread_4_unsubscribe_flag = 0;
    glob_thread_4_publish_flag = 0;
    glob_thread_4_status_code = 0;

    glob_thread_5_index = -1;
    glob_thread_5_connect_flag = 0;
    glob_thread_5_disconnect_flag = 0;
    glob_thread_5_subscribe_flag = 0;
    glob_thread_5_unsubscribe_flag = 0;
    glob_thread_5_publish_flag = 0;
    glob_thread_5_status_code = 0;

    glob_thread_6_index = -1;
    glob_thread_6_connect_flag = 0;
    glob_thread_6_disconnect_flag = 0;
    glob_thread_6_subscribe_flag = 0;
    glob_thread_6_unsubscribe_flag = 0;
    glob_thread_6_publish_flag = 0;
    glob_thread_6_status_code = 0;

    glob_thread_7_index = -1;
    glob_thread_7_connect_flag = 0;
    glob_thread_7_disconnect_flag = 0;
    glob_thread_7_subscribe_flag = 0;
    glob_thread_7_unsubscribe_flag = 0;
    glob_thread_7_publish_flag = 0;
    glob_thread_7_status_code = 0;

    glob_thread_8_index = -1;
    glob_thread_8_connect_flag = 0;
    glob_thread_8_disconnect_flag = 0;
    glob_thread_8_subscribe_flag = 0;
    glob_thread_8_unsubscribe_flag = 0;
    glob_thread_8_publish_flag = 0;
    glob_thread_8_status_code = 0;

    glob_thread_9_index = -1;
    glob_thread_9_connect_flag = 0;
    glob_thread_9_disconnect_flag = 0;
    glob_thread_9_subscribe_flag = 0;
    glob_thread_9_unsubscribe_flag = 0;
    glob_thread_9_publish_flag = 0;
    glob_thread_9_status_code = 0;

    glob_thread_10_index = -1;
    glob_thread_10_connect_flag = 0;
    glob_thread_10_disconnect_flag = 0;
    glob_thread_10_subscribe_flag = 0;
    glob_thread_10_unsubscribe_flag = 0;
    glob_thread_10_publish_flag = 0;
    glob_thread_10_status_code = 0;

    glob_thread_11_index = -1;
    glob_thread_11_connect_flag = 0;
    glob_thread_11_disconnect_flag = 0;
    glob_thread_11_subscribe_flag = 0;
    glob_thread_11_unsubscribe_flag = 0;
    glob_thread_11_publish_flag = 0;
    glob_thread_11_status_code = 0;

    glob_thread_12_index = -1;
    glob_thread_12_connect_flag = 0;
    glob_thread_12_disconnect_flag = 0;
    glob_thread_12_subscribe_flag = 0;
    glob_thread_12_unsubscribe_flag = 0;
    glob_thread_12_publish_flag = 0;
    glob_thread_12_status_code = 0; 

    glob_thread_13_index = -1;
    glob_thread_13_connect_flag = 0;
    glob_thread_13_disconnect_flag = 0;
    glob_thread_13_subscribe_flag = 0;
    glob_thread_13_unsubscribe_flag = 0;
    glob_thread_13_publish_flag = 0;
    glob_thread_13_status_code = 0; 

    glob_thread_14_index = -1;
    glob_thread_14_connect_flag = 0;
    glob_thread_14_disconnect_flag = 0;
    glob_thread_14_subscribe_flag = 0;
    glob_thread_14_unsubscribe_flag = 0;
    glob_thread_14_publish_flag = 0;
    glob_thread_14_status_code = 0; 

    glob_thread_15_index = -1;
    glob_thread_15_connect_flag = 0;
    glob_thread_15_disconnect_flag = 0;
    glob_thread_15_subscribe_flag = 0;
    glob_thread_15_unsubscribe_flag = 0;
    glob_thread_15_publish_flag = 0;
    glob_thread_15_status_code = 0; 

    glob_thread_16_index = -1;
    glob_thread_16_connect_flag = 0;
    glob_thread_16_disconnect_flag = 0;
    glob_thread_16_subscribe_flag = 0;
    glob_thread_16_unsubscribe_flag = 0;
    glob_thread_16_publish_flag = 0;
    glob_thread_16_status_code = 0; 

    glob_thread_17_index = -1;
    glob_thread_17_connect_flag = 0;
    glob_thread_17_disconnect_flag = 0;
    glob_thread_17_subscribe_flag = 0;
    glob_thread_17_unsubscribe_flag = 0;
    glob_thread_17_publish_flag = 0;
    glob_thread_17_status_code = 0; 

    glob_thread_18_index = -1;
    glob_thread_18_connect_flag = 0;
    glob_thread_18_disconnect_flag = 0;
    glob_thread_18_subscribe_flag = 0;
    glob_thread_18_unsubscribe_flag = 0;
    glob_thread_18_publish_flag = 0;
    glob_thread_18_status_code = 0;

    glob_thread_19_index = -1;
    glob_thread_19_connect_flag = 0;
    glob_thread_19_disconnect_flag = 0;
    glob_thread_19_subscribe_flag = 0;
    glob_thread_19_unsubscribe_flag = 0;
    glob_thread_19_publish_flag = 0;
    glob_thread_19_status_code = 0;

    glob_thread_20_index = -1;
    glob_thread_20_connect_flag = 0;
    glob_thread_20_disconnect_flag = 0;
    glob_thread_20_subscribe_flag = 0;
    glob_thread_20_unsubscribe_flag = 0;
    glob_thread_20_publish_flag = 0;
    glob_thread_20_status_code = 0;    
}

static sakura_void_t testcase_setup(sakura_void_t)
{
    test_network_up();
    test_clear_all_vars();
    test_init();
}
static sakura_void_t testcase_teardown(sakura_void_t)
{
    test_cleanup();
}

static sakura_void_t client_on_connect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============== on_connect ==============\n");
    printf("====   client index[%2d], code = %2d  ====\n", index, code);
    if(code == SAKURA_MQTT_CONNECT_ACCEPTED){
        if(index == glob_thread_1_index){
            glob_thread_1_connect_flag++;
        } else if(index == glob_thread_2_index){
            glob_thread_2_connect_flag++;
        } else if(index == glob_thread_3_index){
            glob_thread_3_connect_flag++;
        } else if(index == glob_thread_4_index){
            glob_thread_4_connect_flag++;
        }else if(index == glob_thread_5_index){
            glob_thread_5_connect_flag++;
        } else if(index == glob_thread_6_index){
            glob_thread_6_connect_flag++;
        } else if(index == glob_thread_7_index){
            glob_thread_7_connect_flag++;
        }else if(index == glob_thread_8_index){
            glob_thread_8_connect_flag++;
        } else if(index == glob_thread_9_index){
            glob_thread_9_connect_flag++;
        } else if(index == glob_thread_10_index){
            glob_thread_10_connect_flag++;
        }else if(index == glob_thread_11_index){
            glob_thread_11_connect_flag++;
        } else if(index == glob_thread_12_index){
            glob_thread_12_connect_flag++;
        } else if(index == glob_thread_13_index){
            glob_thread_13_connect_flag++;
        }else if(index == glob_thread_14_index){
            glob_thread_14_connect_flag++;
        } else if(index == glob_thread_15_index){
            glob_thread_15_connect_flag++;
        } else if(index == glob_thread_16_index){
            glob_thread_16_connect_flag++;
        }else if(index == glob_thread_17_index){
            glob_thread_17_connect_flag++;
        } else if(index == glob_thread_18_index){
            glob_thread_18_connect_flag++;
        } else if(index == glob_thread_19_index){
            glob_thread_19_connect_flag++;
        } else if(index == glob_thread_20_index){
            glob_thread_20_connect_flag++;
        } else {
            printf("unknow client index!\n");
        }
    }
}

static sakura_void_t client_on_disconnect(sakura_int32_t index, sakura_int32_t code)
{
    printf("============== on_disconn ==============\n");
    printf("====   client index[%2d], code = %2d  ====\n", index, code);
    if(code == SAKURA_MQTT_CONNECT_DISCONNECTED){
        if(index == glob_thread_1_index){
            glob_thread_1_disconnect_flag++;
        } else if(index == glob_thread_2_index){
            glob_thread_2_disconnect_flag++;
        } else if(index == glob_thread_3_index){
            glob_thread_3_disconnect_flag++;
        } else if(index == glob_thread_4_index){
            glob_thread_4_disconnect_flag++;
        }else if(index == glob_thread_5_index){
            glob_thread_5_disconnect_flag++;
        } else if(index == glob_thread_6_index){
            glob_thread_6_disconnect_flag++;
        } else if(index == glob_thread_7_index){
            glob_thread_7_disconnect_flag++;
        }else if(index == glob_thread_8_index){
            glob_thread_8_disconnect_flag++;
        } else if(index == glob_thread_9_index){
            glob_thread_9_disconnect_flag++;
        } else if(index == glob_thread_10_index){
            glob_thread_10_disconnect_flag++;
        }else if(index == glob_thread_11_index){
            glob_thread_11_disconnect_flag++;
        } else if(index == glob_thread_12_index){
            glob_thread_12_disconnect_flag++;
        } else if(index == glob_thread_13_index){
            glob_thread_13_disconnect_flag++;
        }else if(index == glob_thread_14_index){
            glob_thread_14_disconnect_flag++;
        } else if(index == glob_thread_15_index){
            glob_thread_15_disconnect_flag++;
        } else if(index == glob_thread_16_index){
            glob_thread_16_disconnect_flag++;
        }else if(index == glob_thread_17_index){
            glob_thread_17_disconnect_flag++;
        } else if(index == glob_thread_18_index){
            glob_thread_18_disconnect_flag++;
        } else if(index == glob_thread_19_index){
            glob_thread_19_disconnect_flag++;
        } else if(index == glob_thread_20_index){
            glob_thread_20_disconnect_flag++;
        } else {
            printf("unknow client index!\n");
        }
    }
}

static sakura_void_t client_on_subscribe(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num)
{
    sakura_uint32_t i;
    printf("============= on_subscribe =============\n");
    for (i = 0; i < code_num; i++){
        printf("===  client [%d] topic_%d code = %d\n", index, i, (sakura_int32_t)suback_code[i]);
    }
    if(index == glob_thread_1_index){
        glob_thread_1_subscribe_flag++;
    } else if(index == glob_thread_2_index){
        glob_thread_2_subscribe_flag++;
    } else if(index == glob_thread_3_index){
        glob_thread_3_subscribe_flag++;
    } else if(index == glob_thread_4_index){
        glob_thread_4_subscribe_flag++;
    }else if(index == glob_thread_5_index){
        glob_thread_5_subscribe_flag++;
    } else if(index == glob_thread_6_index){
        glob_thread_6_subscribe_flag++;
    } else if(index == glob_thread_7_index){
        glob_thread_7_subscribe_flag++;
    }else if(index == glob_thread_8_index){
        glob_thread_8_subscribe_flag++;
    } else if(index == glob_thread_9_index){
        glob_thread_9_subscribe_flag++;
    } else if(index == glob_thread_10_index){
        glob_thread_10_subscribe_flag++;
    }else if(index == glob_thread_11_index){
        glob_thread_11_subscribe_flag++;
    } else if(index == glob_thread_12_index){
        glob_thread_12_subscribe_flag++;
    } else if(index == glob_thread_13_index){
        glob_thread_13_subscribe_flag++;
    }else if(index == glob_thread_14_index){
        glob_thread_14_subscribe_flag++;
    } else if(index == glob_thread_15_index){
        glob_thread_15_subscribe_flag++;
    } else if(index == glob_thread_16_index){
        glob_thread_16_subscribe_flag++;
    }else if(index == glob_thread_17_index){
        glob_thread_17_subscribe_flag++;
    } else if(index == glob_thread_18_index){
        glob_thread_18_subscribe_flag++;
    } else if(index == glob_thread_19_index){
        glob_thread_19_subscribe_flag++;
    } else if(index == glob_thread_20_index){
        glob_thread_20_subscribe_flag++;
    } else {
        printf("unknow client index!\n");
    }
}

static sakura_void_t client_on_unsubscribe(sakura_int32_t index, sakura_int32_t code)
{
    printf("============ on_unsubscribe ============\n");
    printf("====   client index[%2d], code = %2d  ====\n", index, code);
    if(code == SAKURA_MQTT_UNSUBSCRIBE_SUCCESS){
        if(index == glob_thread_1_index){
            glob_thread_1_unsubscribe_flag++;
        } else if(index == glob_thread_2_index){
            glob_thread_2_unsubscribe_flag++;
        } else if(index == glob_thread_3_index){
            glob_thread_3_unsubscribe_flag++;
        } else if(index == glob_thread_4_index){
            glob_thread_4_unsubscribe_flag++;
        }else if(index == glob_thread_5_index){
            glob_thread_5_unsubscribe_flag++;
        } else if(index == glob_thread_6_index){
            glob_thread_6_unsubscribe_flag++;
        } else if(index == glob_thread_7_index){
            glob_thread_7_unsubscribe_flag++;
        }else if(index == glob_thread_8_index){
            glob_thread_8_unsubscribe_flag++;
        } else if(index == glob_thread_9_index){
            glob_thread_9_unsubscribe_flag++;
        } else if(index == glob_thread_10_index){
            glob_thread_10_unsubscribe_flag++;
        }else if(index == glob_thread_11_index){
            glob_thread_11_unsubscribe_flag++;
        } else if(index == glob_thread_12_index){
            glob_thread_12_unsubscribe_flag++;
        } else if(index == glob_thread_13_index){
            glob_thread_13_unsubscribe_flag++;
        }else if(index == glob_thread_14_index){
            glob_thread_14_unsubscribe_flag++;
        } else if(index == glob_thread_15_index){
            glob_thread_15_unsubscribe_flag++;
        } else if(index == glob_thread_16_index){
            glob_thread_16_unsubscribe_flag++;
        }else if(index == glob_thread_17_index){
            glob_thread_17_unsubscribe_flag++;
        } else if(index == glob_thread_18_index){
            glob_thread_18_unsubscribe_flag++;
        } else if(index == glob_thread_19_index){
            glob_thread_19_unsubscribe_flag++;
        } else if(index == glob_thread_20_index){
            glob_thread_20_unsubscribe_flag++;
        } else {
            printf("unknow client index!\n");
        }    
    }
}

static sakura_void_t client_on_status(sakura_int32_t index, sakura_int32_t code)
{
    printf("============== on_status  ==============\n");
    printf("====   client index[%2d], code = %2d  ====\n", index, code);
    if(index == glob_thread_1_index){
        glob_thread_1_status_code = code;
    } else if(index == glob_thread_2_index){
        glob_thread_2_status_code = code;
    } else if(index == glob_thread_3_index){
        glob_thread_3_status_code = code;
    } else if(index == glob_thread_4_index){
        glob_thread_4_status_code = code;
    }else if(index == glob_thread_5_index){
        glob_thread_5_status_code = code;
    } else if(index == glob_thread_6_index){
        glob_thread_6_status_code = code;
    } else if(index == glob_thread_7_index){
        glob_thread_7_status_code = code;
    }else if(index == glob_thread_8_index){
        glob_thread_8_status_code = code;
    } else if(index == glob_thread_9_index){
        glob_thread_9_status_code = code;
    } else if(index == glob_thread_10_index){
        glob_thread_10_status_code = code;
    }else if(index == glob_thread_11_index){
        glob_thread_11_status_code = code;
    } else if(index == glob_thread_12_index){
        glob_thread_12_status_code = code;
    } else if(index == glob_thread_13_index){
        glob_thread_13_status_code = code;
    }else if(index == glob_thread_14_index){
        glob_thread_14_status_code = code;
    } else if(index == glob_thread_15_index){
        glob_thread_15_status_code = code;
    } else if(index == glob_thread_16_index){
        glob_thread_16_status_code = code;
    }else if(index == glob_thread_17_index){
        glob_thread_17_status_code = code;
    } else if(index == glob_thread_18_index){
        glob_thread_18_status_code = code;
    } else if(index == glob_thread_19_index){
        glob_thread_19_status_code = code;
    } else if(index == glob_thread_20_index){
        glob_thread_20_status_code = code;
    } else {
        printf("unknow client index!\n");
    } 
}

static sakura_void_t client_on_message(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *msg, sakura_uint32_t msg_len)
{
    sakura_uint8_t out_buf[2048] = {0};
    printf("============== on_message ==============\n");
    printf("index = %d, topic = %s, len = %d\n", index, topic, msg_len);
    memcpy(out_buf, msg, msg_len);
    printf("message : %s\n", out_buf);
}

static sakura_void_t client_on_publish(sakura_int32_t index, sakura_int32_t code)
{
    if(index == glob_thread_1_index){
        glob_thread_1_publish_flag++;
    } else if(index == glob_thread_2_index){
        glob_thread_2_publish_flag++;
    } else if(index == glob_thread_3_index){
        glob_thread_3_publish_flag++;
    } else if(index == glob_thread_4_index){
        glob_thread_4_publish_flag++;
    }else if(index == glob_thread_5_index){
        glob_thread_5_publish_flag++;
    } else if(index == glob_thread_6_index){
        glob_thread_6_publish_flag++;
    } else if(index == glob_thread_7_index){
        glob_thread_7_publish_flag++;
    }else if(index == glob_thread_8_index){
        glob_thread_8_publish_flag++;
    } else if(index == glob_thread_9_index){
        glob_thread_9_publish_flag++;
    } else if(index == glob_thread_10_index){
        glob_thread_10_publish_flag++;
    }else if(index == glob_thread_11_index){
        glob_thread_11_publish_flag++;
    } else if(index == glob_thread_12_index){
        glob_thread_12_publish_flag++;
    } else if(index == glob_thread_13_index){
        glob_thread_13_publish_flag++;
    }else if(index == glob_thread_14_index){
        glob_thread_14_publish_flag++;
    } else if(index == glob_thread_15_index){
        glob_thread_15_publish_flag++;
    } else if(index == glob_thread_16_index){
        glob_thread_16_publish_flag++;
    }else if(index == glob_thread_17_index){
        glob_thread_17_publish_flag++;
    } else if(index == glob_thread_18_index){
        glob_thread_18_publish_flag++;
    } else if(index == glob_thread_19_index){
        glob_thread_19_publish_flag++;
    } else if(index == glob_thread_20_index){
        glob_thread_20_publish_flag++;
    } else {
        printf("unknow client index!\n");
    }
}

static sakura_void_t *thread_1(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    mqtt_message_t message = {0};
    sakura_char_t * payload = TEST_THREAD_1_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_2;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_3;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_4;

    index = sakura_mqtt_open(TEST_THREAD_1);
    CU_ASSERT_NOT_EQUAL(index, -1);
    glob_thread_1_index = index;
    sakura_mqtt_connect(index, &account, &test_cbs);

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* connect */
        if(glob_thread_1_connect_flag == 1 && glob_thread_1_subscribe_flag == 0){
            sakura_mqtt_subscribe(index, sub_list, 3);
            glob_thread_1_connect_flag ++;
        }
        
        if(glob_thread_1_subscribe_flag == 1 && tick%50 == 0){
            sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
        }
    }
}

static sakura_void_t *thread_2(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    mqtt_message_t message = {0};
    sakura_char_t * payload = TEST_THREAD_2_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_1;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_3;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_4;

    index = sakura_mqtt_open(TEST_THREAD_2);
    CU_ASSERT_NOT_EQUAL(index, -1);
    glob_thread_2_index = index;
    sakura_mqtt_connect(index, &account, &test_cbs);

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* connect */
        if(glob_thread_2_connect_flag == 1 && glob_thread_2_subscribe_flag == 0){
            sakura_mqtt_subscribe(index, sub_list, 3);
            glob_thread_2_connect_flag ++;
        }
        
        if(glob_thread_2_subscribe_flag == 1 && tick%50 == 0){
            sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
        }
    }
}

static sakura_void_t *thread_3(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    mqtt_message_t message = {0};
    sakura_char_t * payload = TEST_THREAD_3_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_1;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_2;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_4;

    index = sakura_mqtt_open(TEST_THREAD_3);
    CU_ASSERT_NOT_EQUAL(index, -1);
    glob_thread_3_index = index;
    sakura_mqtt_connect(index, &account, &test_cbs);

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* connect */
        if(glob_thread_3_connect_flag == 1 && glob_thread_3_subscribe_flag == 0){
            sakura_mqtt_subscribe(index, sub_list, 3);
            glob_thread_3_connect_flag ++;
        }
        
        if(glob_thread_3_subscribe_flag == 1 && tick%50 == 0){
            sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
        }
    }
}

static sakura_void_t *thread_4(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    mqtt_message_t message = {0};
    sakura_char_t * payload = TEST_THREAD_4_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_1;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_2;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_3;

    index = sakura_mqtt_open(TEST_THREAD_4);
    CU_ASSERT_NOT_EQUAL(index, -1);
    glob_thread_4_index = index;
    sakura_mqtt_connect(index, &account, &test_cbs);

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* connect */
        if(glob_thread_4_connect_flag == 1 && glob_thread_4_subscribe_flag == 0){
            sakura_mqtt_subscribe(index, sub_list, 3);
            glob_thread_4_connect_flag ++;
        }
        
        if(glob_thread_4_subscribe_flag == 1 && tick%50 == 0){
            sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
            sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
        }
    }
}

static sakura_void_t *thread_01(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_1_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_2;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_3;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_4;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }

        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_1);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_1_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_1_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_1_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %30 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_1_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_1_index = -1;
                glob_thread_1_connect_flag = 0;
                glob_thread_1_disconnect_flag = 0;
                glob_thread_1_subscribe_flag = 0;
                glob_thread_1_unsubscribe_flag = 0;
                glob_thread_1_publish_flag = 0;
                glob_thread_1_status_code = 0;
                sleep(5);
            }
        }
        
        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_02(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_2_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_1;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_3;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_4;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_2);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_2_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_2_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_2_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_2_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_2_index = -1;
                glob_thread_2_connect_flag = 0;
                glob_thread_2_disconnect_flag = 0;
                glob_thread_2_subscribe_flag = 0;
                glob_thread_2_unsubscribe_flag = 0;
                glob_thread_2_publish_flag = 0;
                glob_thread_2_status_code = 0;
                sleep(5);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_03(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_3_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_1;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_2;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_4;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_3);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_3_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_3_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_3_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %40 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_3_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_3_index = -1;
                glob_thread_3_connect_flag = 0;
                glob_thread_3_disconnect_flag = 0;
                glob_thread_3_subscribe_flag = 0;
                glob_thread_3_unsubscribe_flag = 0;
                glob_thread_3_publish_flag = 0;
                glob_thread_3_status_code = 0;
                sleep(5);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_04(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_4_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_1;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_2;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_3;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_4);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_4_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_4_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_4_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_4_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_4_index = -1;
                glob_thread_4_connect_flag = 0;
                glob_thread_4_disconnect_flag = 0;
                glob_thread_4_subscribe_flag = 0;
                glob_thread_4_unsubscribe_flag = 0;
                glob_thread_4_publish_flag = 0;
                glob_thread_4_status_code = 0;
                sleep(5);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_05(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_5_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_6;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_7;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_8;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_5);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_5_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_5_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_5_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %30 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_5_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_5_index = -1;
                glob_thread_5_connect_flag = 0;
                glob_thread_5_disconnect_flag = 0;
                glob_thread_5_subscribe_flag = 0;
                glob_thread_5_unsubscribe_flag = 0;
                glob_thread_5_publish_flag = 0;
                glob_thread_5_status_code = 0;
                sleep(3);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_06(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_6_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_5;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_7;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_8;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_6);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_6_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_6_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_6_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_6_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_6_index = -1;
                glob_thread_6_connect_flag = 0;
                glob_thread_6_disconnect_flag = 0;
                glob_thread_6_subscribe_flag = 0;
                glob_thread_6_unsubscribe_flag = 0;
                glob_thread_6_publish_flag = 0;
                glob_thread_6_status_code = 0;
                sleep(3);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_07(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_7_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_5;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_6;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_8;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_7);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_7_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_7_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_7_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %40 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_7_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_7_index = -1;
                glob_thread_7_connect_flag = 0;
                glob_thread_7_disconnect_flag = 0;
                glob_thread_7_subscribe_flag = 0;
                glob_thread_7_unsubscribe_flag = 0;
                glob_thread_7_publish_flag = 0;
                glob_thread_7_status_code = 0;
                sleep(3);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_08(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_8_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_5;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_6;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_7;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_8);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_8_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_8_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_8_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_8_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_8_index = -1;
                glob_thread_8_connect_flag = 0;
                glob_thread_8_disconnect_flag = 0;
                glob_thread_8_subscribe_flag = 0;
                glob_thread_8_unsubscribe_flag = 0;
                glob_thread_8_publish_flag = 0;
                glob_thread_8_status_code = 0;
                sleep(3);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_09(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_9_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_10;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_11;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_12;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_9);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_9_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_9_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_9_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %30 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_9_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_9_index = -1;
                glob_thread_9_connect_flag = 0;
                glob_thread_9_disconnect_flag = 0;
                glob_thread_9_subscribe_flag = 0;
                glob_thread_9_unsubscribe_flag = 0;
                glob_thread_9_publish_flag = 0;
                glob_thread_9_status_code = 0;
                sleep(4);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_10(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_10_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_9;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_11;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_12;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_10);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_10_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_10_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_10_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_10_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_10_index = -1;
                glob_thread_10_connect_flag = 0;
                glob_thread_10_disconnect_flag = 0;
                glob_thread_10_subscribe_flag = 0;
                glob_thread_10_unsubscribe_flag = 0;
                glob_thread_10_publish_flag = 0;
                glob_thread_10_status_code = 0;
                sleep(4);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_11(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_11_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_9;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_10;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_12;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_11);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_11_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_11_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_11_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %40 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_11_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_11_index = -1;
                glob_thread_11_connect_flag = 0;
                glob_thread_11_disconnect_flag = 0;
                glob_thread_11_subscribe_flag = 0;
                glob_thread_11_unsubscribe_flag = 0;
                glob_thread_11_publish_flag = 0;
                glob_thread_11_status_code = 0;
                sleep(4);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_12(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_12_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_9;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_10;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_11;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_12);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_12_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_12_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_12_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_12_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_12_index = -1;
                glob_thread_12_connect_flag = 0;
                glob_thread_12_disconnect_flag = 0;
                glob_thread_12_subscribe_flag = 0;
                glob_thread_12_unsubscribe_flag = 0;
                glob_thread_12_publish_flag = 0;
                glob_thread_12_status_code = 0;
                sleep(4);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_13(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_13_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_14;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_15;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_16;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_13);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_13_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_13_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_13_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %30 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_13_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_13_index = -1;
                glob_thread_13_connect_flag = 0;
                glob_thread_13_disconnect_flag = 0;
                glob_thread_13_subscribe_flag = 0;
                glob_thread_13_unsubscribe_flag = 0;
                glob_thread_13_publish_flag = 0;
                glob_thread_13_status_code = 0;
                sleep(7);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_14(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_14_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_13;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_15;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_16;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_14);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_14_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_14_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_14_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_14_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_14_index = -1;
                glob_thread_14_connect_flag = 0;
                glob_thread_14_disconnect_flag = 0;
                glob_thread_14_subscribe_flag = 0;
                glob_thread_14_unsubscribe_flag = 0;
                glob_thread_14_publish_flag = 0;
                glob_thread_14_status_code = 0;
                sleep(7);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_15(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_15_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_13;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_14;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_16;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_15);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_15_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_15_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_15_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %40 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_15_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_15_index = -1;
                glob_thread_15_connect_flag = 0;
                glob_thread_15_disconnect_flag = 0;
                glob_thread_15_subscribe_flag = 0;
                glob_thread_15_unsubscribe_flag = 0;
                glob_thread_15_publish_flag = 0;
                glob_thread_15_status_code = 0;
                sleep(7);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_16(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_16_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_13;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_14;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_15;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_16);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_16_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_16_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_16_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_16_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_16_index = -1;
                glob_thread_16_connect_flag = 0;
                glob_thread_16_disconnect_flag = 0;
                glob_thread_16_subscribe_flag = 0;
                glob_thread_16_unsubscribe_flag = 0;
                glob_thread_16_publish_flag = 0;
                glob_thread_16_status_code = 0;
                sleep(7);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_17(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_17_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_18;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_19;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_20;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_17);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_17_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_17_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_17_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %30 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_17_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_17_index = -1;
                glob_thread_17_connect_flag = 0;
                glob_thread_17_disconnect_flag = 0;
                glob_thread_17_subscribe_flag = 0;
                glob_thread_17_unsubscribe_flag = 0;
                glob_thread_17_publish_flag = 0;
                glob_thread_17_status_code = 0;
                sleep(1);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_18(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_18_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_17;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_19;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_20;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_18);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_18_index = index;
            open_flag = 1;
        }
        /* connect */
        if(open_flag == 1 && glob_thread_18_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_18_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_18_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_18_index = -1;
                glob_thread_18_connect_flag = 0;
                glob_thread_18_disconnect_flag = 0;
                glob_thread_18_subscribe_flag = 0;
                glob_thread_18_unsubscribe_flag = 0;
                glob_thread_18_publish_flag = 0;
                glob_thread_18_status_code = 0;
                sleep(1);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_19(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_19_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_17;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_18;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_20;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_19);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_19_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_19_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_19_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %40 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_19_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_19_index = -1;
                glob_thread_19_connect_flag = 0;
                glob_thread_19_disconnect_flag = 0;
                glob_thread_19_subscribe_flag = 0;
                glob_thread_19_unsubscribe_flag = 0;
                glob_thread_19_publish_flag = 0;
                glob_thread_19_status_code = 0;
                sleep(1);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t *thread_20(sakura_void_t)
{
    sakura_uint32_t tick = 0;
    sakura_int32_t index = SAKURA_MQTT_ERROR;
    sakura_int32_t open_flag = 0;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    mqtt_message_t message = {0};
    sakura_mqtt_topic_t sub_list[3] = {0};
    sakura_uint32_t net_error_flag = 0;
    sakura_char_t * payload = TEST_THREAD_20_PAYLOAD;
    message.qos = QOS1;
    message.payloadlen = strlen(payload);
    message.retained = 0;
    message.payload = (sakura_uint8_t*)payload;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;

    broker.hostname = TEST_HOST;
    broker.port = TEST_PORT;
    
    account.broker = &broker;
    sub_list[0].qos = QOS1;
    sub_list[0].topic = TEST_THREAD_17;
    sub_list[1].qos = QOS1;
    sub_list[1].topic = TEST_THREAD_18;
    sub_list[2].qos = QOS1;
    sub_list[2].topic = TEST_THREAD_19;

    while(glob_thread_end_flag == 0){
        tick ++;
        /* reset tick */
        if(tick > 300){
            tick = 0;
        }

        /* set net error flag */
        if(glob_thread_1_status_code == SAKURA_MQTT_NETWORK_ERROR){
            net_error_flag = 1;
        }

        /* reconnect success, reset value */
        if(net_error_flag == 1 && glob_thread_1_status_code == SAKURA_MQTT_CONNECT_ACCEPTED){
            tick = 1;
            net_error_flag == 0;
        }

        /* check network state */
        if(net_error_flag == 1){
            continue;
        }
        usleep(100 * 1000);
        /* open */
        if(open_flag == 0){
            index = sakura_mqtt_open(TEST_THREAD_20);
            CU_ASSERT_NOT_EQUAL(index, -1);
            glob_thread_20_index = index;
            open_flag = 1;
            sleep(1);
        }
        /* connect */
        if(open_flag == 1 && glob_thread_20_connect_flag == 0){
            sakura_mqtt_connect(index, &account, &test_cbs);
            open_flag = 2;
            sleep(1);
        }
        /* subscribe */
        if(open_flag == 2 && glob_thread_20_connect_flag == 1){
            sakura_mqtt_subscribe(index, sub_list, 3);
            open_flag = 3;
            sleep(1);
        }
        if(glob_thread_publish_flag == 1){
            if(open_flag == 3 && tick %35 == 0){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
            }
        } else {
            /* publish */
            if(open_flag == 3 && glob_thread_20_subscribe_flag == 1){
                sakura_mqtt_publish(index, sub_list[0].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[1].topic, &message, NULL);
                sakura_mqtt_publish(index, sub_list[2].topic, &message, NULL);
                open_flag = 4;
                sleep(1);
            }
            /* close */
            if(open_flag == 4){
                sleep(3);
                sakura_mqtt_close(index);
                open_flag = 0;
                glob_thread_20_index = -1;
                glob_thread_20_connect_flag = 0;
                glob_thread_20_disconnect_flag = 0;
                glob_thread_20_subscribe_flag = 0;
                glob_thread_20_unsubscribe_flag = 0;
                glob_thread_20_publish_flag = 0;
                glob_thread_20_status_code = 0;
                sleep(1);
            }
        }

        /* test end */
        if(tick == 1000){
            break;
        }
    }
}

static sakura_void_t test_multithread_publish(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_uint32_t tick = 0;
    pthread_t p_1 = 0;
    pthread_t p_2 = 0;
    pthread_t p_3 = 0;
    pthread_t p_4 = 0;
    do
    {
        /* create 4 thread */
        ret = pthread_create(&p_1, NULL, thread_1, NULL);
        ret = pthread_create(&p_2, NULL, thread_2, NULL);
        ret = pthread_create(&p_3, NULL, thread_3, NULL);
        ret = pthread_create(&p_4, NULL, thread_4, NULL);   
        while(1){
            tick++;
            usleep(100 * 1000);
            if(tick > 500){
                glob_thread_end_flag = 1;
                break;
            }
        }
    } while (SAKURA_FALSE);
    pthread_join(p_1, NULL);
    pthread_join(p_2, NULL);
    pthread_join(p_3, NULL);
    pthread_join(p_4, NULL);
}

static sakura_void_t test_weak_net(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_uint32_t net_flag = 0;
    sakura_uint32_t tick = 0;
    pthread_t p_1 = 0;
    pthread_t p_2 = 0;
    pthread_t p_3 = 0;
    pthread_t p_4 = 0;
    do
    {
        /* create 4 thread */
        ret = pthread_create(&p_1, NULL, thread_1, NULL);
        ret = pthread_create(&p_2, NULL, thread_2, NULL);
        ret = pthread_create(&p_3, NULL, thread_3, NULL);
        ret = pthread_create(&p_4, NULL, thread_4, NULL);   
        while(1){
            /* control network */
            sleep(5);
            test_network_down();
            sleep(5);
            test_network_up();

            /* break; */
            if(tick++ > 30){
                glob_thread_end_flag = 1;
                break;
            }
        }
    } while (SAKURA_FALSE);
    pthread_join(p_1, NULL);
    pthread_join(p_2, NULL);
    pthread_join(p_3, NULL);
    pthread_join(p_4, NULL);

    if(net_flag == 1){
        test_network_up();
    }
}

static sakura_void_t test_open_close(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_uint32_t net_flag = 0;
    sakura_uint32_t tick = 0;
    pthread_t p_1 = 0;
    pthread_t p_2 = 0;
    pthread_t p_3 = 0;
    pthread_t p_4 = 0;
    do
    {
        /* create 4 thread */
        ret = pthread_create(&p_1, NULL, thread_01, NULL);
        ret = pthread_create(&p_2, NULL, thread_02, NULL);
        ret = pthread_create(&p_3, NULL, thread_03, NULL);
        ret = pthread_create(&p_4, NULL, thread_04, NULL);   
        while(1){
            tick++;
            usleep(100 * 1000);

            /* break */
            if(tick > 1000){
                glob_thread_end_flag = 1;
                break;
            }
        }
    } while (SAKURA_FALSE);
    pthread_join(p_1, NULL);
    pthread_join(p_2, NULL);
    pthread_join(p_3, NULL);
    pthread_join(p_4, NULL);
}

static sakura_void_t test_20_clients_work(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_uint32_t net_flag = 0;
    sakura_uint32_t tick = 0;
    pthread_t p_1 = 0;
    pthread_t p_2 = 0;
    pthread_t p_3 = 0;
    pthread_t p_4 = 0;
    pthread_t p_5 = 0;
    pthread_t p_6 = 0;
    pthread_t p_7 = 0;
    pthread_t p_8 = 0;
    pthread_t p_9 = 0;
    pthread_t p_10 = 0;
    pthread_t p_11 = 0;
    pthread_t p_12 = 0;
    pthread_t p_13 = 0;
    pthread_t p_14 = 0;
    pthread_t p_15 = 0;
    pthread_t p_16 = 0;
    pthread_t p_17 = 0;
    pthread_t p_18 = 0;
    pthread_t p_19 = 0;
    pthread_t p_20 = 0;
    do
    {
        /* create 4 thread */
        ret = pthread_create(&p_1,  NULL, thread_01, NULL);
        ret = pthread_create(&p_2,  NULL, thread_02, NULL);
        ret = pthread_create(&p_3,  NULL, thread_03, NULL);
        ret = pthread_create(&p_4,  NULL, thread_04, NULL); 
        ret = pthread_create(&p_5,  NULL, thread_05, NULL);
        ret = pthread_create(&p_6,  NULL, thread_06, NULL);
        ret = pthread_create(&p_7,  NULL, thread_07, NULL);
        ret = pthread_create(&p_8,  NULL, thread_08, NULL); 
        ret = pthread_create(&p_9,  NULL, thread_09, NULL);
        ret = pthread_create(&p_10, NULL, thread_10, NULL);
        ret = pthread_create(&p_11, NULL, thread_11, NULL);
        ret = pthread_create(&p_12, NULL, thread_12, NULL); 
        ret = pthread_create(&p_13, NULL, thread_13, NULL);
        ret = pthread_create(&p_14, NULL, thread_14, NULL);
        ret = pthread_create(&p_15, NULL, thread_15, NULL);
        ret = pthread_create(&p_16, NULL, thread_16, NULL); 
        ret = pthread_create(&p_17, NULL, thread_17, NULL);
        ret = pthread_create(&p_18, NULL, thread_18, NULL);
        ret = pthread_create(&p_19, NULL, thread_19, NULL);
        ret = pthread_create(&p_20, NULL, thread_20, NULL);   
        while(1){
            tick++;
            usleep(100 * 1000);

            /* break */
            if(tick > 1500){
                glob_thread_end_flag = 1;
                break;
            }
        }
    } while (SAKURA_FALSE);
    pthread_join(p_1,  NULL);
    pthread_join(p_2,  NULL);
    pthread_join(p_3,  NULL);
    pthread_join(p_4,  NULL);
    pthread_join(p_5,  NULL);
    pthread_join(p_6,  NULL);
    pthread_join(p_7,  NULL);
    pthread_join(p_8,  NULL);
    pthread_join(p_9,  NULL);
    pthread_join(p_10, NULL);
    pthread_join(p_11, NULL);
    pthread_join(p_12, NULL);
    pthread_join(p_13, NULL);
    pthread_join(p_14, NULL);
    pthread_join(p_15, NULL);
    pthread_join(p_16, NULL);
    pthread_join(p_17, NULL);
    pthread_join(p_18, NULL);
    pthread_join(p_19, NULL);
    pthread_join(p_20, NULL);
}

static sakura_void_t twenty_clients_work_wrapper(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_uint32_t net_flag = 0;
    sakura_uint32_t tick = 0;
    pthread_t p_1 = 0;
    pthread_t p_2 = 0;
    pthread_t p_3 = 0;
    pthread_t p_4 = 0;
    pthread_t p_5 = 0;
    pthread_t p_6 = 0;
    pthread_t p_7 = 0;
    pthread_t p_8 = 0;
    pthread_t p_9 = 0;
    pthread_t p_10 = 0;
    pthread_t p_11 = 0;
    pthread_t p_12 = 0;
    pthread_t p_13 = 0;
    pthread_t p_14 = 0;
    pthread_t p_15 = 0;
    pthread_t p_16 = 0;
    pthread_t p_17 = 0;
    pthread_t p_18 = 0;
    pthread_t p_19 = 0;
    pthread_t p_20 = 0;
    do
    {
        /* create 4 thread */
        ret = pthread_create(&p_1,  NULL, thread_01, NULL);
        ret = pthread_create(&p_2,  NULL, thread_02, NULL);
        ret = pthread_create(&p_3,  NULL, thread_03, NULL);
        ret = pthread_create(&p_4,  NULL, thread_04, NULL); 
        ret = pthread_create(&p_5,  NULL, thread_05, NULL);
        ret = pthread_create(&p_6,  NULL, thread_06, NULL);
        ret = pthread_create(&p_7,  NULL, thread_07, NULL);
        ret = pthread_create(&p_8,  NULL, thread_08, NULL); 
        ret = pthread_create(&p_9,  NULL, thread_09, NULL);
        ret = pthread_create(&p_10, NULL, thread_10, NULL);
        ret = pthread_create(&p_11, NULL, thread_11, NULL);
        ret = pthread_create(&p_12, NULL, thread_12, NULL); 
        ret = pthread_create(&p_13, NULL, thread_13, NULL);
        ret = pthread_create(&p_14, NULL, thread_14, NULL);
        ret = pthread_create(&p_15, NULL, thread_15, NULL);
        ret = pthread_create(&p_16, NULL, thread_16, NULL); 
        ret = pthread_create(&p_17, NULL, thread_17, NULL);
        ret = pthread_create(&p_18, NULL, thread_18, NULL);
        ret = pthread_create(&p_19, NULL, thread_19, NULL);
        ret = pthread_create(&p_20, NULL, thread_20, NULL);  

        while(1){
            /* control network */
            if(glob_network_flag == 1){
                sleep(5);
                test_network_down();
                sleep(5);
                test_network_up();                
            } else {
                sleep(10);
            }

            /* break; */
            if(tick++ > 30){
                glob_thread_end_flag = 1;
                break;
            }
        }
    } while (SAKURA_FALSE);
    pthread_join(p_1,  NULL);
    pthread_join(p_2,  NULL);
    pthread_join(p_3,  NULL);
    pthread_join(p_4,  NULL);
    pthread_join(p_5,  NULL);
    pthread_join(p_6,  NULL);
    pthread_join(p_7,  NULL);
    pthread_join(p_8,  NULL);
    pthread_join(p_9,  NULL);
    pthread_join(p_10, NULL);
    pthread_join(p_11, NULL);
    pthread_join(p_12, NULL);
    pthread_join(p_13, NULL);
    pthread_join(p_14, NULL);
    pthread_join(p_15, NULL);
    pthread_join(p_16, NULL);
    pthread_join(p_17, NULL);
    pthread_join(p_18, NULL);
    pthread_join(p_19, NULL);
    pthread_join(p_20, NULL);
}

static sakura_void_t test_twenty_clients_work()
{
    /* set publish flag */
    glob_thread_publish_flag = 0;
    glob_network_flag = 0;
    glob_thread_end_flag = 0;
    twenty_clients_work_wrapper();  
}

static sakura_void_t test_twenty_clients_weak_net()
{
    /* set publish flag */
    glob_thread_publish_flag = 1;
    glob_network_flag = 1;
    glob_thread_end_flag = 0;
    twenty_clients_work_wrapper();
}

sakura_int32_t test_multithread_suite()
{
    CU_TestInfo multithread_testcases[] = {
        {"multithread 001 publish\n",                                    test_multithread_publish},
        {"multithread 002 weak net\n",                                   test_weak_net},
        {"multithread 003 open/close\n",                                 test_open_close},
        {"multithread 004 20 clients work\n",                            test_twenty_clients_work},
        {"multithread 005 20 clients net\n",                             test_twenty_clients_weak_net},
        CU_TEST_INFO_NULL
    };

    CU_SuiteInfo multithread_suite[] = {
        {"multithread cases", NULL, NULL, testcase_setup, testcase_teardown, multithread_testcases},
        CU_SUITE_INFO_NULL
    };

    if (CUE_SUCCESS != CU_register_suites(multithread_suite)) {
        printf("Failed to add multithread suite\n");
        return -1;
    }

    return 0;
}