/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_socket.c
 * @brief socket implement in unix platform
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

/*
 *
 * This file gives the implementation of asynchronous socket API in Linux platform.
 *
 * It is better to use hash or map to quickly find the socket handler from the socket
 * identifier, however, usually there are only several sockets, the sequencial search
 * is more simple to implement.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>

#include "sakura_list.h"
#include "sakura_socket.h"
#include "sakura_log.h"
#include "sakura_mem.h"
#include "sakura_mutex.h"
#include "sakura_dns.h"
#include "sakura_utils.h"

#define SOCK_INACTIVE           0   /* inactive */
#define SOCK_CONNECTED          1   /* connection to peer is established */
#define SOCK_CONNECTING         2   /* connection in progress */
#define SSL_CONNECTING          3   /* connection in progress */
#define SOCK_CONNECT_OK         4   /* just connected to the peer */
#define SOCK_CONNECT_ERROR      5   /* connect failed */
#define SOCK_SSL_INIT           6   /* ssl context init */
#define SOCK_RECV_LOG_STEP      100 /* recv_log printing frequency */
#define SERVER_IP_LEN           24  /* ip addr, max length is 24 bytes */
#define SOCK_OK                 0
#define SOCK_ERROR              -1

/* should use the log API defined in the main repo */
#define LOG_TAG                 "SOCK"
#define SOCK_LOGE               SAKURA_LOGE
#define SOCK_LOGI               SAKURA_LOGI
#define SOCK_LOGD               SAKURA_LOGD
#define SOCK_LOGW               SAKURA_LOGW
#ifndef READ_BUFFER_LENGTH
#define READ_BUFFER_LENGTH      CONFIG_READ_BUFFER_LENGTH
#endif


/* send req state */
typedef enum
{
    SEND_REQ_FREE = 0x00,               /* free */
    SEND_REQ_SENDING                    /* sending */
}SEND_REQ_STATE;

/* send req struct */
typedef struct {
        struct list_head list;
        SEND_REQ_STATE state;
        const sakura_uint8_t *data;
        sakura_uint32_t len;
} send_req_t;

typedef struct sock_handle_str{
    sakura_int32_t state;
    struct list_head list;
    pthread_t conn_thread_pid;

    sakura_int32_t sock;
    sakura_sock_callbacks_t cbs;

    struct queue_t send_queue;
    sakura_uint8_t protocol;
    sakura_uint16_t server_port;
    sakura_uint8_t server_ip[SERVER_IP_LEN];
    sakura_dns_status_t status;
    struct sock_handle_str *udp_handle; /*udp handle for DNS domain name resolution*/
} sock_handle_t;

typedef struct {
    sakura_int32_t sock;
    sakura_uint16_t port;
    char *host;
} connect_req_t;

typedef struct {
    sakura_int32_t state;
    sakura_mutex_t sock_pool_lock;
    /* a list of sockets */
    struct list_head sock_list;
} sock_pool_t;

/* init sock pool */
static sock_pool_t glob_sock_pool = {0};
static pthread_mutex_t glob_socket_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef CONFIG_LOG
/* socket error code */
static sakura_char_t *sock_err_code_array[] =
{
    "SAKURA_SOCK_ERR_DISCONNECTED",
    "SAKURA_SOCK_ERR_CONNECT",
    "SAKURA_SOCK_ERR_SEND",
    "SAKURA_SOCK_ERR_RECV",
    "SAKURA_SOCK_ERR_UNKNOWN",
};
#define GET_SOCK_ERR_CODE(type)                  sock_err_code_array[(abs(type + 1) & 0x0fU)]
#endif

static sakura_int32_t data_ready_to_send(sock_handle_t *handle, const sakura_uint8_t *data, sakura_uint32_t len);
static sakura_void_t send_queue_clear(queue_t *queue);

/*
 * @brief find socket handler by sock id
 *
 * @param[in]  sock:       sock id
 * @return:
 *          success: socket handler
 *           fail: NULL
 */
static sock_handle_t *find_handle_by_sock(sakura_int32_t sock) {
    sock_handle_t *ret = NULL;
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;
    /* loop find socket handler by sock id */
    list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
        node = list_entry(pos, sock_handle_t, list);
        /* match sock id */
        if (node->sock == sock) {
            SOCK_LOGD("find a handler by sock %d\n", sock);
            ret = node;
            break;
        }
    }
    return ret;
}

/*
 * @brief close socket by sock handle
 *
 * @param[in]  handle:       socket handler
 * @return:
 *          0: ok
 *         -1: fail
 */
static sakura_int32_t sock_close_by_handle(sock_handle_t *handle)
{
    sakura_int32_t ret = SOCK_OK;
    /* check parameter handle */
    if (handle != NULL && handle->sock >= 0) {
        /* close sock */
        SOCK_LOGD("del a sock [%d]\n", handle->sock);
        (sakura_void_t)close(handle->sock);
        handle->sock = -1;
    }else {
        ret = SOCK_ERROR;
        SOCK_LOGE("invalid parameter in close sock\n");
    }
    return ret;
}

/*
 * @brief destroy sock handle
 *
 * @param[in]  handle:       socket handler
 * @return:
 *          0: ok
 *         -1: fail
 */
static sakura_void_t sock_destroy_handler(sock_handle_t* handle)
{
    /* check parameter handle */
    if (handle != NULL) {
        /* destroy socket handle */
        list_del(&handle->list);
        sakura_free(handle);
    }else {
        SOCK_LOGE("invalid parameter in destroy handler\n");
    }
}

/*
 * @brief delete socket handler by sock id
 *
 * @param[in]  sock:       sock id
 * @return:
 *          0: ok
 *         -1: fail
 */
static sakura_void_t sock_pool_del(sakura_int32_t sock)
{
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;

    do
    {
        /* loop find socket handler from socket pool */
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            /* match sock id */
            if (node->sock == sock) {
                if (node->protocol == (sakura_uint8_t)SAKURA_TCP) {
                    if (node->udp_handle != NULL) {
                        /* release udp handle */
                        (sakura_void_t)sock_close_by_handle(node->udp_handle);
                        sock_destroy_handler(node->udp_handle);
                        node->udp_handle = NULL;
                    }
                }
                /* destroy socket handler */
                (sakura_void_t)sock_close_by_handle(node);
                sock_destroy_handler(node);
            }
        }       
    } while (SAKURA_FALSE);
}

/*
 * @brief add socket handler node to sock pool
 *
 * @param[in]  item:     socket handler node
 * @return: void
 */
static sakura_void_t sock_pool_add(struct list_head *item)
{
    do
    {
        /* check parameter */
        if(item == NULL){
            break;
        }
        list_add_tail(item, &glob_sock_pool.sock_list);      
    } while (SAKURA_FALSE);
}

/*
 * @brief cleanup all socket handler from sock pool
 *
 * @param[in]  void
 * @return:
 *          0:ok
 *         -1:fail
 */
static sakura_int32_t sock_pool_cleanup(sakura_void_t) {
    /* init */
    sakura_int32_t ret = SOCK_OK;
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;

    do
    {
        /* loop find socket handler from socket pool */
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            /* send_req_list cleanup */
            send_queue_clear(&node->send_queue);
            /* handle TCP socket node */
            if (node->protocol == (sakura_uint8_t)SAKURA_TCP) {
                /* release UDP socket handler */
                if (node->udp_handle != NULL) {
                    /* close udp sock */
                    (sakura_void_t)sock_close_by_handle(node->udp_handle);
                    /* release node */
                    sock_destroy_handler(node->udp_handle);
                    node->udp_handle = NULL;
                }
                /* close tcp sock */
                (sakura_void_t)sock_close_by_handle(node);
                /* release node */
                sock_destroy_handler(node);
            }
        }     
    } while (SAKURA_FALSE);
    
    return ret;
}

/*
 * @brief socket loop send data
 *
 * @param[in]  sock:     sock id
 * @param[in]  data:     data to send
 * @param[in]  len:      data length
 * @return:
 *         >0:sent data length
 *         -1:sent fail
 */
static sakura_int32_t sock_send_bytes(sakura_int32_t sock, const sakura_uint8_t *data, sakura_uint32_t len)
{
    sakura_int32_t ret = SOCK_OK;
    const sakura_uint8_t *ptr = data;
    sakura_int32_t rest = len;
    /* check parameter */
    if (NULL != data && len > 0U) {
        /* loop until data transmission is complete */
        do {
            errno = 0;
            /* send data to server */
            ret = sakura_sock_send(sock, ptr, rest);
            if (ret < 0) {
                /* try again */
                if (errno == EAGAIN) {
                    /* socket busy */
                    ret = 0;
                }else {
                    SOCK_LOGE("send data fail, ret:%d, errno:%d, info = %s\n", ret, errno, strerror(errno));
                    break;
                }
            } else {
                /* update data pointer and rest length */
                ptr  += ret;
                rest -= ret;
            }
            usleep(4 * 1000);
        } while (rest > 0);

        /* send result */
        if (ret < 0) {
            ret = SOCK_ERROR;
        } else {
            ret = SOCK_OK;
        }
    }else {
        /* invalid parameter */
        ret = SOCK_ERROR;
        SOCK_LOGE("send data fail[sock:%d]\n", sock);
    }
    return ret;
}

/*
 * @brief send data in socket handler
 *
 * @param[in]  handle:   socket handler
 * @return:
 *         0:ok
 *        -1:sent fail
 */
static sakura_int32_t sock_send(sock_handle_t *handler)
{
    sakura_int32_t ret = SOCK_OK;
    queue_node_t* queue_node = NULL;
    
    do
    {
        /* check params */
        if(handler == NULL){
            ret = SOCK_ERROR;
            SOCK_LOGE("invalid params!\n");
            break;
        }

        do
        {
            queue_node = dequeue(&handler->send_queue);
            if(queue_node == NULL){
                break;
            }

            ret = sock_send_bytes(handler->sock, queue_node->data.data, queue_node->data.len);
            if(ret == 0){
                /* send success */
                if(handler->cbs.cb_send != NULL) {
                    /* call send callback */
                    (sakura_void_t)handler->cbs.cb_send(handler->sock);
                }
            }

            delete_queue_node(queue_node);
        } while (queue_node != NULL || ret == 0);   

    } while (SAKURA_FALSE);

    return ret;
}

/*
 * @brief send dns request
 *
 * @param[in]  handle:   socket handler
 * @return:
 *         0:ok
 *        -1:sent fail
 */
static sakura_int32_t sock_send_dns_request(sock_handle_t *handle)
{
    sakura_int32_t ret = SOCK_OK;
    /* check handle */
    if (handle != NULL && handle->sock != -1) {
        sakura_dns_send_request(handle->sock, &handle->status);
    }else {
        ret = SOCK_ERROR;
        SOCK_LOGE("udp handle[%p] is invalid\n", handle);
    }
    return ret;
}

/*
 * @brief socket is on error
 *
 * @param[in]  handle:   socket handler
 * @param[in]  code:     status code
 * @return:void
 */
static sakura_void_t sock_on_error(sock_handle_t *handler, sakura_int32_t code)
{
    /* check parameter */
    if (NULL != handler) {
        SOCK_LOGE("sock %d has %s failure!\n", handler->sock, GET_SOCK_ERR_CODE(code));
        if (code == SAKURA_SOCK_ERR_DISCONNECTED) {
            /* when recv disconnect */
            if (handler->cbs.cb_recv != NULL) {
                (sakura_void_t)handler->cbs.cb_recv(handler->sock, NULL, 0);
            }
        }else {
            if (handler->cbs.cb_status != NULL) {
                (sakura_void_t)handler->cbs.cb_status(handler->sock, code);
            }
        }
        handler->state = SOCK_INACTIVE;
    }
}

/*
 * @brief initialize the socket management
 *
 * @param[in]  void
 * @return:
 *          0:ok
 *         -1:fail
 */
sakura_int32_t sakura_socket_init(sakura_void_t)
{
    sakura_int32_t ret = SOCK_OK;

    (sakura_void_t)pthread_mutex_lock(&glob_socket_mutex);
    do {
        /* check socket pool state */
        if (glob_sock_pool.state > 0) {
            SOCK_LOGD("socket is already initialized\n");
            ret = SOCK_ERROR;
            break;
        }

        /* DO IT FIRST */
        INIT_LIST_HEAD(&glob_sock_pool.sock_list);

        /* update state */
        glob_sock_pool.state = 1;
    } while (SAKURA_FALSE);

    pthread_mutex_unlock(&glob_socket_mutex);
    return ret;
}

/*
 * @brief connect to server
 *
 * @param[in]  sock:        sock id
 * @param[in]  ip:          server ip
 * @param[in]  port:        server port
 * @param[in]  flag:        connect flag
 * @return:
 *          0:ok
 *        < 0:fail
 */
sakura_int32_t connect_wrapper(sakura_int32_t sock,
                             sakura_uint32_t ip,
                             sakura_uint16_t port,
                             sakura_uint32_t flag)
{
    sakura_int32_t ret = SOCK_OK;
    sakura_int32_t flags = 0;
    struct sockaddr_in dest;
    sakura_int32_t optval = 1;

    do {
        /* check ip */
        if (ip == 0U) {
            ret = SOCK_ERROR;
            break;
        }
        /* set socket addr */
        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_port = port;
        dest.sin_addr.s_addr = ip;
        /* config socket */
        flags = fcntl(sock, F_GETFL, 0);
        (sakura_void_t)fcntl(sock, F_SETFL, (sakura_uint32_t)flags | flag);
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        ret = connect(sock, (struct sockaddr*)&dest, sizeof(dest));
    } while (SAKURA_FALSE);

    return ret;
}

/*
 * @brief connect to server by server ip
 *
 * @param[in]  node:        socket handler
 * @param[in]  server_ip:   server ip
 * @return:    void
 */
static sakura_void_t socket_connect_by_server_ip(sock_handle_t *node, sakura_uint32_t server_ip)
{
    sakura_int32_t ret = SOCK_OK;
    /* check node is valid */
    if (node != NULL) {
        //delete udp handle
        if (node->udp_handle != NULL) {
            /* release udp handler */
            (sakura_void_t)sock_close_by_handle(node->udp_handle);
            sock_destroy_handler(node->udp_handle);
            node->udp_handle = NULL;
        }
        errno = 0;
        node->state = SOCK_CONNECTING;
        /* connect to server */
        ret = connect_wrapper(node->sock, server_ip, node->server_port, O_NONBLOCK);
        if(ret == -1
            && errno != EISCONN
            && errno != EINTR
            && errno != EINPROGRESS) {
                node->state = SOCK_CONNECT_ERROR;
                SOCK_LOGD("socket[%d] connect_wrapper to [%s:%u] fail\n", node->sock,
                                node->status.server_host, sakura_ntohs(node->server_port));
        } else {
            /* connecting */
            SOCK_LOGD("socket[%d] connect async return, errno:%d, info = %s\n", node->sock, errno, strerror(errno));
        }
    }
}

/*
 * @brief loop find sock to get max socket id
 *
 * @param[in]  read_fds:        read flag
 * @param[in]  write_fds:       write flag
 * @param[in]  error_fds:       error flag
 * @return:     >= 0:max socket id
 *               < 0:get fail
 */
static sakura_int32_t socket_get_max_socket(fd_set *read_fds, fd_set *write_fds, fd_set *error_fds)
{
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;
    sakura_int32_t max_sock = -1;
    do {
        /* check params validity */
        if (read_fds == NULL || write_fds == NULL || error_fds == NULL) {
            break;
        }

        /* loop search socket list */
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            if ((node->sock >= 0)
                && (node->state == SOCK_CONNECTING
                || node->state == SOCK_CONNECTED
                || node->state == SOCK_CONNECT_OK)) {
                /* set write,read,error flag */
                FD_SET(node->sock, write_fds);
                FD_SET(node->sock, read_fds);
                FD_SET(node->sock, error_fds);
                if (node->sock > max_sock) {
                    max_sock = node->sock;
                }
            }
        }
    } while (SAKURA_FALSE);
    return max_sock;
}

/*
 * @brief loop find sock to connect to server
 *
 * @return:    void
 */
static sakura_void_t socket_establish_connect(sakura_void_t)
{
    /* init */
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;
    sakura_int32_t ret = SOCK_OK;
    sakura_uint32_t server_ip = 0;

    do
    {
        /* loop search socket list */
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            /* UDP,continue */
            if (node->protocol == (sakura_uint8_t)SAKURA_UDP) {
                continue;
            }
            if(node->state == SOCK_INACTIVE) {
                /* get ip by host */
                server_ip = sakura_dns_get_ip_by_host((const sakura_char_t *)node->status.server_host);
                if(server_ip > 0u) {
                    socket_connect_by_server_ip(node, server_ip);
                }else {
                    if(node->udp_handle != NULL){
                        ret = sock_send_dns_request(node->udp_handle);
                        if (ret == 0 && node->udp_handle != NULL) {
                            node->udp_handle->state = SOCK_CONNECT_OK;
                        }else {
                            /* report socket error */
                            sock_on_error(node->udp_handle, SAKURA_SOCK_ERR_UNKNOWN);
                        }                        
                    }
                }
            }

            if (node->state == SOCK_CONNECT_ERROR) {
                /* report socket error */
                sock_on_error(node, SAKURA_SOCK_ERR_CONNECT);
                continue;
            }
            /* update node state */
            if (node->state == SOCK_CONNECT_OK) {
                node->state = SOCK_CONNECTED;
                if (node->cbs.cb_connect != NULL) {
                    (sakura_void_t)node->cbs.cb_connect(node->sock);
                }
            }
        }           
    } while (SAKURA_FALSE);
}

/*
 * @brief try to read data from sock buffer
 *
 * @param[in]  handle:          socket handler
 * @param[in]  rdbuf:           recv buffer
 * @param[in]  rdbuf_len:       recv buffer length
 * @return:
 *          0:recv success
 *          1:jump to next loop
 */
static sakura_int32_t socket_try_recv_data(sock_handle_t *handle, unsigned char *rdbuf, sakura_int32_t rdbuf_len)
{
    sakura_int32_t is_continue = 0;
    sakura_int32_t rdlen = 0;

    do {
        /* check params validity */
        if (handle == NULL || rdbuf == NULL || rdbuf_len < 0) {
            is_continue = 1;
            break;
        }

        /* just read once */
        do {
            rdlen = sakura_sock_recv(handle->sock, rdbuf, rdbuf_len);
        } while (rdlen == -1 && errno == EINTR);
        /* check recv state */
        if (rdlen <= 0) {
            if (rdlen == 0) {
                SOCK_LOGE("sakura_socket:connection [%d] is closed by peer\n", handle->sock);
                /* report socket error */
                sock_on_error(handle, SAKURA_SOCK_ERR_DISCONNECTED);
            }else if (errno != EAGAIN) {
                SOCK_LOGE("recv data fail.rdlen:%d, errno:%d, info = %s\n",rdlen, errno, strerror(errno));
                /* report socket error */
                sock_on_error(handle, SAKURA_SOCK_ERR_RECV);
                is_continue = 1;
            }else {
                /* continue */
            }
        }else if (handle->cbs.cb_recv != NULL) {
            /* save received data */
            (sakura_void_t)handle->cbs.cb_recv(handle->sock, rdbuf, rdlen);
        }else {
            /* continue */
        }
    } while (SAKURA_FALSE);
    return is_continue;
}

/*
 * @brief check whether there is data reading
 *
 * @param[in]  node:            socket handler
 * @param[in]  rdbuf:           recv buffer
 * @param[in]  rdbuf_len:       recv buffer length
 * @return:
 *          0:recv success
 *          1:jump to next loop
 */
static sakura_int32_t socket_check_fds_recv(sock_handle_t *node, unsigned char *rdbuf, sakura_int32_t rdbuf_len)
{
    sakura_int32_t ret = SOCK_OK;

    do {
        /* check params validity */
        if (node == NULL || rdbuf == NULL || rdbuf_len < 0) {
            /* although can't be NULL, but default continue to next node.*/
            ret = 1;
            break;
        }
        if ((node->protocol == (sakura_uint8_t)SAKURA_TCP) && (node->state == SOCK_CONNECTING
            || node->state == SOCK_SSL_INIT || node->state == SSL_CONNECTING)) {
            /* set socket state to connect error */
            SOCK_LOGE("sock:%d select read when connecting\n", node->sock);
            node->state = SOCK_CONNECT_ERROR;
            ret = 1;
            break;
        }
        /* try to read data */
        ret = socket_try_recv_data(node, rdbuf, rdbuf_len);
        if (ret == 0 && node->protocol == (sakura_uint8_t)SAKURA_UDP) {
            ret = 1;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/*
 * @brief try to send data to sock buffer
 *
 * @param[in]  node:            socket handler
 * @return:
 *          0:recv success
 *          1:jump to next loop
 */
static sakura_int32_t socket_check_fds_send(sock_handle_t *node)
{
    sakura_int32_t ret = SOCK_OK;
    sakura_int32_t is_continue = 0;

    /* check params validity */
    if (node == NULL) {
        /* although can't be NULL, but default continue to next node.*/
        is_continue = 1;
    }else {

        if (node->state == SOCK_CONNECTING) {
            /* update socket handle status */
            node->state = SOCK_CONNECT_OK;
            /* tcp connect successfully */
            SOCK_LOGI("Socket[%d] connect to [%s:%u] successfully\n", node->sock,
                                node->status.server_host, sakura_ntohs(node->server_port));
            is_continue = 1;
        }
        if (node->state == SOCK_CONNECTED) {
            /* send data */
            ret = sock_send(node);
            if (ret < 0) {
                sock_on_error(node, SAKURA_SOCK_ERR_SEND);
            }
        }
    }
    return is_continue;
}

/*
 * @brief check socket select status
 *
 * @param[in]  read_fds:        read flag
 * @param[in]  write_fds:       write flag
 * @param[in]  error_fds:       error flag
 * @param[out] node:            socket handle
 * @return:
 *          0:success
 *         <0:fail
 */
static sakura_int32_t socket_handle_net_event(sock_handle_t *node, fd_set *read_fds, fd_set *write_fds, fd_set *error_fds)
{
    /* init */
    sakura_uint8_t rdbuf[READ_BUFFER_LENGTH] = {0};
    sakura_int32_t ret = SOCK_OK;

    do {
        /* node can't be NULL */
        if (node != NULL && read_fds != NULL && write_fds != NULL && error_fds != NULL) {
            /* check error first */
            if (FD_ISSET(node->sock, error_fds)) {
                node->state = SOCK_CONNECT_ERROR;
                SOCK_LOGE("socket[%d] is error\n", node->sock);
                break;
            }

            /* check if readable */
            if (FD_ISSET(node->sock, read_fds)) {
                ret = socket_check_fds_recv(node, rdbuf, READ_BUFFER_LENGTH);
            }

            /* check if writable */
            if (FD_ISSET(node->sock, write_fds) && ret == 0) {
                (sakura_void_t)socket_check_fds_send(node);
            }
        }else {
            ret = SOCK_ERROR;
        }
    }while (SAKURA_FALSE);
    return ret;
}

/*
 * @brief check socket select status
 *
 * @param[in]  read_fds:        read flag
 * @param[in]  write_fds:       write flag
 * @param[in]  error_fds:       error flag
 * @param[out] max_sock:        max sock id
 * @return:    void
 */
static sakura_void_t socket_check_fds(fd_set *read_fds, fd_set *write_fds, fd_set *error_fds, sakura_int32_t max_sock)
{
    sakura_int32_t ret = SOCK_OK;
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;
    /* The period for the select operation, this would determine the response time for the network event. */
    struct timeval tval = {0, 100 * 1000};

    do
    {
        /* check params validity */
        if (read_fds == NULL || write_fds == NULL || error_fds == NULL || max_sock < 0) {
            SOCK_LOGE("invalid parameter in checking fds\n");
            break;
        }

        /* reset errno */
        errno = 0;

        /* Monitor socket descriptor */
        ret = select(max_sock + 1, read_fds, write_fds, error_fds, &tval);
        if (ret <= 0) {
            if(errno == 0){
                SOCK_LOGW("select timeout(100ms)! max_sock:%d, ret=%d\n", max_sock, ret);
            } else {
                SOCK_LOGE("select error. max_sock:%d, ret=%d, errno:%d, info = %s\n", max_sock, ret, errno, strerror(errno));
            }
            break;
        }

        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            /* handle network event */
            ret = socket_handle_net_event(node, read_fds, write_fds, error_fds);
        }
    } while (SAKURA_FALSE);
}

/*
 * @brief loop tick all socket handler to handle socket message
 *
 * @param[in]  void
 * @return:    void
 */
sakura_void_t sakura_socket_loop_once(sakura_void_t)
{
    sakura_int32_t max_sock = -1;
    fd_set read_fds;
    fd_set write_fds;
    fd_set error_fds;

    (sakura_void_t)pthread_mutex_lock(&glob_socket_mutex);
    do {

        socket_establish_connect();
        /* init socket flag */
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&error_fds);
        max_sock = socket_get_max_socket(&read_fds, &write_fds, &error_fds);
        /* no active socket, exit */
        if (max_sock < 0) {
            break;
        }
        socket_check_fds(&read_fds, &write_fds, &error_fds, max_sock);
    } while (SAKURA_FALSE);

    pthread_mutex_unlock(&glob_socket_mutex);
}

/*
 * @brief cleanup socket management
 *
 * @param[in]  void
 * @return:
 *          0:ok
 *         -1:fail
 */
sakura_int32_t sakura_socket_cleanup(sakura_void_t)
{
    sakura_int32_t ret = SOCK_OK;

    (sakura_void_t)pthread_mutex_lock(&glob_socket_mutex);
    do {
        /* check socket pool state */
        if (glob_sock_pool.state <= 0) {
            SOCK_LOGD("socket is not initialized yet\n");
            ret = SOCK_ERROR;
            break;
        }
        glob_sock_pool.state = 0;
        /*
        * clear the socket handlers here or depending on `sakura_sock_close` ?
        */
        (sakura_void_t)sock_pool_cleanup();
        ret = SOCK_OK;
    } while (SAKURA_FALSE);

    pthread_mutex_unlock(&glob_socket_mutex);
    return ret;
}

/*
 * @brief Create a socket handler
 *
 * @param[in]  void
 * @return:
 *          >= 0: succeeds, identifier of the socket handler
 *            -1: failed;
 */
sakura_int32_t sakura_sock_create(sakura_void_t)
{
    sakura_int32_t sock = 0;

    do {
        /* create sock */
        errno = 0;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            SOCK_LOGE("socket create failed! errno = %d, info = %s\n", errno, strerror(errno));
            break;
        }

    } while (SAKURA_FALSE);

    return sock;
}

/*
 * @brief set socket parameter
 *
 * @param[in]  sock_handler:        sock handler
 * @param[in]  sock:                sock id
 * @param[in]  protocol:            sock protocol
 * @param[in]  serv:                broker information
 * @param[in]  cbs:                 sock callback function
 * @return:    void
 */
static sakura_void_t sock_handler_set_socket_param(sock_handle_t *sock_handler, sakura_int32_t sock,
        sakura_int32_t protocol, sakura_sock_host_t* serv, sakura_sock_callbacks_t* cbs)
{
    /* check params validity */
    if (sock_handler != NULL && serv != NULL && cbs != NULL) {
        /* set socket parameter */
        sock_handler->sock    = sock;
        memcpy(&sock_handler->cbs, cbs, sizeof(sakura_sock_callbacks_t));
        sock_handler->protocol = protocol;
        (sakura_void_t)snprintf(sock_handler->status.server_host, sizeof(sock_handler->status.server_host), "%s", serv->hostname);
    }
}

/*
 * @brief create socket handler
 *
 * @param[in]  sock:                sock id
 * @param[in]  protocol:            sock protocol
 * @param[in]  serv:                broker information
 * @param[in]  cbs:                 the asynchronous callbacks for the socket event
 * @return:
 *          success:socket handler
 *          fail:NULL
 */
static sock_handle_t* sakura_create_sock_handler(sakura_int32_t sock,
                                               sakura_int32_t protocol,
                                               sakura_sock_host_t* serv,
                                               sakura_sock_callbacks_t* cbs)
{
    sock_handle_t *sock_handler = NULL;
    do {
        errno = 0;
        /* malloc socket handler */
        sock_handler = (sock_handle_t *)sakura_malloc(sizeof(sock_handle_t));
        if (sock_handler == NULL) {
            SOCK_LOGE("create socket handle fail\n");
            break;
        }
        memset(sock_handler, '\0', sizeof(sock_handle_t));
        INIT_LIST_HEAD(&sock_handler->list);
        init_queue(&sock_handler->send_queue);
        /* set socket parameter */
        sock_handler_set_socket_param(sock_handler, sock, protocol, serv, cbs);
        if (protocol == SAKURA_TCP) {
            sock_handler->server_port = sakura_htons(serv->port);
        } else {
            /* update udp state */
            sock_handler->status.index = 0;
            sock_handler->status.times = 0;
        }
        
        sock_handler->state = SOCK_INACTIVE;
        sock_pool_add(&sock_handler->list);
    } while (SAKURA_FALSE);
    return sock_handler;
}

/*
 * @brief create udp socket handler
 *
 * @param[in]  serv:                broker information
 * @return:
 *          success: udp socket handler
 *          fail: NULL
 */
static sock_handle_t* sakura_sock_create_udp_handle(sakura_sock_host_t *serv)
{
    /* init */
    sakura_int32_t err  = 0;
    sakura_int32_t udp_fd = -1;
    sock_handle_t *sock_handler_udp = NULL;
    sakura_sock_callbacks_t udp_sock_cbs = {0};

    do {
        /* check params validity */
        if (serv == NULL || serv->hostname == NULL) {
            SOCK_LOGE("Invalid argument\n");
            err = -1;
            break;
        }
        udp_sock_cbs.cb_recv = sakura_dns_recv_response;

        /* create udp socket */
        udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_fd < 0) {
            err = -1;
            break;
        }

        /* create udp socket handler */
        sock_handler_udp = sakura_create_sock_handler(udp_fd, SAKURA_UDP, serv, &udp_sock_cbs);
        if (sock_handler_udp == NULL) {
            err = -1;
            break;
        }
        sock_handler_udp->udp_handle = NULL;
    } while (SAKURA_FALSE);

    /* check udp handler */
    if (err != 0 && udp_fd >= 0) {
        (sakura_void_t)close(udp_fd);
    }
    return sock_handler_udp;
}


/*
 * @brief try to connect to the server specified by the argument `serv`
 *
 * @param[in]  sock:                sock id
 * @param[in]  serv:                broker information
 * @param[in]  cbs:                 the asynchronous callbacks for the socket event
 * @return:
 *          0: succeeds;
 *         -1: fails;
 *
 * NOTICE:
 * Try to connect to the server specified by the argument `serv`, watch if there are some events happen
 * to the socket in background and if it has, call the callbacks defined in `cbs` to notify user.
 * The return value only indicates the result of calling this function rather than whether the connection
 * to the peer has been established or not.
 * Once the connection to the peer has been setup, it should call the `cb_connect` in `cbs` to notify the
 * user.
 */
sakura_int32_t sakura_sock_connect_wrapper(sakura_int32_t sock, sakura_sock_host_t *serv, sakura_sock_callbacks_t *cbs)
{
    /* init */
    sakura_int32_t err  = -1;
    sakura_int32_t ret   = 0;
    sock_handle_t *sock_handler_tcp = NULL;
    sock_handle_t *sock_handler_udp = NULL;

    (sakura_void_t)pthread_mutex_lock(&glob_socket_mutex);
    do {
        /* check params validity */
        if (sock < 0 || serv == NULL || serv->hostname == NULL || cbs == NULL) {
            SOCK_LOGE("Invalid argument\n");
            break;
        }

        /* check ipv4 addr */
        if(sakura_is_ipv4_address(serv->hostname) == SAKURA_FALSE){
            /* get server ip by host */
            ret = sakura_dns_get_ip_by_host(serv->hostname);
            if (ret == 0) {
                /* create udp handle */
                sock_handler_udp = sakura_sock_create_udp_handle(serv);
                if (sock_handler_udp == NULL) {
                    break;
                }
            }
        } else {
            /* set server ip by host */
            (void)sakura_dns_set_ip_by_host(serv->hostname, sakura_inet_addr(serv->hostname));
        }

        /* create tcp socket handler */
        sock_handler_tcp = sakura_create_sock_handler(sock, SAKURA_TCP, serv, cbs);
        if (sock_handler_tcp == NULL) {
            break;
        }
        sock_handler_tcp->udp_handle = sock_handler_udp;
        err = 0;
    } while (SAKURA_FALSE);
    /* check udp handler */
    if (err != 0 && sock_handler_udp != NULL) {
        sock_pool_del(sock_handler_udp->sock);
    }

    pthread_mutex_unlock(&glob_socket_mutex);
    return err;
}

/*
 * @brief try to send some data to the peer
 *
 * @param[in]  sock: socket handler
 * @param[in]  data: pointer of the data to be sent
 * @param[in]  len: length of the data
* @return:
 *          0: succeeds
 *         -1: fails
 *
 * NOTICE:
 * Try to send some data to the peer.
 * The return value only indicates the result of calling this function rather than whether the data has been
 * sent completely or not, or something error happens.
 * Once the data has been completely sent to the peer, it should call the `cb_send` in `cbs` to notify user.
 */
sakura_int32_t sakura_sock_send(sakura_int32_t sock, const sakura_uint8_t *send_buf, sakura_uint32_t len)
{
    sakura_int32_t ret = SOCK_OK;
    do {
        /* check params validity */
        if (NULL != send_buf) {
            /* send data */
            ret = send(sock, send_buf, len, MSG_DONTWAIT);
            break;
        }else {
            /* invalid parameter */
            ret = SOCK_ERROR;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/*
 * @brief convert socket data to socket handler
 *
 * @param[in]  sock:                sock id
 * @param[in]  data:                data to send
 * @param[in]  len:                 data length
 * @return:
 *          0: succeeds
 *         -1: fails
 */
sakura_int32_t sakura_sock_send_wrapper(sakura_int32_t sock, const sakura_uint8_t *data, sakura_uint32_t len)
{
    /* init variable */
    sakura_int32_t ret = SOCK_ERROR;
    sakura_uint32_t find_flag = 0;
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;

    (sakura_void_t)pthread_mutex_lock(&glob_socket_mutex);
    do {
        /* check params validity */
        if (sock < 0 || data == NULL || len == 0U) {
            SOCK_LOGE("Invalid argument\n");
            break;
        }

        /* loop find socket handler */
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            /* match sock */
            if (node->sock == sock) {
                find_flag = 1;
                ret = data_ready_to_send(node, data, len);
                break;
            }
        }

        /* not found */
        if(find_flag == 0){
            SOCK_LOGE("this sock[%d] is not in the linked list!\n", sock);
        }
    } while (SAKURA_FALSE);

    pthread_mutex_unlock(&glob_socket_mutex);
    return ret;
}

/*
 * @brief recv data from socket buffer
 *
 * @param[in]  sock:                sock id
 * @param[in]  recv_buf:            recv buffer
 * @param[in]  len:                 recv buffer length
 * @return:
 *          0: succeeds
 *         -1: fails
 */
sakura_int32_t sakura_sock_recv(sakura_int32_t sock, sakura_uint8_t *recv_buf, sakura_uint32_t len)
{
    sakura_int32_t ret = SOCK_OK;
    do {
        /* check params validity */
        if (NULL != recv_buf) {
            errno = 0;
            ret = recv(sock, recv_buf, len, MSG_DONTWAIT);
        }else {
            /* invalid parameter */
            ret = SOCK_ERROR;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/*
 * @brief close a socket handler
 *
 * @param[in]  sock: socket id.
 * @return:
 *          0: succeeds;
 *         -1: fails;
 */
sakura_int32_t sakura_sock_close(sakura_int32_t sock)
{
    sakura_int32_t ret = SOCK_OK;
    sock_handle_t *handler = NULL;
    do {
        /* check params validity */
        if (sock < 0) {
            SOCK_LOGE("Invalid argument\n");
            ret = SOCK_ERROR;
            break;
        }
        /* find handler by sock id */
        handler = find_handle_by_sock(sock);
        if (handler != NULL) {
            send_queue_clear(&handler->send_queue);
            sock_pool_del(sock);
        } else {
            ret = SOCK_ERROR;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/*
 * @brief try to close a socket handler by sock id
 *
 * @param[in]  sock: socket id.
 * @return:
 *          0: succeeds;
 *         -1: fails;
 */
sakura_int32_t sakura_sock_close_wrapper(sakura_int32_t sock)
{
    sakura_int32_t ret = 0;
    (sakura_void_t)pthread_mutex_lock(&glob_socket_mutex);
    ret = sakura_sock_close(sock);
    pthread_mutex_unlock(&glob_socket_mutex);
    return ret;
}

static sakura_int32_t data_ready_to_send(sock_handle_t *handle, const sakura_uint8_t *data, sakura_uint32_t len)
{
    sakura_int32_t ret = SOCK_OK;
    queue_node_t *node = NULL;

    do
    {
        /* check params */
        if(handle == NULL || data == NULL || len == 0){
            SOCK_LOGE("invalid params!\n");
            ret = SOCK_ERROR;
            break;
        }
        node = create_queue_node(data, len);
        if(node == NULL){
            ret = SOCK_ERROR;;
            break;
        }
        enqueue(&handle->send_queue, node);
    } while (SAKURA_FALSE);

    return ret;   
}

static sakura_void_t send_queue_clear(queue_t *queue)
{
    queue_node_t *node = NULL;

    if(queue != NULL){
        /* clear queue */
        do
        {
            /* dequeue */
            node = dequeue(queue);
            if(node != NULL){
                delete_queue_node(node);
            }
        } while (node != NULL);
        
        init_queue(queue);        
    }
}