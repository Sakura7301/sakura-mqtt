/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_dns.c
 * @brief dns implement according to DNS protocol
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */
#include <unistd.h>
#include "sakura_types.h"
#include "sakura_socket.h"
#include "sakura_log.h"
#include "sakura_mem.h"
#include "sakura_mutex.h"
#include "sakura_dns.h"
#include "sakura_list.h"
#include "sakura_utils.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#ifdef CONFIG_SYS_UNIX
#include <pthread.h>
#endif

#define LOG_TAG                     "DNS"
#define DNS_LOGD                    SAKURA_LOGD
#define DNS_LOGW                    SAKURA_LOGW
#define DNS_LOGI                    SAKURA_LOGI
#define DNS_LOGE                    SAKURA_LOGE
#define SAKURA_DNS_SERVER_NUM         4
#define SAKURA_DNS_PORT               53
#define SAKURA_DNS_NAME_SIZE          2
#define SAKURA_DNS_TYPE_SIZE          2
#define SAKURA_DNS_CLASS_SIZE         2
#define SAKURA_DNS_TTL_SIZE           4
#define SAKURA_DNS_DATALEN_SIZE       2
#define SAKURA_DNS_TYPE_ANSWER        0x01
#define SAKURA_DNS_TYPE_CNAME         0x05
#define DNS_TRANS_ID                0x4D51      /* 'MQ', just an id, no special meaning */

#define DNS_OK                      0
#define DNS_ERROR                   -1
#define DNS_DEFAULT_STR_LEN         0


struct dns_header {
    sakura_uint16_t trans_id;
    sakura_uint16_t flags;
    sakura_uint16_t question_count;
    sakura_uint16_t answer_count;
    sakura_uint16_t authority_count;
    sakura_uint16_t additional_count;
};

typedef struct {
    sakura_uint32_t left;
    sakura_uint32_t total;
}dns_info_length_t;

typedef struct {
    sakura_char_t* dot_str;
    sakura_int32_t dot_size;
}dot_data_t;

typedef struct {
    sakura_char_t* ques_name;
    const sakura_uint8_t* recv_buf;
    dot_data_t dot_data;
}dns_ans_str_t;

typedef struct {
    const sakura_char_t *decode_pos;
    sakura_uint8_t label_data_len;
    sakura_uint16_t encode_len;
    sakura_uint16_t plain_str_len;
    sakura_uint8_t is_first_jump_flag;
}storage_data_t;

typedef struct {
    sakura_uint16_t answer_type;
    sakura_uint16_t answer_data_len;
}dns_answer_dt_t;

typedef struct {
    struct list_head    list;
    sakura_char_t         host[DOT_NAME_LEN];
    sakura_uint32_t       ip;
} dns_handle_t;

static struct 
{
    struct list_head    list;
    sakura_int32_t        state;
    sakura_mutex_t        mutex_lock;
} glob_dns_context = {0};
#ifdef CONFIG_SYS_UNIX
static pthread_mutex_t glob_dns_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static sakura_char_t ac_dns_servers[4][16] = {SAKURA_DNS_SERVERS, {0}};

static dns_handle_t* dns_create();
static sakura_void_t dns_clear_list(sakura_void_t);
static sakura_int32_t sakura_dns_set_ip_by_host_internal(const sakura_char_t* host, sakura_uint32_t ip);


/**
 * @brief get server ip
 * @param[out] host host
 * @return success: ip   fail: 0
 */
sakura_uint32_t sakura_dns_get_ip_by_host(const sakura_char_t *host)
{
    sakura_uint32_t ret = DNS_OK;
    struct list_head *pos   = NULL;
    struct list_head *next  = NULL;
    dns_handle_t *node      = NULL;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_dns_mutex);
#endif
    do {
        /* check params */
        if (host == NULL){
            break;
        }

        /* query the corresponding ip */
        list_for_each_safe(pos, next, &glob_dns_context.list) {
            node = list_entry(pos, dns_handle_t, list);
            if(strcmp(node->host, host) == 0){
                ret = node->ip;
                break;
            }
        }

    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    pthread_mutex_unlock(&glob_dns_mutex);
#endif
    return ret;
}

/**
 * @brief set ip by host
 * @param[out] host host
 * @param[in] ip ip
 * @return success: server ip   fail: -1
 */
static sakura_int32_t sakura_dns_set_ip_by_host_internal(const sakura_char_t* host, sakura_uint32_t ip)
{
    sakura_int32_t ret = DNS_ERROR;
    struct list_head *pos   = NULL;
    struct list_head *next  = NULL;
    dns_handle_t *node      = NULL;
    dns_handle_t *handler   = NULL;
    sakura_uint32_t find_flag = 0;

    do {
        /* check params */
        if (ip == 0U || ip == INADDR_NONE || host == NULL) {
            ret = DNS_ERROR;
            break;
        }

        DNS_LOGD("sakura_dns_set_ip_by_host-->host [%s], ip [%u]\n", host, ip);

        /* find and set IP */
        list_for_each_safe(pos, next, &glob_dns_context.list) {
            node = list_entry(pos, dns_handle_t, list);
            /* host is set, set ip */
            if(strcmp(node->host, host) == 0){
                node->ip = ip;
                ret = DNS_OK;
                find_flag = 1;
                break;
            }
        }

        /* does not exist in the list */
        if(find_flag == 0){
            handler = dns_create();
            if(handler == NULL){
                DNS_LOGE("create dns handler failed!\n");
                ret = DNS_ERROR;
                break;
            }

            handler->ip = ip;
            (sakura_void_t)snprintf(handler->host, DOT_NAME_LEN, "%s", host);
            /* add to list */
            list_add_tail(&handler->list, &glob_dns_context.list);
        }
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief set ip by host
 * @param[out] host host
 * @param[in] ip ip
 * @return success: server ip   fail: -1
 */
sakura_int32_t sakura_dns_set_ip_by_host(const sakura_char_t* host, sakura_uint32_t ip)
{
    sakura_int32_t ret = DNS_ERROR;
    struct list_head *pos   = NULL;
    struct list_head *next  = NULL;
    dns_handle_t *node      = NULL;
    dns_handle_t *handler   = NULL;
    sakura_uint32_t find_flag = 0;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_dns_mutex);
#endif
    do {
        /* check params */
        if (ip == 0U || ip == INADDR_NONE || host == NULL) {
            ret = DNS_ERROR;
            break;
        }

        DNS_LOGD("sakura_dns_set_ip_by_host-->host [%s], ip [%u]\n", host, ip);

        /* find and set IP */
        list_for_each_safe(pos, next, &glob_dns_context.list) {
            node = list_entry(pos, dns_handle_t, list);
            /* host is set, set ip */
            if(strcmp(node->host, host) == 0){
                node->ip = ip;
                ret = DNS_OK;
                find_flag = 1;
                break;
            }
        }

        /* does not exist in the list */
        if(find_flag == 0){
            handler = dns_create();
            if(handler == NULL){
                DNS_LOGE("create dns handler failed!\n");
                ret = DNS_ERROR;
                break;
            }

            handler->ip = ip;
            (sakura_void_t)snprintf(handler->host, DOT_NAME_LEN, "%s", host);
            /* add to list */
            list_add_tail(&handler->list, &glob_dns_context.list);
        }
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    pthread_mutex_unlock(&glob_dns_mutex);
#endif
    return ret;
}

/*
 * convert "www.baidu.com" to "\x03www\x05baidu\x03com"
 * 0x0000 03 77 77 77 05 62 61 69 64 75 03 63 6f 6d 00 ff
 */
static sakura_int32_t encode_dot_str(const sakura_char_t* dot_str,
                                        sakura_char_t* encoded_str,
                                        sakura_uint16_t encoded_str_size);

/*
 * convert "\x03www\x05baidu\x03com\x00" to "www.baidu.com"
 * 0x0000 03 77 77 77 05 62 61 69 64 75 03 63 6f 6d 00 ff
 * convert "\x03www\x05baidu\xc0\x13" to "www.baidu.com"
 * 0x0000 03 77 77 77 05 62 61 69 64 75 c0 13 ff ff ff ff
 * 0x0010 ff ff ff 03 63 6f 6d 00 ff ff ff ff ff ff ff ff
 */

/**
 * @brief string encode
 * @param[out] dot_str dot string
 * @param[out] encoded_str encoded String
 * @param[in] encoded_str_size encode string size
 * @return success: 0   fail: -1
 */
static sakura_int32_t encode_dot_str(const sakura_char_t* dot_str,
                                        sakura_char_t* encoded_str,
                                        sakura_uint16_t encoded_str_size)
{
    /* init */
    sakura_int32_t ret = DNS_OK;
    sakura_uint16_t dot_str_len = 0;
    sakura_char_t* dot_str_copy = NULL;
    sakura_char_t* p_label       = NULL;
    sakura_uint16_t label_len   = 0;
    sakura_uint16_t encoded_str_len = 0;

    do {
        /* filter invalid data. */
        if (dot_str == NULL
        || encoded_str == NULL
        || encoded_str_size < dot_str_len + 2U) {
            ret = DNS_ERROR;
            break;
        }

        /* request a space to copy the source string */
        dot_str_len = strlen(dot_str);
        dot_str_copy = (sakura_char_t*)sakura_malloc(dot_str_len + 1U);
        if (dot_str_copy == NULL) {
            ret = DNS_ERROR;
            break;
        }
        /* Copy and split strings */
        strcpy(dot_str_copy, dot_str);
        p_label    = strtok(dot_str_copy, ".");
        while (p_label != NULL) {
            label_len = strlen(p_label);
            if (label_len != 0U) {
                /* append p_label */
                (sakura_void_t)sprintf(encoded_str + encoded_str_len, "%c%s", label_len, p_label);
                encoded_str_len += (label_len + 1U);
            }
            /* split buf */
            p_label = strtok(NULL, ".");
        }
    } while (SAKURA_FALSE);

    if (dot_str_copy != NULL) {
        sakura_free(dot_str_copy);
    }
    return ret;
}

/**
 * @brief covert reused domain to dot string
 * @param[out] str_data storage data structure
 * @param[in] packet_start_pos packet start position
 * @param[in] data_length data length
 * @return success: 0   fail: -1
 */
static sakura_int32_t encode_covert_reuse_domain(storage_data_t *str_data, const sakura_char_t* packet_start_pos, dns_info_length_t *data_length)
{
    /* init */
    sakura_uint16_t jump_pos = 0;
    sakura_uint16_t decoder_pos_wrapper = 0;
    sakura_int32_t ret = DNS_OK;
    
    do {
        /* filter invalid data */
        if (str_data == NULL || str_data->decode_pos == NULL || packet_start_pos == NULL || data_length == NULL) {
            ret = DNS_ERROR;
            break;
        }

        /*
        * The TriCore kernel cannot handle the non alignment of odd addresses
        * when the read operation is greater than or equal to 2 bytes,
        * and trap will occur directly. There is 'char->short/int' in our code, which will lead to trap.
        * Memory copy has been used to solve this problem.
        */
        memcpy(&decoder_pos_wrapper,str_data->decode_pos,sizeof(sakura_uint16_t));
        jump_pos = sakura_ntohs(decoder_pos_wrapper) & 0x3fffU;

        if (jump_pos >= data_length->total) {
            ret = DNS_ERROR;
            break;
        }
        data_length->left = data_length->total - jump_pos;
        /* update start location */
        str_data->decode_pos = packet_start_pos + jump_pos;
        if (str_data->is_first_jump_flag == 0U) {
            /* when entering for the first time, make flag equal to 1 and fill the length. */
            str_data->encode_len += SAKURA_DNS_NAME_SIZE;
            str_data->is_first_jump_flag = 1U;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief covert encoded string to dot string
 * @param[out] dot dot data
 * @param[out] str_data storage data structure
 * @param[in] data_length data length
 * @return success: 0   fail: -1
 */
static sakura_int32_t encode_covert_dot_str(dot_data_t* dot, storage_data_t *str_data, dns_info_length_t *data_length)
{
    /* init */
    const sakura_char_t *str_point = ".";
    sakura_int32_t ret = DNS_OK;
    
    do {
        /* filter invalid data */
        if (dot == NULL || dot->dot_str == NULL || str_data == NULL \
            || str_data->decode_pos == NULL || data_length == NULL) {
            ret = DNS_ERROR;
            break;
        }

        /*LabelDataLen + Label*/
        if (str_data->label_data_len >= data_length->left) {
            ret = DNS_ERROR;
            break;
        }
        if (((sakura_uint32_t)(str_data->plain_str_len) + (sakura_uint32_t)str_data->label_data_len + 1U) > (sakura_uint32_t)dot->dot_size) {
            ret = DNS_ERROR;
            break;
        }

        memcpy(dot->dot_str + str_data->plain_str_len, str_data->decode_pos + 1, str_data->label_data_len);
        memcpy(dot->dot_str + str_data->plain_str_len + str_data->label_data_len, str_point, 1);
        /* refresh length and start position */
        str_data->decode_pos += (str_data->label_data_len + 1U);
        str_data->plain_str_len += (str_data->label_data_len + 1U);
        data_length->left -= (str_data->label_data_len + 1U);
        if (str_data->is_first_jump_flag == 0U) {
            /* append the length of the encoded string on the first iteration */
            str_data->encode_len += (str_data->label_data_len + 1U);
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief check decode success or not
 * @param[out] dot_str dot string
 * @param[out] encode_len encode string length
 * @param[in] plain_str_len plain string length
 * @param[in] packet_start_pos packet start location
 * @param[in] jump_flag jump flag
 * @return success: 0   fail: -1
 */
static sakura_int32_t encode_check_fail(sakura_char_t* dot_str, sakura_uint16_t* encode_len, sakura_uint16_t plain_str_len, sakura_uint8_t jump_flag)
{
    /* init */
    sakura_int32_t ret = DNS_OK;

    /* filter invalid data */
    if (dot_str == NULL || encode_len == NULL || plain_str_len < 1U) {
        ret = DNS_ERROR;
    }else {
        dot_str[plain_str_len - 1U] = '\0';
        if (jump_flag == 0U) {
            /*add end '\0'*/
            *encode_len += 1U;
        }
    }
    return ret;
}

/**
 * @brief parse encoded string
 * @param[out] encoded_str encode string
 * @param[in] encode_len encode string length
 * @param[out] dot_data dot data
 * @param[out] packet_start_pos packet start location
 * @param[in] dot_str_size dot string size
 * @return success: 0   fail: -1
 */
static sakura_int32_t decode_dot_str(const sakura_char_t* encoded_str,
                                        sakura_uint16_t* encode_len,
                                        dot_data_t *dot_data,
                                        const sakura_char_t* packet_start_pos,
                                        dns_info_length_t data_length)
{
    /* init */
    storage_data_t str_msg = {0};
    sakura_int32_t ret = DNS_OK;

    /* filter invalid data */
    if (encoded_str == NULL || encode_len == NULL || dot_data == NULL \
        || dot_data->dot_str == NULL || data_length.total < data_length.left) {
        ret = DNS_ERROR;
    }else {
        /* init storage message */
        memset(&str_msg, 0, sizeof(storage_data_t));
        str_msg.decode_pos = encoded_str;

        /* Loop until '*decode_pos' equals 0 */
        while ((str_msg.label_data_len = *(str_msg.decode_pos)) != 0x00U && ret >= 0) {
            if ((str_msg.label_data_len & 0xc0U) == 0U) {
                /* covert encode string to dot string */
                ret = encode_covert_dot_str(dot_data, &str_msg, &data_length);
            } else {
                /* covert reused domain name to dot string */
                ret = encode_covert_reuse_domain(&str_msg, packet_start_pos, &data_length);
            }
        }
        *encode_len = str_msg.encode_len;
        if (ret >= 0) {
            /* decoding fails if the length or size is incorrect */
            ret = encode_check_fail(dot_data->dot_str, encode_len, str_msg.plain_str_len, str_msg.is_first_jump_flag);
        }
    }
    return ret;
}

/**
 * @brief request packet
 * @param[out] write_dns_packet packets to be sent
 * @param[out] dns_packet_size packet size
 * @param[out] encoded_domain_name encoded domain name
 * @param[in] type dns type
 * @param[in] dns_class dns_class
 * @return success: 0   fail: -1
 */
static sakura_void_t dns_send_req_packet(sakura_char_t* write_dns_packet, sakura_uint16_t *dns_packet_size,
    sakura_char_t* encoded_domain_name, sakura_uint16_t type, sakura_uint16_t dns_class)
{
    sakura_char_t *src_buf = NULL;
    sakura_uint16_t encoded_domain_name_len = 0;
    /* filter invalid data. */
    if (write_dns_packet != NULL && dns_packet_size != NULL && encoded_domain_name != NULL) {
        encoded_domain_name_len = strlen(encoded_domain_name) + 1U;

        /* domain name after appending code */
        write_dns_packet += sizeof(struct dns_header);
        memcpy(write_dns_packet, encoded_domain_name, encoded_domain_name_len);

        /* append dns_type */
        write_dns_packet += encoded_domain_name_len;
        src_buf = (sakura_char_t*)(&type);
        memcpy(write_dns_packet, src_buf, SAKURA_DNS_TYPE_SIZE);

        /* append dns_class */
        write_dns_packet += SAKURA_DNS_TYPE_SIZE;
        src_buf = (sakura_char_t*)(&dns_class);
        memcpy(write_dns_packet, src_buf, SAKURA_DNS_CLASS_SIZE);

        /* update total size */
        *dns_packet_size = sizeof(struct dns_header) + encoded_domain_name_len
            + (sakura_uint16_t)SAKURA_DNS_TYPE_SIZE + (sakura_uint16_t)SAKURA_DNS_CLASS_SIZE;
    }
}

/**
 * @brief send request
 * @param[in] sock use send request socket
 * @param[out] dns_server dns server
 * @param[out] domain_name domain
 * @return success: 0   fail: -1
 */
static sakura_int32_t dns_send_req(sakura_int32_t sock,
                                      const sakura_char_t* dns_server,
                                      const sakura_char_t* domain_name)
{
    /* init */
    (sakura_void_t)(dns_server);
    sakura_char_t buf[1024] = {0};
    sakura_char_t* write_dns_packet = NULL;
    sakura_char_t* encoded_domain_name = NULL;
    sakura_uint16_t dns_packet_size = 0;
    sakura_uint16_t type = 0;
    sakura_uint16_t dns_class = 0;
    sakura_uint16_t domain_name_en = 0;
    struct dns_header* dns_hd = NULL;
    sakura_int32_t ret = DNS_OK;

    do {
        /* filter invalid data */
        if (sock < 0 || dns_server == NULL || domain_name == NULL) {
            ret = DNS_ERROR;
            break;
        }
        /* set variable */
        type = sakura_htons(0x0001);
        dns_class = sakura_htons(0x0001);
        write_dns_packet = buf;
        domain_name_en = strlen(domain_name);

        /* init dns handle */
        dns_hd                   = (struct dns_header*)write_dns_packet;
        /* endianness conversion */
        dns_hd->trans_id         = sakura_htons((sakura_uint16_t)DNS_TRANS_ID);
        dns_hd->flags            = sakura_htons(0x0100);
        dns_hd->question_count   = sakura_htons(0x0001);
        dns_hd->answer_count     = 0x0000;
        dns_hd->authority_count  = 0x0000;
        dns_hd->additional_count = 0x0000;

        /* sakura_malloc */
        encoded_domain_name = (sakura_char_t*)sakura_malloc(domain_name_en + 2U);
        if (encoded_domain_name == NULL) {
            DNS_LOGE("try called sakura_malloc failed!\n");
            ret = DNS_ERROR;
            break;
        }
        memset(encoded_domain_name, 1, domain_name_en + 2U);
        /* string encoding */
        if (encode_dot_str(domain_name, encoded_domain_name, domain_name_en + 2U) == -1) {
            DNS_LOGE("string encode failed!\n");
            ret = DNS_ERROR;
            break;
        }

        /* the content of the packaging request */
        dns_send_req_packet(write_dns_packet, &dns_packet_size, encoded_domain_name, type, dns_class);

        /* send data */
        ret = sakura_sock_send(sock, (const sakura_uint8_t*)buf, dns_packet_size);
        if (ret < 0) {
            DNS_LOGE("sock send failed!\n");
            ret = DNS_ERROR;
            break;
        }
    } while (SAKURA_FALSE);

    /* free encoded_domain_name */
    if (encoded_domain_name != NULL) {
        sakura_free(encoded_domain_name);
    }
    return ret;
}

/**
 * @brief send a request
 * @param[in] sock use send request socket
 * @param[out] status dns status
 * @return success: 0   fail: -1
 */
sakura_void_t sakura_dns_send_request(sakura_int32_t sock, sakura_dns_status_t* status)
{
    /* init */
#ifdef CONFIG_LOG
    sakura_uint8_t index = 0;
    const sakura_char_t *dns_server = NULL;
#endif
    sakura_int32_t ret = DNS_OK;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_dns_mutex);
#endif
    do {
        /* confirm dns status */
        if (status == NULL){
            break;
        }
        if (status->times == 0U) {
            status->times++;
    #if (defined(__linux__) || defined(SAKURA_SDK_WINDOWS) || defined(__APPLE__)) || defined(__QNX__) || defined(__QNXNTO__)
            /* connect dns server */
            errno = 0;
            ret = connect_wrapper(sock, sakura_inet_addr(ac_dns_servers[status->index%((sakura_uint8_t)SAKURA_DNS_SERVER_NUM)]), sakura_htons(SAKURA_DNS_PORT), O_NONBLOCK);
            if(ret == -1
            && errno != EISCONN
            && errno != EINTR
            && errno != EINPROGRESS) {
                /* if the value of 'errno' is abnormal,
                 *it is considered that the connection to the DNS server fails.
                 */
                DNS_LOGW("dns Socket connect_wrapper fail, errno = %d, info = %s\n", errno, strerror(errno));
                break;;
            }
    #endif
#ifdef CONFIG_LOG
            index = status->index % ((sakura_uint8_t)SAKURA_DNS_SERVER_NUM);
            dns_server = (const sakura_char_t *)(ac_dns_servers[index]);
            DNS_LOGD("dns_send_req sock [%d], dns_server [%s], domain_name [%s]\n", sock, dns_server, status->server_host);
#endif
            /* send request */
            (sakura_void_t)dns_send_req(sock, ac_dns_servers[status->index%((sakura_uint8_t)SAKURA_DNS_SERVER_NUM)], status->server_host);
        }else if (status->times >= 100U) {
            /* refresh time */
            status->index++;
            status->times = 0;
        }else {
            status->times++;
        }
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    pthread_mutex_unlock(&glob_dns_mutex);
#endif
}

/**
 * @brief handle dns answer data by type
 * @param[in] dns_data point to the response dns data
 * @param[in] recv_buf recv data buffer
 * @param[in] ans_str  answer string structure
 * @param[in] answer_type answer type
 * @param[in] data_length dns data length
 * @return
 *          < 0: fail
 *            0: handle dns answer data success
 */
static sakura_int32_t dns_handle_answer_by_type(const sakura_char_t* dns_data, const sakura_uint8_t* recv_buf, dns_ans_str_t *ans_str, sakura_uint16_t answer_type, dns_info_length_t data_length)
{
    /* init */
    sakura_uint16_t en_len = 0;
    sakura_uint32_t ip = 0;
    sakura_int32_t ret = DNS_OK;
    sakura_uint32_t dns_data_wrapper_32 = 0;

    do {
        /* check parameter validity */
        if (dns_data == NULL || recv_buf == NULL || ans_str == NULL 
            || ans_str->ques_name == NULL || data_length.total < data_length.left) {
            ret = DNS_ERROR;
            break;
        }
        if (answer_type == SAKURA_DNS_TYPE_ANSWER) {
            /*
            * Memory copy is used to solve the trap problem of TriCore.
            */
            memcpy(&dns_data_wrapper_32, dns_data, sizeof(sakura_uint32_t));
            ip = dns_data_wrapper_32;
            if (ans_str->ques_name[0] == '\0') {
                /* There is no decoded string in queries_name. */
                (sakura_void_t)sakura_dns_set_ip_by_host_internal((const sakura_char_t *)ans_str->dot_data.dot_str, ip);
            } else {
                /* If you get the decoded data, set the server according to 'queries_name'. */
                (sakura_void_t)sakura_dns_set_ip_by_host_internal((const sakura_char_t *)ans_str->ques_name, ip);
            }
            ret = DNS_OK;
        } else if (answer_type == SAKURA_DNS_TYPE_CNAME) {
            /* decodes the source string  */
            ret = decode_dot_str(dns_data, &en_len, &(ans_str->dot_data), (const sakura_char_t*)recv_buf, data_length);
        } else {
            /* fall through */
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief decode answer information
 * @param[in] dns_data point to the response dns data
 * @param[in] recv_buf recv data buffer
 * @param[in] dot_msg  dot message
 * @param[in] answer_dt answer type and length information
 * @param[in] data_length dns data length
 * @return
 *          < 0: fail
 *          >=0: decode answer information position
 */
static sakura_int32_t dns_decode_answer_info(const sakura_char_t* dns_data, const sakura_uint8_t* recv_buf, dot_data_t* dot_msg, dns_answer_dt_t *answer_dt, dns_info_length_t data_length)
{
    /* init */
    sakura_uint16_t en_len = 0;
    sakura_int32_t ret = DNS_DEFAULT_STR_LEN;
    sakura_uint32_t offset = 0;
    sakura_uint16_t dns_data_wrapper = 0;

    do {
        /* check parameter validity */
        if (dns_data == NULL || dot_msg == NULL || answer_dt == NULL || data_length.total < data_length.left) {
            ret = DNS_ERROR;
            break;
        }
        /* decodes the source string  */
        ret = decode_dot_str(dns_data, &en_len, dot_msg, (const sakura_char_t*)recv_buf, data_length);
        if (ret < 0) {
            ret = DNS_ERROR;
            break;
        }
        if ((sakura_uint32_t)en_len > data_length.left) {
            /* data length exception */
            ret = DNS_ERROR;
            break;
        }

        dns_data += en_len;
        data_length.left -= ((sakura_uint32_t)en_len);
        ret += (sakura_int32_t)en_len;

        /* Memory copy is used to solve the trap problem of TriCore */
        offset = (sakura_uint32_t)SAKURA_DNS_TYPE_SIZE + (sakura_uint32_t)SAKURA_DNS_CLASS_SIZE + \
                (sakura_uint32_t)SAKURA_DNS_TTL_SIZE + (sakura_uint32_t)SAKURA_DNS_DATALEN_SIZE;
        if (offset > data_length.left) {
            /* data length exception */
            ret = DNS_ERROR;
            break;
        }
        /* gets the response type and message length. */
        memcpy(&dns_data_wrapper, dns_data, sizeof(sakura_uint16_t));
        answer_dt->answer_type  = sakura_ntohs(dns_data_wrapper);
        memcpy(&dns_data_wrapper, (dns_data + SAKURA_DNS_TYPE_SIZE + SAKURA_DNS_CLASS_SIZE + SAKURA_DNS_TTL_SIZE), sizeof(sakura_uint16_t));
        answer_dt->answer_data_len = sakura_ntohs(dns_data_wrapper);
        /* append dns_data */
        ret += (sakura_int32_t)offset;
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief parse dns answer
 * @param[in] dns_data point to the response dns data
 * @param[in] ans_str  answer string structure
 * @param[in] count answer count
 * @param[in] data_length dns data length
 * @return
 *          < 0: fail
 *            0: parse answer information success
 */
static sakura_int32_t dns_parse_answer(const sakura_char_t* dns_data, dns_ans_str_t *ans_str, sakura_uint16_t count, dns_info_length_t data_length)
{
    /* init */
    sakura_int32_t i = 0;
    sakura_int32_t ret = DNS_OK;
    dns_answer_dt_t answer_dt = {0};

    /* check parameter validity */
    if (dns_data == NULL || ans_str == NULL || count == 0u || data_length.total < data_length.left) {
        ret = DNS_ERROR;
    }else {
        /* parse dns answer */
        for (i = 0; i != (sakura_int32_t)count; ++i) {
            /* decode answer information */
            ret = dns_decode_answer_info(dns_data, ans_str->recv_buf, &(ans_str->dot_data), &answer_dt, data_length);
            if (ret < 0 || ret > (sakura_int32_t)data_length.left) {
                ret = DNS_ERROR;
                break;
            }
            /* append dns_data */
            dns_data += ret;
            data_length.left -= (sakura_uint32_t)ret;

            /* handle answer information by type */
            ret = dns_handle_answer_by_type(dns_data, ans_str->recv_buf, ans_str, answer_dt.answer_type, data_length);

            if (ret < 0 || (sakura_uint32_t)(answer_dt.answer_data_len) > data_length.left) {
                break;
            }
            /* data length exception */
            dns_data += (sakura_int32_t)(answer_dt.answer_data_len);
            data_length.left -= (sakura_uint32_t)(answer_dt.answer_data_len);
        }
    }
    return ret;

}

/**
 * @brief parse dns question
 * @param[in] dns_data point to the response dns data
 * @param[in] dot_msg dot message
 * @param[in] count question count
 * @param[in] data_length dns data length
 * @return
 *          < 0: fail
 *          >=0: handled data's postion
 */
static sakura_int32_t dns_parse_question(const sakura_char_t* dns_data, dot_data_t* dot_msg, sakura_uint16_t count, dns_info_length_t data_length)
{
    /* init */
    sakura_int32_t i = 0;
    sakura_int32_t ret = DNS_DEFAULT_STR_LEN;
    sakura_int32_t offset = 0;
    sakura_uint16_t encoded_name_len = 0;

    /* check parameter validity */
    if (dns_data == NULL || dot_msg == NULL || count == 0u || data_length.total < data_length.left) {
        ret = DNS_ERROR;
    }else {
        /* parse dns question */
        for (i = 0; i != (sakura_int32_t)count; ++i) {
            /* decodes the source string  */
            ret = decode_dot_str(dns_data, &encoded_name_len, dot_msg, NULL, data_length);
            if (ret < 0) {
                ret = DNS_ERROR;
                break;
            }
            offset = (sakura_int32_t)encoded_name_len + SAKURA_DNS_TYPE_SIZE + SAKURA_DNS_CLASS_SIZE;
            if ((sakura_uint32_t)offset > data_length.left) {
                /* data length exception */
                ret = DNS_ERROR;
                break;
            }
            /* append dns_data */
            dns_data += offset;
            data_length.left -= (sakura_uint32_t)offset;
            ret += offset;
        }
    }
    return ret;

}

/**
 * @brief dns receive phase response
 * @param[in] sock use recv socket
 * @param[in] len recv data_len
 * @param[out] recv_buf use recv buffer
 * @return success: 0   fail: -1
 */
sakura_int32_t sakura_dns_recv_response(sakura_int32_t sock,
                                   const sakura_uint8_t* recv_buf,
                                   sakura_uint32_t len)
{
    /* init */
    const sakura_char_t* dns_data   = NULL;
    sakura_char_t dot_name[DOT_NAME_LEN] = {0};
    sakura_char_t queries_name[DOT_NAME_LEN] = {0};
    const struct dns_header* dns_hd = NULL;
    sakura_uint16_t question_count = 0;
    sakura_uint16_t answer_count = 0;
    dns_info_length_t data_length = {0};
    dot_data_t qus_msg = {0};
    dns_ans_str_t ans_str = {0};
    sakura_int32_t ret = DNS_ERROR;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_dns_mutex);
#endif
    do {
        /* no data returned directly from buffer */
        if (sock < 0 || recv_buf == NULL || len <= sizeof(struct dns_header)) {
            ret = DNS_ERROR;
            break;
        }
        dns_hd = (const struct dns_header*)recv_buf;

        /* Endianness conversion */
        question_count = sakura_ntohs(dns_hd->question_count);
        answer_count   = sakura_ntohs(dns_hd->answer_count);
        /* set dns data length */
        data_length.total = len;
        /* filter DNS_TRANS_ID */
        if (dns_hd->trans_id == sakura_ntohs((sakura_uint16_t)DNS_TRANS_ID)
            && answer_count > 0U) {
            /* get data after dns_header */
            data_length.left = len - (sakura_uint32_t)(sizeof(struct dns_header));
            dns_data = (const sakura_char_t*)recv_buf + sizeof(struct dns_header);
            qus_msg.dot_str = queries_name;
            qus_msg.dot_size = sizeof(queries_name);

            /* parse dns question */
            ret = dns_parse_question(dns_data, &qus_msg, question_count, data_length);
            if (ret >= 0 && ret <= (sakura_int32_t)data_length.left) {
                /* append dns_data */
                dns_data += ret;
                data_length.left -= (sakura_uint32_t)ret;
            }else {
                break;
            }

            /* init ans_str structure */
            ans_str.dot_data.dot_str = dot_name;
            ans_str.dot_data.dot_size = sizeof(dot_name);
            ans_str.ques_name = queries_name;
            ans_str.recv_buf = recv_buf;
            /* parse answer message */
            ret = dns_parse_answer(dns_data, &ans_str, answer_count, data_length);
        }
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    pthread_mutex_unlock(&glob_dns_mutex);
#endif
    return ret;
}

/**
 * @brief set dns server
 * @param[out] dns_server address of the dns server
 * @param[in] len server len
 * @return success: 0   fail: -1
 */
sakura_int32_t sakura_dns_set_server(sakura_char_t* dns_server, sakura_int32_t len)
{
    /* init */
    sakura_int32_t ret = DNS_OK;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_dns_mutex);
#endif
    do {
        /* filter invalid data */
        if (dns_server == NULL
            || len > (sakura_int32_t)sizeof(ac_dns_servers[3])) {
            ret = DNS_ERROR;
            break;
        }
        /* set dns server */
        memset(ac_dns_servers[3], '\0', sizeof(ac_dns_servers[3]));
        memcpy(ac_dns_servers[3], dns_server, len);
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    pthread_mutex_unlock(&glob_dns_mutex);
#endif
    return ret;
}

/**
 * @brief init mutex and clear host ip.
 *
 * @return void
 */
sakura_void_t sakura_dns_init(sakura_void_t)
{
#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_dns_mutex);
#endif
    do {
        /* check dns state */
        if (glob_dns_context.state != 0) {
            DNS_LOGE("already init, denied\n");
            break;
        }

        /* init list */
        INIT_LIST_HEAD(&glob_dns_context.list);

        /* init dns server */

        DNS_LOGD("DNS init\n");
        glob_dns_context.state = 1;
        
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    pthread_mutex_unlock(&glob_dns_mutex);
#endif
}

/**
 * @brief destroy ac_dns_servers and mutex.
 *
 * @return void
 */
sakura_void_t sakura_dns_cleanup(sakura_void_t)
{
#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_dns_mutex);
#endif
    do
    {
        /* check state */
        if(glob_dns_context.state == 0){
            DNS_LOGE("not init, denied\n");
            break;
        }

        /* clear ac_dns_servers */
        memset(ac_dns_servers[3], '\0', sizeof(ac_dns_servers[3]));

        /* clear handler list */
        dns_clear_list();

        /* reset dns state */
        glob_dns_context.state = 0;
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    pthread_mutex_unlock(&glob_dns_mutex);
#endif
}

static dns_handle_t* dns_create()
{
    dns_handle_t *handler = NULL;
    handler = (dns_handle_t*)sakura_malloc(sizeof(dns_handle_t));
    if(handler != NULL){
        memset(handler->host, 0, DOT_NAME_LEN);
        handler->ip = 0;
        INIT_LIST_HEAD(&handler->list);
    } else {
        DNS_LOGE("call malloc failed\n");
    }

    return handler;
}

static sakura_void_t dns_clear_list(sakura_void_t)
{
    struct list_head *pos   = NULL;
    struct list_head *next  = NULL;
    dns_handle_t *node      = NULL;
    list_for_each_safe(pos, next, &glob_dns_context.list) {
        node = list_entry(pos, dns_handle_t, list);
        if(node != NULL){
            sakura_free(node);
            node = NULL;
        }
    }
}