/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_memory.c
 * @brief memory operation in unix platform
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#include "sakura_mem.h"

/**
 * @brief Request Memory Space.
 *
 * @param[in] size memory size
 * @return success: Pointer to managed memory    fail:NULL
 */
sakura_void_t *sakura_malloc(sakura_size_t size)
{
    /* Use the malloc function of the standard C library. */
    return malloc(size);
}

/**
 * @brief Free memory.
 *
 * @param[out] ptr The pointer that manages this memory.
 * @return sakura_void_t
 */
sakura_void_t sakura_free(sakura_void_t *ptr)
{
    /* Use the free function of the standard C library. */
    free(ptr);
}

