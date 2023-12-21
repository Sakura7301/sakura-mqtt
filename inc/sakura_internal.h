/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sdk_internal.h
 * @brief internal header of sdk
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#ifndef SAKURA_SDK_INTERNAL_H__
#define SAKURA_SDK_INTERNAL_H__

#include "sakura_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_DAEMON
sakura_int32_t sakura_http_tick(sakura_uint64_t tick);

/**
 * @brief drive the MQTT life cycle
 *        This interface is allowed only if the platform is embedded.
 * 
 * @param tick tick
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_tick(sakura_uint64_t tick);
#endif
#ifdef __cplusplus
}
#endif

#endif /* SAKURA_SDK_INTERNAL_H__ */
