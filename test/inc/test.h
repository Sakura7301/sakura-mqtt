#ifndef TEST_H__
#define TEST_H__
extern "C"
{
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <CUnit/Console.h>
#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include "sakura_mqtt.h"
#include "sakura_internal.h"
#include "sakura_mem.h"
#include "sakura_common.h"

}
#include "mock_api.h"

#define TEST_CLIENT_ID              "test_client"
#define TEST_CLIENT_ID_2            "test_client_2"
#define TEST_TOPIC                  "test/topic"
#define TEST_TOPIC_A                "test/topic/A"
#define TEST_TOPIC_B                "test/topic/B"
#define TEST_PAYLOAD                "this is a test payload"
#define TEST_HOST                   "broker.mqttdashboard.com"
#define TEST_WILL_TOPIC             "test/1/err"
#define TEST_WILL_PAYLOAD           "test_client is offline."
#define TEST_PORT                   1883
#define TEST_LARGE_PAYLOAD          "The sun is bright, the trees are green and the animals live happily. Mufasa: Look, Simba, everything the light touches is our kingdom. Simba: Wow! Mufasa: A king's time as ruler rises and falls like the sun. One day, Simba, the sun will set on my time here and rise with you as the new king. Simba: And this all be mine? Mufasa: Everything! Simba: Everything the light touches! What about that shadowy place? Mufase: That's beyond our borders, you must never go there, Simba. Simba: But I thought a king can do whatever he wants. Mufasa: Oh, there's more to being a king than getting your way all the time. Simba: There's more? Mufasa: Simba, everything you see exists together in a delicate balance. As king, you need to understand that balance and respect all the creatures from the crawling ant to the leaping antelope. Simba: But dad, don't we eat the antelope? Mufase: Yes, Simba. But let me explain. When we die, our bodies become the grass and the antelope eat the grass, and so we are all connected in the great circle of life. Mufasa: Simba, let me tell you something that my father told me. Look at the stars. The great kings of the past look down on us from those stars. Simba: Really? Mufasa: Yes, so whenever you feel alone, just remember that those kings will always be there to guide you, and so am I."

#define SAKURA_TEST_NETWORK_NAME      "enp0s3"

void test_network_up();
void test_network_down();
int test_init(void);
void test_cleanup(void);
int test_log_print(const char* log_str);

#endif
