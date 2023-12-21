/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_daemon.c
 * @brief sdk daemon implement in unix platform
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */
#define _GNU_SOURCE

#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "sakura_mqtt_client.h"
#include "sakura_log.h"
#include "sakura_daemon.h"
#include "sakura_internal.h"

#define LOG_TAG              "DAEMON"
#define DAEMON_LOGE           SAKURA_LOGE
#define DAEMON_LOGD           SAKURA_LOGD

#ifdef __cplusplus
extern "C" {
#endif

static pthread_t glob_daemon_pid;
static sakura_uint8_t glob_daemon_flag = 0;

/**
 * @brief daemon entry
 *
 * @param[in] arg any data type
 * @return sakura_void_t* default return NULL
 */
static sakura_void_t *daemon_entry()
{
    /* init */
    struct timespec tp = {0};
    do {
        while (glob_daemon_flag != 0U) {
            /* drive SDK run once, monotonic time*/
            (sakura_void_t)clock_gettime(CLOCK_MONOTONIC, &tp);
            (sakura_void_t)sakura_mqtt_tick(tp.tv_sec);
            (sakura_void_t)usleep(1000 * 10);
        }
    }while (SAKURA_FALSE);

    return NULL;
}

/**
 * @brief start daemon
 *
 * @return sakura_int32_t 0 is suc, other is failed
 */
sakura_int32_t sakura_daemon_start(void)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do {
        /* check flag */
        if (glob_daemon_flag > 0U) {
            DAEMON_LOGE("already init\n");
            ret = SAKURA_MQTT_ERR_INITIALIZED;
            break;
        }

        glob_daemon_flag = 1U;
        errno = 0;
        /* create thread */
        ret = pthread_create(&glob_daemon_pid, NULL, daemon_entry, NULL);
        if (ret < 0) {
            DAEMON_LOGE("pthread create failed! errno = %d, info = %s\n", errno, strerror(errno));
            ret = SAKURA_MQTT_ERROR;
            break;
        }
        (sakura_void_t)pthread_setname_np(glob_daemon_pid, "sakura_mqtt_daemon");
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief stop daemon
 *
 * @return sakura_int32_t 0 is suc, other is failed
 */
sakura_int32_t sakura_daemon_stop(void)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_void_t *status = NULL;

    do {
        /* check flag */
        if (glob_daemon_flag == 0U) {
            DAEMON_LOGE("not init, denied\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        glob_daemon_flag = 0;
        errno = 0;
        /* add to main thread */
        ret = pthread_join(glob_daemon_pid, &status);
        if (ret < 0) {
            DAEMON_LOGE("pthread join failed!, errno = %d, info = %s\n", errno, strerror(errno));
            ret = SAKURA_MQTT_ERROR;
            break;
        }
    } while (SAKURA_FALSE);

    return ret;
}

#ifdef __cplusplus
}
#endif