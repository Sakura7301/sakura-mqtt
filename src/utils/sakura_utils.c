#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "sakura_utils.h"
#include "sakura_mem.h"


/**
 * @brief delete queue node
 * 
 * @param queue_node queue node
 * @return sakura_void_t 
 */
sakura_void_t delete_queue_node(queue_node_t* queue_node)
{
    if(queue_node != NULL){
        if(queue_node->data.data != NULL){
            sakura_free(queue_node->data.data);
            queue_node->data.data = NULL;
        }
        sakura_free(queue_node);
        queue_node = NULL;
    }
}


/**
 * @brief Create a queue node object
 * 
 * @param data string data
 * @param len length
 * @return queue_node_t* 
 */
queue_node_t* create_queue_node(const sakura_uint8_t* data, sakura_uint32_t len)
{
    queue_node_t* node = NULL;
    sakura_uint32_t out_of_mem = 0;

    do
    {
        /* check params */
        if(data == NULL || len == 0){
            break;
        }
        node = (queue_node_t*)sakura_malloc(sizeof(queue_node_t));
        if(node == NULL){
            out_of_mem = 1;
            break;
        }

        node->data.data = (sakura_uint8_t*)sakura_malloc(len + 1);
        if(node == NULL){
            out_of_mem = 1;
            break;
        }

        /* set value */
        memset(node->data.data, 0, len + 1);
        node->data.len = len;
        memcpy(node->data.data, data, len);
        INIT_LIST_HEAD(&node->list);
    } while (SAKURA_FALSE);

    /* check out of memory */
    if(out_of_mem == 1){
        delete_queue_node(node);
        node = NULL;
    }
    return node;
}

/**
 * @brief check if the input string is an IPv4 formatted address
 * 
 * @param str string
 * @return bool
 */
sakura_bool_t sakura_is_ipv4_address(const sakura_char_t *str)
{
    sakura_int32_t num = 0;
    sakura_int32_t dots = 0;
    sakura_bool_t ret = SAKURA_FALSE;

    do
    {
        /* check params */
        if(str == NULL){
            break;
        }

        while (*str) {
            if (*str == '.') {
                dots++;
                /* check dot number */
                if (dots > 3){
                    break;
                }
                /* check number wether in range 0-255 */
                if (num >= 256){
                    break;
                }
                num = 0;
            } else if (isdigit((unsigned char)*str)) {
                /* accumulated value */
                num = num * 10 + (*str - '0');
            } else {
                break;
            }
            str++;
        }

        /* check result */
        if (dots != 3 || num >= 256){
            break;
        }

        ret = SAKURA_TRUE;
    } while (SAKURA_FALSE);
    
    return ret;
}

/**
 * @brief host to net (short number)
 * 
 * @param value input value
 * @return sakura_uint16_t 
 */
sakura_uint16_t sakura_htons(sakura_uint16_t value)
{
    sakura_uint16_t number = 0x1234;
    sakura_uint8_t* ptr = (sakura_uint8_t*)&number;
    sakura_uint16_t ret = 0;
    if(*ptr == (sakura_uint8_t)0x12){
        /* big endian */
        ret = value;
    } else {
        /* little endian */
        ret = BigLittleSwap16(value);
    }

    return ret;
}

/**
 * @brief net to host (short number)
 * 
 * @param value input number
 * @return sakura_uint16_t 
 */
sakura_uint16_t sakura_ntohs(sakura_uint16_t value)
{
    sakura_uint16_t number = 0x1234;
    sakura_uint8_t* ptr = (sakura_uint8_t*)&number;
    sakura_uint16_t ret = 0;
    if(*ptr == (sakura_uint8_t)0x12){
        /* big endian */
        ret = value;
    } else {
        /* little endian */
        ret = BigLittleSwap16(value);
    }

    return ret;
}

/**
 * @brief host to net (long number)
 * 
 * @param value input value
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_htonl(sakura_uint32_t value)
{
    sakura_uint32_t number = 0x12345678;
    sakura_uint8_t* ptr = (sakura_uint8_t*)&number;
    sakura_uint32_t ret = 0;
    if(*ptr == (sakura_uint8_t)0x12){
        /* big endian */
        ret = value;
    } else {
        /* little endian */
        ret = BigLittleSwap32(value);
    }

    return ret;
}

/**
 * @brief net to host (long number)
 * 
 * @param value input value
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_ntohl(sakura_uint32_t value)
{
    sakura_uint32_t number = 0x12345678;
    sakura_uint8_t* ptr = (sakura_uint8_t*)&number;
    sakura_uint32_t ret = 0;
    if(*ptr == (sakura_uint8_t)0x12){
        /* big endian */
        ret = value;
    } else {
        /* little endian */
        ret = BigLittleSwap32(value);
    }

    return ret;
}


/**
 * @brief inet addr
 * 
 * @param ip ip addr string
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_inet_addr(const sakura_char_t *ip)
{
    /* init */
    sakura_uint32_t ip_uint = 0;
    sakura_char_t* p_uint = (sakura_char_t*)&ip_uint;
    const sakura_char_t* p = ip;
    
    while(p){
        *p_uint++ = atoi(p);
        p = strchr(p, '.');
        if(p){
            p++;
        }
    }

    return ip_uint;
}

/**
 * @brief converts ip to a Dot-decimal notation string.
 * 
 * @param ip ip_uint
 * @return sakura_char_t* 
 */
sakura_char_t* sakura_inet_ntoa(sakura_uint32_t ip)
{
    /* static str */
	static sakura_char_t ip_str[MAX_IP_ADDR_LEN] = {0};
    memset(ip_str, 0, sizeof(ip_str));
    /* formatting */
	(void)snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u", 
					(ip >> 24) & 0xFFU,
					(ip >> 16) & 0xFFU,
					(ip >>  8) & 0xFFU,
					 ip & 0xFFU);
	return ip_str;
}