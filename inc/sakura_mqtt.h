/*
 * Copyright (C) 2021-2023 by SAKURA, All rights reserved.
 */
#ifndef __SAKURA_MQTT_H__
#define __SAKURA_MQTT_H__

#include "sakura_types.h"
#include "sakura_mqtt_client.h"
#include "sakura_mem.h"
#include "sakura_mutex.h"
#include "sakura_log.h"
#include "sakura_socket.h"
#include "sakura_list.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern sakura_char_t *mqtt_message_name[16];
extern sakura_char_t *mqtt_client_state[8];

/* macro */
#define MAX_MSG_STATE_ARRAY_LEN                     24      /* should be no more than 0x80, one byte(signed) for the array index */
#define MAX_MESSAGE_ID                              65535   /* according to the MQTT specification - do not change! */
#define MAX_TOPIC_NUM_OF_ONCE                       8       /* the maximum number of once subscribe topics */
#define MAX_CLIENT_ID_STR_LEN                       64      /* the maximum length of a client id */
#define MAX_BROKER_ID_STR_LEN                       64      /* the maximum length of a broker id */
#define DEFAULT_MQTT_TIMEOUT_INTERVAL               15      /* default timeout interval */
#define DEFAULT_MQTT_KEEPALIVE_INTERVAL             15      /* default keepalive interval, max value is 65535 second */
#define DEFAULT_MQTT_RECONNECT_INTERVAL             3       /* default is to reconnect every one seconds. */
#define DEFAULT_MQTT_CLEANSESSION                   1       /* default clean session */
#define DEFAULT_MQTT_WILL_FLAG                      0       /* default not use will */
#define DEFAULT_EMBED_TICK_RATE                     10      /* the number of ticks in a second. */
#define DEFAULT_MAX_CLIENTS                         8       /* default max client num */
#define DEFAULT_NET_BUF_SIZE                        10240   /* default net buffer size is 10k */
#define DEFAULT_ACCOUNT_LEN                         64      /* default username/password length */
#define DEFAULT_DOT_NAME_LEN                        256     /* default dot main length */
#define MQTT_PROTOCOL_VERSION_3                     "MQTT"  /* MQTT protocol 3.1 */
#define MQTT_PROTOCOL_VERSION_4                     "MQIsdp"/* MQTT protocol 3.1.1 */
#define MQTT_SEND_PKT_HEADER_LEN                    6       /* MQTT packet header len */
#define MQTT_MAX_CLIENTS_NUM                        32      /* max clients number is 32 */
#define MQTT_PROPERTIES_BUF_SIZE                    2048    /* properties buffer size */
#define MQTT_SDK_COMMIT_LEN                         128     /* mqtt sdk commit id length */
#define MQTT_MAX_SUB_NUM                            8       /* every time the most subscribe to 8 topics at the same time. */

/* MQTT parse packet macro */
#define MQTT_SEND_BUF_HEAD(client)                  ((client)->net.send.buf + (client)->net.send.len)
#define MQTT_SEND_BUF_REST(client)                  ((client)->net.send.size - (client)->net.send.len)
#define MQTT_SEND_PKT_HEAD(client)                  (MQTT_SEND_BUF_HEAD(client) + (sakura_uint8_t)MQTT_SEND_PKT_HEADER_LEN)
#define MQTT_SEND_PKT_REST(client)                  (MQTT_SEND_BUF_REST(client) - (sakura_uint8_t)MQTT_SEND_PKT_HEADER_LEN)
#define MQTT_RECV_BUF_HEAD(client)                  ((client)->net.recv.buf + (client)->net.recv.len)
#define MQTT_RECV_BUF_REST(client)                  ((client)->net.recv.size - (client)->net.recv.len)
#define MQTT_BACKUP_BUF_HEAD(client)                ((client)->net.backup.buf + (client)->net.backup.len)
#define MQTT_BACKUP_BUF_REST(client)                ((client)->net.backup.size - (client)->net.backup.len)
#define MQTT_BACKUP_ENABLE(client)                  ((client)->net.backup.buf != NULL && (client)->net.backup.size != 0U)
#define MQTT_WILL_DATA_INITIALIZER                  { QOS0, 0, NULL, NULL, 0 }
#define MQTT_CONNECT_OPTION_INITIALIZER             { 4, NULL, DEFAULT_MQTT_KEEPALIVE_INTERVAL, 1, 0, MQTT_WILL_DATA_INITIALIZER, NULL, NULL }

/* The standard MQTT message type */
typedef enum
{
    Reserved = 0x00,
    CONNECT,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT
} MQTT_MSG_TYPE;

/* track client state */
typedef enum
{
/**
 *  Reconnection is described here:
 * 
 *  1.After network error, save the topic list, 
 *  reset the status of the client, and then switch the status to `MQTT_CLIENT_NOT_CONNECTED`.
 *  2.When the status of the client is "MQTT_CLIENT_NOT_CONNECTED", we will reconnect the client.
 *  3.State is MQTT_CLIENT_CONNECTED, at this time, you need to subscribe to the topic.
 *  (Each subscription or unsubscription of the client will update the subscription list, 
 *  and when the client needs to reconnect, it will re-subscribe the contents in the list.)
 */
    MQTT_CLIENT_FREE = 0x00,                                /* client free, unused. */
    MQTT_CLIENT_NOT_CONNECTED,                              /* client client not connect, has been initialized, waiting for the connection. */
    MQTT_CLIENT_TCP_CONNECTING,                             /* client tcp connecting. */
    MQTT_CLIENT_TCP_CONNECTED,                              /* client tcp connected. */
    MQTT_CLIENT_CONNECTING,                                 /* client MQTT connecting */
    MQTT_CLIENT_CONNECTED,                                  /* client MQTT connected, */
    MQTT_CLIENT_DISCONNECTING,                              /* client MQTT disconnecting */
    MQTT_CLIENT_CONNECT_ERROR = 0x07,                       /* client connect error, the client need to reset. */
} MQTT_CLIENT_STATE;



/* topic state */
typedef enum
{
    MQTT_TOPIC_STATE_TO_SUBSCRIBE,                          /* now, the topic is to subscribe. */
    MQTT_TOPIC_STATE_SUBSCRIBING,                           /* the topic is being subscribe. */
    MQTT_TOPIC_STATE_SUBSCRIBED,                            /* the topic is already subscribed. */
}TOPIC_STATE;

/* mqtt connect data */
typedef struct
{
    sakura_uint8_t                        version;             /* version of MQTT to be used.  3 = 3.1, 4 = 3.1.1 */
    sakura_char_t                         *client_id;          /* client id */
    sakura_uint16_t                       keepalive_interval;  /* DEFAULT_MQTT_KEEPALIVE_INTERVAL */
    sakura_uint8_t                        cleansession;        /* clean session flag */
    sakura_uint8_t                        will_flag;           /* will flag */
    mqtt_will_t                         will;                /* will data */
    sakura_char_t                         *username;           /* user name */
    sakura_char_t                         *password;           /* user password */
} mqtt_connect_data_t;

/* track each message state */
typedef enum
{
    MSG_STATE_FREE = 0x00,                                  /* message free, can be used */
    MSG_STATE_FINISH,                                       /* message finish, end of the message of life cycle */
    MSG_STATE_REDELIVER,                                    /* message is the retransmission */
    MSG_STATE_WAITING,                                     /* message in buffer, waiting send */
    MSG_STATE_SENDING,                                      /* message sending */
    MSG_STATE_SENT,                                         /* message sent */
} MSG_STATE;

/**
 * message tracker
 * It is designed for recording the entire messages of a request between client and broker. 
*/
typedef struct
{
    MSG_STATE                           state;              /* message state */
    sakura_int8_t                         message_type;       /* message type */
    sakura_int8_t                         expect_type;        /* the event expected from broker: PUBACK/SUBACK/.../-1: no expect event */
    sakura_uint8_t                        redeliver_opt;      /* message re-delivery options: first 4 bits is re-delivery count, next 4 bits is whether re-deliver the message */
    sakura_uint16_t                       message_id;         /* the message id of the sent message */
    sakura_uint32_t                       expired_tick;       /* the message expired_tick(expired tick ==> client->tick + client->timeout) */
    sakura_int32_t                        ret_code;           /* carry a message return code  */
    mqtt_async_callback_t               callback;           /* report message state */
} mqtt_msg_tracker_t;

/* message info */
typedef struct 
{
    sakura_uint8_t                        backup;             /* backup flag, PUBLISH(1/2),SUB,UNSUB */
    MQTT_MSG_TYPE                       message_type;       /* message type */
    MQTT_MSG_TYPE                       expect_type;        /* the event expected from broker: PUBACK/SUBACK/.../-1: no expect event */
    sakura_uint16_t                       message_id;         /* message id */
    sakura_uint32_t                       expired_tick;       /* the message expired_tick(expired tick ==> client->tick + client->timeout) */
} mqtt_msg_info_t;

/* mqtt message identifier */
typedef struct 
{
    sakura_uint8_t                        dup;                /* dup flag */
    sakura_uint16_t                       message_id;         /* message id */
}mqtt_msg_ident_t;

/* mqtt topic message */
typedef struct 
{
    sakura_char_t                         *topic;
    sakura_char_t                         *topic_ex;
    sakura_uint32_t                       topic_len;
}mqtt_topic_msg_t;

/* mqtt message structure */
typedef struct 
{
    const sakura_uint8_t                  *msg;               /* message body */
    sakura_uint32_t                       msg_len;            /* message length */
    sakura_uint16_t                       message_id;         /* message id */
}mqtt_msg_t;

/* mqtt message remove */
typedef struct
{
    sakura_uint8_t                        *head;              /* message head pointer */
    sakura_uint8_t                        state;              /* message state */
    sakura_uint8_t                        msg_type;           /* message type */
    sakura_uint32_t                       len;                /* message length */
    sakura_uint32_t                       size;               /* message size */
}mqtt_remove_msg_t;

/* MQTT header */
typedef union 
{
    sakura_uint8_t                        byte;               /* all header byte */
    struct {
        sakura_uint32_t                   retain : 1;         /* retain bit */
        sakura_uint32_t                   qos : 2;            /* qos bit */
        sakura_uint32_t                   dup : 1;            /* dup bit */
        sakura_uint32_t                   type : 4;           /* type bit */
    } bits;
} mqtt_header_t;

/* CONNECT message flags */
typedef union 
{
    sakura_uint8_t                        all;                /* all connect flags */
    struct {
        sakura_uint32_t                   : 1;                /* resevered, not used, it must be 0 */
        sakura_uint32_t                   cleansession : 1;   /* cleansession */
        sakura_uint32_t                   will : 1;           /* will */
        sakura_uint32_t                   will_qos : 2;       /* will qos */
        sakura_uint32_t                   will_retain : 1;    /* will retain */
        sakura_uint32_t                   password : 1;       /* password */
        sakura_uint32_t                   username : 1;       /* username */
    } bits;
} mqtt_connect_flags_t;


/* mqtt net handle */
typedef struct
{
    sakura_uint8_t                        send_state;         /* idle / sending / sent */
    sakura_int32_t                        sock;               /* socket */
    mqtt_net_data_t                     send;               /* send handle */
    mqtt_net_data_t                     recv;               /* recv handle */
    mqtt_net_data_t                     backup;             /* backup handle */
} mqtt_net_t;

/* topic list */
typedef struct{
    struct list_head                    list;               /* list head */
    QoS                                 qos;                /* message qos */
    sakura_char_t                         *topic;             /* topic string */
    sakura_uint32_t                       topic_len;          /* topic length */
    TOPIC_STATE                         state;              /* topic state */
    sakura_uint16_t                       tracker_idx;        /* the message tracker index */
}mqtt_topic_list_t;

/* user account information */
typedef struct{
    sakura_char_t                         hostname[DEFAULT_DOT_NAME_LEN];         /* hostname */
    sakura_uint16_t                       port;                                   /* port */
    sakura_char_t                         username[DEFAULT_ACCOUNT_LEN];          /* username */
    sakura_char_t                         password[DEFAULT_ACCOUNT_LEN];          /* password */
}mqtt_account_t;

/* user account information */
typedef struct{
    sakura_uint64_t                       reConnectTimer;     /* hostname */
    sakura_uint32_t                       reConnectCount;     /* port */
}mqtt_reconnect_t;


/**
 * MQTT client
 * This structure users not to open, for internal use only.
 * When the user obtains a client ID through open, the client will be initialized as the default attribute.
 * If you want to change, so need through ` sakura_mqtt_set_option ` interface to set up.
 */
typedef struct
{
    sakura_uint32_t                       index;                                          /* client index */
    MQTT_CLIENT_STATE                   state;                                          /* tracking client state  */
    sakura_uint8_t                        ping_outstanding;                               /* ping outstanding  */
    sakura_uint8_t                        cleansession;                                   /* cleansession flag */
    sakura_uint8_t                        willFlag;                                       /* will flag */
    sakura_uint16_t                       message_id;                                     /* message id */
    mqtt_will_t                         *will;                                          /* will */
    sakura_uint64_t                       tick;                                           /* client tick */
    sakura_uint64_t                       last_keepalive_tick;                            /* last keepalive tick */
    sakura_uint32_t                       timeout_interval;                               /* timeout interval */
    sakura_uint32_t                       keepalive_interval;                             /* keepalive interval */
    sakura_uint16_t                       reconnect_flag;                                 /* reconnect flag */
    sakura_mutex_t                        mutex_lock;                                     /* client mutex */
    mqtt_net_t                          net;                                            /* net sub module */
    mqtt_cbs_t                          cbs;                                            /* callbacks */
    list_head_t                         topic_list;                                     /* topic list */
    mqtt_account_t                      *account_info;                                  /* account information */
    sakura_uint32_t                       wait_sub_topic_num;                             /* waiting subscribe topic number */
    sakura_char_t                         client_id[MAX_CLIENT_ID_STR_LEN];               /* client id */
    sakura_char_t                         broker_id[MAX_BROKER_ID_STR_LEN];               /* broker id */
    mqtt_msg_tracker_t                  msg_tracker_array[MAX_MSG_STATE_ARRAY_LEN];     /* message tracker array */
} mqtt_client_t;

/**
 * @brief drive a client life cycle
 * 
 * @param index client index
 * @param tick tick
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t sakura_mqtt_client_tick(sakura_int32_t index, sakura_uint64_t tick);

/**
 * @brief according to an index lookup and returned to the client
 * 
 * @param index client id
 * @return   client: success
 *             null: failed
 */
mqtt_client_t* mqtt_get_client_by_index(sakura_int32_t index);

/**
 * @brief write remaining len
 * 
 * @param pptr remain bytes
 * @param rem_len remain length
 * @return sakura_int32_t 
 */
sakura_int32_t write_remaining_len(sakura_uint8_t **pptr, sakura_int32_t rem_len);

/**
 * @brief assembly sent package in header
 * 
 * @param header header
 * @param index client index
 * @param msg_type message type
 * @param len buffer length
 */
sakura_void_t assembly_send_packet_header(sakura_uint8_t *header, sakura_int32_t index, sakura_uint8_t msg_type, sakura_int32_t len);

/**
 * @brief assembly sent package in header
 * 
 * @param header header
 * @param index client index
 * @param msg_type message type
 * @param len buffer length
 */
sakura_void_t parse_send_packet_header(sakura_uint8_t *header, sakura_int32_t *index, sakura_uint8_t *msg_type, sakura_uint32_t *len);

/**
 * @brief drive the MQTT client life cycle
 * 
 * @param client MQTT client
 * @param tick tick
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_tick(mqtt_client_t *client, sakura_uint64_t tick);

/**
 * @brief ready the MQTT CONNECT packet
 * 
 * @param client MQTT client
 * @param account_info account information
 * @param cb connect callback
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_connect(mqtt_client_t *client, const sakura_mqtt_account_info_t *account_info, mqtt_async_callback_t cb);

/**
 * @brief ready the MQTT DISCONNECT packet
 * 
 * @param client MQTT client
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_disconnect(mqtt_client_t *client);

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
sakura_int32_t mqtt_client_publish(mqtt_client_t *client, const sakura_char_t *topic, const mqtt_message_t *message, mqtt_async_callback_t pub_cb);

/**
 * @brief ready the MQTT SUBSCRIBE packet
 * 
 * @param client MQTT client
 * @param sub_topic_list list of topics to be subscribed
 * @param topic_num topics num
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_subscribe(mqtt_client_t *client, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num);

/**
 * @brief ready the MQTT UNSUBSCRIBE packet
 * 
 * @param client MQTT client
 * @param unsub_topic_list list of topics to be unsubscribed
 * @param topic_num topics num
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t mqtt_client_unsubscribe(mqtt_client_t *client, const sakura_char_t* unsub_topic_list[], sakura_uint32_t topic_num);

/**
 * @brief clear message tracker array, reset the state for MSG_STATE_FREE
 * 
 * @param msg_tracker_array message track array
 * @return sakura_void_t 
 */
sakura_void_t clear_message_tracker_array(mqtt_msg_tracker_t *msg_tracker_array);

/**
 * @brief set item message tracker state to finish
 * 
 * @param item message tracker item
 * @return sakura_void_t 
 */
sakura_void_t finish_message_state(mqtt_msg_tracker_t *item);

/**
 * @brief check the legitimacy of the index
 * 
 * @param index client index
 * @return sakura_void_t 
 */
sakura_int32_t mqtt_check_index(sakura_int32_t index);

/**
 * @brief establish an MQTT connection
 * 
 * @param client client handle
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_connect_wrapper(mqtt_client_t *client);

/**
 * @brief try clear send buffer of client
 * 
 * @param client MQTT client
 * @return sakura_void_t 
 */
sakura_void_t mqtt_try_clear_send_buffer(mqtt_client_t *client);

/**
 * @brief free send buffer of client
 * 
 * @param client MQTT client
 * @return sakura_void_t 
 */
sakura_void_t mqtt_free_net_buffer(mqtt_client_t *client);

/**
 * @brief try send message
 * 
 * @param client MQTT client
 * @return 0: OK
 *        -1: fail
 */
sakura_int32_t mqtt_try_send(mqtt_client_t *client);

/**
 * @brief according to an socket lookup and returned to the client
 * 
 * @param sock socket id
 * @return client: success
 *           null: fail
 */
mqtt_client_t* mqtt_get_client_by_sock(sakura_int32_t sock);

/**
 * @brief socket connect callback function
 * 
 * @param sock socket id
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_connect(sakura_int32_t sock);

/**
 * @brief socket send callback function
 * 
 * @param sock socket id
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_send(sakura_int32_t sock);

/**
 * @brief socket recv callback function
 * 
 * @param sock socket id
 * @param buf buffer
 * @param len data length
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_recv(sakura_int32_t sock, const sakura_uint8_t *buf, sakura_uint32_t len);

/**
 * @brief report socket status code callback function
 * 
 * @param sock socket id
 * @param status status code
 * @return 0: OK
 *        -1: fail 
 */
sakura_int32_t mqtt_net_on_status(sakura_int32_t sock, sakura_int32_t status);

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
sakura_int32_t mqtt_encode_publish(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_msg_ident_t pub_ident, const sakura_char_t *topic, const mqtt_message_t *msg);

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
sakura_int32_t mqtt_encode_subscribe(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_msg_ident_t sub_ident, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num);

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
sakura_int32_t mqtt_encode_unsubscribe(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_msg_ident_t unsub_ident, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num);

/**
 * @brief the connect data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @param conn_data connect data
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_connect(sakura_uint8_t* buf, sakura_int32_t buflen, mqtt_connect_data_t* conn_data);

/**
 * @brief the disconnect data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_disconnect(sakura_uint8_t* buf, sakura_int32_t buflen);

/**
 * @brief the ack data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @param ack_ident ack message identifier
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_ack(sakura_uint8_t* buf, sakura_int32_t buflen, sakura_uint8_t type, mqtt_msg_ident_t ack_ident);

/**
 * @brief the pingreq data packet encoding
 * 
 * @param buf buffer
 * @param buflen buffer len
 * @return >0: susses
 *         -1: fail 
 */
sakura_int32_t mqtt_encode_pingreq(sakura_uint8_t* buf, sakura_int32_t buflen);

/**
 * @brief get the number of clients in service.
 * 
 * @return sakura_int32_t 
 */
sakura_uint32_t mqtt_get_service_client_num_bit(sakura_void_t);

/**
 * @brief append topics to list
 * 
 * @param client MQTT client
 * @param idx this message tracker idx
 * @param sub_topic_list subscribe topic list
 * @param topic_num topic number
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_topics_list_append(mqtt_client_t *client, sakura_int32_t idx, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num);

/**
 * @brief append topics to list
 * 
 * @param client MQTT client
 * @param unsub_topic_list unsubscribe topic list
 * @param topic_num topic number
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_topics_list_delete(mqtt_client_t *client, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num);

/**
 * 
 * @brief topic state update
 * 
 * @param client MQTT client
 * @param idx unsubscribe topic list
 * @param bytes topic number
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_topics_update(mqtt_client_t *client, sakura_uint32_t idx, const sakura_uint8_t *bytes);

/**
 * 
 * @brief try reset all node state from topic_list
 * 
 * @param client MQTT client
 * @return sakura_int32_t 
 */
sakura_void_t mqtt_client_reset(mqtt_client_t *client);

/**
 * @brief check tracker idx legitimacy
 * 
 * @param index tracker index
 * @return sakura_int32_t 
 */
sakura_int32_t mqtt_check_tracker_index(sakura_int32_t index);

/**
 * @brief get net buf size
 * 
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_mqtt_get_net_buf_size(sakura_void_t);

/**
 * @brief get message type string
 * 
 * @param type type num
 * @return const sakura_char_t* 
 */
const sakura_char_t* mqtt_get_message_name(sakura_uint8_t type);

/**
 * @brief get client state name string
 * 
 * @param state state num
 * @return const sakura_char_t* 
 */
const sakura_char_t* mqtt_get_client_state_name(MQTT_CLIENT_STATE state);

/**
 * @brief get next state
 * 
 * @param client client handle
 * @param state state num
 * @return sakura_void_t 
 */
sakura_void_t mqtt_next_state(mqtt_client_t *client, MQTT_CLIENT_STATE state);
#ifdef __cplusplus
}
#endif

#endif

