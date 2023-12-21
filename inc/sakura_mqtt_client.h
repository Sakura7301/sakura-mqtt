/*
 * Copyright (C) 2021-2023 by SAKURA, All rights reserved.
 */

#ifndef __SAKURA_MQTT_CLIENT_H__
#define __SAKURA_MQTT_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* HEADER-BASIC-CLIENT-COPY-BEGIN */

#include "sakura_types.h"

/* status code */

/* error code */
#define SAKURA_MQTT_STAT_OK                            0
#define SAKURA_MQTT_ERROR                             -1
#define SAKURA_MQTT_ERR_MEMORY                        -2
#define SAKURA_MQTT_ERR_INVALID_ARGS                  -3
#define SAKURA_MQTT_ERR_BUSY                          -4
#define SAKURA_MQTT_ERR_REQ_IGN                       -5
#define SAKURA_MQTT_ERR_INITIALIZED                   -6

#define SAKURA_MQTT_NETWORK_ERROR                     -101    /* network error */
#define SAKURA_MQTT_ALL_CLIENTS_IDLE                   105    /* all client is idle */

#define SAKURA_MQTT_CONNECT_ACCEPTED                   201    /* connection accepted */
#define SAKURA_MQTT_CONNECT_DISCONNECTED               210    /* the connection has been disconnected. */
#define SAKURA_MQTT_CONNECT_PROTOCOL_NONSUPPORT       -201    /* connection has been refused, do not support the protocol version */
#define SAKURA_MQTT_CONNECT_INVALID_CLIENT_ID         -202    /* connection has been refused, do unqualified client id */
#define SAKURA_MQTT_CONNECT_INVALID_SERVER            -203    /* connection has been refused, the server is unavailable */
#define SAKURA_MQTT_CONNECT_INVALID_ACCOUNT           -204    /* connection has been refused, the account information is invalid */
#define SAKURA_MQTT_CONNECT_INVALID_UNAUTHORIZED      -205    /* connection has been refused, unauthorized */
#define SAKURA_MQTT_CONNECT_UNKNOWN_CONNACK           -255    /* connection has been refused, return code is unknown */

#define SAKURA_MQTT_SUBACK_QOS0_SUCCESS                300    /* subscription succeeded (QOS0) */
#define SAKURA_MQTT_SUBACK_QOS1_SUCCESS                301    /* subscription succeeded (QOS1) */
#define SAKURA_MQTT_SUBACK_QOS2_SUCCESS                302    /* subscription succeeded (QOS2) */
#define SAKURA_MQTT_SUBACK_FAILURE                    -301    /* subscription failed */
#define SAKURA_MQTT_SUB_UNCONNECTED                   -303    /* subscription failed, connection not established. */
#define SAKURA_MQTT_SUBACK_UNKNOWN_SUBACK             -355    /* unknown suback return code. */

#define SAKURA_MQTT_UNSUBSCRIBE_SUCCESS                401    /* unsubscribe success */           

#define SAKURA_MQTT_SDK_VERSION                       "1.0.0" /* sakura mqtt-sdk version */

/* mqtt callback */
typedef sakura_void_t (*mqtt_async_callback_t)(sakura_int32_t index, sakura_int32_t status_code);
typedef sakura_void_t (*mqtt_async_sub_callback_t)(sakura_int32_t index, const sakura_int32_t *suback_code, sakura_uint32_t code_num);
typedef sakura_void_t (*mqtt_message_handler_t)(sakura_int32_t index, const sakura_char_t *topic, const sakura_uint8_t *message, sakura_uint32_t message_len);

/* set client option */
typedef enum
{
    SAKURA_MQTT_SET_WILL = 0x00,                              /* set will */
    SAKURA_MQTT_SET_KEEPALIVE,                                /* set keepalive interval */
    SAKURA_MQTT_SET_CLEANSESSION,                             /* set cleansession */
    SAKURA_MQTT_SET_TIMEOUT,                                  /* set timeout */
}SAKURA_MQTT_OPT;

/* message level */
typedef enum
{
    QOS0 = 0x00,                                            /* Qos 0 level(at most once) */
    QOS1,                                                   /* Qos 1 level(at least once) */
    QOS2,                                                   /* Qos 2 level(for once) */
    SUBFAIL=0x80                                            /* subscribe failed! */
} QoS;

/* account info */
typedef struct
{
    sakura_sock_host_t                    *broker;            /* broker(hostname,port) */
    sakura_char_t                         *username;          /* username */
    sakura_char_t                         *password;          /* password */
} sakura_mqtt_account_info_t;

/* MQTT subscribe request */
typedef struct
{
    QoS                                 qos;                /* message qos */
    const sakura_char_t                   *topic;             /* topic string */
} sakura_mqtt_topic_t;

/* will message content */
typedef struct
{
    QoS                                 qos;                /* qos level */
    sakura_uint8_t                        retained;           /* publish retain flag */
    sakura_char_t                         *topic;             /* will topic */
    sakura_uint8_t                        *payload;           /* payload */
    sakura_uint32_t                       payloadlen;         /* payload len */
} mqtt_will_t;

/* mqtt callback handle */
typedef struct
{
    mqtt_async_callback_t               cb_connect;         /* connect callback */
    mqtt_async_callback_t               cb_disconnect;      /* disconnect callback */
    mqtt_async_sub_callback_t           cb_subscribe;       /* subscribe callback */
    mqtt_async_callback_t               cb_unsubscribe;     /* unsubscribe callback */
    mqtt_async_callback_t               cb_status;          /* status callback */
    mqtt_message_handler_t              cb_msg;             /* message callback */
} mqtt_cbs_t;

/* net handle */
typedef struct
{
    sakura_uint8_t                        *buf;               /* buffer */
    sakura_uint32_t                       len;                /* effective length */
    sakura_uint32_t                       size;               /* total size */
} mqtt_net_data_t;

/* send handle */
typedef struct
{
    sakura_uint8_t                        *buf;               /* buffer */
    sakura_uint32_t                       read_pos;           /* read pos */
    sakura_uint32_t                       write_pos;          /* write pos */
    sakura_uint32_t                       size;               /* total size */
} mqtt_send_handle_t;

/* message handle */
typedef struct
{
    QoS                                 qos;                /* qos level */
    sakura_uint8_t                        retained;           /* message retain flag */
    sakura_uint8_t                        dup;                /* message dup flag */
    sakura_uint16_t                       id;                 /* message id */
    sakura_uint8_t                        *payload;           /* payload */
    sakura_uint32_t                       payloadlen;         /* payload len */
} mqtt_message_t;

#ifdef CONFIG_LOG
/**
 * @brief set the log properties.
 * Call time:initialized before.
 * 
 * @param conf 
 * @return sakura_int32_t 
 */
sakura_int32_t sakura_mqtt_set_log_config(sakura_sdk_log_conf_t* conf);
#endif

/**
 * @brief init the 'c_max_num' clients
 * 
 * @param c_max_num client max num
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_init(sakura_uint32_t c_max_num);

/**
 * @brief set net buf size in client
 * Call time:After the initialization, open the client before.
 * 
 * @param size buffer sie
 * @return    0: ok
 *           <0: error 
 */
sakura_int32_t sakura_mqtt_set_net_buf_size(sakura_uint32_t size);

/**
 * @brief get mqtt sdk version
 * Call time:any event
 * 
 * @return   version string
 */
const sakura_char_t* sakura_mqtt_get_version(sakura_void_t);

/**
 * @brief query mqtt client properties
 * Call time:After the initialization
 * 
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_query_properties(sakura_void_t);

/**
 * @brief open a mqtt client, return index
 * Call time:After the initialization
 * 
 * @param client_id client id
 * @return >=0: success, return client index
 *          <0: failed
 */
sakura_int32_t sakura_mqtt_open(const sakura_char_t *client_id);

/**
 * @brief close client by index
 * Call time:After open the client
 * 
 * @param index client index
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_close(sakura_int32_t index);

/**
 * @brief set config by index
 *        calls need to be init after initialization and before connecting.
 * Call time:After open the client
 * 
 * @param index client index
 * @param opt option
 * @param arg arg
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_set_option(sakura_int32_t index, SAKURA_MQTT_OPT opt, sakura_void_t *arg);

#ifndef CONFIG_SYS_UNIX
/**
 * @brief set async tick_rate
 * Call time:After the initialization
 * 
 * @param tick_rate 
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_set_async_tick_rate(sakura_uint32_t tick_rate);

/**
 * @brief drive the MQTT life cycle
 *        This interface is allowed only if the platform is embedded.
 * Call time:After the initialization
 * 
 * @param tick tick
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_tick(sakura_uint64_t tick);
#endif

/**
 * @brief mqtt connect, the results will be sent to you by cb_connect notifies the user. 
 * Call time:After the initialization
 * 
 * @param index client index
 * @param account_info broker id, username, password
 * @param cbs mqtt callbacks
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_connect(sakura_int32_t index, const sakura_mqtt_account_info_t *account_info, const mqtt_cbs_t *cbs);

/**
 * @brief mqtt disconnect, the results will be sent to you by cb_disconnect notifies the user. 
 * Call time:After open the client
 * 
 * @param index client index
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_disconnect(sakura_int32_t index);


/**
 * @brief mqtt subscribe, the results will be sent to you by cb_subscribe notifies the user. 
 * Call time:After the connect
 * 
 * @param index client index
 * @param sub_topic_list subscribe topic list
 * @param topic_num topic number
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_subscribe(sakura_int32_t index, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num);

/**
 * @brief mqtt unsubscribe, the results will be sent to you by cb_unsubscribe notifies the user. 
 * Call time:After the connect
 * 
 * @param index client index
 * @param unsub_topic_list unsubscribe topic list
 * @param topic_num topic number
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_unsubscribe(sakura_int32_t index, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num);

/**
 * @brief mqtt publish message, If it fails, will advise the caller by cb_status interface.
 * Call time:After the connect
 * 
 * @param index client index
 * @param topic topic string
 * @param message message
 * @param pub_cb publish callback, after successful release trigger(can be NULL)
 * @return    0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_publish(sakura_int32_t index, const sakura_char_t *topic, const mqtt_message_t *message, mqtt_async_callback_t pub_cb);


/**
 * @brief release all resources.
 * Call time:After the initialization
 * 
 * @return   sakura_void_t
 */
sakura_void_t sakura_mqtt_cleanup(sakura_void_t);

/* HEADER-BASIC-CLIENT-COPY-END */

#ifdef __cplusplus
}
#endif

#endif