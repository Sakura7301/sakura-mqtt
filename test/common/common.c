#include "test.h"


void test_network_up()
{
    printf("network up\n");
    system("ifconfig "SAKURA_TEST_NETWORK_NAME" up");
}

void test_network_down()
{
    printf("network down\n");
    system("ifconfig "SAKURA_TEST_NETWORK_NAME" down");
}

int test_log_print(const char* log_str)
{
    return printf("%s", log_str);
}

int test_init(void)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    do
    {      
        ret = sakura_mqtt_init(20);
    } while (false);
    return ret;
}

void test_cleanup(void)
{
    sakura_mqtt_cleanup();
    test_network_up();
}