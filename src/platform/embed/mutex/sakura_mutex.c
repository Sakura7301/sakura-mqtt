/*
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * This file implements the mutex lock API in unix-like platform.
 */
#include "sakura_mutex.h"
/*
 * static array mode
 */

sakura_int32_t sakura_mutex_init(sakura_mutex_t *mutex)
{
    (sakura_void_t)mutex;
    return 0;
}

sakura_int32_t sakura_mutex_lock(sakura_mutex_t *mutex)
{
    (sakura_void_t)mutex;
    return 0;
}

sakura_int32_t sakura_mutex_unlock(sakura_mutex_t *mutex)
{
    (sakura_void_t)mutex;
    return 0;
}

sakura_int32_t sakura_mutex_destroy(sakura_mutex_t *mutex)
{
    (sakura_void_t)mutex;
    return 0;
}
