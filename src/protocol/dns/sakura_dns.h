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

#define SAKURA_DNS_SERVERS                        "223.5.5.5", "223.6.6.6", "114.114.114.114"
#define DOT_NAME_LEN                            256

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffffU
#endif


typedef struct {
    sakura_uint8_t index;       /*ac_dns_servers index*/
    sakura_uint8_t times;
    sakura_char_t server_host[DOT_NAME_LEN];
} sakura_dns_status_t;

sakura_int32_t sakura_dns_recv_response(sakura_int32_t sock, const sakura_uint8_t* recv_buf, sakura_uint32_t len);
sakura_void_t sakura_dns_send_request(sakura_int32_t sock, sakura_dns_status_t* status);
sakura_int32_t sakura_dns_set_server(sakura_char_t* dns_server, sakura_int32_t len);
sakura_int32_t sakura_dns_set_ip_by_host(const sakura_char_t* host, sakura_uint32_t ip);
sakura_uint32_t sakura_dns_get_ip_by_host(const sakura_char_t *host);
sakura_void_t sakura_dns_init(sakura_void_t);
sakura_void_t sakura_dns_cleanup(sakura_void_t);

#ifdef __cplusplus
}
#endif

#endif /* SAKURA_SDK_INTERNAL_H__ */
