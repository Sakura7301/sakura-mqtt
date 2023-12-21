/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_log.h
 * @brief header of sdk log print
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#ifndef SAKURA_LOG_H_
#define SAKURA_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sakura_types.h"

#ifdef CONFIG_LOG

sakura_int32_t sakura_set_log_config(sakura_sdk_log_conf_t *conf);

#ifdef CONFIG_SYS_UNIX
sakura_int32_t sakura_unix_printf(const sakura_char_t* log_str);
#endif

sakura_uint32_t log_config_get_level();

sakura_int32_t sakura_log_init(void);

sakura_int32_t sakura_log_deinit(void);

sakura_void_t sakura_log_tick(sakura_void_t);

/* log core */
sakura_void_t sakura_log(SAKURA_LOG_LEVEL level, const sakura_char_t *component, const sakura_char_t* func_name, const sakura_char_t *format, ...);

/*
 * To avoid compile error when utilize the symbol `##` in IAR Embedded Workbench,
 * we convert the one log call to twice.
 * Suggest using the following statement.
 * #define SAKURA_LOGD(fmt, ...)    sakura_log(LOG_LEVEL_FATAL, LOG_TAG, fmt, ##__VA_ARGS__);
 */
#define SAKURA_LOGF(...)          sakura_log(LOG_LEVEL_FATAL,   LOG_TAG, NULL,      __VA_ARGS__);
#define SAKURA_LOGE(...)          sakura_log(LOG_LEVEL_ERROR,   LOG_TAG, __func__,  __VA_ARGS__);
#define SAKURA_LOGW(...)          sakura_log(LOG_LEVEL_WARNING, LOG_TAG, NULL,      __VA_ARGS__);
#define SAKURA_LOGI(...)          sakura_log(LOG_LEVEL_INFO,    LOG_TAG, NULL,      __VA_ARGS__);
#define SAKURA_LOGD(...)          sakura_log(LOG_LEVEL_DEBUG,   LOG_TAG, NULL,      __VA_ARGS__);
#define SAKURA_LOGV(...)          sakura_log(LOG_LEVEL_VERBOSE, LOG_TAG, NULL,      __VA_ARGS__);
#else
#define SAKURA_LOGF(...)
#define SAKURA_LOGE(...)
#define SAKURA_LOGW(...)
#define SAKURA_LOGI(...)
#define SAKURA_LOGD(...)
#define SAKURA_LOGV(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* SAKURA_LOG_H_ */
