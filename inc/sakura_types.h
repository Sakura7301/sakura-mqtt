#ifndef SAKURA_MQTT_TYPES_H__
#define SAKURA_MQTT_TYPES_H__


#ifdef __cplusplus
extern "C" {
#endif
#include "sakura_autoconfig.h"


/* HEADER-BASIC-TYPES-COPY-BEGIN */

#include <stddef.h> /* NULL */

/*
 * basic types definition
 */
/* character */
#ifndef SAKURA_CHAR_T
#define SAKURA_CHAR_T
typedef char sakura_char_t;
#else
    #error "Error, sakura_char_t has been defined!"
#endif

/* 8bits signed integer */
#ifndef SAKURA_INT8_T
#define SAKURA_INT8_T
typedef signed char sakura_int8_t;
#else
    #error "Error, sakura_int8_t has been defined!"
#endif

/* 8bits unsigned integer */
#ifndef SAKURA_UINT8_T
#define SAKURA_UINT8_T
typedef unsigned char sakura_uint8_t;
#else
    #error "Error, sakura_uint8_t has been defined!"
#endif

/* 16bits signed integer */
#ifndef SAKURA_INT16_T
#define SAKURA_INT16_T
typedef signed short sakura_int16_t;
#else
    #error "Error, sakura_int16_t has been defined!"
#endif

/* 16bits unsigned integer */
#ifndef SAKURA_UINT16_T
#define SAKURA_UINT16_T
typedef unsigned short sakura_uint16_t;
#else
    #error "Error, sakura_uint16_t has been defined!"
#endif

/* 32bits signed integer */
#ifndef SAKURA_INT32_t
#define SAKURA_INT32_t
typedef signed int sakura_int32_t;
#else
    #error "Error, sakura_int32_t has been defined!"
#endif

/* 32bits unsigned integer */
#ifndef SAKURA_UINT32_T
#define SAKURA_UINT32_T
typedef unsigned int sakura_uint32_t;
#else
    #error "Error, sakura_uint32_t has been defined!"
#endif

#ifndef SAKURA_ULONG32_T
#define SAKURA_ULONG32_T
typedef unsigned long sakura_ulong32_t;
#else
    #error "Error, sakura_ulong32_t has been defined!"
#endif

#ifndef SAKURA_LONG32_T
#define SAKURA_LONG32_T
typedef signed long sakura_long32_t;
#else
    #error "Error, sakura_long32_t has been defined!"
#endif

/* size_t */
#ifndef SAKURA_SIZE_T
#define SAKURA_SIZE_T
typedef size_t sakura_size_t;
#else
    #error "Error, sakura_size_t has been defined!"
#endif

/* 64bits signed integer */
#ifndef SAKURA_INT64_T
#define SAKURA_INT64_T
typedef signed long long sakura_int64_t;
#else
    #error "Error, sakura_int64_t has been defined!"
#endif

/* 64bits unsigned integer */
#ifndef SAKURA_UINT64_T
#define SAKURA_UINT64_T
typedef unsigned long long sakura_uint64_t;
#else
    #error "Error, sakura_uint64_t has been defined!"
#endif

/* single precision float number */
#ifndef SAKURA_FLOAT32_T
#define SAKURA_FLOAT32_T
typedef float sakura_float32_t;
#else
    #error "Error, sakura_float32_t has been defined!"
#endif

/* double precision float number */
#ifndef SAKURA_FLOAT64_T
#define SAKURA_FLOAT64_T
typedef double sakura_float64_t;
#else
    #error "Error, sakura_float64_t has been defined!"
#endif

/* void */
#ifndef SAKURA_VOID_T
#define SAKURA_VOID_T
typedef void sakura_void_t;
#else
    #error "Error, sakura_void_t has been defined!"
#endif

/* time_t */
#ifndef SAKURA_TIME_T
#define SAKURA_TIME_T
typedef sakura_uint64_t sakura_time_t;
#else
    #error "Error, sakura_time_t has been defined!"
#endif

/* NULL */
#ifndef SAKURA_NULL
#define SAKURA_NULL    ( void * ) 0
#else
    #error "Error, SAKURA_NULL has been defined!"
#endif

/* boolean representation */
#ifndef SAKURA_BOOT_T
#define SAKURA_BOOT_T
typedef enum {
    /* FALSE value */
    SAKURA_FALSE,
    /* TRUE value */
    SAKURA_TRUE
} sakura_bool_t;
#else
    #error "Error, sakura_bool_t has been defined!"
#endif

#ifndef SAKURA_SOCK_HOST_T
#define SAKURA_SOCK_HOST_T

/* socket host struct */
typedef struct {
    const sakura_char_t *hostname;
    sakura_uint16_t port;
} sakura_sock_host_t;
#else
    #error "Error, sakura_sock_host_t has been defined!"
#endif

#ifndef SAKURA_LOG_LEVEL_T
#define SAKURA_LOG_LEVEL_T
/* log level */
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE,
} SAKURA_LOG_LEVEL;
#else
    #error "Error, SAKURA_LOG_LEVEL has been defined!"
#endif

/* log print function pointer */
#ifndef SAKURA_LOG_PRINT_T
#define SAKURA_LOG_PRINT_T
typedef sakura_int32_t (*sakura_log_print_func_t)(const sakura_char_t* log_str);
#else
    #error "Error, sakura_log_print_func_t has been defined!"
#endif

#ifndef SAKURA_SDK_LOG_CONF_T
#define SAKURA_SDK_LOG_CONF_T
/* log config */
typedef struct {
    SAKURA_LOG_LEVEL level;
    sakura_bool_t is_color_console;
    sakura_log_print_func_t print_function;
} sakura_sdk_log_conf_t;
#else
    #error "Error, sakura_sdk_log_conf_t has been defined!"
#endif

/* HEADER-BASIC-TYPES-COPY-END */

#ifdef __cplusplus
}
#endif

#endif