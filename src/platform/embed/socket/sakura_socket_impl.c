/*
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
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
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "sakura_list.h"
#include "sakura_socket.h"
#include "sakura_log.h"
#include "sakura_utils.h"


/* should use the log API defined in the main repo */
#define LOG_TAG             "SOCK"
#define SOCK_LOGE           SAKURA_LOGE
#define SOCK_LOGD           SAKURA_LOGD
#define SOCK_LOGI           SAKURA_LOGI
#define SOCK_LOGW           SAKURA_LOGW

/**
 * @brief Create a socket handler.
 * 
 * @return:
 *    >= 0: succeeds, identifier of the socket handler;
 *      -1: failed;
 */
sakura_int32_t sakura_sock_create(sakura_void_t)
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

/**
 * @brief Establish a connection to the server specified by the argument `serv` in a blocking way.
 * 
 * @param sock socket handler;
 * @param serv the server info, see `sakura_sock_host_t`;
 * @return: 
 *      0: connecting succeeded;
 *     -1: connecting failed;
 */
sakura_int32_t sakura_sock_connect(sakura_int32_t sock, sakura_sock_host_t *serv)
{
    sakura_int32_t ret = -1;
    sakura_char_t port[16] = {0};
    struct addrinfo hints;
    struct sockaddr_in dest;
    struct addrinfo *result = NULL, *rp = NULL;

    do
    {
        /* check params */
        if (sock < 0 || serv == NULL || serv->hostname == NULL) {
            SOCK_LOGE("Invalid argument\n");
            ret = -1;
            break;
        }

        /* set hints */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;
        sprintf(port, "%d", serv->port);

        SOCK_LOGD("[CONNECT] sock = %d, host = [%s:%d]\n", sock, serv->hostname, serv->port);

        if(sakura_is_ipv4_address(serv->hostname) == SAKURA_FALSE){
            /* get ip address */
            ret = getaddrinfo(serv->hostname, port, &hints, &result);
            if(ret < 0){
                SOCK_LOGE("getaddrinfo: %s\n", gai_strerror(ret));
                ret = -1;
                break;
            }
            for (rp = result; rp != NULL; rp = rp->ai_next) {
                ret = connect(sock, rp->ai_addr, rp->ai_addrlen);
                if (ret != -1) {
                    SOCK_LOGD("Connect to [%s:%s] succeed\n", serv->hostname, port);
                    ret = 0;
                    break;
                } else {
                    SOCK_LOGE("connect failed, ret = %d", ret);
                }
            }              
        } else {
            /* set socket addr */
            memset(&dest, 0, sizeof(dest));
            dest.sin_family = AF_INET;
            dest.sin_port = sakura_htons(serv->port);
            dest.sin_addr.s_addr = sakura_inet_addr(serv->hostname);
            ret = connect(sock, (struct sockaddr*)&dest, sizeof(dest));
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

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
sakura_int32_t sakura_sock_send(sakura_int32_t sock, const sakura_uint8_t *send_buf, sakura_uint32_t len)
{
    return send(sock, send_buf, len, MSG_DONTWAIT);
}

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
sakura_int32_t sakura_sock_recv(sakura_int32_t sock, sakura_uint8_t *recv_buf, sakura_uint32_t len)
{
    sakura_int32_t rdlen = 0;
    sakura_int32_t ret = 0;
    do
    {
        rdlen = recv(sock, recv_buf, len, MSG_DONTWAIT);
        if (rdlen == -1 && (errno == EAGAIN || errno == EBADF || errno == EINTR)) {
            ret = -1;
            break;
        }
        if (rdlen < 0) {
            ret = -2;
            break;
        }
        if (rdlen == 0) {
            ret = 0;
            break;
        } 
        ret = rdlen;
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief close a socket handler.
 * 
 * @param sock socket handler.
 * @return: 
 *      0: succeeds;
 *     -1: fails;
 */
sakura_int32_t sakura_sock_close(sakura_int32_t sock)
{
    return close(sock);
}
