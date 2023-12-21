#include "sakura_mqtt.h"

#define     MQTT_MAX_REMAINING_LENGTH       268435455
#define     LOG_TAG                         "PACK"
#define     PACKET_LOGD                     SAKURA_LOGD
#define     PACKET_LOGE                     SAKURA_LOGE
#define     PACKET_LOGI                     SAKURA_LOGI
#define     PACKET_LOGW                     SAKURA_LOGW

/**
 * The static functions declare
 */
static sakura_int32_t mqtt_encode_zero(sakura_uint8_t* buf, sakura_int32_t buflen, sakura_uint8_t type);
static sakura_int32_t mqtt_calculate_packet_len(sakura_int32_t rem_len);
static sakura_int32_t mqtt_calculate_connect_len(mqtt_connect_data_t* conn_data);
static sakura_int32_t mqtt_calculate_publish_len(sakura_int32_t qos, const sakura_char_t *topic, sakura_int32_t payload_len);
static sakura_int32_t mqtt_calculate_subscribe_len(const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num);
static sakura_int32_t mqtt_calculate_unsubscribe_len(const sakura_char_t *unsub_topic_list[], sakura_int32_t topic_num);
static sakura_void_t write_char(sakura_uint8_t** pptr, sakura_uint8_t value);
static sakura_void_t write_short(sakura_uint8_t** pptr, sakura_int32_t value);
static sakura_void_t write_string(sakura_uint8_t **pptr, const sakura_char_t *str);
static sakura_void_t write_data(sakura_uint8_t **pptr, const sakura_uint8_t *data, sakura_int32_t len);
static sakura_void_t mqtt_encode_conn_msg_by_flags(sakura_uint8_t **ptr, mqtt_connect_data_t conn_data);
static sakura_void_t mqtt_encode_conn_msg_by_flags_sub_module(sakura_uint8_t **ptr, mqtt_connect_flags_t *flags, mqtt_connect_data_t conn_opt);


/**
 * @brief the publish data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @param pub_ident publish message identifier
 * @param topic topic
 * @param msg message
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_publish(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_msg_ident_t pub_ident, const sakura_char_t *topic, const mqtt_message_t *msg)
{
    sakura_uint32_t length_bytes = 0;
    sakura_int32_t rem_len = 0;
    sakura_int32_t total_len = 0;
    sakura_uint8_t *ptr = buf;
    mqtt_header_t header = {0};
    do {
        /* check params */
        if(buf == NULL || buflen < 0 || topic == NULL || msg == NULL){
            total_len = -1;
            PACKET_LOGE("input params is invalid!encode publish failed\n");
            break;
        }

        /* calculate the publish message length */
        rem_len = mqtt_calculate_publish_len(msg->qos, topic, msg->payloadlen);
        if (rem_len > MQTT_MAX_REMAINING_LENGTH || rem_len < 0) {
            total_len = -1;
            PACKET_LOGE("calculate the publish message length failed!\n");
            break;
        }

        /* calculate the total length of an MQTT packet */
        total_len = mqtt_calculate_packet_len(rem_len);
        if (total_len > buflen) {
            total_len = -1;
            PACKET_LOGE("calculate the total length of an MQTT packet failed!\n");
            break;
        }

        header.bits.type = PUBLISH;
        header.bits.dup = pub_ident.dup;
        header.bits.qos = msg->qos;
        header.bits.retain = msg->retained;

        /* header */
        write_char(&ptr, header.byte);

        /* remaining length */
        length_bytes = write_remaining_len(&ptr, rem_len);
        if(length_bytes == 0){
            PACKET_LOGE("write remain length failed! rem_len = %d, length_bytes = %d\n", rem_len, length_bytes);
            break;
        }

        /* topic */
        write_string(&ptr, topic);

        /* message id if need */
        if ((sakura_int8_t)(msg->qos) > 0) {
            write_short(&ptr, pub_ident.message_id);
        }

        /* payload */
        if (msg->payloadlen > 0) {
            write_data(&ptr, msg->payload, msg->payloadlen);
        }

        // PACKET_LOGD("publish message encode complete! topic = [%s], total_len = %d\n", topic, total_len);
    } while (SAKURA_FALSE);

    /* return total packet length */
    return total_len;
}

/**
 * @brief the disconnect data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_disconnect(sakura_uint8_t* buf, sakura_int32_t buflen)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(buf == NULL || buflen < 0){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            PACKET_LOGE("input params is invalid!encode disconnect failed\n");
            break;
        }

        /* encode a zero length packet */
        ret = mqtt_encode_zero(buf, buflen, DISCONNECT);
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief the connect data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @param conn_data connect data
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_connect(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_connect_data_t* conn_data)
{
    sakura_uint32_t length_bytes = 0;
    sakura_uint8_t *ptr = buf;
    sakura_int32_t remain_len = 0;
    sakura_int32_t total_len = 0;
    mqtt_header_t header = {0};
    do {
        /* check params */
        if(buf == NULL || buflen <= 0 || conn_data == NULL){
            total_len = -1;
            PACKET_LOGE("input params is invalid!encode connect failed\n");
        }

        /* calculate the connect message length */
        remain_len = mqtt_calculate_connect_len(conn_data);

        /* calculate the total length of an MQTT packet */
        total_len = mqtt_calculate_packet_len(remain_len);
        if (total_len > buflen) {
            total_len = -1;
            PACKET_LOGE("calculate the total length of an MQTT packet failed!\n");
            break;
        }
        header.byte = 0;
        header.bits.type = CONNECT;
        /* header */
        write_char(&ptr, header.byte);

        /* remaining length */
        length_bytes = write_remaining_len(&ptr, remain_len);
        if(length_bytes == 0){
            PACKET_LOGE("write remain length failed! rem_len = %d, length_bytes = %d\n", remain_len, length_bytes);
            break;
        }

        /* version of MQTT to be used.  3 = 3.1; 4 = 3.1.1 */
        if (conn_data->version == 4U) {
            /* protocol name */
            write_string(&ptr, MQTT_PROTOCOL_VERSION_3);
            /* protocol version number */
            write_char(&ptr, 4);
        } else {
            write_string(&ptr, MQTT_PROTOCOL_VERSION_4);
            write_char(&ptr, 3);
        }

        /* encode connect message by flags */
        mqtt_encode_conn_msg_by_flags(&ptr, *conn_data);

        // PACKET_LOGD("connect message encode complete! total_len = %d\n", total_len);
    } while (SAKURA_FALSE);

    /* return total packet length */
    return total_len;
}

/**
 * @brief the subscribe data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @param sub_ident subscribe message identifier
 * @param sub_topic_list the theme of the need to subscribe to the list
 * @param topic_num topic number
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_subscribe(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_msg_ident_t sub_ident, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num)
{
    sakura_uint32_t length_bytes = 0;
    sakura_int32_t rem_len = 0;
    sakura_int32_t total_len = 0;
    sakura_uint32_t loop = 0;
    sakura_uint8_t *ptr = buf;
    mqtt_header_t header = {0};
    do {
        /* check params */
        if(buf == NULL || buflen < 0 || sub_topic_list == NULL){
            total_len = -1;
            PACKET_LOGE("input params is invalid!encode subscribe failed\n");
            break;
        }

        /* calculate the subscribe message length */
        rem_len = mqtt_calculate_subscribe_len(sub_topic_list, topic_num);

        /* calculate the total length of an MQTT packet */
        total_len = mqtt_calculate_packet_len(rem_len);
        if (total_len > buflen) {
            total_len = -1;
            PACKET_LOGE("calculate the total length of an MQTT packet failed!\n");
            break;
        }

        header.byte = 0;
        header.bits.type = SUBSCRIBE;
        header.bits.dup = sub_ident.dup;
        header.bits.qos = 1;

        /* header */
        write_char(&ptr, header.byte);

        /* remaining length */
        length_bytes = write_remaining_len(&ptr, rem_len);
        if(length_bytes == 0){
            PACKET_LOGE("write remain length failed! rem_len = %d, length_bytes = %d\n", rem_len, length_bytes);
            break;
        }

        /* message id */
        write_short(&ptr, sub_ident.message_id);

        /* write topic info */
        for (loop = 0; loop < topic_num; ++loop) {
            if(sub_topic_list[loop].topic != NULL){
                write_string(&ptr, sub_topic_list[loop].topic);
                write_char(&ptr, sub_topic_list[loop].qos);   
            }
        }

        // PACKET_LOGD("subscribe message encode complete! total_len = %d\n", total_len);
    } while (SAKURA_FALSE);

    /* return total packet length */
    return total_len;
}

/**
 * @brief the unsubscribe data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @param unsub_ident unsubscribe message identifier
 * @param unsub_topic_list the theme of the need to unsubscribe to the list
 * @param topic_num topic number
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_unsubscribe(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_msg_ident_t unsub_ident, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num)
{
    sakura_uint32_t length_bytes = 0;
    sakura_int32_t rem_len = 0;
    sakura_int32_t total_len = 0;
    sakura_uint32_t loop = 0;
    sakura_uint8_t *ptr = buf;
    mqtt_header_t header = {0};
    do {
        /* check params */
        if(buf == NULL || buflen < 0 || unsub_topic_list == NULL || *unsub_topic_list == NULL){
            total_len = -1;
            PACKET_LOGE("input params is invalid!encode unsubscribe failed\n");
            break;
        }

        /* calculate the unsubscribe message length */
        rem_len = mqtt_calculate_unsubscribe_len(unsub_topic_list, topic_num);

        /* calculate the total length of an MQTT packet */
        total_len = mqtt_calculate_packet_len(rem_len);
        if (total_len > buflen) {
            total_len = -1;
            PACKET_LOGE("calculate the total length of an MQTT packet failed!\n");
            break;
        }

        header.byte = 0;
        header.bits.type = UNSUBSCRIBE;
        header.bits.dup = unsub_ident.dup;
        header.bits.qos = 1;

        /* header */
        write_char(&ptr, header.byte);

        /* remaining length */
        length_bytes = write_remaining_len(&ptr, rem_len);
        if(length_bytes == 0){
            PACKET_LOGE("write remain length failed! rem_len = %d, length_bytes = %d\n", rem_len, length_bytes);
            break;
        }

        /* message id */
        write_short(&ptr, unsub_ident.message_id);

        /* write topic */
        for (loop = 0; loop < topic_num; ++loop) {
            write_string(&ptr, unsub_topic_list[loop]);
        }

        // PACKET_LOGD("unsubscribe message encode complete! total_len = %d\n", total_len);
    } while (SAKURA_FALSE);
    
    /* return total packet length */
    return total_len;
}

/**
 * @brief the ack data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @param ack_ident ack message identifier
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_ack(sakura_uint8_t* buf, sakura_int32_t buflen, sakura_uint8_t type, mqtt_msg_ident_t ack_ident)
{
    sakura_uint32_t length_bytes = 0;
    sakura_int32_t ret_len = 0;
    sakura_uint8_t *ptr = buf;
    mqtt_header_t header = {0};
    do {
        /* check params */
        if(buf == NULL || buflen < 0){
            ret_len = -1;
            PACKET_LOGE("input params is invalid!encode ack failed\n");
            break;
        }

        /* check buffer length */
        if (buflen < 4) {
            ret_len = -1;
            break;
        }

        /* set packet header */
        header.byte = 0;
        header.bits.type = type;
        header.bits.dup = ack_ident.dup;
        header.bits.qos = (type == (sakura_uint8_t)PUBREL) ? 1U : 0;

        /* header */
        write_char(&ptr, header.byte);

        /* remaining length */
        length_bytes = write_remaining_len(&ptr, 2);
        if(length_bytes == 0){
            PACKET_LOGE("write remain length failed! rem_len = %d, length_bytes = %d\n", 2, length_bytes);
            break;
        }

        /* message id */
        write_short(&ptr, ack_ident.message_id);
        ret_len = 4;   /* fixed */

        // PACKET_LOGD("common ack message encode complete! total_len = %d\n", ret_len);
    } while (SAKURA_FALSE);

    /* return total packet length */
    return ret_len;
}

/**
 * @brief the pingreq data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_pingreq(sakura_uint8_t* buf, sakura_int32_t buflen)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        /* check params */
        if(buf == NULL || buflen < 0){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            PACKET_LOGE("input params is invalid!encode pingreq failed\n");
            break;            
        }

        ret = mqtt_encode_zero(buf, buflen, PINGREQ);
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_int32_t mqtt_encode_zero(sakura_uint8_t* buf, sakura_int32_t buflen, sakura_uint8_t type)
{
    sakura_uint32_t length_bytes = 0;
    sakura_int32_t ret_len = 0;
    mqtt_header_t header = {0};
    sakura_uint8_t *ptr = buf;
    do {
        if (buf == NULL || buflen < 2) {
            /* buf is invalid or buffer length is too short */
            ret_len = -1;
            PACKET_LOGE("input params is invalid!\n");
            break;
        }

        header.byte = 0;
        header.bits.type = type;

        /* header */
        write_char(&ptr, header.byte);

        /* remaining length */
        length_bytes = write_remaining_len(&ptr, 0);
        if(length_bytes == 0){
            PACKET_LOGE("write remain length failed! rem_len = %d, length_bytes = %d\n", 0, length_bytes);
            break;
        }

        ret_len = 2;
    } while (SAKURA_FALSE);

    return ret_len;
}

static sakura_int32_t mqtt_calculate_packet_len(sakura_int32_t rem_len)
{
    sakura_int32_t bytes = 0;

    /* now remaining_length field */
    if (rem_len < 128) {
        bytes += 1;
    } else if (rem_len < 16384) {
        bytes += 2;
    } else if (rem_len < 2097151) {
        bytes += 3;
    } else {
        bytes += 4;
    }

    return (1 /* fix header */ + bytes /* bytes for remaining length */ + rem_len /* remaining content */);
}

static sakura_int32_t mqtt_calculate_connect_len(mqtt_connect_data_t* conn_data)
{
    sakura_int32_t len = 0;
    do
    {
        /* check params */
        if(conn_data == NULL){
            PACKET_LOGE("connect data is invalid!\n");
            break;
        }

        /* variable depending on MQTT or MQIsdp */
        if (conn_data->version == 4U) {
            len = 10;
        } else {
            len = 12;
        }

        /* client id */
        if (conn_data->client_id != NULL) {
            len += (sakura_int32_t)strlen(conn_data->client_id) + 2;
        }

        /* will message */
        if (conn_data->will_flag != 0U && conn_data->will.topic != NULL && conn_data->will.payloadlen > 0) {
            len += (sakura_int32_t)strlen(conn_data->will.topic) + 2 + conn_data->will.payloadlen + 2;
        }

        /* username */
        if (conn_data->username != NULL) {
            len += (sakura_int32_t)strlen(conn_data->username) + 2;
        }

        /* password */
        if (conn_data->password != NULL) {
            len += (sakura_int32_t)strlen(conn_data->password) + 2;
        }
    } while (SAKURA_FALSE);
    
    return len;
}

static sakura_int32_t mqtt_calculate_publish_len(sakura_int32_t qos, const sakura_char_t *topic, sakura_int32_t payload_len)
{
    sakura_int32_t len = 0;
    do
    {
        /* check params */
        if(topic == NULL){
            len = -1;
            PACKET_LOGE("input topic is invalid!\n");
            break;
        }

        /* get packet length */
        len += 2 + (sakura_int32_t)strlen(topic) + payload_len;
        if (qos > 0) {
            /* packet id/message id */
            len += 2;
        }        
    } while (SAKURA_FALSE);

    return len;
}

static sakura_int32_t mqtt_calculate_subscribe_len(const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num)
{
    sakura_uint32_t loop = 0;
    sakura_int32_t len = 2; /* packet id length is 2 bytes */
    do
    {
        /* check params */
        if(sub_topic_list == NULL){
            len = 0;
            PACKET_LOGE("input subscribe topic list is invalid!\n");
            break;            
        }

        /* get packet length */
        for (loop = 0; loop < topic_num; ++loop) {
            if(sub_topic_list[loop].topic == NULL){
                continue;
            }
            len += 2 + (sakura_int32_t)strlen(sub_topic_list[loop].topic) + 1;    /* length + topic + req_qos */
        }
    } while (SAKURA_FALSE);

    return len;
}

static sakura_int32_t mqtt_calculate_unsubscribe_len(const sakura_char_t *unsub_topic_list[], sakura_int32_t topic_num)
{
    sakura_int32_t loop = 0;
    sakura_int32_t len = 2; /* packet id length is 2 bytes */
    do
    {
        /* check params */
        if(unsub_topic_list == NULL || *unsub_topic_list == NULL){
            len = 0;
            PACKET_LOGE("input unsubscribe topic list is invalid!\n");
            break;            
        }

        /* get packet length */
        for (loop = 0; loop < topic_num; ++loop) {
            if(unsub_topic_list[loop] == NULL){
                continue;
            }
            len += 2 + (sakura_int32_t)strlen(unsub_topic_list[loop]);    /* length + topic */
        }
    } while (SAKURA_FALSE);

    return len;
}

static sakura_void_t write_char(sakura_uint8_t** pptr, sakura_uint8_t value)
{
    do
    {
        /* check params */
        if(pptr == NULL || *pptr == NULL){
            PACKET_LOGE("invalid params!\n");
            break;
        }

        /* save input value */
        **pptr = value;
        ++(*pptr);        
    } while (SAKURA_FALSE);
}

static sakura_void_t write_short(sakura_uint8_t** pptr, sakura_int32_t value)
{
    do
    {
        /* check params */
        if(pptr == NULL || *pptr == NULL){
            PACKET_LOGE("invalid params!\n");
            break;
        }

        /* save input value */
        **pptr = ((sakura_uint32_t)value >> (sakura_uint8_t)8) & 0xffU;   /* MSB */
        ++(*pptr);
        **pptr = ((sakura_uint32_t)value & 0xffU);        /* LSB */
        ++(*pptr);        
    } while (SAKURA_FALSE);
}

static sakura_void_t write_string(sakura_uint8_t **pptr, const sakura_char_t *str)
{
    sakura_int32_t len = 0;
    sakura_char_t *buf = NULL;
    do
    {
        /* check params */
        if(pptr == NULL || *pptr == NULL || str == NULL){
            PACKET_LOGE("invalid params!\n");
            break;
        }

        len = strlen(str);
        /* save input string */
        write_short(pptr, len);
        buf = (sakura_char_t*)*pptr;
        memcpy(buf, str, len);
        *pptr += len;        
    } while (SAKURA_FALSE);
}

static sakura_void_t write_data(sakura_uint8_t **pptr, const sakura_uint8_t *data, sakura_int32_t len)
{
    do
    {
        /* check params */
        if(pptr == NULL || *pptr == NULL || data == NULL || len <= 0){
            PACKET_LOGE("invalid params!\n");
            break;
        }

        /* save input data */
        memcpy(*pptr, data, len);
        *pptr += len;        
    } while (SAKURA_FALSE);
}

sakura_int32_t write_remaining_len(sakura_uint8_t **pptr, sakura_int32_t rem_len)
{
    sakura_uint32_t d = 0;
    sakura_uint32_t is_break = 0;
    sakura_uint32_t bytes = 0;
    do
    {
        /* check params */
        if (pptr == NULL || *pptr == NULL || rem_len < 0){
            PACKET_LOGD("invalid params!\n");
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
    } while (SAKURA_FALSE);

    return bytes;
}

static sakura_void_t mqtt_encode_conn_msg_by_flags(sakura_uint8_t **ptr, mqtt_connect_data_t conn_data)
{
    mqtt_connect_flags_t flags = {0};
    do
    {
        /* check params */
        if(ptr == NULL){
            PACKET_LOGE("invalid params!\n");
            break;
        }

        /* encapsulate the flags */
        flags.all = 0U;
        /* clear the flags first */
        flags.bits.cleansession = conn_data.cleansession != 0U ? 1U : 0U;
        /* set will flag */
        flags.bits.will = conn_data.will_flag != 0U ? 1U : 0U;
        if (flags.bits.will != 0U) {
            flags.bits.will_qos = conn_data.will.qos;
            flags.bits.will_retain = conn_data.will.retained;
        }
        /* username */
        if (conn_data.username != NULL) {
            flags.bits.username = 1U;
        }
        /* password */
        if (conn_data.password != NULL) {
            flags.bits.password = 1U;
        }

        /* connect flags */
        write_char(ptr, flags.all);

        /* encode sub-module */
        mqtt_encode_conn_msg_by_flags_sub_module(ptr, &flags, conn_data);        
    } while (SAKURA_FALSE);
}

static sakura_void_t mqtt_encode_conn_msg_by_flags_sub_module(sakura_uint8_t **ptr, mqtt_connect_flags_t *flags, mqtt_connect_data_t conn_opt)
{
    do
    {
        /* check params */
        if(ptr == NULL){
            PACKET_LOGE("invalid params!\n");
            break;
        }

        /* keep alive timer */
        write_short(ptr, conn_opt.keepalive_interval);

        /* client identifier */
        write_string(ptr, conn_opt.client_id);

        if (flags->bits.will != 0U) {
            /* will topic */
            write_string(ptr, conn_opt.will.topic);
            /*
            * will message
            * In MQTT v3.1 the will message should be UTF-8 encoded string,
            * while in MQTT v3.1.1, it has no limitation on it.
            */
            write_short(ptr, conn_opt.will.payloadlen);
            write_data(ptr, conn_opt.will.payload, conn_opt.will.payloadlen);
        }

        if (flags->bits.username != 0U) {
            /* username */
            write_string(ptr, conn_opt.username);
        }

        if (flags->bits.password != 0U) {
            /* password */
            write_string(ptr, conn_opt.password);
        }        
    } while (SAKURA_FALSE);
}