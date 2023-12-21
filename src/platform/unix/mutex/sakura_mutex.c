/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_mutex.c
 * @brief sdk mutex implement in unix platform
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#include "sakura_mutex.h"
#include "sakura_mem.h"
#include "sakura_log.h"
#include <pthread.h>
#include <unistd.h>

#define LOG_TAG                     "LOCK"
#define LOCK_LOGD                   SAKURA_LOGD
#define LOCK_LOGE                   SAKURA_LOGE
#define LOCK_LOGW                   SAKURA_LOGW
#define DEFAULT_MAX_WAIT_TIME       500
#define MUTEX_OK                    0
#define MUTEX_ERROR                 -1


typedef struct {
    pthread_mutex_t mutex_entity;   /* mutex */
    pthread_t owning_thread;        /* owner */
    sakura_uint32_t lock_count;       /* lock count */
} sakura_mutex_internal_t;


/**
 * @brief Initialize the mutex.
 *
 * @param[out] mutex Mutex that needs to be initialized.
 * @return success: 0        fail:-1
 */
sakura_int32_t sakura_mutex_init(sakura_mutex_t *mutex)
{
    sakura_int32_t ret = MUTEX_ERROR;

    do
    {
        /* check params */
        if(mutex == NULL){
            LOCK_LOGE("mutex init failed!\n");
            break;
        }

        /* init mutex */
        sakura_mutex_internal_t* mutex_internal = (sakura_mutex_internal_t*)sakura_malloc(sizeof(sakura_mutex_internal_t));
        if(mutex_internal == NULL){
            LOCK_LOGE("mutex malloc memory is failed!\n");
            ret = -1;
            break;
        }
        ret = pthread_mutex_init(&mutex_internal->mutex_entity, NULL);
        if(ret == MUTEX_OK){
            /* init success */
            mutex_internal->owning_thread = (pthread_t)0;
            mutex_internal->lock_count = 0;
            mutex->mutex_entity = mutex_internal;
        } else {
            /* init fail */
            if(mutex_internal != NULL){
                sakura_free(mutex_internal);
            }
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

/**
 * @brief Gets the mutex.
 *
 * @param[out] mutex The mutex you want to get.
 * @return success: 0        fail:-1 or some other negative number.
 */
sakura_int32_t sakura_mutex_lock(sakura_mutex_t *mutex)
{
    sakura_int32_t ret = MUTEX_ERROR;
    sakura_uint32_t count = 1;
    sakura_uint32_t try_count = 0;
    pthread_t self_id = (pthread_t)0;
    sakura_mutex_internal_t* mutex_internal = NULL;

    do {
        /* Returns -1 if the mutex does not exist. */
        if (mutex == NULL || mutex->mutex_entity == NULL) {
            ret = MUTEX_ERROR;
            break;
        }

        /* get self thread id */
        self_id = pthread_self();

        mutex_internal = (sakura_mutex_internal_t*)mutex->mutex_entity;
        if (mutex_internal->owning_thread == self_id) {
            /* the current thread already has a lock, directly increase the lock count */
            mutex_internal->lock_count++;
            ret = MUTEX_OK;
            break;
        }

        /* spinlock */
        while (mutex != NULL && mutex->mutex_entity != NULL) {
            if (mutex_internal->owning_thread != (pthread_t)0 && mutex_internal->owning_thread != self_id) {
                /* [owner]: other (delay) */
                if(count % DEFAULT_MAX_WAIT_TIME == 0){
                    LOCK_LOGE("take mutex failed! Current thread: %lu, Owning thread: %lu\n", self_id, mutex_internal->owning_thread);
                    break;
                }
                /* sleep 10 ms */
                usleep(10 * 1000);
                count ++;
            } else if(mutex_internal->owning_thread  == (pthread_t)0){
                /* [owner]: none (modify owner) */
                mutex_internal->owning_thread = self_id;
                mutex_internal->lock_count = 1;
            } else {
                /* [owner]: self (try lock) */
                if(try_count < 5){
                    ret = pthread_mutex_trylock(&mutex_internal->mutex_entity);
                    if (ret == MUTEX_OK) {
                        /* lock success, break */
                        break;
                    }
                    if(try_count == 0){
                        /* lock failed, try unlock once */
                        (sakura_void_t)pthread_mutex_unlock(&mutex_internal->mutex_entity);  
                    }
                    try_count++;
                } else {
                    /* take file 3th, break */
                    LOCK_LOGE("try take mutex failed! Current thread: %lu\n", self_id);
                    mutex_internal->owning_thread = (pthread_t)0;
                    mutex_internal->lock_count = 0;
                    break;
                }
                /* sleep 10 ms */
                usleep(10 * 1000);
            }
        }
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief release the mutex.
 *
 * @param[out] mutex The mutex you want to get.
 * @return success: 0        fail:-1 or some other negative number.
 */
sakura_int32_t sakura_mutex_unlock(sakura_mutex_t *mutex)
{
    sakura_int32_t ret = MUTEX_ERROR;
    sakura_mutex_internal_t* mutex_internal = NULL;

    do {
        /* Returns -1 if the mutex does not exist. */
        if (mutex == NULL || mutex->mutex_entity == NULL) {
            ret = MUTEX_ERROR;
            break;
        }

        mutex_internal = (sakura_mutex_internal_t*)mutex->mutex_entity;
        if (mutex_internal->owning_thread != pthread_self()) {
            /* the current thread does not have a lock and cannot be unlocked */
            ret = MUTEX_ERROR;
            break;
        }

        if (mutex_internal->lock_count > 1) {
            /* the current thread repeatedly acquires locks, only reducing the count without unlocking */
            mutex_internal->lock_count--;
            ret = MUTEX_OK;
            break;
        }

        /* Release the mutex. */
        ret = pthread_mutex_unlock(&mutex_internal->mutex_entity);
        mutex_internal->owning_thread = (pthread_t)0;
        mutex_internal->lock_count = 0;
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief Destroy the mutex.
 *
 * @param[out] mutex The amount of mutexes to destroy.
 * @return success: 0        fail:-1 or some other negative number.
 */
sakura_int32_t sakura_mutex_destroy(sakura_mutex_t *mutex)
{
    sakura_int32_t ret = MUTEX_ERROR;
    sakura_mutex_internal_t* mutex_internal = NULL;

    do {
        /* Returns -1 if the mutex does not exist. */
        if (mutex == NULL || mutex->mutex_entity == NULL) {
            ret = MUTEX_ERROR;
            break;
        }

        mutex_internal = (sakura_mutex_internal_t*)mutex->mutex_entity;

        /* check whether the lock was not held */
        if (mutex_internal->owning_thread != (pthread_t)0) {
            ret = pthread_mutex_unlock(&mutex_internal->mutex_entity);
            mutex_internal->owning_thread = (pthread_t)0;
            mutex_internal->lock_count = 0;
        }

        /* mutex destroy */
        ret = pthread_mutex_destroy(&mutex_internal->mutex_entity);
        if (mutex_internal != NULL) {
            sakura_free(mutex_internal); 
        }
        mutex->mutex_entity = NULL;
    } while (SAKURA_FALSE);

    return ret;
}