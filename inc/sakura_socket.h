/**
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 *
 * @file sakura_socket.h
 * @brief header of sdk socket
 * @version 1.0.0
 * @author Sakura
 * @date   2023-12-20
 *
 * CHANGELOG:
 * DATE             AUTHOR          REASON
 * 2023-12-20       Sakura          Init version;
 */

#ifndef SAKURA_SOCKET_H__
#define SAKURA_SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "sakura_types.h"
/* HEADER-BODY-COPY-BEGIN */

#define SAKURA_UDP 0
#define SAKURA_TCP 1

/* net error code */
#define SAKURA_SOCK_ERR_DISCONNECTED                  -1      /* socket is closed by peer */
#define SAKURA_SOCK_ERR_CONNECT                       -2      /* something wrong happens when try to connect to the server */
#define SAKURA_SOCK_ERR_SEND                          -3      /* something wrong happens when try to send data to server */
#define SAKURA_SOCK_ERR_RECV                          -4      /* something wrong happens when try to receive data from server */
#define SAKURA_SOCK_ERR_UNKNOWN                       -5      /* unknown error happens to socket */


/*
 * Call the following callbacks to notify the user that some events happen to the socket.
 */
typedef struct {
    sakura_int32_t (*cb_connect)(sakura_int32_t sock);                                              /* the connection to server has been established */
    sakura_int32_t (*cb_send)(sakura_int32_t sock);                                                 /* the last send command has been finished */
    sakura_int32_t (*cb_recv)(sakura_int32_t sock, const sakura_uint8_t *buf, sakura_uint32_t len);     /* some data has been received */
    sakura_int32_t (*cb_status)(sakura_int32_t sock, sakura_int32_t status);                          /* something wrong happens to the socket */
} sakura_sock_callbacks_t;

/**
 * @brief Create a socket handler.
 * 
 * @return:
 *    >= 0: succeeds, identifier of the socket handler;
 *      -1: failed;
 */
sakura_int32_t sakura_sock_create(sakura_void_t);

/**
 * @brief Establish a connection to the server specified by the argument `serv` in a blocking way.
 * 
 * @param sock socket handler;
 * @param serv the server info, see `sakura_sock_host_t`;
 * @return: 
 *      0: connecting succeeded;
 *     -1: connecting failed;
 */
sakura_int32_t sakura_sock_connect(sakura_int32_t sock, sakura_sock_host_t *serv);

/**
 * @brief Send data in a non-blocking way.
 * 
 * @param sock socket handler;
 * @param send_buf pointer of the buffer to be sent;
 * @param len length of the buffer;
 * @return:
 *    >=0: the number of bytes sent;
 *     -1: fails;
 */
sakura_int32_t sakura_sock_send(sakura_int32_t sock, const sakura_uint8_t *send_buf, sakura_uint32_t len);

/**
 * @brief Receive data in a non-blocking way.
 * 
 * @param sock socket handler;
 * @param recv_buf recv_buf: pointer of the buffer to recv the data;
 * @param len len: length of the buffer;
 * @return:
 *     >0: the number of bytes received;
 *      0: connection is closed by peer
 *     -1: no data received this time
 *     -2: error occur;
 */
sakura_int32_t sakura_sock_recv(sakura_int32_t sock, sakura_uint8_t *recv_buf, sakura_uint32_t len);

/**
 * @brief close a socket handler.
 * 
 * @param sock socket handler.
 * @return: 
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_sock_close(sakura_int32_t sock);

/* HEADER-BODY-COPY-END */

/**
 * @brief socket module init
 * 
 * @return:
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_socket_init(sakura_void_t);

/**
 * @brief drive the sock run once
 * 
 * @return sakura_void_t 
 */
sakura_void_t sakura_socket_loop_once(sakura_void_t);

/**
 * @brief socket module cleanup
 * 
 * @return:
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_socket_cleanup(sakura_void_t);

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
sakura_int32_t sakura_sock_connect_wrapper(sakura_int32_t sock, sakura_sock_host_t *serv, sakura_sock_callbacks_t *cbs);

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
sakura_int32_t sakura_sock_send_wrapper(sakura_int32_t sock, const sakura_uint8_t *data, sakura_uint32_t len);

/**
 * @brief Close a socket handler.
 * 
 * @param sock socket handler.
 * @return: 
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_sock_close_wrapper(sakura_int32_t sock);


/**
 * @brief establish a connection using the incoming IP and port.
 * 
 * @param sock socket
 * @param ip ip addr
 * @param port port
 * @param flag connect flag
 * @return sakura_int32_t 
 */
sakura_int32_t connect_wrapper(sakura_int32_t sock, sakura_uint32_t ip, sakura_uint16_t port, sakura_uint32_t flag);


#ifdef __cplusplus
}
#endif

#endif /* SAKURA_SOCKET_H__ */
