#ifndef SAKURA_MEMORY_H__
#define SAKURA_MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include "sakura_types.h"

#ifdef CONFIG_IMPL_STATIC_MEMORY
    sakura_int32_t sakura_mem_init();
    sakura_void_t sakura_mem_cleanup();
#endif


/* HEADER-BODY-COPY-BEGIN */

/**
 * @brief Request Memory Space.
 *
 * @param[in] size memory size
 * @return success: Pointer to managed memory    fail:NULL
 */
sakura_void_t *sakura_malloc(sakura_size_t size);

/**
 * @brief Free memory.
 *
 * @param[out] ptr The pointer that manages this memory.
 * @return sakura_void_t
 */
sakura_void_t sakura_free(sakura_void_t *ptr);

/* HEADER-BODY-COPY-END */
#ifdef __cplusplus
}
#endif
#endif
