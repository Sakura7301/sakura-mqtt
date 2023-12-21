/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_daemon.h
 * @brief header of sdk daemon
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#ifndef SAKURA_DAEMON_H__
#define SAKURA_DAEMON_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "sakura_types.h"

sakura_int32_t sakura_daemon_start(void);
sakura_int32_t sakura_daemon_stop(void);

#ifdef __cplusplus
}
#endif
#endif