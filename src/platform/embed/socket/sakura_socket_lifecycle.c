/*
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * This file gives the implementation of asynchronous socket API in Linux platform.
 *
 * It is better to use hash or map to quickly find the socket handler from the socket
 * identifier, however, usually there are only several sockets, the sequencial search
 * is more simple to implement.
 */

#include <string.h>
#include "sakura_list.h"
#include "sakura_mutex.h"
#include "sakura_mem.h"
#include "sakura_log.h"
#include "sakura_socket.h"
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


typedef struct {
    sakura_int32_t state;
    list_head_t list;
    sakura_int32_t sock;
    sakura_sock_callbacks_t cbs;

    struct queue_t send_queue;

    /*
     * to be simple, we only hold one send request for a connection;
     * for more practical, it should be queue
     */
    struct {
        const sakura_uint8_t *data;
        sakura_uint32_t len;
    } send_req;
} sock_handle_t;

typedef struct {
    sakura_int32_t sock;
    sakura_uint16_t port;
    sakura_char_t *host;
} connect_req_t;

typedef struct {
    sakura_int32_t state;
    sakura_mutex_t sock_pool_lock;
    list_head_t sock_list; /* a list of sockets */
} sock_pool_t;

#ifdef CONFIG_LOG
/* message type */
static sakura_char_t *sock_state_array[] =
{
    "SOCK_INACTIVE",
    "SOCK_CONNECTING",
    "SOCK_CONNECT_OK",
    "SOCK_CONNECTED",
    "SOCK_CONNECT_ERROR"
};

/* socket error code */
static sakura_char_t *sock_err_code_array[] =
{
    "SAKURA_SOCK_ERR_DISCONNECTED",
    "SAKURA_SOCK_ERR_CONNECT",
    "SAKURA_SOCK_ERR_SEND",
    "SAKURA_SOCK_ERR_RECV",
    "SAKURA_SOCK_ERR_UNKNOWN",
};
#endif

static sock_pool_t glob_sock_pool = {0};

#define GET_SOCK_STATE(type)                     sock_state_array[(type) & 0x0fU]
#define GET_SOCK_ERR_CODE(type)                  sock_err_code_array[(abs(type + 1) & 0x0fU)]

/* should use the log API defined in the main repo */
#define LOG_TAG                                 "SOCK"
#define SOCK_LOGE                               SAKURA_LOGE
#define SOCK_LOGD                               SAKURA_LOGD
#define SOCK_LOGI                               SAKURA_LOGI
#define SOCK_LOGW                               SAKURA_LOGW

#ifndef READ_BUFFER_LENGTH
#define READ_BUFFER_LENGTH      CONFIG_READ_BUFFER_LENGTH
#endif

/* function declaration */
static sakura_void_t sock_on_error(sock_handle_t *handler, sakura_int32_t code);
static sakura_void_t sock_pool_del(sakura_int32_t sock);
static sakura_void_t sock_pool_add(list_head_t *item);
static sakura_int32_t sock_connect_state_change(sakura_int32_t sock, sakura_int32_t state);
static sakura_int32_t sock_connect_succeed(sakura_int32_t sock);
static sakura_int32_t sock_connect_failed(sakura_int32_t sock);
static sakura_void_t sock_pool_cleanup(sock_handle_t *node);
static sakura_int32_t data_ready_to_send(sock_handle_t *handle, const sakura_uint8_t *data, sakura_uint32_t len);
static sakura_void_t send_queue_clear(queue_t *queue);
static sakura_int32_t sock_send(sock_handle_t *handler);
static sakura_int32_t sock_send_bytes(sakura_int32_t sock, const sakura_uint8_t *data, sakura_uint32_t len);


/**
 * @brief socket module init
 * 
 * @return:
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_socket_init(sakura_void_t)
{
    sakura_int32_t ret = 0;
    do
    {
        /* check init state */
        if (glob_sock_pool.state > 0) {
            SOCK_LOGE("socket is already initialized\n");
            ret = -1;
            break;
        }
    } while (SAKURA_FALSE);

    /* DO IT FIRST */
    INIT_LIST_HEAD(&glob_sock_pool.sock_list);

    glob_sock_pool.state = 1;
    return ret;
}

/**
 * @brief drive the sock run once
 * 
 * @return sakura_void_t 
 */
sakura_void_t sakura_socket_loop_once(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sakura_int32_t rdlen = 0;
    sakura_uint8_t rdbuf[READ_BUFFER_LENGTH] = {0};
    /*
     * The period for the select operation, this would determine the response time for the network event.
     */
    sock_handle_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {

        /* each sock list */
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            if (node) {
                if (node->state == SOCK_INACTIVE || node->state == SOCK_CONNECTING) {
                    continue;
                } else if (node->state == SOCK_CONNECT_ERROR) {
                    sock_on_error(node, SAKURA_SOCK_ERR_CONNECT);
                    continue;
                } else if (node->state == SOCK_CONNECT_OK) {
                    node->state = SOCK_CONNECTED;
                    if (node->cbs.cb_connect) {
                        (sakura_void_t)node->cbs.cb_connect(node->sock);
                    }
                } else {
                    /* fall through */
                }
            }
        }
        pos = next = NULL;
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            if (node && node->state == SOCK_CONNECTED) {
                /* check if readable */
                rdlen = sakura_sock_recv(node->sock, rdbuf, READ_BUFFER_LENGTH); /* read it in non-blocking mode */
                if (rdlen == -2) {
                    sock_on_error(node, SAKURA_SOCK_ERR_RECV);
                } else if (rdlen == -1) {
                    //do nothing, no data recv, just wait.
                } else if (rdlen == 0) {
                    SOCK_LOGE("lifecycle:connection [%d] is closed by peer\n", node->sock);
                    sock_on_error(node, SAKURA_SOCK_ERR_DISCONNECTED);
                } else if (node->cbs.cb_recv) {
                    node->cbs.cb_recv(node->sock, rdbuf, rdlen);
                } else {
                    /* fall through */
                }

                /* check if writable */
                ret = sock_send(node);
                if (ret < 0) {
                    sock_on_error(node, SAKURA_SOCK_ERR_SEND);
                } else if (ret == (sakura_int32_t)node->send_req.len) {
                    if (node->cbs.cb_send) {
                        (sakura_void_t)node->cbs.cb_send(node->sock);
                    }
                    node->send_req.len = 0;
                    node->send_req.data = NULL;
                } else {
                    /* fall through */
                }
            }
        }
    } while (SAKURA_FALSE);
}

/**
 * @brief socket module cleanup
 * 
 * @return:
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_socket_cleanup(sakura_void_t)
{
    sakura_int32_t ret = 0;
    sock_handle_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* check */
        if (glob_sock_pool.state <= 0) {
            SOCK_LOGD("socket is not initialized yet\n");
            ret = -1;
        }

        /* loop find socket handler from socket pool */
        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            /* socket clean */
            sock_pool_cleanup(node);
        }
        glob_sock_pool.state = 0;
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief Try to connect to the server specified by the argument `serv`, watch if there are some events happen
 *        to the socket in background and if it has, call the callbacks defined in `cbs` to notify user.
 *        The return value only indicates the result of calling this function rather than whether the connection
 *        to the peer has been established or not.
 *        Once the connection to the peer has been setup, it should call the `cb_connect` in `cbs` to notify the user.
 * 
 * @param sock socket handler;
 * @param serv the server info, see `sakura_sock_host_t`;
 * @param cbs the asynchronous callbacks for the socket event, see `sakura_sock_callbacks_t`;
 * @return:
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_sock_connect_wrapper(sakura_int32_t sock, sakura_sock_host_t *serv, sakura_sock_callbacks_t *cbs)
{
    sakura_int32_t ret = 0;
    sock_handle_t *sock_handler = NULL;
    do
    {
        if (sock < 0 || serv == NULL || serv->hostname == NULL || cbs == NULL) {
            SOCK_LOGE("Invalid argument.\n");
            ret = -1;
            break;
        }
        sock_handler = (sock_handle_t *)sakura_malloc(sizeof(sock_handle_t));
        if (sock_handler == NULL) {
            SOCK_LOGE("call sakura_malloc failed.\n");
            ret = -1;
            break;
        }

        memset(sock_handler, 0, sizeof(sock_handle_t));
        sock_handler->sock  = sock;
        memcpy(&sock_handler->cbs, cbs, sizeof(sakura_sock_callbacks_t));

        /* add it first */
        SOCK_LOGD("add a sock[%d] to the list.\n", sock_handler->sock);
        sock_pool_add(&sock_handler->list);
        init_queue(&sock_handler->send_queue);
        sock_handler->state = SOCK_CONNECTING;

        /*
        * TODO: it is better to let the workers in thread pool to do the blocking work;
        */
        if (sakura_sock_connect(sock, serv) == 0) {
            SOCK_LOGD("connect succeeded, do callback.\n");
            sock_connect_succeed(sock);
            ret = 0;
            break;
        } else {
            SOCK_LOGE("connect failed!\n");
            sock_connect_failed(sock);
        }

    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief Try to send some data to the peer.
 *        The return value only indicates the result of calling this function rather than whether the data has been
 *        sent completely or not, or something error happens.
 *        Once the data has been completely sent to the peer, it should call the `cb_send` in `cbs` to notify user.
 * 
 * @param sock socket handler;
 * @param data pointer of the data to be sent;
 * @param len length of the data;
 * @return:
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_sock_send_wrapper(sakura_int32_t sock, const sakura_uint8_t *data, sakura_uint32_t len)
{
    /* init variable */
    sakura_int32_t ret = SOCK_ERROR;
    sakura_uint32_t find_flag = 0;
    sock_handle_t *node = NULL;
    struct list_head *pos  = NULL;
    struct list_head *next = NULL;

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

    return ret;
}

/**
 * @brief Close a socket handler.
 * 
 * @param sock socket handler.
 * @return: 
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_sock_close_wrapper(sakura_int32_t sock)
{
    sakura_int32_t ret = 0;
    do
    {
        if (sock < 0) {
            SOCK_LOGE("Invalid argument\n");
            ret = -1;
            break;
        }
        sock_pool_del(sock);
        ret = sakura_sock_close(sock);
    } while (SAKURA_FALSE);
    return ret;
}


/**
 * static functions
 */
static sakura_void_t sock_on_error(sock_handle_t *handler, sakura_int32_t code)
{
    SOCK_LOGE("sock %d has %s failure!\n", handler->sock, GET_SOCK_ERR_CODE(code));
    if (code == SAKURA_SOCK_ERR_DISCONNECTED){
        if (handler->cbs.cb_recv) {
            (sakura_void_t)handler->cbs.cb_recv(handler->sock, NULL, 0);
        }
    }
    /* report status code */
    if (handler->cbs.cb_status) {
        (sakura_void_t)handler->cbs.cb_status(handler->sock, code);
    }
    /* reset sock handler state */
    handler->state = SOCK_INACTIVE;
}

static sakura_void_t sock_pool_del(sakura_int32_t sock)
{
    sock_handle_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* check params */
        if(sock < 0){
            SOCK_LOGE("socket is invalid!\n");
            break;
        }

        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            if (node && node->sock == sock) {
                SOCK_LOGD("del a sock [%d]\n", sock);
                send_queue_clear(&node->send_queue);
                list_del(&node->list);
                sakura_free(node);
                break;
            }
        }       
    } while (SAKURA_FALSE);
}
static sakura_void_t sock_pool_add(list_head_t *item)
{
    do
    {
        /* check params */ 
        if(item == NULL){
            SOCK_LOGE("invalid params!\n");
            break;
        }

        list_add_tail(item, &glob_sock_pool.sock_list);
    } while (SAKURA_FALSE);
}

/* in case of user close the socket before connection is finished */
static sakura_int32_t sock_connect_state_change(sakura_int32_t sock, sakura_int32_t state)
{
    sakura_int32_t ret = -1;
    sock_handle_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* check params */
        if(sock < 0){
            SOCK_LOGE("socket is invalid!\n");
            break;
        }

        list_for_each_safe(pos, next, &glob_sock_pool.sock_list) {
            node = list_entry(pos, sock_handle_t, list);
            if (node && node->sock == sock) {
                if (node->state == SOCK_CONNECTING) {
                    SOCK_LOGD("sock [%d] state change to [%s]\n", sock, GET_SOCK_STATE(state));
                    node->state = state;
                    ret = 0;
                }
                break;
            }
        }     
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t sock_connect_succeed(sakura_int32_t sock)
{
    return sock_connect_state_change(sock, SOCK_CONNECT_OK);
}

static sakura_int32_t sock_connect_failed(sakura_int32_t sock)
{
    return sock_connect_state_change(sock, SOCK_CONNECT_ERROR);
}

static sakura_void_t sock_pool_cleanup(sock_handle_t *node)
{
    if(node != NULL){
        (sakura_void_t)sakura_sock_close(node->sock);
        send_queue_clear(&node->send_queue);
        list_del(&node->list);
        sakura_free(node);        
    }
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
            /* send data to server */
            ret = sakura_sock_send(sock, ptr, rest);
            if (ret < 0) {
                /* try again */
                SOCK_LOGE("send data fail, ret:%d\n", ret);
                break;
            } else {
                /* update data pointer and rest length */
                ptr  += ret;
                rest -= ret;
            }
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
