/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_mutex.h
 * @brief header of sdk mutex
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#ifndef SAKURA_MUTEX_H__
#define SAKURA_MUTEX_H__

#ifdef __cplusplus
extern "C" {
#endif

/* HEADER-BODY-COPY-BEGIN */
#include "sakura_types.h"

/*
 * Mutex lock
 * If you only use one thread, you can make all mutex interfaces return 0.
 */

#ifndef SAKURA_MUTEX_T
#define SAKURA_MUTEX_T

typedef struct {
    sakura_void_t     *mutex_entity;
} sakura_mutex_t;
#endif

sakura_int32_t sakura_mutex_init(sakura_mutex_t *mutex);
sakura_int32_t sakura_mutex_lock(sakura_mutex_t *mutex);
sakura_int32_t sakura_mutex_unlock(sakura_mutex_t *mutex);
sakura_int32_t sakura_mutex_destroy(sakura_mutex_t *mutex);
/* HEADER-BODY-COPY-END */
#ifdef __cplusplus
}
#endif
#endif
