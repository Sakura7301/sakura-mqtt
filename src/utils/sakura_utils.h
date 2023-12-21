/*
 * Copyright (C) 2021-2023 by SAKURA, All rights reserved.
 */
#ifndef __SAKURA_MQTT_H__
#define __SAKURA_MQTT_H__

#include "sakura_list.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_IP_ADDR_LEN         16U

#define BigLittleSwap16(A)      ((((sakura_uint16_t)(A) & 0xff00U) >> 8) | \
                                (((sakura_uint16_t)(A) & 0x00ffU) << 8))

#define BigLittleSwap32(A)      ((((sakura_uint32_t)(A) & 0xff000000U) >> 24) | \
                                (((sakura_uint32_t)(A) & 0x00ff0000U) >> 8) | \
                                (((sakura_uint32_t)(A) & 0x0000ff00U) << 8) | \
                                (((sakura_uint32_t)(A) & 0x000000ffU) << 24))

/**
 * @brief delete queue node
 * 
 * @param queue_node queue node
 * @return sakura_void_t 
 */
sakura_void_t delete_queue_node(queue_node_t* queue_node);

/**
 * @brief Create a queue node object
 * 
 * @param data string data
 * @param len length
 * @return queue_node_t* 
 */
queue_node_t* create_queue_node(const sakura_uint8_t* data, sakura_uint32_t len);

/**
 * @brief check if the input string is an IPv4 formatted address
 * 
 * @param str string
 * @return bool
 */
sakura_bool_t sakura_is_ipv4_address(const sakura_char_t *str);

/**
 * @brief host to net (short number)
 * 
 * @param value input value
 * @return sakura_uint16_t 
 */
sakura_uint16_t sakura_htons(sakura_uint16_t value);

/**
 * @brief net to host (short number)
 * 
 * @param value input number
 * @return sakura_uint16_t 
 */
sakura_uint16_t sakura_ntohs(sakura_uint16_t value);

/**
 * @brief host to net (long number)
 * 
 * @param value input value
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_htonl(sakura_uint32_t value);
/**
 * @brief net to host (long number)
 * 
 * @param value input value
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_ntohl(sakura_uint32_t value);

/**
 * @brief inet addr
 * 
 * @param ip ip addr string
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_inet_addr(const sakura_char_t *ip);

/**
 * @brief converts ip to a Dot-decimal notation string.
 * 
 * @param ip ip_uint
 * @return sakura_char_t* 
 */
sakura_char_t* sakura_inet_ntoa(sakura_uint32_t ip);

#ifdef __cplusplus
}
#endif

#endif