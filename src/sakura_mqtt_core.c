#include <string.h>
#include "sakura_mqtt.h"

#define LOG_TAG                             "WORK"
#define WORK_LOGD                           SAKURA_LOGD
#define WORK_LOGE                           SAKURA_LOGE
#define WORK_LOGI                           SAKURA_LOGI
#define WORK_LOGW                           SAKURA_LOGW
#define MAX_NUM_OF_REMAINING_LENGTH_BYTES   4


/* for reduce useless log printing times */
static sakura_uint32_t sg_record_mqtt_recv_len = 0u;

/* message type string */
sakura_char_t *mqtt_message_name[] =
{
    "Reserved",
    "CONNECT",
    "CONNACK",
    "PUBLISH",
    "PUBACK",
    "PUBREC",
    "PUBREL",
    "PUBCOMP",
    "SUBSCRIBE",
    "SUBACK",
    "UNSUBSCRIBE",
    "UNSUBACK",
    "PINGREQ",
    "PINGRESP",
    "DISCONNECT",
    "Reserved"
};

/* client state string */ 
sakura_char_t *mqtt_client_state[] = 
{
    "MQTT_CLIENT_FREE",
    "MQTT_CLIENT_NOT_CONNECTED",
    "MQTT_CLIENT_TCP_CONNECTING",
    "MQTT_CLIENT_TCP_CONNECTED",
    "MQTT_CLIENT_CONNECTING",
    "MQTT_CLIENT_CONNECTED",
    "MQTT_CLIENT_DISCONNECTING",
    "MQTT_CLIENT_CONNECT_ERROR"
};

/**
 * static functions declare
 */
static sakura_int32_t mqtt_handle_tcp_connected(mqtt_client_t *client);
static sakura_int32_t mqtt_poll_all_event(mqtt_client_t *client);
static sakura_int32_t mqtt_update_message_state(mqtt_client_t *client, mqtt_msg_info_t msg, sakura_int32_t index, sakura_int32_t len, mqtt_async_callback_t cb);
static sakura_int32_t mqtt_pack_publish(mqtt_client_t *client, const sakura_char_t *topic, const mqtt_message_t *message, mqtt_async_callback_t pub_cb);
static sakura_int32_t mqtt_pack_subscribe(mqtt_client_t *client, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num);
static sakura_int32_t mqtt_pack_unsubscribe(mqtt_client_t *client, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num);
static sakura_int32_t mqtt_handle_messages(mqtt_client_t *client);
static sakura_int32_t mqtt_loop_once(mqtt_client_t *client);
static sakura_int32_t mqtt_keepalive(mqtt_client_t *client);
static sakura_int32_t mqtt_append_backup_message(mqtt_client_t *client, sakura_uint8_t *msg, sakura_uint16_t msglen);
static sakura_void_t set_message_state(mqtt_msg_tracker_t *item, mqtt_msg_info_t mqtt_msg, mqtt_async_callback_t cb);
static sakura_int32_t get_free_message_state(mqtt_msg_tracker_t *msg_tracker_array);
static sakura_uint16_t gen_message_id(mqtt_client_t *client);
static sakura_int32_t mqtt_read_message(const sakura_uint8_t *data, sakura_uint32_t len, sakura_uint32_t *remain_len_bytes);
static sakura_int32_t mqtt_message_dispatch(mqtt_client_t *client, mqtt_header_t *header, sakura_uint8_t *payload, sakura_uint32_t payload_len, sakura_uint32_t message_len);
static sakura_int32_t mqtt_handle_timeout(mqtt_client_t *client, sakura_int32_t idx);
static sakura_int32_t mqtt_do_redeliver(mqtt_client_t *client, sakura_int32_t idx);
static sakura_int32_t mqtt_send_pingreq(mqtt_client_t *client);
static sakura_int32_t mqtt_handle_connack(mqtt_client_t *client, mqtt_header_t *header, const sakura_uint8_t *body, sakura_uint32_t body_len);
static sakura_int32_t mqtt_handle_publish(mqtt_client_t *client, mqtt_header_t *header, sakura_uint8_t *body, sakura_uint32_t body_len);
static sakura_int32_t mqtt_handle_simple_ack(mqtt_client_t *client, sakura_uint8_t msg_type, const sakura_uint8_t *body, sakura_uint32_t body_len);
static sakura_int32_t mqtt_handle_pingresp(mqtt_client_t *client);
static sakura_int32_t mqtt_read_remaining_length(const sakura_uint8_t *data, sakura_uint32_t len, sakura_uint32_t *value);
static sakura_int32_t mqtt_redeliver_message(mqtt_client_t *client, sakura_int32_t idx);
static sakura_void_t mqtt_remove_send_message(mqtt_client_t *client, sakura_int32_t key, sakura_uint8_t state);
static sakura_void_t mqtt_remove_backup_message(mqtt_client_t *client, sakura_int32_t idx);
static sakura_int32_t mqtt_send_backup_message(mqtt_client_t *client, sakura_int32_t idx);
static sakura_int32_t mqtt_send_common_ack(mqtt_client_t *client, sakura_uint8_t msg_type, sakura_int8_t expect_type, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t get_matched_msg_state(mqtt_msg_tracker_t *msg_tracker_array, sakura_uint8_t msg_type, sakura_uint16_t message_id);
static sakura_int32_t mqtt_report_message(mqtt_client_t *client, mqtt_header_t *header, mqtt_topic_msg_t topic_msg, sakura_int32_t idx, mqtt_msg_t mqtt_msg);
static sakura_int32_t mqtt_response_publish(mqtt_client_t *client, mqtt_header_t *header, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t mqtt_handle_ack_by_type(mqtt_client_t *client, sakura_uint8_t msg_type, mqtt_msg_t msg, sakura_int32_t idx);
static sakura_int32_t mqtt_check_buffer(sakura_uint32_t len, sakura_uint32_t total, sakura_uint8_t msg_type, sakura_int32_t idx);
static sakura_uint8_t *mqtt_get_backup_message(mqtt_client_t *client, sakura_int32_t key, sakura_uint16_t *message_len, sakura_uint32_t *msg_offset);
static sakura_int32_t mqtt_send_pubrec(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t mqtt_send_puback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t mqtt_handle_puback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t mqtt_handle_pubrec(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t mqtt_handle_pubrel(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t mqtt_handle_pubcomp(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx);
static sakura_int32_t mqtt_handle_suback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx, const sakura_uint8_t *payload, sakura_uint32_t payloadlen);
static sakura_int32_t mqtt_handle_unsuback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx);
static mqtt_topic_list_t* check_node_no_repeat(const sakura_char_t *topic, list_head_t *head);
static sakura_void_t set_async_connack_ret_code(mqtt_client_t *client, sakura_uint8_t ret_code);
static sakura_void_t set_async_suback_ret_code(sakura_int32_t *set_code, sakura_uint8_t ret_code);
static mqtt_topic_list_t* mqtt_create_topic_node(const sakura_char_t* topic);
static sakura_uint32_t get_mqtt_packet_length(const sakura_uint8_t* buffer);
static sakura_void_t write_mqtt_remain_len(sakura_uint8_t **pptr, sakura_int32_t rem_len);


/**
 * @brief drive the MQTT client life cycle
 * 
 * @param client MQTT client
 * @param tick tick
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t  mqtt_client_tick(mqtt_client_t *client, sakura_uint64_t tick)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;

    do {
        /* check parameter */
        if(client == NULL){
            ret = SAKURA_MQTT_ERROR;
            WORK_LOGE("client is invalid! tick failed!\n");
            break;
        }

        /*
        * The tick and last_keepalive_tick in client is zero when the client is just initialized, while the
        * input tick may be a large number. In this situation, the client tick would jump from zero to
        * a large number. In case of this transition, we should also update the last_keepalive_tick, otherwise,
        * pseudo tcp connection timeout would happen.
        */
        if (client->last_keepalive_tick == 0U) {
            /* update keepalive tick */
            client->last_keepalive_tick = tick;
        }

        /*
        * If and only if the client was TCP connected, we make the client actually tick.
        * Otherwise, nothing to do.
        */
        if (client->state < MQTT_CLIENT_TCP_CONNECTED) {
            ret = SAKURA_MQTT_STAT_OK;
            break;
        }
        if (client->state == MQTT_CLIENT_TCP_CONNECTED) {
            /*
            * Right now, the TCP is just connected while MQTT is still not connected, that is the moment that
            * we should update MQTT CONNECT expired time, because it is zero when we encapsulate the CONNECT packet.
            * For more details, see the implementation of API[mqtt_client_connect].
            */
            ret = mqtt_handle_tcp_connected(client);
            break;
        }

        /*
        * try send once
        * handle received data
        * loop once to handle msg state items
        * keep alive to broker
        */
        ret = mqtt_poll_all_event(client);
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief ready the MQTT CONNECT packet
 * 
 * @param client MQTT client
 * @param account_info account information
 * @param cb connect callback
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_connect(mqtt_client_t *client, const sakura_mqtt_account_info_t *account_info, mqtt_async_callback_t cb)
{
    /* init */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t len = 0;
    mqtt_msg_info_t msg_info = {0};
    mqtt_connect_data_t mqtt_conn_opt = MQTT_CONNECT_OPTION_INITIALIZER;
    do {
        /* check parameter */
        if (client == NULL || account_info == NULL) {
            WORK_LOGE("client or account info is invalid!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        /* update mqtt connect options */
        mqtt_conn_opt.client_id                 = client->client_id;
        mqtt_conn_opt.keepalive_interval        = client->keepalive_interval;
        mqtt_conn_opt.username                  = account_info->username;
        mqtt_conn_opt.password                  = account_info->password;
        mqtt_conn_opt.cleansession              = client->cleansession;
        mqtt_conn_opt.will_flag                 = client->willFlag;

        /* check will flag */
        if (mqtt_conn_opt.will_flag == 1U && client->will != NULL) {
            /* update will data */
            mqtt_conn_opt.will.qos              = client->will->qos;
            mqtt_conn_opt.will.retained         = client->will->retained;
            mqtt_conn_opt.will.topic            = client->will->topic;
            mqtt_conn_opt.will.payload          = client->will->payload;
            mqtt_conn_opt.will.payloadlen       = client->will->payloadlen;
        }else {
            /* clear will flag */
            mqtt_conn_opt.will_flag = 0U;
        }
        
        /* encode connect encapsulate */
        len = mqtt_encode_connect(MQTT_SEND_PKT_HEAD(client), MQTT_SEND_PKT_REST(client), &mqtt_conn_opt);
        if (len <= 0) {
            WORK_LOGW("no enough send buffer\n");
            ret = SAKURA_MQTT_ERROR;
            
            break;
        }
        /* update message info */
        msg_info.message_type   = CONNECT;
        msg_info.expect_type    = CONNACK;
        msg_info.expired_tick   = 0U;
        msg_info.message_id     = 0U;
        /* update connect state */
        ret = mqtt_update_message_state(client, msg_info, 0, len, cb);
        
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief ready the MQTT DISCONNECT packet
 * 
 * @param client MQTT client
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_disconnect(mqtt_client_t *client)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t result = SAKURA_MQTT_STAT_OK;
    sakura_int32_t len = 0;
    sakura_int32_t idx = -1;
    mqtt_msg_info_t msg_info = {0};
    do {
        /* check parameter */
        if (client == NULL) {
            WORK_LOGE("client is NULL! disconnect failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* check client state */
        if (client->state == MQTT_CLIENT_DISCONNECTING) {
            WORK_LOGE("already in disconnecting\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* client must is connected */
        if (client->state != MQTT_CLIENT_CONNECTED) {
            WORK_LOGE("client[%02d|%s] not connected.\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* clear send buffer */
        mqtt_try_clear_send_buffer(client);
        /* clear tracker array */
        clear_message_tracker_array(client->msg_tracker_array);
        len = mqtt_encode_disconnect(MQTT_SEND_PKT_HEAD(client), MQTT_SEND_PKT_REST(client));
        if (len <= 0) {
            WORK_LOGW("no enough send buffer\n");
            ret = SAKURA_MQTT_ERROR;
            
            break;
        }

        /* get a free message index */
        idx = get_free_message_state(client->msg_tracker_array);
        if (idx < 0) {
            WORK_LOGW("client[%02d|%s] full msg state array\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_BUSY;
            
            break;
        }

        /* update message info */
        msg_info.message_type       = DISCONNECT;
        msg_info.expect_type        = -1;
        msg_info.expired_tick       = client->tick + client->timeout_interval;
        msg_info.message_id         = 0U;

        /* update disconnect state */
        ret = mqtt_update_message_state(client, msg_info, idx, len, client->cbs.cb_disconnect);
        
        
        /* maybe we should try to send it immediately */
        result = mqtt_try_send(client);
        if (ret < 0 || result < 0) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);
    
    return ret;
}

/**
 * @brief ready the MQTT PUBLISH packet
 * 
 * @param client MQTT client
 * @param topic topic
 * @param message message information
 * @param pub_cb callback
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_publish(mqtt_client_t *client, const sakura_char_t *topic, const mqtt_message_t *message, mqtt_async_callback_t pub_cb)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t idx = -1;
    do {
        /* check parameter */
        if (client == NULL || topic == NULL || message == NULL) {
            WORK_LOGE("input params is invalid! publish failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* client must is connected */
        if (client->state != MQTT_CLIENT_CONNECTED) {
            WORK_LOGE("client[%02d|%s] not connected.\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* pack publish encapsulate */
        idx = mqtt_pack_publish(client, topic, message, pub_cb);
        if(idx < 0){
            ret = SAKURA_MQTT_ERR_BUSY;
            break;            
        }

        /* maybe we should try to send it immediately */
        if (mqtt_try_send(client) < 0) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief ready the MQTT SUBSCRIBE packet
 * 
 * @param client MQTT client
 * @param sub_topic_list list of topics to be subscribed
 * @param topic_num topics num
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_subscribe(mqtt_client_t *client, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t idx = -1;
    do {
        /* check parameter */
        if (client == NULL || sub_topic_list == NULL) {
            WORK_LOGE("input params is invalid! subscribe failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* client must is connected */
        if (client->state != MQTT_CLIENT_CONNECTED) {
            WORK_LOGE("client[%02d|%s] not connected.\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* pack subscribe encapsulate */
        idx = mqtt_pack_subscribe(client, sub_topic_list, topic_num);
        if(idx < 0){
            ret = SAKURA_MQTT_ERR_BUSY;
            break;            
        }

        /* maybe we should try to send it immediately */
        if (mqtt_try_send(client) < 0) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief ready the MQTT UNSUBSCRIBE packet
 * 
 * @param client MQTT client
 * @param unsub_topic_list list of topics to be unsubscribed
 * @param topic_num topics num
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_unsubscribe(mqtt_client_t *client, const sakura_char_t* unsub_topic_list[], sakura_uint32_t topic_num)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t idx = -1;
    do {
        /* check parameter */
        if (client == NULL || unsub_topic_list == NULL || unsub_topic_list[0] == NULL) {
            WORK_LOGE("input params is invalid! unsubscribe failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* client must is connected */
        if (client->state != MQTT_CLIENT_CONNECTED) {
            WORK_LOGE("client[%02d|%s] not connected.\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* pack unsubscribe encapsulate */
        idx = mqtt_pack_unsubscribe(client, unsub_topic_list, topic_num);
        if(idx < 0){
            ret = SAKURA_MQTT_ERR_BUSY;
            break;            
        }

        /* maybe we should try to send it immediately */
        if (mqtt_try_send(client) < 0) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief clear message tracker array, reset the state for MSG_STATE_FREE
 * 
 * @param msg_tracker_array message track array
 * @return sakura_void_t 
 */
sakura_void_t clear_message_tracker_array(mqtt_msg_tracker_t *msg_tracker_array)
{
    sakura_uint32_t loop;
    do
    {
        /* check params */
        if(msg_tracker_array == NULL){
            WORK_LOGE("input msg_tracker_array is NULL!\n");
            break;
        }

        /* clear message state */
        for(loop = 0; loop < (sakura_uint32_t)MAX_MSG_STATE_ARRAY_LEN; loop++) {
            msg_tracker_array[loop].state = (sakura_uint8_t)MSG_STATE_FREE;
        }        
    } while (SAKURA_FALSE);
}

/**
 * @brief try clear send buffer of client
 * 
 * @param client MQTT client
 * @return sakura_void_t 
 */
sakura_void_t mqtt_try_clear_send_buffer(mqtt_client_t *client)
{
    /* initialize variable */
    sakura_uint8_t msg_type = 0;
    sakura_uint8_t *head = NULL;
    sakura_int32_t idx = 0;
    sakura_uint32_t len = 0;
    sakura_uint32_t total = 0;
    sakura_uint32_t net_buf_size = 0;

    do
    {
        /* check params */
        if(client == NULL){
            WORK_LOGE("input client is invalid!\n");
            break;
        }

        total = client->net.send.len;
        head = client->net.send.buf;
        if (total > 0U) {
            /* parse package header */
            parse_send_packet_header(head, &idx, &msg_type, &len);
            /*
            *   If first message has been partly sent or it has no tracing state, 
            *   just keep it in send buffer and clear other messages, 
            *   otherwise, clear all messages.
            */
            if (idx == -1 || ((idx >= 0 && idx < MAX_MSG_STATE_ARRAY_LEN) \
                && client->msg_tracker_array[idx].state == MSG_STATE_SENDING)) {
                /* no msg state associated with it */
                head[0] = -1;
                total = (sakura_uint32_t)MQTT_SEND_PKT_HEADER_LEN + len;
            }else {
                total =  0U;
            }
            client->net.send.len = total;
        }

        net_buf_size = sakura_mqtt_get_net_buf_size();
        memset(client->net.recv.buf, 0, net_buf_size);
        client->net.recv.len = 0;
        memset(client->net.backup.buf, 0, net_buf_size);
        client->net.backup.len = 0;

        WORK_LOGD("clear client[%02d|%s] send buffer\n", client->index, client->client_id);
    } while (SAKURA_FALSE);
}

/**
 * @brief free send buffer of client
 * 
 * @param client MQTT client
 * @return sakura_void_t 
 */
sakura_void_t mqtt_free_net_buffer(mqtt_client_t *client)
{
    do
    {
        /* check params */
        if(client == NULL){
            WORK_LOGE("input client is invalid!\n");
            break;
        }

        /* clear send */
        if(client->net.send.buf != NULL){
            sakura_free(client->net.send.buf);
            client->net.send.buf = NULL;
            client->net.send.len = 0;
            client->net.send.size = 0;
        }

        /* clear recv */
        if(client->net.recv.buf != NULL){
            sakura_free(client->net.recv.buf);
            client->net.recv.buf = NULL;
            client->net.recv.len = 0;
            client->net.recv.size = 0;
        }

        /* clear backup */ 
        if(client->net.backup.buf != NULL){
            sakura_free(client->net.backup.buf);
            client->net.backup.buf = NULL;
            client->net.backup.len = 0;
            client->net.backup.size = 0;
        }
    } while (SAKURA_FALSE);
}

/**
 * @brief assembly sent package in header
 * 
 * @param header header
 * @param index client index
 * @param msg_type message type
 * @param len buffer length
 * 
 * NOTICE:
 * |             header              |    MQTT packet       |
 * +---------+---------+-------------+---------...----------+
 * |  index  | msgtype |     len     |         data         |
 * +---------+---------+-------------+---------...----------+
 * |  1Byte  |  1Byte  |  1-4 Bytes  |      len bytes       |
 * |  signed | unsigned|   unsigned  |                      |
 */
sakura_void_t assembly_send_packet_header(sakura_uint8_t *header, sakura_int32_t index, sakura_uint8_t msg_type, sakura_int32_t len)
{
    sakura_uint8_t *ptr = header;

    do
    {
        /* check params */
        if(header == NULL || len < 0){
            WORK_LOGE("invalid params\n");
            break;
        }

        header[0] = index;
        header[1] = msg_type;
        ptr += 2;/* skip index& message_type */
        write_mqtt_remain_len(&ptr, len);     
    } while (SAKURA_FALSE);
}

/**
 * @brief assembly sent package in header
 * 
 * @param header header
 * @param index client index
 * @param msg_type message type
 * @param len buffer length
 */
sakura_void_t parse_send_packet_header(sakura_uint8_t *header, sakura_int32_t *index, sakura_uint8_t *msg_type, sakura_uint32_t *len)
{

    do
    {
        /* check params */
        if(header == NULL || index == NULL || msg_type == NULL || len == NULL){
            WORK_LOGE("invalid params\n");
            break;
        }

        /* this byte is signed, it may be -1 */
        *index      = (sakura_int8_t)header[0];
        *msg_type   = header[1];
        *len        = get_mqtt_packet_length(&header[2]);
    } while (SAKURA_FALSE);
}

/**
 * @brief append topics to list
 * 
 * @param client MQTT client
 * @param idx this message tracker idx
 * @param sub_topic_list subscribe topic list
 * @param topic_num topic number
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_topics_list_append(mqtt_client_t *client, sakura_int32_t idx, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_topic_list_t* node_arr[MQTT_MAX_SUB_NUM] = {NULL};
    
    do
    {
        /* check params */
        if(client == NULL || sub_topic_list == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!\n");
            break;
        }

        /* add topic_node to list */  
        for (sakura_uint32_t i = 0; i < topic_num; i++){
            /* check topic wether NULL */
            if(sub_topic_list[i].topic == NULL){
                continue;
            }

            WORK_LOGD("client[%02d|%s] prepares to subscribe to the topic '%s'\n", client->index, client->client_id, sub_topic_list[i].topic);
            node_arr[i] = check_node_no_repeat(sub_topic_list[i].topic, &client->topic_list);
            if(node_arr[i] != NULL){
                /* already exists, update value */
                node_arr[i]->qos = sub_topic_list[i].qos;
                node_arr[i]->state = MQTT_TOPIC_STATE_SUBSCRIBING;
                node_arr[i]->tracker_idx = idx;
                continue;
            }

            /* get a new node */
            node_arr[i] = mqtt_create_topic_node(sub_topic_list[i].topic);
            if(node_arr[i] == NULL){
                ret = SAKURA_MQTT_ERR_MEMORY;
                WORK_LOGE("try malloc memory failed!\n");
                break;
            }
            
            /* set value */
            node_arr[i]->qos = sub_topic_list[i].qos;
            node_arr[i]->state = MQTT_TOPIC_STATE_SUBSCRIBING;
            node_arr[i]->tracker_idx = idx;

            /* add node */
            list_add_tail(&node_arr[i]->list, &client->topic_list);
            client->wait_sub_topic_num ++;
        }
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief append topics to list
 * 
 * @param client MQTT client
 * @param unsub_topic_list unsubscribe topic list
 * @param topic_num topic number
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_topics_list_delete(mqtt_client_t *client, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t num = 0;
    mqtt_topic_list_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* check params */
        if(client == NULL || unsub_topic_list == NULL || *unsub_topic_list == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("inout params value has invalid!\n");
            break;
        }

        /* get topics number  */
        num = ((topic_num -1) > 0) ? (topic_num -1) : 0;

        /* del topic_node from list  */
        list_for_each_safe(pos, next, &client->topic_list) {
            node = list_entry(pos, mqtt_topic_list_t, list);
            for (sakura_uint32_t i = 0; i < num; i++){
                if(strcmp(node->topic, unsub_topic_list[i]) == 0){
                    WORK_LOGD("client[%02d|%s] del topic(%s) from list\n", client->index, client->client_id, unsub_topic_list[i]);
                    list_del(&node->list);
                    sakura_free(node->topic);
                    sakura_free(node);
                    node = NULL;
                }
            }
        }

        WORK_LOGD("client[%02d|%s] total del %d topics from list\n", client->index, client->client_id, topic_num);
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief topic state update
 * 
 * @param client MQTT client
 * @param idx unsubscribe topic list
 * @param bytes topic number
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_topics_update(mqtt_client_t *client, sakura_uint32_t idx, const sakura_uint8_t *bytes)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_topic_list_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* check params */
        if(client == NULL || bytes == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("inout params value has invalid!\n");
            break;
        }

        /* del topic_node from list  */
        for (sakura_uint32_t i = 0; i < strlen((const sakura_char_t*)bytes); i++){
            list_for_each_safe(pos, next, &client->topic_list) {
                node = list_entry(pos, mqtt_topic_list_t, list);
                /* check idx  */ 
                if(node->tracker_idx == idx && node->qos == bytes[i]){
                    if(bytes[i] == SUBFAIL && node->state == MQTT_TOPIC_STATE_SUBSCRIBING){
                        node->state = MQTT_TOPIC_STATE_TO_SUBSCRIBE;
                    } else {
                        node->state = MQTT_TOPIC_STATE_SUBSCRIBED;
                        if(client->wait_sub_topic_num > 0){
                            client->wait_sub_topic_num --;
                        }
                        WORK_LOGI("client[%02d|%s] sub[%s] success!\n", client->index, client->client_id, node->topic);
                    }
                    continue;
                }
            }
        }

        
    } while (SAKURA_FALSE);
    
    return ret;
}

/**
 * 
 * @brief try reset all node state from topic_list
 * 
 * @param client MQTT client
 * @return sakura_int32_t 
 */
sakura_void_t mqtt_client_reset(mqtt_client_t *client)
{
    mqtt_topic_list_t *node = NULL;
    sakura_uint32_t topic_num = 0;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* check params */
        if(client == NULL){
            WORK_LOGE("invalid params!\n");
            break;
        }

        /* reset topic node state */
        list_for_each_safe(pos, next, &client->topic_list) {
            node = list_entry(pos, mqtt_topic_list_t, list);
            node->state = MQTT_TOPIC_STATE_TO_SUBSCRIBE;
            topic_num ++;
        }

        /* update waiting subscribe topic number */
        client->wait_sub_topic_num = topic_num;

        /* clear net buffer */
        mqtt_try_clear_send_buffer(client);

        /* clear message tracker array, messages not sent before disconnection will no longer be processed. */
        clear_message_tracker_array(client->msg_tracker_array);

        /* reset socket */
        if(client->net.sock >= 0){
            /* close sock */
            (sakura_void_t)sakura_sock_close_wrapper(client->net.sock);
            client->net.sock = -1;
        }

        /* reset ping_outstanding */
        client->ping_outstanding = 0;

        MQTT_NEXT_STATE(client, MQTT_CLIENT_NOT_CONNECTED);
        
    } while (SAKURA_FALSE);
}

/**
 * @brief check tracker idx legitimacy
 * 
 * @param index tracker index
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_check_tracker_index(sakura_int32_t index)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        if(index < 0 || (sakura_uint32_t)index >= MAX_MSG_STATE_ARRAY_LEN){
            ret = SAKURA_MQTT_ERROR;
            break;
        }
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_handle_tcp_connected(mqtt_client_t *client)
{
    /* init */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input client is invalid! tcp connect failed!\n");
            break;        
        }

        /* update expired tick */
        if (client->msg_tracker_array[0].expect_type == (sakura_int8_t)CONNACK &&
            client->msg_tracker_array[0].expired_tick == 0U &&
            client->msg_tracker_array[0].state != (sakura_uint8_t)MSG_STATE_FREE) {
            client->msg_tracker_array[0].expired_tick = client->tick + client->timeout_interval;
        }

        ret = mqtt_try_send(client);
        /* send MQTT CONNECT packet */
        if (ret < 0) {
            WORK_LOGE("send MQTT CONNECT failed\n");
            if(client->cbs.cb_connect != NULL) {
                client->cbs.cb_connect(client->index, ret);
            }
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        MQTT_NEXT_STATE(client, MQTT_CLIENT_CONNECTING);
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_poll_all_event(mqtt_client_t *client)
{
    /* init */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do {
        if(client == NULL){
            ret = SAKURA_MQTT_ERROR;
            WORK_LOGE("input client is invalid! drive event failed!\n");
            break;
        }

        /* try send message */
        ret = mqtt_try_send(client);
        if (ret < 0) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* handle mqtt messages */
        ret = mqtt_handle_messages(client);
        if (ret < 0) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* message state check, timeout, redeliver  */
        ret = mqtt_loop_once(client);
        if (ret < 0) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* keepalive */
        if(client->state == MQTT_CLIENT_CONNECTED){
            ret = mqtt_keepalive(client);
            if (ret < 0) {
                ret = SAKURA_MQTT_ERROR;
                break;
            }            
        }
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_update_message_state(mqtt_client_t *client, mqtt_msg_info_t msg, sakura_int32_t index, sakura_int32_t len, mqtt_async_callback_t cb)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t backup = 0;
    sakura_uint8_t *sendbuf_head = NULL;
    mqtt_msg_info_t msg_state = {0};
    
    do
    {
        /* check params */
        if(client == NULL || len <= 0){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid! upstate message state failed!\n");
            break;
        }
        sendbuf_head = MQTT_SEND_BUF_HEAD(client);
        /* update send length */
        client->net.send.len += (sakura_uint32_t)len + (sakura_uint32_t)MQTT_SEND_PKT_HEADER_LEN;
        /* assembly send package header */
        assembly_send_packet_header(sendbuf_head, index, msg.message_type, len);
        if (MQTT_BACKUP_ENABLE(client) && msg.message_id > 0) {
            // WORK_LOGD("client[%d] append message[%s:%d] to backup buffer\n", client->index, GET_MQTT_MSG_NAME(msg.message_type), msg.message_id);
            /* append message to backup buffer */
            backup = mqtt_append_backup_message(client, sendbuf_head, len + MQTT_SEND_PKT_HEADER_LEN);
            if(backup < 0){
                backup = 0;
            } else {
                backup = 1;
            }
        }

        /* set message state */
        msg_state.backup            = backup;
        msg_state.message_type      = msg.message_type;
        msg_state.expect_type       = msg.expect_type;
        msg_state.expired_tick      = msg.expired_tick;
        msg_state.message_id        = msg.message_id;
        set_message_state(client->msg_tracker_array + index, msg_state, cb);
        ret = index;
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_pack_publish(mqtt_client_t *client, const sakura_char_t *topic, const mqtt_message_t *message, mqtt_async_callback_t pub_cb)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t idx = -1;
    sakura_int32_t len = 0;
    mqtt_msg_ident_t pub_ident = {0};
    mqtt_msg_info_t msg_info = {0};

    do
    {
        /* check params */
        if(client == NULL || topic == NULL || message == NULL){
            WORK_LOGE("input params is invalid! packet publish message failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get free message state */
        idx = get_free_message_state(client->msg_tracker_array);
        if (idx < 0) {
            WORK_LOGW("client[%02d|%s] full msg state array\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_BUSY;
            break;
        }

        /*
        * QOS0, the message id is unused,
        * for sake of safety, we set it zero in case of conflict.
        */
        if (message->qos == QOS1 || message->qos == QOS2) {
            msg_info.message_id = gen_message_id(client);
        }

        /* encode publish package */
        pub_ident.dup = 0;
        pub_ident.message_id = msg_info.message_id;
        len = mqtt_encode_publish(MQTT_SEND_PKT_HEAD(client), MQTT_SEND_PKT_REST(client), pub_ident, topic, message);
        if (len <= 0) {
            WORK_LOGW("no enough send buffer\n");
            ret = SAKURA_MQTT_ERR_BUSY;
            
            break;
        }

        /* update message info */
        msg_info.message_type       = PUBLISH;
        msg_info.expired_tick       = client->tick + client->timeout_interval;
        if ((sakura_int8_t)(message->qos) == (sakura_int8_t)QOS0) {
            msg_info.expect_type = -1;
        }else if ((sakura_int8_t)(message->qos) == (sakura_int8_t)QOS1) {
            msg_info.expect_type = PUBACK;
        }else {
            msg_info.expect_type = PUBREC;
        }
        /* update publish state */
        ret = mqtt_update_message_state(client, msg_info, idx, len, pub_cb);
        
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_pack_subscribe(mqtt_client_t *client, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t idx = -1;
    sakura_int32_t len = 0;
    mqtt_msg_ident_t sub_ident = {0};
    mqtt_msg_info_t msg_info = {0};
    do
    {
        /* check params */
        if(client == NULL || sub_topic_list == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid! packet subscribe message failed!\n");
            break;
        }

        /* get a free message_tracker */
        idx = get_free_message_state(client->msg_tracker_array);
        if (idx < 0) {
            WORK_LOGW("client[%02d|%s] full msg state array\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_BUSY;
            break;
        }

        /* update topics list */
        ret = mqtt_topics_list_append(client, idx, sub_topic_list, topic_num);
        if(ret < 0){
            WORK_LOGE("add topic to list failed!\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* get message id */
        msg_info.message_id = gen_message_id(client);
        /* encode subscribe package */
        sub_ident.dup = 0;
        sub_ident.message_id = msg_info.message_id;
        len = mqtt_encode_subscribe(MQTT_SEND_PKT_HEAD(client), MQTT_SEND_PKT_REST(client), sub_ident, sub_topic_list, topic_num);
        if (len <= 0) {
            /* send buffer is not enough */
            WORK_LOGW("no enough send buffer\n");
            ret = SAKURA_MQTT_ERR_BUSY;
            
            break;
        }

        /* update message info */
        msg_info.message_type   = SUBSCRIBE;
        msg_info.expect_type    = SUBACK;
        msg_info.expired_tick   = client->tick + client->timeout_interval;
        /* update subscribe state */
        ret = mqtt_update_message_state(client, msg_info, idx, len, NULL);
        
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_pack_unsubscribe(mqtt_client_t *client, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t idx = -1;
    sakura_int32_t len = 0;
    mqtt_msg_ident_t unsub_ident = {0};
    mqtt_msg_info_t msg_info = {0};
    
    do
    {
        /* check params */
        if(client == NULL || unsub_topic_list == NULL || *unsub_topic_list == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid! packet unsubscribe message failed!\n");
            break;
        }


        /* get free message state */
        idx = get_free_message_state(client->msg_tracker_array);
        if (idx < 0) {
            WORK_LOGW("client[%02d|%s] full msg state array\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_BUSY;
            break;
        }

        /* remove the corresponding topic from 'topic_list' */
        mqtt_topics_list_delete(client, unsub_topic_list, topic_num);

        /* get message id */
        msg_info.message_id = gen_message_id(client);
        /* encode unsubscribe package */
        unsub_ident.dup = 0;
        unsub_ident.message_id = msg_info.message_id;
        len = mqtt_encode_unsubscribe(MQTT_SEND_PKT_HEAD(client), MQTT_SEND_PKT_REST(client), unsub_ident, unsub_topic_list, topic_num);
        if (len <= 0) {
            WORK_LOGW("no enough send buffer\n");
            ret = SAKURA_MQTT_ERR_BUSY;
            
            break;
        }

        /* update message info */
        msg_info.message_type   = UNSUBSCRIBE;
        msg_info.expect_type    = UNSUBACK;
        msg_info.expired_tick   = client->tick + client->timeout_interval;
        /* update subscribe state */
        ret = mqtt_update_message_state(client, msg_info, idx, len, client->cbs.cb_unsubscribe);
        
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_handle_messages(mqtt_client_t *client)
{
    /* initialize variable */
    sakura_uint8_t *buf = NULL;
    sakura_uint8_t *payload_buf = NULL;
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t now_buf_len = 0;
    sakura_uint32_t msg_len = 0;
    sakura_uint32_t origin_len = 0;
    sakura_uint32_t payload_len = 0;
    sakura_uint32_t remain_buf_len = 0;

    mqtt_header_t header = {0};
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input client is invalid! handle message failed!\n");
            break;
        }
        /* record the original length */
        origin_len      = client->net.recv.len;
        now_buf_len     = client->net.recv.len;
        buf             = client->net.recv.buf;
        /* check data length */
        while (now_buf_len > 0) {
            /* get a MQTT message from buffer */
            ret = mqtt_read_message(buf, now_buf_len, &remain_buf_len);
            if (ret < 0) {
                break;
            }
            /* handle data message */
            msg_len = (sakura_uint32_t)ret;
            header.byte = buf[0];
            payload_len = msg_len - 1U - remain_buf_len;
            payload_buf = buf + 1U + remain_buf_len;
            buf         += msg_len;
            now_buf_len -= msg_len;
            /* dispatch message */
            ret = mqtt_message_dispatch(client, &header, payload_buf, payload_len, msg_len);
            if (ret < 0) {
                /* fail,break loop */
                // WORK_LOGE("mqtt message dispatch failed, break loop!\n");
                break;
            }
        }

        /*
        * fatal error, clear the receive buffer and report the error code
        */
        if (ret == -1) {
            WORK_LOGE("handle message failed, clear the recv buffer\n");
            /* clear recv buffer */
        	memset(client->net.recv.buf, 0, client->net.recv.len);
            client->net.recv.len = 0;
            
            ret = SAKURA_MQTT_STAT_OK; /* discard the recv buffer instead of reset the connection */
        }else {
            /* reset the return value */
            ret = SAKURA_MQTT_STAT_OK;
            /* update recv buffer */
            if (client->net.recv.len >= origin_len) {
                /* update recv length */
                client->net.recv.len = now_buf_len + client->net.recv.len - origin_len;
                if (client->net.recv.len > 0) {
                    /* remove handled message */
                    memmove(client->net.recv.buf, buf, client->net.recv.len);
                }
            }
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_loop_once(mqtt_client_t *client)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t loop = 0;
    sakura_int32_t async_cb_data = 0;
    mqtt_async_callback_t async_cb;
    sakura_int8_t message_type = 0;

    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input client is invalid! drive mqtt failed!\n");
            break;
        }

        /* match message state array */
        for (loop = 0; ret >= 0 && loop < MAX_MSG_STATE_ARRAY_LEN; ++loop) {
            /* state is free,continue */
            if (client->msg_tracker_array[loop].state == (sakura_uint8_t)MSG_STATE_FREE) {
                continue;
            }

            /* finished */
            if (client->msg_tracker_array[loop].state == (sakura_uint8_t)MSG_STATE_FINISH) {
                async_cb = client->msg_tracker_array[loop].callback;
                async_cb_data = client->msg_tracker_array[loop].ret_code;
                message_type = client->msg_tracker_array[loop].message_type;
                /* reset message state */
                client->msg_tracker_array[loop].state = (sakura_uint8_t)MSG_STATE_FREE;
                if (async_cb != NULL) {
                    /* trigger callback */
                    async_cb(client->index, async_cb_data);
                }

                /* check DISCONNECT message */
                if(message_type == DISCONNECT){
                    /* close socket */
                    if (client->net.sock >= 0) {
                        (sakura_void_t)sakura_sock_close_wrapper(client->net.sock);
                        client->net.sock = -1;
                    }

                    MQTT_NEXT_STATE(client, MQTT_CLIENT_NOT_CONNECTED);
                }
                continue;
            }

            /* timeout */
            if (client->tick > client->msg_tracker_array[loop].expired_tick) {
                /* expired,handle timeout */
                WORK_LOGW("client[%d:%s] message[%d] timed out, processing...\n", client->index, client->client_id, client->msg_tracker_array[loop].message_id);
                ret = mqtt_handle_timeout(client, loop);
                if(ret < 0){
                    WORK_LOGE("handle timeout failed!\n");
                }
                continue;
            }

            /* redeliver */
            if (client->msg_tracker_array[loop].state == (sakura_uint8_t)MSG_STATE_REDELIVER &&
                    (client->msg_tracker_array[loop].redeliver_opt & 0x0fU) < (sakura_uint8_t)CONFIG_REDELIVER_COUNT) {
                /* redeliver message */
                ret = mqtt_do_redeliver(client, loop);
                continue;
            }
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_keepalive(mqtt_client_t *client)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check parameter */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("invalid params, keepalive fail!\n");
            break;
        }

        /* tick timeout, update last_ka_tick, send ping */
        if (client->tick > (client->last_keepalive_tick + client->keepalive_interval)) {
            /* already timeout */
            client->last_keepalive_tick = client->tick;
            if (client->ping_outstanding != 0) {
                /* PINGRESP not received in the past keepalive interval */
                WORK_LOGE("PINGRESP response timed out!\n");
                ret = SAKURA_MQTT_ERROR;
            }else {
                ret = mqtt_send_pingreq(client);
                /* Here should we handle no enough send buffer error or just let it go ? */
                if (ret != -1) {
                    client->ping_outstanding = 1;
                }
            }
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_append_backup_message(mqtt_client_t *client, sakura_uint8_t *msg, sakura_uint16_t msglen)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;

    do
    {
        /* check params */
        if(client == NULL || msg == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid! add backup message failed!\n");
            break;
        }

        /* check message length */
        if (msglen > MQTT_BACKUP_BUF_REST(client)) {
            WORK_LOGE("no enough backup buffer\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        // WORK_LOGD("append [%s:%d] to backup buffer\n", GET_MQTT_MSG_NAME(msg[1]), msgid);
        /* copy message to backup buffer */
        memcpy(MQTT_BACKUP_BUF_HEAD(client), msg, msglen);

        /* update backup buffer length */
        client->net.backup.len += msglen;
        ret = 1;
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_void_t set_message_state(mqtt_msg_tracker_t *item, mqtt_msg_info_t mqtt_msg, mqtt_async_callback_t cb)
{
    if (item != NULL) {
        /* set message state */
        item->state         = MSG_STATE_WAITING;
        /* set message type */
        item->message_type  = mqtt_msg.message_type;
        /* set expect type */
        item->expect_type   = mqtt_msg.expect_type;
        /* the high 4 bits */
        item->redeliver_opt = mqtt_msg.backup << 4U;
        /* set message id */
        item->message_id    = mqtt_msg.message_id;
        /* set expired tick */
        item->expired_tick  = mqtt_msg.expired_tick;
        /* message result */
        item->ret_code      = 0;
        /* message callback function */
        item->callback      = cb;
    } else {
        WORK_LOGE("input item is invalid! set message state failed!\n");
    }
}

static sakura_int32_t get_free_message_state(mqtt_msg_tracker_t *msg_tracker_array)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t loop = 0;

    do
    {
        /* check params */
        if(msg_tracker_array == NULL){
            WORK_LOGE("invalid params!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get free tracker index */
        for (loop = 0; loop < MAX_MSG_STATE_ARRAY_LEN; ++loop) {
            if (msg_tracker_array[loop].state == (sakura_uint8_t)MSG_STATE_FREE) {
                ret = loop;
                break;
            }
        }
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_uint16_t gen_message_id(mqtt_client_t *client)
{
    sakura_uint16_t msg_id = 0;
    do
    {
        /* check params */
        if(client == NULL){
            WORK_LOGE("invalid params!\n");
            break;
        }
        /* gen key */
        msg_id = (client->message_id == (sakura_uint32_t)MAX_MESSAGE_ID) ? 1U : client->message_id + 1U;
        client->message_id = msg_id;
    } while (SAKURA_FALSE);
    
    return msg_id;
}

static sakura_int32_t mqtt_message_dispatch(mqtt_client_t *client, mqtt_header_t *header, sakura_uint8_t *payload, sakura_uint32_t payload_len, sakura_uint32_t message_len)
{
    /* init */
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_uint8_t msg_type = 0;

    do
    {
        /* check params */
        if(client == NULL || header == NULL || payload == NULL){
            WORK_LOGE("invalid params!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        msg_type = header->bits.type;
        if(msg_type == PUBLISH
        || msg_type == PUBACK
        || msg_type == PUBREC
        || msg_type == PUBCOMP
        || msg_type == PUBREL){
            WORK_LOGD("[%02d|%s] <=== [%s]: %s[%d]\n", client->index, client->client_id, client->broker_id, GET_MQTT_MSG_NAME(msg_type), message_len);
        } else {
            WORK_LOGI("[%02d|%s] <=== [%s]: %s[%d]\n", client->index, client->client_id, client->broker_id, GET_MQTT_MSG_NAME(msg_type), message_len);
        }
        
        switch (msg_type) {
            case CONNECT:
            case SUBSCRIBE:
            case UNSUBSCRIBE:
            case PINGREQ:
            case DISCONNECT:
                /* invalid message type */
                ret = SAKURA_MQTT_ERROR;
                WORK_LOGE("Not support message type: %s\n", GET_MQTT_MSG_NAME(msg_type));
                break;

            case CONNACK:
                /* handle connack message */
                ret = mqtt_handle_connack(client, header, payload, payload_len);
                break;
            case PUBLISH:
                /* handle publish message */
                ret = mqtt_handle_publish(client, header, payload, payload_len);
                break;

            case PUBACK:
            case PUBREC:
            case PUBREL:
            case PUBCOMP:
            case SUBACK:
            case UNSUBACK:
                /* handle simple ack message */
                ret = mqtt_handle_simple_ack(client, msg_type, payload, payload_len);
                break;

            case PINGRESP:
                /* handle ping request message */
                ret = mqtt_handle_pingresp(client);
                break;

            default:
                /* unknown message type */
                ret = SAKURA_MQTT_ERROR;
                WORK_LOGE("Unknown message type: %d\n", msg_type);
                break;
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

/**
 * @brief read a message
 * 
 * @param data data buffer
 * @param len now the length of the buffer
 * @param remain_len_bytes read a message after the remain length
 * @return >0: success
 *         <0: fail
 */
static sakura_int32_t mqtt_read_message(const sakura_uint8_t *data, sakura_uint32_t len, sakura_uint32_t *remain_len_bytes)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t msg_len = 0;
    sakura_uint32_t remain_len = 0;
    do {
        /* check params */
        if(data == NULL || remain_len_bytes == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!read message failed!\n");
            break;
        }

        /* short than fixed header, length is invalid */
        if (len < 2U) {
            ret = -2;
            break;
        }

        /* one byte message type and flags */
        data += 1;
        len  -= 1U;

        ret = mqtt_read_remaining_length(data, len, &remain_len);
        if (ret < 0) {
            WORK_LOGE("read remaining_length failed, ret=%d\n",ret);
            break;
        }

        /* several bytes of remaining length */
        len -= (sakura_uint32_t)ret;

        /* incomplete */
        if (remain_len > len) {
            /* if recv len is the same as record len, do not print log */
            if (len != sg_record_mqtt_recv_len) {
                WORK_LOGE("read message failed, recv_len=%d, expect_len=%d\n", len, remain_len);
                sg_record_mqtt_recv_len = len;
            }
            ret = -2;
            break;
        }
        sg_record_mqtt_recv_len = 0;

        /* one byte flags in header, remaining length bytes and body length */
        msg_len = 1U + (sakura_uint32_t)ret + remain_len;
        *remain_len_bytes = ret;

        ret = msg_len;
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_handle_timeout(mqtt_client_t *client, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_async_callback_t async_cb;

    do {
        /* check parameter */
        if(client == NULL){
            ret = SAKURA_MQTT_ERROR;
            WORK_LOGE("input client is NULL!unable to process a timeout!\n");
            break;
        }
        /*
        * redeliver the message if possible
        * (1) the message has a backup and redelivery count is not expired;
        * OR
        * (2) the message is PUBREL and redelivery count is not expired;
        */
        if ((((client->msg_tracker_array[idx].redeliver_opt & 0xf0) != 0) || client->msg_tracker_array[idx].expect_type == (sakura_int8_t)PUBCOMP) &&
            (client->msg_tracker_array[idx].redeliver_opt & 0x0fU) < (sakura_uint8_t)CONFIG_REDELIVER_COUNT) {
            /* reset the expired tick */
            client->msg_tracker_array[idx].expired_tick = client->tick + client->timeout_interval;

            if (mqtt_redeliver_message(client, idx) < 0) {
                ret = SAKURA_MQTT_ERROR;
                break;
            }
        }else {
            /*
            * if the WHOLE message is still buffered, we should remove it from send buffer
            * if part message has been sent, just release its msg state item
            */
            if (client->msg_tracker_array[idx].state == MSG_STATE_WAITING ||
                client->msg_tracker_array[idx].state == MSG_STATE_SENDING) {
                mqtt_remove_send_message(client, idx, client->msg_tracker_array[idx].state);
            }

            /* remove the backup message */
            if ((client->msg_tracker_array[idx].redeliver_opt & 0xf0) != 0) {
                mqtt_remove_backup_message(client, idx);
            }

            /* triger callback */
            async_cb = client->msg_tracker_array[idx].callback;
            if (async_cb != NULL) {
                async_cb(client->index, SAKURA_MQTT_ERROR);
            }
            
            if(client->msg_tracker_array[idx].expect_type == CONNACK){
                /* change client state */
                WORK_LOGE("client[%02d|%s] connect timeout!\n", client->index, client->client_id);
                MQTT_NEXT_STATE(client, MQTT_CLIENT_CONNECT_ERROR);
            }

            /* release it message tracker */
            client->msg_tracker_array[idx].state = MSG_STATE_FREE;
        }
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_do_redeliver(mqtt_client_t *client, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* invalid parameter */
        if(client == NULL){
            ret = SAKURA_MQTT_ERROR;
            WORK_LOGE("input client is NULL!do redeliver failed!\n");
            break;
        }

        /* what message we expect from broker? */
        switch (client->msg_tracker_array[idx].expect_type) {
            /* redeliver message */
            case PUBACK:
            case PUBREC:
            case SUBACK:
            case UNSUBACK:
                ret = mqtt_send_backup_message(client, idx);
                break;
            /* redeliver ACK */
            case PUBCOMP:
                // WORK_LOGD("client[%d:%s] redeliver [PUBREL:%d]\n", client->index, client->client_id, client->msg_tracker_array[idx].message_id);
                ret = mqtt_send_common_ack(client, PUBREL, PUBCOMP, client->msg_tracker_array[idx].message_id, idx);
                break;
            default:
                /* invalid type */
                WORK_LOGE("client[%d:%s]  redelivery was not supported when expect %s\n", client->index, client->client_id, GET_MQTT_MSG_NAME((sakura_uint8_t)(client->msg_tracker_array[idx].expect_type)));
                break;
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_send_pingreq(mqtt_client_t *client)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t len = 0;
    sakura_uint8_t *sendbuf_head = NULL;
    
    do {
        /* check parameter */
        if(client == NULL){
            ret = SAKURA_MQTT_ERROR;
            WORK_LOGE("input client is NULL!send pingreq failed!\n");
            break;
        }

        sendbuf_head = MQTT_SEND_BUF_HEAD(client);
        /* encode ping request package */
        len = mqtt_encode_pingreq(MQTT_SEND_PKT_HEAD(client), MQTT_SEND_PKT_REST(client));
        /* check package length */
        if (len <= 0) {
            WORK_LOGW("no enough send buffer\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }
        client->net.send.len += (sakura_uint32_t)len + (sakura_uint32_t)MQTT_SEND_PKT_HEADER_LEN;
        /* assembly package header */
        assembly_send_packet_header(sendbuf_head, -1, PINGREQ, len);
        /* try send message */
        ret = mqtt_try_send(client);
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_read_remaining_length(const sakura_uint8_t *data, sakura_uint32_t len, sakura_uint32_t *value)
{
    /* initialize variable */
    sakura_uint8_t byte = 0;
    sakura_int32_t idx = 0;
    sakura_uint32_t multiplier = 1;
    sakura_uint32_t remain_len = 0;

    do {
        /* check parameter */
        if (data != NULL && value != NULL) {
            do {
                if (idx == (sakura_uint32_t)MAX_NUM_OF_REMAINING_LENGTH_BYTES) {
                     /* illegal remaining length */
                    idx = -1;
                    break;
                }

                if (idx == (sakura_int32_t)len) {
                    /* too short data */
                    idx = -2;
                    break;
                }

                byte = data[idx];
                /* calculation length */
                remain_len += (byte & 127U) * multiplier;
                multiplier *= 128U;

                ++idx;
            } while ((byte & 128U) != 0U);
            *value = remain_len;
        }else {
            idx = -1;
            WORK_LOGE("input params is invalid!read failed!\n");
        }
    } while (SAKURA_FALSE);

    /* bytes contains the remain length */
    return idx;
}

static sakura_int32_t mqtt_handle_connack(mqtt_client_t *client, mqtt_header_t *header, const sakura_uint8_t *body, sakura_uint32_t body_len)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint8_t retcode = 0;
    // sakura_uint8_t session_present = 0U;
    (sakura_void_t)(header);
    do
    {
        /* parameter is invalid */
        if(client == NULL || body == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle connack failed!\n");   
            break;         
        }   
        /* check client state */
        if (client->state != MQTT_CLIENT_CONNECTING) {
            WORK_LOGE("not in connecting\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }
        /* check body length */
        if (body_len < 2U) {
            WORK_LOGE("too short message\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /*
        * byte1: session present flag
        * byte2: connect return code
        */
        // session_present = (body[0] & 0x01u); /* bits(1-7) are reserved */
        retcode = body[1];
        /* set return code */
        set_async_connack_ret_code(client, retcode);
        WORK_LOGD("[%02d|%s]: connect return code [%d]\n", client->index, client->client_id, retcode);

        if (retcode == 0U) {
            /* set client state */
            MQTT_NEXT_STATE(client, MQTT_CLIENT_CONNECTED);
            WORK_LOGI("[%02d|%s] becomes connected\n", client->index, client->client_id);
        }

        /*
        * just report connect return code to user
        * the session present flag is reserved for protocol only
        */
        /* index = 0 */
        // client->msg_tracker_array[0].ret_code = retcode;
        finish_message_state(&client->msg_tracker_array[0]);
        
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_handle_publish(mqtt_client_t *client, mqtt_header_t *header, sakura_uint8_t *body, sakura_uint32_t body_len)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t idx = -1;
    sakura_uint16_t message_id = 0;
    mqtt_topic_msg_t topic_msg = {0};
    mqtt_msg_t mqtt_msg = {0};
    do {
        /* parameter is invalid */
        if(client == NULL || header == NULL || body == NULL || header->bits.qos > (sakura_uint8_t)QOS2){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle publish failed!\n");
            break;
        }
        /* valid body length */
        if (body_len < 4U) {
            WORK_LOGE("too short message\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }
        /* for QOS0 */
        topic_msg.topic_ex    = (sakura_char_t *)body + 1;
        topic_msg.topic       = (sakura_char_t *)body + 2;
        /* topic length */
        topic_msg.topic_len   = ((sakura_uint32_t)body[0] << 8) + body[1];
        /* body length is mismatch */
        if (body_len < (topic_msg.topic_len + 4U)) {
            WORK_LOGE("invalid content length\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }
        /* update body */
        body        += 2U + topic_msg.topic_len;
        body_len    -= 2U + topic_msg.topic_len;

        /*
        * NOTICE:
        * If the QoS of the message coming from broker is QOS0, there is no message id before message content in payload.
        */
        if (header->bits.qos > (sakura_uint8_t)QOS0) {
            message_id  = ((sakura_uint16_t)body[0] << 8U) + body[1];
            body       += 2U;
            body_len   -= 2U;
        }

        mqtt_msg.msg        = body;
        mqtt_msg.msg_len    = body_len;
        mqtt_msg.message_id = message_id;

        /* dup is set, this is a relay messages again. */
        /* for the client, only QoS2 messages can be passed again. */
        if ((header->bits.dup != 0U) && header->bits.qos == (sakura_uint8_t)QOS2) {
            idx = get_matched_msg_state(client->msg_tracker_array, PUBREL, message_id);
        }
        /* handle the message */
        (sakura_void_t)mqtt_report_message(client, header, topic_msg, idx, mqtt_msg);
        /* response publish */
        ret = mqtt_response_publish(client, header, message_id, idx);
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_handle_simple_ack(mqtt_client_t *client, sakura_uint8_t msg_type, const sakura_uint8_t *body, sakura_uint32_t body_len)
{
    sakura_uint16_t message_id = 0;
    sakura_int32_t idx = 0;
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_msg_t msg = {0};
    do
    {
        /* parameter is invalid */
        if(client == NULL || body == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle simple ack failed!\n");   
            break;         
        }
        /* check body length */
        if (body_len < 2U) {
            WORK_LOGE("too short message\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }
        message_id      = ((sakura_uint16_t)(body[0]) << 8u) + body[1];
        msg.msg         = body;
        msg.msg_len     = body_len;
        msg.message_id  = message_id;
        /* get matched message state array */
        idx = get_matched_msg_state(client->msg_tracker_array, msg_type, message_id);

        /* handle ack message by message type */
        ret = mqtt_handle_ack_by_type(client, msg_type, msg, idx);

        /* okay, it's time to discard the backup message */
        if (MQTT_BACKUP_ENABLE(client) && (ret == 0) &&
            mqtt_check_tracker_index(idx) == 0 && ((client->msg_tracker_array[idx].redeliver_opt & 0xF0) != 0U)) {
            mqtt_remove_backup_message(client, idx);
            /* clear backup flag and reset the redelivery count */
            client->msg_tracker_array[idx].redeliver_opt = 0;
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_handle_pingresp(mqtt_client_t *client)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    /* check parameter */
    if (client != NULL) {
        ret = SAKURA_MQTT_STAT_OK;
        client->ping_outstanding = 0;
    }else {
        /* parameter is invalid */
        ret = SAKURA_MQTT_ERROR;
        WORK_LOGE("input client is NULL!handle pingresp failed!\n");
    }
    return ret;
}

static sakura_int32_t mqtt_redeliver_message(mqtt_client_t *client, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* invalid parameter */
        if(client == NULL){
            ret = SAKURA_MQTT_ERROR;
            WORK_LOGE("input client is NULL!redeliver message failed!\n");    
            break;        
        }

        /* update redelivery count */
        ++client->msg_tracker_array[idx].redeliver_opt;

        /* If the message has been totally sent or just waiting for redelivery, we redeliver it. */
        if (client->msg_tracker_array[idx].state == MSG_STATE_SENT || client->msg_tracker_array[idx].state == MSG_STATE_REDELIVER) {
            /*
            * set a FLAG first
            * The redelivery of the backup message may be failed because of the limited length of the send buffer.
            * Then, the state of the message would still be 'MSG_STATE_REDELIVER'. And we would try to redeliver
            * it later.
            */
            client->msg_tracker_array[idx].state = MSG_STATE_REDELIVER;
            ret = mqtt_do_redeliver(client, idx);
            break;
        }

    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_void_t mqtt_remove_send_message(mqtt_client_t *client, sakura_int32_t key, sakura_uint8_t state)
{
    /* initialize variable */
    sakura_int32_t idx = 0;
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint8_t *msg_head = NULL;
    sakura_uint32_t msg_size = 0U;
    sakura_uint8_t msg_state = 0U;
    sakura_uint8_t msg_type = 0U;
    sakura_uint32_t msg_len = 0;
    do
    {
        /* check params*/
        if(client == NULL){
            WORK_LOGE("input client is NULL!remove send message failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        msg_head    = client->net.send.buf;
        msg_size    = client->net.send.len;
        msg_state   = state;
        /* check send data length */
        while (msg_size > 0) {
            /* parse send package header */
            parse_send_packet_header(msg_head, &idx, &(msg_type), &(msg_len));
            /* whole packet length */
            msg_len += (sakura_uint8_t)MQTT_SEND_PKT_HEADER_LEN;
            /* if send buffer is messy, just return, DO NOT handle the problem here */
            ret = mqtt_check_buffer(msg_len, msg_size, msg_type, idx);
            if (ret < 0) {
                break;
            }
            /* check message index */
            if (key == idx) {
                /* remove message */
                if (ret >= 0) {
                    /* if waiting for the send buffer is messy, just return, do not handle the problem here */
                    if (msg_state == (sakura_uint8_t)MSG_STATE_WAITING) {
                        WORK_LOGD("remove send message, [%s:%d] from send buffer\n",
                            GET_MQTT_MSG_NAME(msg_type), client->msg_tracker_array[idx].message_id);
                        /* check data length */
                        if (msg_size - msg_len > 0) {
                            /* remove send data from send buffer */
                            memmove(msg_head, msg_head + msg_len, msg_size - msg_len);
                        }
                        client->net.send.len -= msg_len;
                    }else if (msg_state == (sakura_uint8_t)MSG_STATE_SENDING) {
                        /* just release the msg state item index */
                        msg_head[0] = MAX_MSG_STATE_ARRAY_LEN + 1;
                    }
                }
            }
            msg_head  += msg_len;
            msg_size -= msg_len;
        }
        
    } while (SAKURA_FALSE);
}

static sakura_void_t mqtt_remove_backup_message(mqtt_client_t *client, sakura_int32_t idx)
{
    /* initialize variable */
    sakura_uint8_t *msg = NULL;
    sakura_uint16_t msglen = 0;
    sakura_uint32_t len = 0;
    sakura_uint32_t msg_offset = 0;
    do
    {
        /* check params */
        if(client == NULL){
            WORK_LOGE("input client is NULL!remove backup message failed!\n");
            break;
        }

        /* get backup message */
        msg = mqtt_get_backup_message(client, idx, &msglen, &msg_offset);
        if (msg != NULL) {
            /* message is valid */
            // WORK_LOGD("client[%d] remove message[%s:%d] from backup buffer\n", client->index, GET_MQTT_MSG_NAME(msg[1]), client->msg_tracker_array[idx].message_id);
            len = client->net.backup.len - msg_offset - msglen;
            if (len > 0) {
                /* remove message from backup buffer */
                memmove(msg, msg + msglen, len);
            }
            client->net.backup.len -= msglen;
        }
    } while (SAKURA_FALSE);
}

static sakura_int32_t mqtt_send_backup_message(mqtt_client_t *client, sakura_int32_t idx)
{
    /* initialize variable */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint8_t *msg = NULL;
    sakura_uint8_t msgtype = 0;
    sakura_uint16_t msglen = 0;
    do {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!send backup message failed!\n");
            break;
        }

        /* get backup message */
        msg = mqtt_get_backup_message(client, idx, &msglen, NULL);
        if (msg == NULL) {
            WORK_LOGD("message with id [%d] not found in backup buffer\n", client->msg_tracker_array[idx].message_id);
            /*
            * If once the message is not found in backup buffer, we would never try redeliver it later.
            * (1) update the message state to 'MSG_STATE_SENT';
            * (2) clear the redelivery flags;
            * Then, treat it as timeout.
            */
            client->msg_tracker_array[idx].state = MSG_STATE_SENT;
            client->msg_tracker_array[idx].redeliver_opt = 0;
            ret = SAKURA_MQTT_STAT_OK;
            
            break;
        }

        /*
        * It is not a fatal error if no enough send buffer is available right now,
        * we would try it later.
        */
        if (MQTT_SEND_BUF_REST(client) < msglen) {
            WORK_LOGW("no enough send buffer\n");
            ret = SAKURA_MQTT_STAT_OK;
            
            break;
        }

        msgtype = msg[1] & 0x0fU; /* the second byte is message type */
        /*
        * According to the section 2.2.2 about Dup flag in MQTT V3.1.1 specification,
        * the Dup flag is only used for PUBLISH message when redelivery.
        */
        if (msgtype == (sakura_uint8_t)PUBLISH) {
            /* set DUP flag */
            msg[MQTT_SEND_PKT_HEADER_LEN] |= 0x08U; /* the first 4 bytes is header, and from the fifth byte is MQTT fixed header */
        }
        /* copy message to send buffer */
        memcpy(MQTT_SEND_BUF_HEAD(client), msg, msglen);
        client->net.send.len += msglen;
        /* set message state */
        client->msg_tracker_array[idx].state = MSG_STATE_WAITING;
        // WORK_LOGD("client[%d:%s] redeliver [%s:%d]\n", client->index, client->client_id, GET_MQTT_MSG_NAME(msgtype), client->msg_tracker_array[idx].message_id);
        ret = mqtt_try_send(client);
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t  mqtt_send_common_ack(mqtt_client_t *client, sakura_uint8_t msg_type, sakura_int8_t expect_type, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t len = 0;
    mqtt_msg_ident_t msg_ident = {0};
    sakura_uint8_t *sendbuf_head = NULL;
    do {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!send common ack failed!\n");
            break;
        }

        /* get send buffer */
        sendbuf_head = MQTT_SEND_BUF_HEAD(client);
        /* encode ack message */
        msg_ident.dup = 0;
        msg_ident.message_id = message_id;
        len = mqtt_encode_ack(MQTT_SEND_PKT_HEAD(client), MQTT_SEND_PKT_REST(client), msg_type, msg_ident);
        if (len <= 0) {
            WORK_LOGW("no enough send buffer to reply %s for message id [%d]\n", GET_MQTT_MSG_NAME(msg_type), message_id);
            /*
            * Here, we do not return error if no enough send buffer available.
            * It is too busy now, we would handle the re-delivery message later if possible.
            */
            ret = SAKURA_MQTT_STAT_OK;
            
            break;
        }
        client->net.send.len += (sakura_uint32_t)len + (sakura_uint32_t)MQTT_SEND_PKT_HEADER_LEN;
        /* assembly package header */
        assembly_send_packet_header(sendbuf_head, idx, msg_type, len);
        /* check message state */
        if (mqtt_check_tracker_index(idx) == 0) {
            client->msg_tracker_array[idx].state = MSG_STATE_WAITING;
            client->msg_tracker_array[idx].expect_type = expect_type;
        }
        
        /* try send message */
        ret = mqtt_try_send(client);
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief set item message tracker state to finish
 * 
 * @param item message tracker item
 * @return sakura_void_t 
 */
sakura_void_t finish_message_state(mqtt_msg_tracker_t *item)
{
    /* check parameter */
    if (item != NULL) {
        /* check callback is valid */
        if (item->callback != NULL) {
            item->state = (sakura_uint8_t)MSG_STATE_FINISH;
        }else {
            item->state = (sakura_uint8_t)MSG_STATE_FREE;
        }
    } else {
        WORK_LOGE("input item is invalid!\n");
    }
}

static sakura_int32_t get_matched_msg_state(mqtt_msg_tracker_t *msg_tracker_array, sakura_uint8_t msg_type, sakura_uint16_t message_id)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t loop = 0;
    do
    {
        /* check params */
        if(msg_tracker_array == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!\n");
            break;
        }
        /* find tracker by type */
        for (loop = 0; loop < MAX_MSG_STATE_ARRAY_LEN; ++loop) {
            /* matched */
            if (msg_tracker_array[loop].state != (sakura_uint8_t)MSG_STATE_FREE &&
                msg_tracker_array[loop].expect_type == (sakura_int16_t)msg_type &&
                msg_tracker_array[loop].message_id == message_id) {
                ret = loop;
                break;
            }
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_report_message(mqtt_client_t *client, mqtt_header_t *header, mqtt_topic_msg_t topic_msg, sakura_int32_t idx, mqtt_msg_t mqtt_msg)
{
    /* init */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;

    do {
        /* check parameter */
        if (client == NULL || header == NULL || topic_msg.topic == NULL \
            || topic_msg.topic_ex == NULL || idx != -1 || mqtt_msg.msg == NULL) {
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!report message failed!\n");
            break;
        }
        /*
        * Move ahead one byte first, so we could fill the terminal character '\0' when the QoS is QOS0;
        */
        if (header->bits.qos == (sakura_uint8_t)QOS0) {
            memmove(topic_msg.topic_ex, topic_msg.topic, topic_msg.topic_len);
            topic_msg.topic = topic_msg.topic_ex;
        }

        topic_msg.topic[topic_msg.topic_len] = '\0';

        /* deliver the message */
        if (client->cbs.cb_msg != NULL) {
            /* message report */
            client->cbs.cb_msg(client->index, topic_msg.topic, mqtt_msg.msg, mqtt_msg.msg_len);
        }
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_response_publish(mqtt_client_t *client, mqtt_header_t *header, sakura_uint16_t message_id, sakura_int32_t idx)
{
    /* init */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_msg_info_t mqtt_msg = {0};

    do {
        /* check parameter */
        if (client == NULL || header == NULL) {
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!response publish failed!\n");
            break;
        }
        if (header->bits.qos == (sakura_uint8_t)QOS2) {
            if (idx == -1) {
                /* idx is -1,this message does not come from the retransmission,get free message state */
                idx = get_free_message_state(client->msg_tracker_array);
            }
            if(mqtt_check_tracker_index(idx) != 0){
                ret = SAKURA_MQTT_ERROR;
            } else {
                /* set message info */
                mqtt_msg.backup         = 0;
                mqtt_msg.expect_type    = PUBREL;
                mqtt_msg.expired_tick   = client->tick + client->timeout_interval;
                mqtt_msg.message_id     = message_id;
                set_message_state(client->msg_tracker_array + idx, mqtt_msg, NULL);    
            }

            
            /* send pubrec msg */
            ret = mqtt_send_pubrec(client, message_id, idx);
        } else if (header->bits.qos == (sakura_uint8_t)QOS1)
        {
            /* send puback message */
            ret = mqtt_send_puback(client, message_id, -1);
        } else if (header->bits.qos == (sakura_uint8_t)QOS0){
            /* QoS 0, don't need to do anything */
        } else {
            WORK_LOGE("the QoS is invalid!\n");
        }
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_check_buffer(sakura_uint32_t len, sakura_uint32_t total, sakura_uint8_t msg_type, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    /* if send buffer is messy, just return, DO NOT handle the problem here */
    if (idx < -1 || idx >= MAX_MSG_STATE_ARRAY_LEN || (msg_type & 0xF0) != 0 || len > total) {
        ret = SAKURA_MQTT_ERROR;
    }
    return ret;
}

static sakura_uint8_t *mqtt_get_backup_message(mqtt_client_t *client, sakura_int32_t key, sakura_uint16_t *message_len, sakura_uint32_t *msg_offset)
{
    /* initialize variable */
    sakura_uint8_t msg_type = 0;
    sakura_uint8_t *head = NULL;
    sakura_uint8_t *ret_buf = NULL;
    sakura_int32_t idx = 0;
    sakura_uint32_t len = 0;
    sakura_uint32_t total = 0;
    do
    {
        /* check params */
        if(client == NULL || message_len == NULL){
            ret_buf = NULL;
            WORK_LOGE("invalid params!\n");
            break;
        }

        head = client->net.backup.buf;
        total = client->net.backup.len;
        /* check send length */
        while (total > 0) {
            /* parse package header */
            parse_send_packet_header(head, &idx, &msg_type, &len);
            /* add header length */
            len += (sakura_uint8_t)MQTT_SEND_PKT_HEADER_LEN;
            /* if the data in backup buffer is messy, clear it */
            if (mqtt_check_tracker_index(idx) == -1 || (msg_type & 0xf0) != 0 || len > total) {
                client->net.backup.len = 0U;
                break;
            }
            if (key == idx) {
                /* return the whole packet */
                *message_len = len;
                ret_buf = head;
                /* check break loop */
                msg_offset = NULL;
                break;
            }else {
                /* update head and total length */
                head  += len;
                total -= len;
            }
        }
        /* update message offset */
        if (NULL != msg_offset) {
            *msg_offset = head - client->net.backup.buf;
        }
    } while (SAKURA_FALSE);
    
    return ret_buf;
}

static sakura_int32_t mqtt_send_puback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!send puback failed!\n");
            break;
        }
        ret = mqtt_send_common_ack(client, PUBACK, -1, message_id, idx);
    } while (SAKURA_FALSE);
    
    return ret;
}

/*
 * @brief send pubrec message
 *
 * @param[in] client:       mqtt client
 * @param[in] message_id    message id
 * @param[in] idx           message state index
 * @return:
 *      -1: fatal error
 *       0: ok
 */
static sakura_int32_t mqtt_send_pubrec(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!send pubrec failed!\n");
            break;
        }
        ret = mqtt_send_common_ack(client, PUBREC, PUBREL, message_id, idx);
    } while (SAKURA_FALSE);
    
    return ret;
}

/*
 * @brief send pubrel message
 *
 * @param[in] client:       mqtt client
 * @param[in] message_id    message id
 * @param[in] idx           message state index
 * @return:
 *      -1: fatal error
 *       0: ok
 */
static sakura_int32_t mqtt_send_pubrel(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(client == NULL){
           ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!send pubrel failed!\n"); 
            break;
        }
        ret = mqtt_send_common_ack(client, PUBREL, PUBCOMP, message_id, idx);
    } while (SAKURA_FALSE);
    
    return ret;
}

/*
 * @brief send pubcomp message
 *
 * @param[in] client:       mqtt client
 * @param[in] message_id    message id
 * @param[in] idx           message state index
 * @return:
 *      -1: fatal error
 *       0: ok
 */
static sakura_int32_t mqtt_send_pubcomp(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!send pubcomp failed!\n");
            break;            
        }
        ret = mqtt_send_common_ack(client, PUBCOMP, -1, message_id, idx);
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_handle_ack_by_type(mqtt_client_t *client, sakura_uint8_t msg_type, mqtt_msg_t msg, sakura_int32_t idx)
{
    /* init */
    sakura_int32_t ret = SAKURA_MQTT_ERROR;

    do {
        /* check parameter */
        if (client == NULL || msg.msg == NULL || mqtt_check_tracker_index(idx) == -1) {
            // WORK_LOGE("params is invalid!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get matched message state array */
        switch (msg_type) {
            case PUBACK:
                /* handle puback message */
                ret = mqtt_handle_puback(client, msg.message_id, idx);
                break;
            case PUBREC:
                /* handle pubrec message */
                ret = mqtt_handle_pubrec(client, msg.message_id, idx);
                break;
            case PUBREL:
                /* handle pubrel message */
                ret = mqtt_handle_pubrel(client, msg.message_id, idx);
                break;
            case PUBCOMP:
                /* handle pubcomp message */
                ret = mqtt_handle_pubcomp(client, msg.message_id, idx);
                break;
            case SUBACK:
                /* handle suback message */
                ret = mqtt_handle_suback(client, msg.message_id, idx, msg.msg + 2U, msg.msg_len - 2U);
                break;
            case UNSUBACK:
                /* handle unsuback message */
                ret = mqtt_handle_unsuback(client, msg.message_id, idx);
                break;
            default:
                /* invalid message */
                ret = SAKURA_MQTT_ERROR;
                WORK_LOGD("not support message type\n");
                break;
        }
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_handle_puback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    (sakura_void_t)(message_id);
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle puback failed!\n");
            break;            
        }
        
        /* check message state */
        if (mqtt_check_tracker_index(idx) == 0) {
            finish_message_state(client->msg_tracker_array + idx);
        }
        
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_handle_pubrec(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle pubrec failed!\n");
            break;            
        }
        /* reply this message though no matched msg state item found */
        ret = mqtt_send_pubrel(client, message_id, idx);
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_handle_pubrel(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle pubrel failed!\n");
            break;            
        }

        /* check message state */
        if (mqtt_check_tracker_index(idx) == 0) {
            finish_message_state(client->msg_tracker_array + idx);
        }
        
        /*
        * if no matched msg state item found,
        * maybe the PUBREL message was re-transmission,
        * just send PUBCOMP
        */
        ret = mqtt_send_pubcomp(client, message_id, idx);
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_handle_pubcomp(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    (sakura_void_t)(message_id);
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle pubcomp failed!\n");
            break;
        }

        /* check message state */
        if (mqtt_check_tracker_index(idx) == 0) {
            finish_message_state(client->msg_tracker_array + idx);
        }
        
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_handle_suback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx, const sakura_uint8_t *payload, sakura_uint32_t payloadlen)
{
    /* initialize variable */
    (sakura_void_t)message_id;
    sakura_uint32_t loop = 0;
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_int32_t suback_code[MAX_TOPIC_NUM_OF_ONCE] = {0};
#ifndef CONFIG_LOG
    (sakura_void_t)message_id;
#endif
    do
    {
        /* check params */
        if (client == NULL || payload == NULL){
            /* parameter is invalid */
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!\n");
            break;
        }

        /* check index */
        if(mqtt_check_tracker_index(idx) == -1){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("tracker index(%d) is invalid!\n", idx);
            break;
        }

        for (loop = 0; loop < payloadlen && loop < (sakura_uint8_t)MAX_TOPIC_NUM_OF_ONCE; ++loop) {
            // bytes[loop] = payload[loop];
            set_async_suback_ret_code(&suback_code[loop], payload[loop]);
            if (payload[loop] == (sakura_uint8_t)SUBFAIL) {
                /* subscribe failure */
                WORK_LOGE("[%d|%s : %d]suback indicates failure\n", client->index, client->client_id, loop + 1U);
            }else if(payload[loop] > 2){
                WORK_LOGE("[%d|%s : %d]unable to identify the suback.\n", client->index, client->client_id, loop + 1U);
            }else {
                WORK_LOGD("[%d|%s : %d] subscribe success, QoS: %d\n", client->index, client->client_id, loop + 1U, payload[loop]);
            }
        }
        /* trigger subscribe callback,report suback_code */
        if(client->cbs.cb_subscribe != NULL){
            mqtt_topics_update(client, idx, payload);
            client->cbs.cb_subscribe(client->index, suback_code, payloadlen);
        }

        /* message tracker finish */
        finish_message_state(client->msg_tracker_array + idx);
        
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_handle_unsuback(mqtt_client_t *client, sakura_uint16_t message_id, sakura_int32_t idx)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    (sakura_void_t)(message_id);
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            WORK_LOGE("input params is invalid!handle unsuback failed!\n");
            break;            
        }

        /* tracker finish */
        if (mqtt_check_tracker_index(idx) == 0) {
            client->msg_tracker_array[idx].ret_code = SAKURA_MQTT_UNSUBSCRIBE_SUCCESS;
            finish_message_state(client->msg_tracker_array + idx);
        }
        
        ret = SAKURA_MQTT_STAT_OK;        
    } while (SAKURA_FALSE);

    return ret;
}


static mqtt_topic_list_t* check_node_no_repeat(const sakura_char_t *topic, list_head_t *head)
{
    mqtt_topic_list_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;

    do
    {
        /* check params */
        if(topic == NULL || head == NULL){
            WORK_LOGE("invalid params!\n");
            break;
        }

        /* no repeat */
        list_for_each_safe(pos, next, head) {
            node = list_entry(pos, mqtt_topic_list_t, list);
            if(strcmp(node->topic, topic) == 0){
                break;
            }
        }
    } while (SAKURA_FALSE);

    return node;
}

static sakura_void_t set_async_connack_ret_code(mqtt_client_t *client, sakura_uint8_t ret_code)
{
    do
    {
        /* check params */
        if(client == NULL){
            WORK_LOGE("invalid params!\n");
            break;
        }    

        /* set async return code */
        switch (ret_code){
            case 0x00:
                /* connect success */
                client->msg_tracker_array[0].ret_code = SAKURA_MQTT_CONNECT_ACCEPTED;
                break;
            case 0x01:
                /* connect failed, protocol is not support */
                client->msg_tracker_array[0].ret_code = SAKURA_MQTT_CONNECT_PROTOCOL_NONSUPPORT;
                break;
            case 0x02:
                /* connection has been refused, do unqualified client id */
                client->msg_tracker_array[0].ret_code = SAKURA_MQTT_CONNECT_INVALID_CLIENT_ID;
                break;
            case 0x03:
                /* connection has been refused, the server is unavailable */
                client->msg_tracker_array[0].ret_code = SAKURA_MQTT_CONNECT_INVALID_SERVER;
                break;
            case 0x04:
                /* connection has been refused, the account information is invalid */
                client->msg_tracker_array[0].ret_code = SAKURA_MQTT_CONNECT_INVALID_ACCOUNT;
                break;
            case 0x05:
                /* connection has been refused, unauthorized */
                client->msg_tracker_array[0].ret_code = SAKURA_MQTT_CONNECT_INVALID_UNAUTHORIZED;
                break;
            default:
                /* return code is unknown */
                WORK_LOGE("CONNACK return code is unknown!\n");
                client->msg_tracker_array[0].ret_code = SAKURA_MQTT_CONNECT_UNKNOWN_CONNACK;
            break;
        }        
    } while (SAKURA_FALSE);
}

static sakura_void_t set_async_suback_ret_code(sakura_int32_t *set_code, sakura_uint8_t ret_code)
{
    do
    {
        /* check params */
        if(set_code == NULL){
            WORK_LOGE("invalid params!\n");
            break;
        }    
        switch (ret_code){
            case 0x00:
                *set_code = SAKURA_MQTT_SUBACK_QOS0_SUCCESS;
                break;
            case 0x01:
                *set_code = SAKURA_MQTT_SUBACK_QOS1_SUCCESS;
                break;
            case 0x02:
                *set_code = SAKURA_MQTT_SUBACK_QOS2_SUCCESS;
                break;
            case 0x80:
                *set_code = SAKURA_MQTT_SUBACK_FAILURE;
                break;
            default:
                WORK_LOGE("SUBACK return code is unknown!\n");
                *set_code = SAKURA_MQTT_SUBACK_UNKNOWN_SUBACK;
                break;
        }        
    } while (SAKURA_FALSE);
}

/**
 * @brief create a new topic node
 * 
 * @param topic topic string
 * @return mqtt_topic_list_t 
 */
static mqtt_topic_list_t* mqtt_create_topic_node(const sakura_char_t* topic)
{
    /* new node */
    mqtt_topic_list_t* node = NULL;
    do
    {
        /* malloc */
        node = (mqtt_topic_list_t*)sakura_malloc(sizeof(mqtt_topic_list_t));
        if(node == NULL){
            WORK_LOGE("try sakura_malloc failed!\n");
            break;
        }        

        /* topic malloc */
        node->topic_len = strlen(topic) + 1;
        node->topic = (sakura_char_t*)sakura_malloc(node->topic_len);
        if(node->topic == NULL){
            WORK_LOGE("try sakura_malloc failed!\n");
            sakura_free(node);
            node = NULL;
            break; 
        }
        /* set value */
        memset(node->topic, 0, node->topic_len);   
        memcpy(node->topic, topic, strlen(topic));     
    } while (SAKURA_FALSE);
    
    return node;
}

/**
 * @brief Get the mqtt packet length object
 * 
 * @param buffer length bytes
 * @return sakura_uint32_t 
 */
static sakura_uint32_t get_mqtt_packet_length(const sakura_uint8_t* buffer) 
{
    sakura_uint32_t length = 0;
    sakura_uint32_t multiplier = 1;
    sakura_uint8_t encoded_byte;
    do
    {
        /* check params */
        if(buffer == NULL){
            WORK_LOGE("invalid params\n");
            break;
        }

        do {
            /* get a byte */
            encoded_byte = *buffer++;
            /* calculated length */
            length += (encoded_byte & 127) * multiplier;
            /* update multiplier */
            multiplier *= 128;
        } while ((encoded_byte & 128) != 0);        
    } while (SAKURA_FALSE);

    return length;
}

static sakura_void_t write_mqtt_remain_len(sakura_uint8_t **pptr, sakura_int32_t rem_len)
{
    sakura_uint32_t d = 0;
    sakura_uint32_t is_break = 0;
    sakura_uint32_t bytes = 0;
    do
    {
        /* check params */
        if (pptr == NULL || *pptr == NULL || rem_len < 0){
            WORK_LOGE("invalid params!\n");
            break;
        }

        /* max 4 bytes */
        for (bytes = 0; bytes <= 4;)
        {
            d = rem_len % 128;
            rem_len /= 128;
            
            /* remaining length is more than 1 byte, highest position of 1 */
            if (rem_len > 0){
                d |= 0X80;
            } else {
                is_break =1;
            }

            /* write byte */
            **pptr = d;
            ++(*pptr);
            bytes++;

            /* check break */
            if(is_break == 1){
                break;
            }
        }

        /* remaining is 4 bytes, need to fill in the remaining bytes */
        while (bytes != 4){
            **pptr = 0x00;
            ++(*pptr);
            bytes++;
        }
    } while (SAKURA_FALSE);
}