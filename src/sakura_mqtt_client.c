#include <stdio.h>
#include <string.h>
#include "sakura_daemon.h"
#include "sakura_mqtt.h"
#include <pthread.h>

#ifdef CONFIG_SYS_UNIX
#include "sakura_dns.h"
#endif

#define LOG_TAG                 "CLIENT"
#define CLIENT_LOGD             SAKURA_LOGD
#define CLIENT_LOGE             SAKURA_LOGE
#define CLIENT_LOGI             SAKURA_LOGI
#define CLIENT_LOGW             SAKURA_LOGW

#ifndef MQTT_SDK_COMMIT_ID
    #define MQTT_SDK_COMMIT_ID  "other"
#endif


/* client module all global variable */
static struct{
    mqtt_client_t               *mqtt_clients;                  /* all clients */
    mqtt_reconnect_t            *reconnect_handle;              /* reconnect handle */ 
    sakura_uint32_t               mqtt_init_flag;                 /* init flag */
    sakura_uint32_t               mqtt_client_num;                /* client num */
    sakura_sock_callbacks_t       mqtt_net_cbs;                   /* callbacks */
#ifndef CONFIG_SYS_UNIX
    sakura_uint32_t               tick_rate;                      /* tick rate */
#endif
    sakura_uint32_t               net_buf_size;                   /* net buffer size */
    pthread_mutex_t             glob_mutex;                     /* glob mutex */
    sakura_char_t                 version[MQTT_SDK_COMMIT_LEN];   /* version info */
}global_mqtt_handle = {0};

#ifdef CONFIG_SYS_UNIX
static pthread_mutex_t glob_mqtt_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* message type string */
sakura_char_t *log_level_str[] =
{
    "LOG_LEVEL_NONE",
    "LOG_LEVEL_FATAL",
    "LOG_LEVEL_ERROR",
    "LOG_LEVEL_WARNING",
    "LOG_LEVEL_INFO",
    "LOG_LEVEL_DEBUG",
    "LOG_LEVEL_VERBOSE"
};

#define GET_LOG_LEVEL(type)                     log_level_str[(type) & 0x0FU]

/**
 * The static functions declare
 */
static sakura_int32_t mqtt_set_keepalive(mqtt_client_t *client, sakura_void_t *arg);
static sakura_int32_t mqtt_set_will(mqtt_client_t * client, sakura_void_t *arg);
#if 0
static sakura_int32_t mqtt_set_send_buf(mqtt_client_t *client, sakura_void_t *arg);
static sakura_int32_t mqtt_set_recv_buf(mqtt_client_t *client, sakura_void_t *arg);
static sakura_int32_t mqtt_set_backup_buf(mqtt_client_t *client, sakura_void_t *arg);
#endif
static sakura_int32_t mqtt_set_timeout(mqtt_client_t *client, sakura_void_t *arg);
static sakura_int32_t mqtt_set_cleansession(mqtt_client_t *client, sakura_void_t *arg);
static sakura_int32_t mqtt_disconnect_wrapper(mqtt_client_t *client);
static sakura_int32_t clear_topic_list(mqtt_client_t* client);
static sakura_int32_t mqtt_tick_do_reconnect(sakura_int32_t index);
static sakura_int32_t mqtt_tick_do_subscribe(sakura_int32_t index);
static sakura_int32_t mqtt_save_account_info(mqtt_client_t* client, const sakura_mqtt_account_info_t *account_info);
static sakura_void_t mqtt_tick_on_error(sakura_int32_t index);
static sakura_int32_t mqtt_close_all_clients(sakura_void_t);
static sakura_int32_t mqtt_client_id_check(const sakura_char_t* client_id);
static sakura_uint32_t mqtt_get_service_client_number(sakura_void_t);
static sakura_int32_t mqtt_clear_will(mqtt_client_t * client);
static sakura_int32_t sakura_mqtt_close_internal(sakura_int32_t index);
static sakura_void_t mqtt_cleanup_sub_module(sakura_void_t);
static sakura_int32_t sakura_mqtt_subscribe_internal(sakura_int32_t index, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num);


/**
 * @brief get net buf size
 * 
 * @return sakura_uint32_t 
 */
sakura_uint32_t sakura_mqtt_get_net_buf_size(sakura_void_t)
{
    return global_mqtt_handle.net_buf_size;
}

#ifdef CONFIG_LOG
sakura_int32_t sakura_mqtt_set_log_config(sakura_sdk_log_conf_t* conf)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do {
        /* legal judgment */
        if (conf == NULL) {
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        ret = sakura_set_log_config(conf);
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}
#endif

/**
 * @brief init the 'c_max_num' clients
 * 
 * @param c_max_num client max num
 * @return   0: ok
 *           <0: error
 */
sakura_int32_t sakura_mqtt_init(sakura_uint32_t c_max_num)
{
    sakura_uint32_t i;
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 1){
            CLIENT_LOGE("mqtt has been initialized.\n");
            ret = SAKURA_MQTT_ERR_INITIALIZED;
            break;
        }

        /* check params */
        if(c_max_num == 0 || c_max_num > MQTT_MAX_CLIENTS_NUM){
            CLIENT_LOGE("max clients num is invalid!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }


#ifdef CONFIG_IMPL_STATIC_MEMORY
        if (sakura_mem_init() < 0) {
            ret = SAKURA_MQTT_ERROR;
            CLIENT_LOGE("init static memory failed!\n");
            break;
        }
#endif

#ifdef CONFIG_LOG
        /* init log */
        (void)sakura_log_init();
#endif

        /* init clients */
        global_mqtt_handle.mqtt_clients = (mqtt_client_t*)sakura_malloc(c_max_num * sizeof(mqtt_client_t));
        if (global_mqtt_handle.mqtt_clients == NULL){
            ret = SAKURA_MQTT_ERR_MEMORY;
            CLIENT_LOGE("clients sakura_malloc failed!\n");
            break;
        }

        /* memset client struct */
        memset(global_mqtt_handle.mqtt_clients, 0, c_max_num * sizeof(mqtt_client_t));

        for(i = 0; i < c_max_num; i++){
            global_mqtt_handle.mqtt_clients[i].index                      =   -1;
            global_mqtt_handle.mqtt_clients[i].state                      =   MQTT_CLIENT_FREE;
            global_mqtt_handle.mqtt_clients[i].will                       =   NULL;
            global_mqtt_handle.mqtt_clients[i].ping_outstanding           =   0;
            global_mqtt_handle.mqtt_clients[i].mutex_lock.mutex_entity    =   NULL;
            global_mqtt_handle.mqtt_clients[i].account_info               =   NULL;
        }

        /* init reconnect handle array */
        global_mqtt_handle.reconnect_handle = (mqtt_reconnect_t*)sakura_malloc(c_max_num * sizeof(mqtt_reconnect_t));
        if(global_mqtt_handle.reconnect_handle == NULL){
            ret = SAKURA_MQTT_ERR_MEMORY;
            CLIENT_LOGE("reconnect handle sakura_malloc failed!\n");
            break;            
        }

        /* sock init */
        ret = sakura_socket_init();
        if(ret < 0){
            CLIENT_LOGE("socket module init failed!\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }
        
        /* set client num */
        global_mqtt_handle.mqtt_client_num          =   c_max_num;
#ifndef CONFIG_SYS_UNIX
        global_mqtt_handle.tick_rate                =   0;
#endif
        global_mqtt_handle.net_buf_size             =   DEFAULT_NET_BUF_SIZE;
        global_mqtt_handle.mqtt_net_cbs.cb_connect  =   mqtt_net_on_connect;
        global_mqtt_handle.mqtt_net_cbs.cb_recv     =   mqtt_net_on_recv;
        global_mqtt_handle.mqtt_net_cbs.cb_send     =   mqtt_net_on_send;
        global_mqtt_handle.mqtt_net_cbs.cb_status   =   mqtt_net_on_status;

#ifdef CONFIG_SYS_UNIX
        /* dns init */
        sakura_dns_init();

        /* tick thread start */
        if(sakura_daemon_start() < 0){
            CLIENT_LOGE("daemon start failed!\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }
#endif
        global_mqtt_handle.mqtt_init_flag           =   1;
        CLIENT_LOGI("mqtt init success! a total of %d clients.\n", global_mqtt_handle.mqtt_client_num);
    } while (SAKURA_FALSE);

    /* if init failed, release resource */
    if(global_mqtt_handle.mqtt_init_flag == 0){
#ifdef CONFIG_SYS_UNIX
        /* dns cleanup */
        sakura_dns_cleanup();
#endif
        /* release reconnect handle */
        if(global_mqtt_handle.reconnect_handle != NULL){
            sakura_free(global_mqtt_handle.reconnect_handle);
            global_mqtt_handle.reconnect_handle = NULL;
        }

        /* release clients */
        if(global_mqtt_handle.mqtt_clients != NULL){
            sakura_free(global_mqtt_handle.mqtt_clients);
            global_mqtt_handle.mqtt_clients = NULL;
        }

        /* reset mqtt client num */
        global_mqtt_handle.mqtt_client_num = 0;

        /* socket cleanup */
        sakura_socket_cleanup();
    }

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

#ifndef CONFIG_SYS_UNIX
/**
 * @brief set async tick_rate
 * 
 * @param tick_rate 
 * @return sakura_int32_t 
 */
sakura_int32_t sakura_mqtt_set_async_tick_rate(sakura_uint32_t tick_rate)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check params */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt sdk no init!\n");
            break;
        }
        if(tick_rate == 0){
            CLIENT_LOGE("Illegal frequency value! The default value(%d) will be applied.\n", global_mqtt_handle.tick_rate);
            break;
        }  
        global_mqtt_handle.tick_rate = tick_rate;
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

#endif
/**
 * @brief set net buf size in client
 * 
 * @param size buffer sie
 * @return    0: ok
 *           <0: error 
 */
sakura_int32_t sakura_mqtt_set_net_buf_size(sakura_uint32_t size)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check params */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt sdk no init!\n");
            break;
        }
        if(size == 0){
            CLIENT_LOGE("invalid net buf size! The default value(%d) will be applied.\n", global_mqtt_handle.net_buf_size);
            break;
        }  

        if(ret < 0){
            CLIENT_LOGE("try take mutex failed!\n");
            break;
        }
        global_mqtt_handle.net_buf_size = size;
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

/**
 * @brief get mqtt sdk version
 * 
 * @return   version string
 */
const sakura_char_t* sakura_mqtt_get_version(sakura_void_t)
{
    sprintf(global_mqtt_handle.version, "%s_%s", SAKURA_MQTT_SDK_VERSION, MQTT_SDK_COMMIT_ID);
    return global_mqtt_handle.version;
}

/**
 * @brief query mqtt client properties
 * 
 * @return sakura_int32_t 
 */
sakura_int32_t sakura_mqtt_query_properties(sakura_void_t)
{
    sakura_char_t buf[MQTT_PROPERTIES_BUF_SIZE] = {0};
    sakura_char_t* p = buf;
    sakura_uint32_t num = 0;
    sprintf(global_mqtt_handle.version, "%s_%s", SAKURA_MQTT_SDK_VERSION, MQTT_SDK_COMMIT_ID);
    num = sprintf(p, "version\t\t\t: %s\n", global_mqtt_handle.version);
    p += num;
#ifndef CONFIG_SYS_UNIX
    num = sprintf(p, "platform\t\t: %s\n", "embed");
    p += num;
    num = sprintf(p, "tick rate\t\t: %d\n", global_mqtt_handle.tick_rate);
    p += num;
#else 
    num = sprintf(p, "platform\t\t: %s\n", "unix");
    p += num;
#endif
#ifdef CONFIG_IMPL_STATIC_MEMORY
    num = sprintf(p, "memory\t\t\t: static\n");
    p += num;
    num = sprintf(p, "memory size\t\t: %d\n", CONFIG_IMPL_DYNAMIC_MEMORY_SIZE);
    p += num;
#else
    num = sprintf(p, "memory\t\t\t: dynamic\n");
    p += num;
#endif
    num = sprintf(p, "total clients num\t: %d\n", global_mqtt_handle.mqtt_client_num);
    p += num;
    num = sprintf(p, "used clients num\t: %d\n", mqtt_get_service_client_number());
    p += num;
    num = sprintf(p, "net buffer size\t\t: %d\n", global_mqtt_handle.net_buf_size);
    p += num;
    num = sprintf(p, "redeliver count\t\t: %d\n", CONFIG_REDELIVER_COUNT);
    p += num;
    num = sprintf(p, "log level\t\t: %s\n", GET_LOG_LEVEL(log_config_get_level()));

    CLIENT_LOGI("SAKURA-MQTT-SDK PROPERTIES:\n%s\n", buf);

    return 0;
}

/**
 * @brief open a mqtt client
 * 
 * @param client_id client id
 * @return >=0: success, return client index
 *          -1: failed
 */
sakura_int32_t sakura_mqtt_open(const sakura_char_t *client_id)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_int32_t is_break = 0;
    sakura_uint32_t loop = 0;
    sakura_uint32_t find = 0;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check client id */
        if(mqtt_client_id_check(client_id) < 0){
            CLIENT_LOGE("client id is invalid!\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        for (loop = 0; loop < global_mqtt_handle.mqtt_client_num; ++loop) {
            if (global_mqtt_handle.mqtt_clients[loop].state == MQTT_CLIENT_FREE) {
                /* set find flag */
                find = 1;
                /* init client state */
                global_mqtt_handle.mqtt_clients[loop].state = MQTT_CLIENT_NOT_CONNECTED;
                /* init client_id */
                strcpy(global_mqtt_handle.mqtt_clients[loop].client_id, client_id);
                global_mqtt_handle.mqtt_clients[loop].index = loop;
                /* init socket */
                global_mqtt_handle.mqtt_clients[loop].net.sock = -1;
                /* init default timeout */
                global_mqtt_handle.mqtt_clients[loop].timeout_interval = DEFAULT_MQTT_TIMEOUT_INTERVAL;
                /* init default keepalive interval */
                global_mqtt_handle.mqtt_clients[loop].keepalive_interval = DEFAULT_MQTT_KEEPALIVE_INTERVAL;
                /* init reconnect flag */
                global_mqtt_handle.mqtt_clients[loop].reconnect_flag = 0;
                /* init reconnect handle */
                global_mqtt_handle.reconnect_handle[loop].reConnectCount = 0;
                global_mqtt_handle.reconnect_handle[loop].reConnectTimer = 0;
                /* init waiting subscribe topic number */
                global_mqtt_handle.mqtt_clients[loop].wait_sub_topic_num = 0;
                /* init cleansession */
                global_mqtt_handle.mqtt_clients[loop].cleansession = DEFAULT_MQTT_CLEANSESSION;
                /* init last keepalive tick */
                global_mqtt_handle.mqtt_clients[loop].tick = 0;
                global_mqtt_handle.mqtt_clients[loop].last_keepalive_tick = 0;
                /* init will flag */
                global_mqtt_handle.mqtt_clients[loop].willFlag = DEFAULT_MQTT_WILL_FLAG;
                /* init topic list */
                INIT_LIST_HEAD(&global_mqtt_handle.mqtt_clients[loop].topic_list);
                /* init net buffer */
                global_mqtt_handle.mqtt_clients[loop].net.send.buf = (sakura_uint8_t*)sakura_malloc(global_mqtt_handle.net_buf_size);
                if(global_mqtt_handle.mqtt_clients[loop].net.send.buf == NULL){
                    ret = SAKURA_MQTT_ERR_MEMORY;
                    CLIENT_LOGE("call malloc failed\n");
                    break;
                }
                memset(global_mqtt_handle.mqtt_clients[loop].net.send.buf, 0, global_mqtt_handle.net_buf_size);
                global_mqtt_handle.mqtt_clients[loop].net.send.len = 0;
                global_mqtt_handle.mqtt_clients[loop].net.send.size = global_mqtt_handle.net_buf_size;
                global_mqtt_handle.mqtt_clients[loop].net.recv.buf = (sakura_uint8_t*)sakura_malloc(global_mqtt_handle.net_buf_size);
                if(global_mqtt_handle.mqtt_clients[loop].net.recv.buf == NULL){
                    ret = SAKURA_MQTT_ERR_MEMORY;
                    CLIENT_LOGE("call malloc failed\n");
                    break;
                }
                memset(global_mqtt_handle.mqtt_clients[loop].net.recv.buf, 0, global_mqtt_handle.net_buf_size);
                global_mqtt_handle.mqtt_clients[loop].net.recv.len = 0;
                global_mqtt_handle.mqtt_clients[loop].net.recv.size = global_mqtt_handle.net_buf_size;
                global_mqtt_handle.mqtt_clients[loop].net.backup.buf = (sakura_uint8_t*)sakura_malloc(global_mqtt_handle.net_buf_size);
                if(global_mqtt_handle.mqtt_clients[loop].net.backup.buf == NULL){
                    ret = SAKURA_MQTT_ERR_MEMORY;
                    CLIENT_LOGE("call malloc failed\n");
                    break;
                }
                memset(global_mqtt_handle.mqtt_clients[loop].net.backup.buf, 0, global_mqtt_handle.net_buf_size);
                global_mqtt_handle.mqtt_clients[loop].net.backup.len = 0;
                global_mqtt_handle.mqtt_clients[loop].net.backup.size = global_mqtt_handle.net_buf_size;
                /* init client account information */
                global_mqtt_handle.mqtt_clients[loop].account_info = NULL;
                /* init broker id buffer */
                memset(global_mqtt_handle.mqtt_clients[loop].broker_id, 0, MAX_BROKER_ID_STR_LEN);
                /* init tracker array */
                memset(global_mqtt_handle.mqtt_clients[loop].msg_tracker_array, 0, sizeof(mqtt_msg_tracker_t) * MAX_MSG_STATE_ARRAY_LEN);
                CLIENT_LOGD("allocate MQTT index[%d] for client[%s]\n", loop, client_id);
                ret = loop;
                is_break = 1;
                break;
            }
        }
        if (is_break == 0 && find == 1) {
            /* init failed! free memory */
            mqtt_free_net_buffer(&global_mqtt_handle.mqtt_clients[loop]);
            memset(&global_mqtt_handle.mqtt_clients[loop], 0, sizeof(mqtt_client_t));
            CLIENT_LOGE("no free client\n");
            ret = SAKURA_MQTT_ERROR;
        }
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

/**
 * @brief close client by id
 * 
 * @param index client index
 * @return  0:close success
 *         -1:close fail
 */
sakura_int32_t sakura_mqtt_close(sakura_int32_t index)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do {       
        /* get client by id */
        client = mqtt_get_client_by_index(index);
        if (client == NULL) {
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            CLIENT_LOGE("no valid client index was obtained!\n");
            break;
        }

        mqtt_next_state(client, MQTT_CLIENT_FREE);

        CLIENT_LOGD("close MQTT index[%d] from client[%s]\n", index, global_mqtt_handle.mqtt_clients[index].client_id);
        /* in case of direct close by caller */
        if (client->net.sock >= 0) {
            (sakura_void_t)sakura_sock_close_wrapper(client->net.sock);
            client->net.sock = -1;
        }

        /* clear net buffer */
        mqtt_free_net_buffer(client);

        /* clear tracker array */
        clear_message_tracker_array(client->msg_tracker_array);
        
        /* clean topic list */
        ret = clear_topic_list(client);

        /* free account info */
        if(client->account_info != NULL){
            sakura_free(client->account_info);
            client->account_info = NULL;
        }

        /* free will */
        if(client->will != NULL){
            if(client->will->topic != NULL){
                sakura_free(client->will->topic);
                client->will->topic = NULL;
            }
            if(client->will->payload != NULL){
                sakura_free(client->will->payload);
                client->will->payload = NULL;
            }
            sakura_free(client->will);
            client->will = NULL;
        }

        /* clear oss client */
        memset(client, 0, sizeof(mqtt_client_t));
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret; 
}

/**
 * @brief close client by id
 * 
 * @param index client index
 * @return  0:close success
 *         -1:close fail
 */
static sakura_int32_t sakura_mqtt_close_internal(sakura_int32_t index)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;

    do {       
        /* get client by id */
        client = mqtt_get_client_by_index(index);
        if (client == NULL) {
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            CLIENT_LOGE("no valid client index was obtained!\n");
            break;
        }

        mqtt_next_state(client, MQTT_CLIENT_FREE);

        CLIENT_LOGD("close MQTT index[%d] from client[%s]\n", index, global_mqtt_handle.mqtt_clients[index].client_id);
        /* in case of direct close by caller */
        if (client->net.sock >= 0) {
            (sakura_void_t)sakura_sock_close_wrapper(client->net.sock);
            client->net.sock = -1;
        }

        /* clear net buffer */
        mqtt_free_net_buffer(client);

        /* clear tracker array */
        clear_message_tracker_array(client->msg_tracker_array);
        
        /* clean topic list */
        ret = clear_topic_list(client);

        /* free account info */
        if(client->account_info != NULL){
            sakura_free(client->account_info);
            client->account_info = NULL;
        }

        /* free will */
        if(client->will != NULL){
            if(client->will->topic != NULL){
                sakura_free(client->will->topic);
                client->will->topic = NULL;
            }
            if(client->will->payload != NULL){
                sakura_free(client->will->payload);
                client->will->payload = NULL;
            }
            sakura_free(client->will);
            client->will = NULL;
        }

        /* clear oss client */
        memset(client, 0, sizeof(mqtt_client_t));
    } while (SAKURA_FALSE);

    return ret; 
}

/**
 * @brief set config by id
 * 
 * @param index client index
 * @param cmd command
 * @param arg arg
 * @return sakura_int32_t 
 */
sakura_int32_t sakura_mqtt_set_option(sakura_int32_t index, SAKURA_MQTT_OPT opt, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    mqtt_client_t *client = NULL;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check params */
        if(arg == NULL){
            CLIENT_LOGE("invalid params!");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get client by id */
        client = mqtt_get_client_by_index(index);
        if (client == NULL || arg == NULL) {
            CLIENT_LOGE("get client failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        switch (opt) {
            /* set keepalive */
            case SAKURA_MQTT_SET_KEEPALIVE:
                ret = mqtt_set_keepalive(client, arg);
                break;
            /* set will */
            case SAKURA_MQTT_SET_WILL:
                ret = mqtt_set_will(client, arg);
                break;
            /* set timeout */
            case SAKURA_MQTT_SET_TIMEOUT:
                ret = mqtt_set_timeout(client, arg);
                break;
            /* set cleansession */
            case SAKURA_MQTT_SET_CLEANSESSION:
                ret = mqtt_set_cleansession(client, arg);
                break; 
            default:
                CLIENT_LOGE("not supported opt[%d]\n", opt);
                break;
        }
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

/**
 * @brief drive the MQTT life cycle
 * 
 * @param index client index
 * @param tick tick
 * @return sakura_int32_t 
 */
sakura_int32_t sakura_mqtt_client_tick(sakura_int32_t index, sakura_uint64_t tick)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;
    do {
        /* get client by id */
        client = mqtt_get_client_by_index(index);
        if (client == NULL) {
            CLIENT_LOGE("did not get the serial number of the client.\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* update tick */
        client->tick = tick;
        if(client->state == MQTT_CLIENT_CONNECT_ERROR){
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* mqtt client tick */
        ret = mqtt_client_tick(client, tick);
        if ( ret < 0) {
            CLIENT_LOGE("client[%02d|%s] tick failed\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        if (client->state == MQTT_CLIENT_TCP_CONNECTING) {
            /* check whether TCP connect timeout or not */
            if (tick > (client->last_keepalive_tick + client->timeout_interval)) {
                CLIENT_LOGE("client[%02d|%s] TCP connect timeout\n", client->index, client->client_id);
                ret = SAKURA_MQTT_ERROR;
                break;
            }
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief drive the MQTT life cycle
 * 
 * @param tick tick
 * @return   0: ok
 *          <0: error
 */
sakura_int32_t sakura_mqtt_tick(sakura_uint64_t tick)
{
    sakura_uint32_t service_byte = 0;
    sakura_uint32_t index;
    sakura_uint64_t seconds_tick = 0;
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t client_num = 0;

    do
    {
        
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            ret = SAKURA_MQTT_ERROR;
            break;
        }

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
        /* socket driver */
        sakura_socket_loop_once();

#ifdef CONFIG_LOG
        /*when log file ops fail, check and recover.*/
        sakura_log_tick();
#endif

#ifdef CONFIG_SYS_UNIX
        /* update seconds_tick */
        seconds_tick = tick;
#else
        seconds_tick = tick / global_mqtt_handle.tick_rate;
#endif

        /* get in service client */
        service_byte = mqtt_get_service_client_num_bit();
        for (index = 0; index < global_mqtt_handle.mqtt_client_num; index++)
        {
            /* check whether the corresponding bit is set */
            if((service_byte >> index) & 1U){
                /* client tick */
                ret = sakura_mqtt_client_tick(index, seconds_tick);
                if(ret < 0){
                    /* tick failed!, report net work error */
                    mqtt_tick_on_error(index);
                }

                /* tick do reconnect */
                (sakura_void_t)mqtt_tick_do_reconnect(index);

                /* tick do subscribe */
                (sakura_void_t)mqtt_tick_do_subscribe(index);
                client_num ++;
            }
        }

        /* all clients is idle */
        if(client_num == 0){
            ret = SAKURA_MQTT_ALL_CLIENTS_IDLE;
        }

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    } while (SAKURA_FALSE);
    
    return ret;
}

/**
 * @brief get the number of clients in service.
 * 
 * @return sakura_int32_t 
 */
sakura_uint32_t mqtt_get_service_client_num_bit(sakura_void_t)
{
    sakura_uint32_t i;
    sakura_uint32_t service_info = 0;
    for (i = 0; i < global_mqtt_handle.mqtt_client_num; i++)
    {
        /* set bit in byte */
        if(global_mqtt_handle.mqtt_clients[i].state != MQTT_CLIENT_FREE){
            service_info |= (1 << i);
        }
    }
    return service_info;
}

/**
 * @brief mqtt connect, the results will be sent to you by cb_connect notifies the user. 
 * 
 * @param index client index
 * @param account_info broker id, username password
 * @param cbs mqtt callback
 * @return   0: ok
 *          -1: error
 *          -2: async send occurs
 *          -3: invalid arguments
 *          -5: request ignore
 */
sakura_int32_t sakura_mqtt_connect(sakura_int32_t index, const sakura_mqtt_account_info_t *account_info, const mqtt_cbs_t *cbs)
{
    /* init */
    mqtt_client_t *client = NULL;
    sakura_int32_t ret = SAKURA_MQTT_ERROR;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check params */
        if((mqtt_check_index(index) == -1) || account_info == NULL){
            CLIENT_LOGE("index or account information is invalid!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get client by index */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            CLIENT_LOGE("no matched client with client index\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* check client connect state */
        if(client->state != MQTT_CLIENT_NOT_CONNECTED){
            CLIENT_LOGE("client[%02d|%s] connection in progress, please do not call again!\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_BUSY;
            break;
        }

        /* save account info */
        ret = mqtt_save_account_info(client, account_info);
        if(ret < 0){
            break;
        }
        if (cbs != NULL) {
            memcpy(&client->cbs, cbs, sizeof(mqtt_cbs_t));
        }
        
        ret = mqtt_connect_wrapper(client);
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

/**
 * @brief mqtt disconnect, the results will be sent to you by cb_disconnect notifies the user. 
 * 
 * @param index client index
 * @return   0: ok
 *          -1: error
 *          -2: async send occurs
 *          -3: invalid arguments
 *          -5: request ignore
 */
sakura_int32_t sakura_mqtt_disconnect(sakura_int32_t index)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check params */
        if((mqtt_check_index(index) == -1)){
            CLIENT_LOGE("index is invalid!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get client by id */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            CLIENT_LOGE("no matched client with client index\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        ret = mqtt_disconnect_wrapper(client);
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}


/**
 * @brief mqtt subscribe, the results will be sent to you by cb_subscribe notifies the user. 
 * 
 * @param index client index
 * @param sub_topic_list sub topic list
 * @param topic_num topic num
 * @return   0: ok
 *          -1: error
 *          -2: async send occurs
 *          -3: invalid arguments
 *          -4: busy now
 *          -5: request ignore
 */
sakura_int32_t sakura_mqtt_subscribe(sakura_int32_t index, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t* client = NULL;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check params */
        if((mqtt_check_index(index) == -1) || sub_topic_list == NULL || topic_num == 0 || topic_num > MQTT_MAX_SUB_NUM){
            CLIENT_LOGE("invalid params!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get client */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            CLIENT_LOGE("failed to get client pointer!\n");
            break;
        }

        /* sub topic */
         ret = mqtt_client_subscribe(client, sub_topic_list, topic_num);
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

/**
 * @brief mqtt subscribe, the results will be sent to you by cb_subscribe notifies the user. 
 * 
 * @param index client index
 * @param sub_topic_list sub topic list
 * @param topic_num topic num
 * @return   0: ok
 *          -1: error
 *          -2: async send occurs
 *          -3: invalid arguments
 *          -4: busy now
 *          -5: request ignore
 */
static sakura_int32_t sakura_mqtt_subscribe_internal(sakura_int32_t index, const sakura_mqtt_topic_t *sub_topic_list, sakura_uint32_t topic_num)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t* client = NULL;

    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check params */
        if((mqtt_check_index(index) == -1) || sub_topic_list == NULL){
            CLIENT_LOGE("index or sub_topic_list is invalid!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get client */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            CLIENT_LOGE("failed to get client pointer!\n");
            break;
        }

        /* sub topic */
         ret = mqtt_client_subscribe(client, sub_topic_list, topic_num);
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief mqtt unsubscribe, the results will be sent to you by cb_unsubscribe notifies the user. 
 * 
 * @param id client id
 * @param topic topic string
 * @return   0: ok
 *          -1: error
 *          -2: async send occurs
 *          -3: invalid arguments
 *          -4: busy now
 *          -5: request ignore
 */
sakura_int32_t sakura_mqtt_unsubscribe(sakura_int32_t index, const sakura_char_t *unsub_topic_list[], sakura_uint32_t topic_num)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t* client = NULL;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check params */
        if((mqtt_check_index(index) == -1) || unsub_topic_list == NULL || unsub_topic_list[0] == NULL){
            CLIENT_LOGE("index or unsub topic list is invalid!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }  

        /* get client */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            CLIENT_LOGE("failed to get client pointer!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* unsub topic */
        ret = mqtt_client_unsubscribe(client, unsub_topic_list, topic_num);
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

/**
 * @brief mqtt publish message, If it fails, will advise the caller by cb_status interface.
 * 
 * @param id client index
 * @param topic topic string
 * @param message message
 * @param pub_cb publish callback, after successful release trigger(can be NULL)
 * @return   0: ok
 *          -1: error
 *          -2: async send occurs
 *          -3: invalid arguments
 *          -4: busy now
 *          -5: request ignore
 */
sakura_int32_t sakura_mqtt_publish(sakura_int32_t index, const sakura_char_t *topic, const mqtt_message_t *message, mqtt_async_callback_t pub_cb)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t* client = NULL;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /* check params */
        if((mqtt_check_index(index) == -1) || topic == NULL || message == NULL){
            CLIENT_LOGE("invalid args, publish failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* get client */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            CLIENT_LOGE("failed to get client pointer!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* publish message */
        ret = mqtt_client_publish(client, topic, message, pub_cb);
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
    return ret;
}

/**
 * @brief release all resources.
 * 
 * @return   sakura_void_t
 */
sakura_void_t sakura_mqtt_cleanup(sakura_void_t)
{
    do
    {
        /* check init state */
        if(global_mqtt_handle.mqtt_init_flag == 0){
            CLIENT_LOGE("mqtt not init!\n");
            break;
        }

#ifdef CONFIG_SYS_UNIX
        /* daemon thread stop */
        (sakura_void_t)sakura_daemon_stop();
#endif

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_mqtt_mutex);
#endif
        mqtt_cleanup_sub_module();
#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_mqtt_mutex);
#endif
        CLIENT_LOGI("mqtt cleanup success!\n"); 
    } while (SAKURA_FALSE);

}

/**
 * @brief according to an index lookup and returned to the client
 * 
 * @param index client id
 * @return   client: success
 *             null: failed
 */
mqtt_client_t* mqtt_get_client_by_index(sakura_int32_t index)
{
    mqtt_client_t* client = NULL;
    do {
        /* check index */
        if ((mqtt_check_index(index) == 0) && global_mqtt_handle.mqtt_clients[index].state != MQTT_CLIENT_FREE) {
            client = &global_mqtt_handle.mqtt_clients[index];
            break;
        }else {
            /* mqtt index is invalid */
            CLIENT_LOGE("invalid MQTT index[%d]\n", index);
            break;
        }
    } while (SAKURA_FALSE);
    return client;   
}

/**
 * @brief according to an socket lookup and returned to the client
 * 
 * @param sock socket id
 * @return client: success
 *           null: fail
 */
mqtt_client_t* mqtt_get_client_by_sock(sakura_int32_t sock)
{
    sakura_uint32_t loop = 0;
    mqtt_client_t* client = NULL;
    /* get client by sock */
    for (loop = 0; loop < global_mqtt_handle.mqtt_client_num; ++loop) {
        /* match client */
        if (global_mqtt_handle.mqtt_clients[loop].state != MQTT_CLIENT_FREE &&
            global_mqtt_handle.mqtt_clients[loop].net.sock == sock) {
            client = &global_mqtt_handle.mqtt_clients[loop];
            break;
        }
    }
    return client;
}

/**
 * @brief check the legitimacy of the index
 * 
 * @param index client index
 * @return sakura_void_t 
 */
sakura_int32_t mqtt_check_index(sakura_int32_t index)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        if(index < 0){
            ret = SAKURA_MQTT_ERROR;
            break;
        }
        if((sakura_uint32_t)index >= global_mqtt_handle.mqtt_client_num){
            ret = SAKURA_MQTT_ERROR;
            break;
        }
    } while (SAKURA_FALSE);
    return ret;
}

sakura_int32_t mqtt_connect_wrapper(mqtt_client_t *client)
{
    sakura_int32_t ret  = SAKURA_MQTT_ERROR;
    sakura_int32_t sock = -1;
    sakura_mqtt_account_info_t account = {0};
    sakura_sock_host_t broker = {0};
    do {
        /* check parameter */
        if (client == NULL) {
            CLIENT_LOGE("invalid args, reconnect end\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* 1. create socket */
        sock = sakura_sock_create();
        if (sock < 0) {
            CLIENT_LOGE("socket create error\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        } else {
            client->net.sock  = sock;
        }

        /* 2. set account information */
        broker.hostname     = client->account_info->hostname;
        broker.port         = client->account_info->port;
        account.broker      = &broker;
        if(client->account_info->username != NULL){
            account.username = client->account_info->username;
        } else {
            account.username = NULL;
        }
        if(client->account_info->password != NULL){
            account.password = client->account_info->password;
        } else {
            account.password = NULL;
        }
        
        /* 3. encapsulate MQTT CONNECT packet */
        ret = mqtt_client_connect(client, &account, client->cbs.cb_connect);  
        if (ret < 0) {
            CLIENT_LOGE("failed to encapsulate CONNECT packet\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* 4. tcp connect */
        ret = sakura_sock_connect_wrapper(sock, account.broker, &global_mqtt_handle.mqtt_net_cbs);
        if (ret == -1) {
            ret = SAKURA_MQTT_ERROR;
            break;
        }else {
            client->last_keepalive_tick = client->tick;
            mqtt_next_state(client, MQTT_CLIENT_TCP_CONNECTING);
        }
    } while (SAKURA_FALSE);

    if(ret < 0){
        /* close sock */
        (sakura_void_t)sakura_sock_close_wrapper(sock);
        client->net.sock = -1;
    }

    return ret;
}

/**
 * @brief set the MQTT keepalive options.
 * 
 * @param client MQTT client
 * @param arg param
 * @return   0: ok
 *          <0: error
 */
static sakura_int32_t mqtt_set_keepalive(mqtt_client_t *client, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t *p = NULL;
    /* check parameter */
    if (client != NULL && arg != NULL) {
        p = (sakura_uint32_t *)arg;
        client->keepalive_interval = *p;
    }else {
        /* parameter is invalid */
        ret = SAKURA_MQTT_ERR_INVALID_ARGS;
        CLIENT_LOGE("params invalid,set keepalive failed!\n");
    }
    return ret;
}

static sakura_int32_t mqtt_clear_will(mqtt_client_t * client)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    do
    {
        if(client == NULL){
            CLIENT_LOGE("invalid args\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        if(client->will != NULL){
            if(client->will->topic != NULL){
                sakura_free(client->will->topic);
                client->will->topic = NULL;
            }

            if(client->will->payload != NULL){
                sakura_free(client->will->payload);
                client->will->payload = NULL;
            }

            sakura_free(client->will);
            client->will = NULL;
        }
    } while (SAKURA_FALSE);
    return ret;
}

/**
 * @brief set the MQTT will message options.
 * 
 * @param client MQTT client
 * @param arg param
 * @return   0: ok
 *           <0: error
 */
static sakura_int32_t mqtt_set_will(mqtt_client_t * client, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    mqtt_will_t* will = (mqtt_will_t *)arg;
    sakura_uint32_t is_break = 0; 
    sakura_uint32_t topic_len = 0;
    do
    {
        /* check params */
        if(client == NULL || will == NULL || will->topic == NULL || will->payload == NULL || will->qos > QOS2 || will->qos < QOS0){
            CLIENT_LOGE("invalid params, set will failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }

        /* check will */
        if(client->will != NULL){
            ret = mqtt_clear_will(client);
            if(ret < 0){
                ret = SAKURA_MQTT_ERROR;
                CLIENT_LOGE("clear old will struct failed!\n");
                break;  
            }
        }

        /* will malloc */
        client->will = (mqtt_will_t *)sakura_malloc(sizeof(mqtt_will_t));
        if(client->will == NULL){
            ret = SAKURA_MQTT_ERR_MEMORY;
            CLIENT_LOGE("will sakura_malloc failed!\n");
            is_break = 1;
            break;
        } 
        memset(client->will, 0, sizeof(mqtt_will_t));
        
        client->will->qos = will->qos;
        client->will->retained = ((will->retained > 0) ? 1 : 0);
        client->will->payloadlen = will->payloadlen; 
        client->willFlag    = 1U;

        /* reset topic */
        if(client->will->topic != NULL){
            sakura_free(client->will->topic);
            client->will->topic = NULL;
        }
        topic_len = strlen(will->topic);
        client->will->topic = (sakura_char_t*)sakura_malloc(topic_len + 1);
        if(client->will->topic == NULL){
            CLIENT_LOGE("called malloc failed!\n");
            ret = SAKURA_MQTT_ERR_MEMORY;
            is_break = 1;
            break;
        }
        memset(client->will->topic, 0, topic_len + 1);
        memcpy(client->will->topic, will->topic, topic_len);


        /* reset payload */
        if(client->will->payload != NULL){
            sakura_free(client->will->payload);
            client->will->payload = NULL;
        }
        client->will->payload = (sakura_uint8_t*)sakura_malloc(client->will->payloadlen + 1);
        if(client->will->payload == NULL){
            CLIENT_LOGE("called malloc failed!\n");
            ret = SAKURA_MQTT_ERR_MEMORY;
            is_break = 1;
            break;
        }
        memset(client->will->payload, 0, client->will->payloadlen + 1);
        memcpy(client->will->payload, will->payload, client->will->payloadlen);
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    /* check memory state */
    if(is_break == 1){
        ret = mqtt_clear_will(client);
    }
    
    return ret;
}

#if 0
/**
 * @brief set the MQTT send buffer options.
 * 
 * @param client MQTT client
 * @param arg param
 * @return   0: ok
 *          <0: error
 */
static sakura_int32_t mqtt_set_send_buf(mqtt_client_t *client, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    mqtt_net_data_t *data = NULL;
    do
    {
        /* check parameter */
        if(client == NULL || arg == NULL){
            CLIENT_LOGE("invalid params, set send buf failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        data = (mqtt_net_data_t*)arg;
        /* set send buffer */
        client->net.send.buf    = data->buf;
        client->net.send.size   = data->size;
        client->net.send.len    = 0;
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief set the MQTT recv buffer options.
 * 
 * @param client MQTT client
 * @param arg param
 * @return   0: ok
 *          <0: error
 */
static sakura_int32_t mqtt_set_recv_buf(mqtt_client_t *client, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    mqtt_net_data_t *data = NULL;
    do
    {
        /* check parameter */
        if(client == NULL || arg == NULL){
            CLIENT_LOGE("invalid params, set recv buf failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        data = (mqtt_net_data_t*)arg;
        /* set recv buffer */
        client->net.recv.buf    = data->buf;
        client->net.recv.size   = data->size;
        client->net.recv.len    =   0;
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief set the MQTT backup buffer options.
 * 
 * @param client MQTT client
 * @param arg param
 * @return   0: ok
 *          <0: error
 */
static sakura_int32_t mqtt_set_backup_buf(mqtt_client_t *client, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    mqtt_net_data_t *data = NULL;
    do
    {
        /* check parameter */
        if(client == NULL || arg == NULL){
            CLIENT_LOGE("invalid params, set backup buf failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        data = (mqtt_net_data_t*)arg;
        /* set backup buffer */
        client->net.backup.buf  = data->buf;
        client->net.backup.size = data->size;
        client->net.backup.len  = 0;
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    return ret;
}
#endif

/**
 * @brief set the MQTT timeout options.
 * 
 * @param client MQTT client
 * @param arg param
 * @return   0: ok
 *           <0: error
 */
static sakura_int32_t mqtt_set_timeout(mqtt_client_t *client, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_uint32_t *p = NULL;
    do
    {
        /* check parameter */
        if(client == NULL || arg == NULL){
            CLIENT_LOGE("invalid params, set timeout interval failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        p = (sakura_uint32_t *)arg;
        /* set timeout interval */
        client->timeout_interval = *p;
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    return ret;
}

/**
 * @brief set the MQTT cleansession options.
 * 
 * @param client MQTT client
 * @param arg param
 * @return   0: ok
 *           <0: error
 */
static sakura_int32_t mqtt_set_cleansession(mqtt_client_t *client, sakura_void_t *arg)
{
    sakura_int32_t ret = SAKURA_MQTT_ERROR;
    sakura_uint32_t *p = NULL;
    do
    {
        /* check parameter */
        if(client == NULL || arg == NULL){
            CLIENT_LOGE("invalid params, set cleansession failed!\n");
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            break;
        }
        p = (sakura_uint32_t *)arg;
        /* set cleansession */
        client->cleansession = ((*p > 0) ? 1 : 0);
        ret = SAKURA_MQTT_STAT_OK;
    } while (SAKURA_FALSE);

    return ret;
}


static sakura_int32_t mqtt_disconnect_wrapper(mqtt_client_t *client)
{
    /* init ret */
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;

    do {
        /* check client validity */
        if (client == NULL) {
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            CLIENT_LOGE("client is invalid!\n");
            break;
        }

        /* check client state */
        if (client->state == MQTT_CLIENT_NOT_CONNECTED || client->state == MQTT_CLIENT_DISCONNECTING) {
            CLIENT_LOGE("client[%02d|%s] not connected, don't need to disconnect.\n", client->index, client->client_id);
            ret = SAKURA_MQTT_ERR_REQ_IGN;
            break;
        }

        /*
        * The 'quiet' determines whether we send OSS DISCONNECT packet before close socket.
        * If TRUE, do; otherwise, do not.
        */
        if (client->state == MQTT_CLIENT_CONNECTED) {
            ret = mqtt_client_disconnect(client);
            if(ret < 0) {
                break;
            }
        }

        mqtt_next_state(client, MQTT_CLIENT_DISCONNECTING);
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t clear_topic_list(mqtt_client_t* client)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_topic_list_t *node = NULL;
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* check params */
        if(client == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            CLIENT_LOGE("input params is invalid!\n");
            break;
        }
        /* del topic_node from list  */
        list_for_each_safe(pos, next, &client->topic_list) {
            node = list_entry(pos, mqtt_topic_list_t, list);
            list_del(&node->list);
            if(node->topic != NULL){
                sakura_free(node->topic);
                node->topic = NULL;
            }
            sakura_free(node);
            node = NULL;
        }
    } while (SAKURA_FALSE);
    return ret;
}

static sakura_int32_t mqtt_tick_do_reconnect(sakura_int32_t index)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;
    do
    {
        /* We should skip this round when we have not obtained the client, 
        as it may have been released during `tick` processing. */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            CLIENT_LOGE("failed to get client pointer.\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* avoid reconnecting when the client is normal. */
        if(client->state != MQTT_CLIENT_CONNECT_ERROR){
            break;
        }

        /* reconnect and reconnect count */
        if(client->tick > global_mqtt_handle.reconnect_handle[index].reConnectTimer){
            global_mqtt_handle.reconnect_handle[index].reConnectCount ++;
            CLIENT_LOGI("client[%s] reconnects for the %dth time.\n", client->client_id, global_mqtt_handle.reconnect_handle[index].reConnectCount);
            /* reset client */
            client->reconnect_flag = 0;
            mqtt_client_reset(client);
            ret = mqtt_connect_wrapper(client);
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_tick_do_subscribe(sakura_int32_t index)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    mqtt_client_t *client = NULL;
    mqtt_topic_list_t *node = NULL;
    sakura_mqtt_topic_t sub_topic = {0};
    list_head_t *pos  = NULL;
    list_head_t *next = NULL;
    do
    {
        /* get client */
       client = mqtt_get_client_by_index(index);
       if(client == NULL){
            CLIENT_LOGE("failed to get client pointer.\n");
            ret = SAKURA_MQTT_ERROR;
            break;
       }

       /* check client state, subscribe wether is complete */
        if(client->state != MQTT_CLIENT_CONNECTED || client->wait_sub_topic_num == 0){
            break;
        }

        /* traversing topic list. */
        list_for_each_safe(pos, next, &client->topic_list) {
            node = list_entry(pos, mqtt_topic_list_t, list);
            if(node->state == MQTT_TOPIC_STATE_TO_SUBSCRIBE){
                sub_topic.topic = node->topic;
                sub_topic.qos   = node->qos;
                ret = sakura_mqtt_subscribe_internal(index, &sub_topic, 1);
                if(ret < 0){
                    CLIENT_LOGE("try subscribe topic[%s] failed! ret = %d\n", sub_topic.topic, ret);
                    break;
                }
                node->state = MQTT_TOPIC_STATE_SUBSCRIBING;
            }
        }
    } while (SAKURA_FALSE);
    
    return ret;
}

static sakura_int32_t mqtt_save_account_info(mqtt_client_t* client, const sakura_mqtt_account_info_t *account_info)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t len = 0;
    do
    {
        /* check params */
        if(client == NULL || account_info == NULL || account_info->broker == NULL || account_info->broker->hostname == NULL){
            ret = SAKURA_MQTT_ERR_INVALID_ARGS;
            CLIENT_LOGE("input params is invalid!\n");
            break;
        }

        /* save broker id */
        (sakura_void_t)snprintf(client->broker_id, MAX_BROKER_ID_STR_LEN, "%s:%d", account_info->broker->hostname, account_info->broker->port);

        /* dynamic application of memory space */ 
        client->account_info = (mqtt_account_t*)sakura_malloc(sizeof(mqtt_account_t));
        if(client->account_info == NULL){
            ret = SAKURA_MQTT_ERR_MEMORY;
            CLIENT_LOGE("called malloc failed!\n");
            break;
        }
        memset(client->account_info, 0, sizeof(mqtt_account_t));

        /* set broker */
        if(account_info->broker->hostname != NULL){
            memcpy(client->account_info->hostname, account_info->broker->hostname, strlen(account_info->broker->hostname));   
        }
        client->account_info->port = account_info->broker->port;

        /* set username */
        if(account_info->username != NULL){
            len = strlen(account_info->username);
            if(len > 0){
                memcpy(client->account_info->username, account_info->username, len);
            }            
        }

        /* set password */
        if(account_info->password != NULL){
            len = strlen(account_info->password);
            if(len > 0){
                memcpy(client->account_info->password, account_info->password, len);
            }            
        }
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_void_t mqtt_tick_on_error(sakura_int32_t index)
{
    mqtt_client_t *client = NULL;

    do
    {
        /* get client */
        client = mqtt_get_client_by_index(index);
        if(client == NULL){
            CLIENT_LOGE("failed to get client pointer.\n");
            break;
        }

        if(client->reconnect_flag == 0){
            /* report network error */
            CLIENT_LOGW("encountered network failure! code = %d\n", SAKURA_MQTT_NETWORK_ERROR);
            if(client->cbs.cb_status != NULL){
                client->cbs.cb_status(index, SAKURA_MQTT_NETWORK_ERROR);
            }

            if(client->net.sock != -1){
                (sakura_void_t)sakura_sock_close_wrapper(client->net.sock);
                client->net.sock = -1;
            }
            
            if(client->state != MQTT_CLIENT_CONNECT_ERROR){
                mqtt_next_state(client, MQTT_CLIENT_CONNECT_ERROR);
            }

            /* delay 3s, reconnect */
            global_mqtt_handle.reconnect_handle[index].reConnectTimer = client->tick + DEFAULT_MQTT_RECONNECT_INTERVAL;
            CLIENT_LOGD(" client[%02d|%s] delay(%ds) reset session...\n", client->index, client->client_id, DEFAULT_MQTT_RECONNECT_INTERVAL);

            client->reconnect_flag = 1;
        }
    } while (SAKURA_FALSE);
}

static sakura_int32_t mqtt_close_all_clients(sakura_void_t)
{
    /* close all client */
    sakura_uint32_t service_byte = 0;
    sakura_uint32_t index;
    service_byte = mqtt_get_service_client_num_bit();
    for (index = 0; index < global_mqtt_handle.mqtt_client_num; index++){
        /* check whether the corresponding bit is set */
        if((service_byte >> index) & 1U){
            CLIENT_LOGW("client [%d] is still in service, it will be forced to shut down!\n", index);
            (sakura_void_t)sakura_mqtt_close_internal(index);
        }
    }
    return SAKURA_MQTT_STAT_OK;
}

static sakura_int32_t mqtt_client_id_check(const sakura_char_t* client_id)
{
    sakura_int32_t ret = SAKURA_MQTT_STAT_OK;
    sakura_uint32_t i = 0;
    do
    {
        /* check client_id */
        if (client_id == NULL) {
            CLIENT_LOGE("NULL client_id\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* check client_id length */
        if (strlen(client_id) >= MAX_CLIENT_ID_STR_LEN) {
            CLIENT_LOGE("too long client_id\n");
            ret = SAKURA_MQTT_ERROR;
            break;
        }

        /* check used client. */
        for (i = 0; i < global_mqtt_handle.mqtt_client_num; i++)
        {
            if(global_mqtt_handle.mqtt_clients[i].state != MQTT_CLIENT_FREE){
                if(strcmp(global_mqtt_handle.mqtt_clients[i].client_id, client_id) == 0){
                    /* name is repeated. */
                    ret = SAKURA_MQTT_ERROR;
                    break;
                }
            }
        }        
    } while (SAKURA_FALSE);

    return ret;
}

static sakura_uint32_t mqtt_get_service_client_number(sakura_void_t)
{
    sakura_uint32_t i;
    sakura_uint32_t client_num = 0;
    for (i = 0; i < global_mqtt_handle.mqtt_client_num; i++)
    {
        /* set bit in byte */
        if(global_mqtt_handle.mqtt_clients[i].state != MQTT_CLIENT_FREE){
            client_num++;
        }
    }
    return client_num;
}

static sakura_void_t mqtt_cleanup_sub_module(sakura_void_t)
{
#ifdef CONFIG_SYS_UNIX
    /* dns cleanup */
    sakura_dns_cleanup();
#endif
    
    /* close all clients */
    (sakura_void_t)mqtt_close_all_clients();

    /* socket cleanup */
    (sakura_void_t)sakura_socket_cleanup();

    /* global pointer release */
    if(global_mqtt_handle.mqtt_clients != NULL){
        sakura_free(global_mqtt_handle.mqtt_clients);
        global_mqtt_handle.mqtt_clients = NULL;
    }
    if(global_mqtt_handle.reconnect_handle != NULL){
        sakura_free(global_mqtt_handle.reconnect_handle);
        global_mqtt_handle.reconnect_handle = NULL;
    }

#ifdef CONFIG_LOG
    (void)sakura_log_deinit();
#endif

#ifdef CONFIG_IMPL_STATIC_MEMORY
    sakura_mem_cleanup();
#endif

    /* clear handle */
    memset(&global_mqtt_handle, 0, sizeof(global_mqtt_handle));
}