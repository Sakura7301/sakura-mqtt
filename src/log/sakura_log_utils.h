#ifndef SAKURA_LOG_UTILS_H_
#define SAKURA_LOG_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sakura_types.h"

/*need by customer, implement*/
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define LOG_HEADER_MAX_LEN      (128)
#define LOG_CONTENT_MAX_LEN     (1024)
#define LOG_PRINT_MAX_LEN       (LOG_CONTENT_MAX_LEN + LOG_HEADER_MAX_LEN)

#define LOG_PATH_MAX_LEN       (128)
//sakura_sdk_xx.log, 15bytes
#define LOG_FILE_MAX_LEN       (506) /* secure require */
#define LOG_FILE_PATH_MAX_LEN  (LOG_PATH_MAX_LEN + LOG_FILE_MAX_LEN)

typedef enum {
    LOG_SUCCESS = 0,
    LOG_ERROR_INVALID_PARAM     = -1,
    LOG_ERROR_OUT_OF_MEMORY     = -2,
    LOG_ERROR_GENERIC           = -3,
    LOG_ERROR_MKDIR_FAIL        = -4,
    LOG_ERROR_FILE_OPEN_FAIL    = -5,
    LOG_ERROR_FILE_WRITE_FAIL   = -6,
    LOG_ERROR_FILE_READ_FAIL    = -7,
    LOG_ERROR_INVALID_FILE_PATH = -8,
    LOG_ERROR_MAX               = -100,
} log_error_t;

#define LOG_USE_NEW_FILE_OPS_IMPL (0)

#ifdef __cplusplus
}
#endif

#endif /* SAKURA_LOG_UTILS_H_ */