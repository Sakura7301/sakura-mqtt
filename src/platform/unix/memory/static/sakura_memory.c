#include <string.h>
#include "sakura_mem.h"
#include "sakura_log.h"
#include "sakura_mutex.h"
#include <pthread.h>
#include "include/heap.h"

#ifdef CONFIG_IMPL_DYNAMIC_MEMORY_SIZE
    #define SAKURA_SMEM_HEAP_SIZE                   CONFIG_IMPL_DYNAMIC_MEMORY_SIZE
#else
    /*
     * It is a experience value in RT-Thread with STM32L475VET6 mcu (5K) and Linux with X86-64 chip (6K).
     */
    #define SAKURA_SMEM_HEAP_SIZE                   (100*1024)
#endif

#define LOG_TAG                 "STATIC_MEM"
#define SMEM_LOGE               SAKURA_LOGE
#define SMEM_LOGI               SAKURA_LOGI
#define SMEM_LOGD               SAKURA_LOGD
#define SMEM_OK                 0
#define SMEM_ERROR              1


/*



 * glob memory struct
 */
static struct {
    enum {
        SMEM_UNINIT = 0,                            /* enum: uninit */
        SMEM_INIT,                                  /* enum: init */
    } state;                                        /* module state */
    sakura_uint8_t mem_buf[SAKURA_SMEM_HEAP_SIZE];        /* memory buffer */
    pthread_mutex_t mutex;                          /* mutex */
    heap_t heap_entity;                             /* heap entity */
    bin_t bins[BIN_COUNT];                          /* bins */
} glob_mem_context          =   {
    .state                  =   SMEM_UNINIT,
    .mem_buf                =   {0},
    .mutex                  =   PTHREAD_MUTEX_INITIALIZER,
    .heap_entity.start      =   0,
    .heap_entity.end        =   0
};



/**
 * @brief memory init
 * 
 * @return sakura_int32_t 
 */
sakura_int32_t sakura_mem_init()
{
    sakura_int32_t ret = SMEM_OK;
    sakura_int32_t loop = 0;

    (sakura_void_t)pthread_mutex_lock(&glob_mem_context.mutex);
    do
    {
        /* check init state */
        if(glob_mem_context.state != SMEM_UNINIT){
            SMEM_LOGE("already init, denied\n");
            ret = SMEM_ERROR;
            break;
        }

        /* clear bins buffer */
        memset(glob_mem_context.bins, 0, sizeof(glob_mem_context.bins));
        for (loop = 0; loop < BIN_COUNT; ++loop) {
            glob_mem_context.heap_entity.bins[loop] = &glob_mem_context.bins[loop];
        }

        /* clear the buffer first */
        memset(glob_mem_context.mem_buf, 0, SAKURA_SMEM_HEAP_SIZE);
        init_heap(&glob_mem_context.heap_entity, (sakura_long32_t)glob_mem_context.mem_buf, SAKURA_SMEM_HEAP_SIZE);

        /* set module state */
        glob_mem_context.state = SMEM_INIT;
        SMEM_LOGI("static memory init\n");
    } while (SAKURA_FALSE);

    pthread_mutex_unlock(&glob_mem_context.mutex);
    return ret;
}

/**
 * @brief mem cleanup
 * 
 * @return void 
 */
sakura_void_t sakura_mem_cleanup()
{
    /* check mem init state */
    if(glob_mem_context.state == SMEM_INIT){
        (sakura_void_t)pthread_mutex_lock(&glob_mem_context.mutex);
        /* set state */
        glob_mem_context.state = SMEM_UNINIT;
        pthread_mutex_unlock(&glob_mem_context.mutex); 
    } else {
        SMEM_LOGE("not init, ignoring cleanup request.\n");
    }
}

/**
 * @brief alloc a memory
 * 
 * @param size mem size
 * @return sakura_void_t* 
 */
sakura_void_t *sakura_malloc(sakura_size_t size)
{
    sakura_void_t *ptr = NULL;

    do
    {
        /* check init state */
        if(glob_mem_context.state == SMEM_UNINIT){
            SMEM_LOGE("memory pool not init!\n");
            break;
        }

        /* check memsize */
        if(size <= 0){
            SMEM_LOGE("invalid memory size!\n");
            break;
        }

        (sakura_void_t)pthread_mutex_lock(&glob_mem_context.mutex);
        ptr = heap_alloc(&glob_mem_context.heap_entity, size);
        pthread_mutex_unlock(&glob_mem_context.mutex);
    } while (SAKURA_FALSE);
    
    return ptr;
}

/**
 * @brief free a memory
 * 
 * @param ptr mem pointer
 * @return sakura_void_t 
 */
sakura_void_t sakura_free(sakura_void_t *ptr)
{
    do
    {
        /* check init state */
        if(glob_mem_context.state == SMEM_UNINIT){
            SMEM_LOGE("memory pool not init!\n");
            break;
        }

        /* check mem pointer */
        if(ptr == NULL){
            SMEM_LOGE("invalid memory pointer!\n");
            break;
        }
        (sakura_void_t)pthread_mutex_lock(&glob_mem_context.mutex);
        /* free mem */
        heap_free(&glob_mem_context.heap_entity, ptr);
        pthread_mutex_unlock(&glob_mem_context.mutex);
    }while (SAKURA_FALSE);
}