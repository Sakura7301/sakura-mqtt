#include "sakura_mqtt.h"

#define     LOG_TAG         "NET "
#define     NET_LOGD        SAKURA_LOGD
#define     NET_LOGE        SAKURA_LOGE
#define     NET_LOGI        SAKURA_LOGI
#define     NET_LOGW        SAKURA_LOGW

/**
 * The static functions declare
 */
static sakura_int32_t mqtt_try_send_wrapper(mqtt_client_t *client, sakura_uint8_t** data);
static sakura_int32_t mqtt_check_msg_state(sakura_int32_t idx, sakura_uint8_t msg_type, sakura_uint32_t len, sakura_uint32_t total);
static sakura_void_t mqtt_update_msg_state(mqtt_client_t *client, sakura_int32_t idx, MQTT_MSG_TYPE msg_type);


/**
 * @brief try send message
 * 
 * @param client MQTT client
 * @return 0: OK
 *        -1: fail
 */
sakura_int32_t mqtt_try_send(mqtt_client_t *client)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint8_t *data = NULL;
    sakura_uint32_t len = 0;

    do
    {
        /* check parameter */
        if(client == NULL){
            NET_LOGE("invalid params!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        /* send message wrapper */
        ret = mqtt_try_send_wrapper(client, &data);
        if(ret < 0){
            NET_LOGE("try send failed!\n");
            break;
        }

        len = (ret > 0) ? (ret + MQTT_SEND_PKT_HEADER_LEN) : 0;
        if(len == 0){
            break;
        }

        /* do network operation out of the mutex lock in case of dead lock */
        if (data != NULL && ret > 0) {
            ret = sakura_sock_send_wrapper(client->net.sock, data, (sakura_uint32_t)ret);
            if(ret < 0){
                ret = SAKURA_MQTT_ERROR;
            }
        }
        
        /* update buffer */
        memmove(client->net.send.buf, client->net.send.buf + len, (client->net.send.len - len));
        if(client->net.send.len >= len){
            client->net.send.len -= len;
        } else {
            client->net.send.len = 0;
        }
        
    } while (SAKURA_FALSE);
    
    return ret;
}

/**
 * @brief socket connect callback function
 * 
 * @param sock socket id
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_connect(sakura_int32_t sock)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    /* get client by sock */
    mqtt_client_t *client = NULL;

    do {
        client = mqtt_get_client_by_sock(sock);

        /* check client */
        if(client == NULL) {
            NET_LOGE("no client matched sock[%d]\n", sock);
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* update keepalive tick */
        client->last_keepalive_tick = client->tick;

        if(client->state != MQTT_CLIENT_TCP_CONNECTING) {
            NET_LOGE("sock[%d] in wrong state[%d]\n", sock, client->state);
            ret = SAKURA_MQTT_ERROR;
            
            break;
        }

        /* the context is socket thread, just change the flag */
        mqtt_next_state(client, MQTT_CLIENT_TCP_CONNECTED);
        ret = SAKURA_MQTT_STAT_OK;

        
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief socket send callback function
 * 
 * @param sock socket id
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_send(sakura_int32_t sock)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;
    do {
        /* get client by socket */
        client = mqtt_get_client_by_sock(sock);
        if(client == NULL) {
            NET_LOGE("no client matched sock[%d]\n", sock);
            ret = SAKURA_MQTT_ERROR;
            break;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief socket recv callback function
 * 
 * @param sock socket id
 * @param buf buffer
 * @param len data length
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_recv(sakura_int32_t sock, const sakura_uint8_t *buf, sakura_uint32_t len)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t rest = 0;
    mqtt_client_t *client = NULL;
    do {
        /* check parameter */
        if (buf == NULL || len == 0U) {
            NET_LOGE("buffer or len is invalid!\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* get client by socket */
        client = mqtt_get_client_by_sock(sock);
        if(client == NULL) {
            NET_LOGE("no client matched sock[%d]\n", sock);
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* update keepalive tick */
        client->last_keepalive_tick = client->tick;

        rest = MQTT_RECV_BUF_REST(client);
        /* recv buffer is full while we could not parse it, just discard it */
        if (rest == 0U) {
            NET_LOGW("recv buffer is full, clear it first!\n");
            client->net.recv.len = 0U;
            rest = client->net.recv.size;
        }

        /* receive what we could receive */
        if (len > rest) {
            NET_LOGE("too large coming data, discard the tail part [%d]!\n", len - rest);
            len = rest;
        }

        /* append the received data to recv buffer */
        memcpy(MQTT_RECV_BUF_HEAD(client), buf, len);
        client->net.recv.len += len;

        
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief report socket status code callback function
 * 
 * @param sock socket id
 * @param status status code
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_status(sakura_int32_t sock, sakura_int32_t status)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;
    do {
        /* get client by socket */
        client = mqtt_get_client_by_sock(sock);
        if(client == NULL) {
            NET_LOGE("no client matched sock[%d]\n", sock);
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* record the status code and report it to user at next tick */
        switch (status){
            case SAKURA_SOCK_ERR_DISCONNECTED:
            case SAKURA_SOCK_ERR_CONNECT:
            case SAKURA_SOCK_ERR_SEND:
            case SAKURA_SOCK_ERR_RECV:
            case SAKURA_SOCK_ERR_UNKNOWN:
                NET_LOGE("net on status: %d\n", status);
                mqtt_next_state(client, MQTT_CLIENT_CONNECT_ERROR);
                
                break;
            default:
                NET_LOGE("unknown error type, code = %d\n", status);
                ret = SAKURA_MQTT_ERROR;
                break;
        }
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_try_send_wrapper(mqtt_client_t *client, sakura_uint8_t** data)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t idx = 0;
    sakura_uint8_t msg_type = 0;
    /* from head */
    sakura_uint32_t len = 0;
    do {
        if (NULL == client || data == NULL) {
            /* invalid parameter */
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            NET_LOGE("params invalid!\n");
            break;
        }

        if(client->net.send.len == 0){
            break;
        }

        /* parse package header */
        parse_send_packet_header(client->net.send.buf, &idx, &msg_type, &len);

        /* check mqtt message state */
        ret = mqtt_check_msg_state(idx, msg_type, (sakura_uint32_t)MQTT_SEND_PKT_HEADER_LEN + len, client->net.send.len);
        if (ret < 0) {
            NET_LOGW("send buffer becomes messy\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* log print */
        if(msg_type == PUBLISH
        || msg_type == PUBACK
        || msg_type == PUBREC
        || msg_type == PUBCOMP
        || msg_type == PUBREL){
            NET_LOGD("[%02d|%s] ===> [%s]: %s[%d]\n", client->index, client->client_id, client->broker_id, mqtt_get_message_name(msg_type), len);
        } else {
            NET_LOGI("[%02d|%s] ===> [%s]: %s[%d]\n", client->index, client->client_id, client->broker_id, mqtt_get_message_name(msg_type), len);
        }

        /* update mqtt message state */
        mqtt_update_msg_state(client, idx, (MQTT_MSG_TYPE)msg_type);
            
        /* update head and total length */
        *data = client->net.send.buf + MQTT_SEND_PKT_HEADER_LEN;
        ret = (sakura_int32_t)len;
        
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_check_msg_state(sakura_int32_t idx, sakura_uint8_t msg_type, sakura_uint32_t len, sakura_uint32_t total)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    /* idx may be -1, check it */
    if (idx < -1 || idx >= MAX_MSG_STATE_ARRAY_LEN || (msg_type & 0xf0) != 0 || len > total) {
        NET_LOGE("send buffer becomes. message type  = %d, len = %d,  total = %d\n", msg_type, len, total);
        ret = SAKURA_MQTT_ERROR;
    }
    return ret;
}

static sakura_void_t mqtt_update_msg_state(mqtt_client_t *client, sakura_int32_t idx, MQTT_MSG_TYPE msg_type)
{
    do
    {
        /* check parameter */
        if(NULL == client){
            NET_LOGE("client is NULL, update msg state failed!\n");
            break;
        }
        /* mark the message was sent */
        if (mqtt_check_tracker_index(idx) == 0) {
            if ((signed char)client->msg_tracker_array[idx].expect_type == -1) {
                if(msg_type == DISCONNECT){
                    /* DISCONNECT */
                    if(client->msg_tracker_array[idx].callback != NULL){
                        client->msg_tracker_array[idx].state = (sakura_uint8_t)MSG_STATE_FINISH;
                        client->msg_tracker_array[idx].ret_code = SAKURA_MQTT_CONNECT_DISCONNECTED;
                    } else {
                        client->msg_tracker_array[idx].state = (sakura_uint8_t)MSG_STATE_FREE;
                    }
                    /* client state next */
                    mqtt_next_state(client, MQTT_CLIENT_NOT_CONNECTED);
                } else {
                    /* PUBLISH */
                    if (client->msg_tracker_array[idx].callback != NULL) {
                        client->msg_tracker_array[idx].state = (sakura_uint8_t)MSG_STATE_FINISH;
                    }else {
                        client->msg_tracker_array[idx].state = (sakura_uint8_t)MSG_STATE_FREE;
                    }
                }
            }else {
                client->msg_tracker_array[idx].state = MSG_STATE_SENT;
            }
        }
    } while (SAKURA_FALSE);
}