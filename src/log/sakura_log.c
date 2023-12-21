#include <stdio.h>
#include <stdarg.h> // va_List
#include <sys/time.h>
#include "sakura_log.h"
#include "sakura_log_utils.h"
#include "pthread.h"

#define LOG_TAG "LOG_CORE"

#define LOG_TICK_TIME_INTERVAL      (3U * 60U * 1000U)  //3mins
#define LOG_LEVEL_NUM               7
#define LOG_MONTH_NUM               48

typedef struct {
    SAKURA_LOG_LEVEL level;
    const sakura_char_t *level_str;
    const sakura_char_t *color_str;
} log_level_info_t;

/* calendar */
typedef struct {
    sakura_uint16_t year;
    sakura_uint8_t  month;
    sakura_uint8_t  day;
    sakura_uint8_t  hour;
    sakura_uint8_t  minute;
    sakura_uint8_t  second;
} sakura_calendar_t;

/* glob log handle */
static struct {
    log_level_info_t level_info[LOG_LEVEL_NUM];
    const sakura_int32_t month_day[LOG_MONTH_NUM];
    struct timeval timeval;
    sakura_sdk_log_conf_t default_conf_info;
    sakura_sdk_log_conf_t user_conf_info;
    sakura_bool_t user_set_flag;
}glob_log_handle = {
    /* level info */
    {
        {LOG_LEVEL_NONE,    "[N]", "\e[0m"},  // reserved, recover color
        {LOG_LEVEL_FATAL,   "[F]", "\e[31m"}, // red:31
        {LOG_LEVEL_ERROR,   "[E]", "\e[31m"}, // red:31
        {LOG_LEVEL_WARNING, "[W]", "\e[33m"}, // yellow:33
        {LOG_LEVEL_INFO,    "[I]", "\e[32m"}, // green:32
        {LOG_LEVEL_DEBUG,   "[D]", "\e[37m"}, // black:30, white:37
        {LOG_LEVEL_VERBOSE, "[V]", "\e[34m"}, // blue:34
    },
    /* month days */
    {
        31,28,31,30,31,30,31,31,30,31,30,31,
        31,28,31,30,31,30,31,31,30,31,30,31,
        31,29,31,30,31,30,31,31,30,31,30,31,
        31,28,31,30,31,30,31,31,30,31,30,31
     },
    /* time */
    {0},
    /* default log config information */
    {
        #ifdef SAKURA_RELEASE
            LOG_LEVEL_NONE,
        #else
            LOG_LEVEL_DEBUG,
        #endif
            SAKURA_TRUE,
        #ifdef CONFIG_SYS_UNIX
            sakura_unix_printf
        #else
            NULL
        #endif
    },
    /* user log config information */
    {0},
    /* user set config flag */
    SAKURA_FALSE
};

#ifdef CONFIG_SYS_UNIX
static pthread_mutex_t glob_log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


static sakura_void_t log_output_console(SAKURA_LOG_LEVEL level, sakura_char_t *str_time, sakura_char_t *str_func, const sakura_char_t* func_name, sakura_char_t* str_content);
static const sakura_char_t *sakura_get_calendar_str(void);
static sakura_calendar_t sakura_time_utc2calendar(sakura_uint64_t utc_sec);
static sakura_uint64_t sakura_get_timestamp(void);
static sakura_void_t gen_ts_func_to_output(SAKURA_LOG_LEVEL level, const sakura_char_t *component, const sakura_char_t* func_name, sakura_char_t* str_content);

/**
 * @brief Set log config.
 *
 * @param conf Log config.
 * @return sakura_int32_t Result of set config.
 */
sakura_int32_t sakura_set_log_config(sakura_sdk_log_conf_t *conf)
{
    sakura_int32_t ret = LOG_SUCCESS;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_log_mutex);
#endif
    do {
        /* check params. */
        if (conf == NULL) {
            /* conf is invalid, return LOG_ERROR_INVALID_PARAM. */
            ret = LOG_ERROR_INVALID_PARAM;
            break;
        }

        /* Store conf into s_log_conf_info. */
        memset(&glob_log_handle.user_conf_info, 0, sizeof(sakura_sdk_log_conf_t));
        memmove(&glob_log_handle.user_conf_info, conf, sizeof(sakura_sdk_log_conf_t));
        glob_log_handle.user_set_flag = SAKURA_TRUE;
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_log_mutex);
#endif
    return ret;
}

#ifdef CONFIG_SYS_UNIX
/**
 * @brief used unix interface print log string
 *
 * @param[in] log_str log string
 * @return sakura_int32_t returns the number of characters written if successful, otherwise returns a negative number
 */
sakura_int32_t sakura_unix_printf(const sakura_char_t* log_str)
{
    /* check string */
    if(log_str != NULL){
        (sakura_void_t)printf("%s", log_str);  
    }
    return LOG_SUCCESS;
}
#endif

/**
 * @brief get config level
 * 
 * @return sakura_uint32_t 
 */
sakura_uint32_t log_config_get_level()
{
    return glob_log_handle.default_conf_info.level;
}

/**
 * @brief log update timestamp
 * 
 * @return sakura_void_t 
 */
sakura_void_t sakura_log_tick(sakura_void_t)
{
#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_log_mutex);
#endif
    /* update global time */
    gettimeofday(&glob_log_handle.timeval, NULL);
#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_log_mutex);
#endif
}


/**
 * @brief handler log, print on console and output to file.
 *
 * @param level log level.
 * @param component log component.
 * @param format log format.
 * @param ...
 * @return sakura_void_t
 */
sakura_void_t sakura_log(SAKURA_LOG_LEVEL level, const sakura_char_t *component, const sakura_char_t* func_name, const sakura_char_t *format, ...)
{
    sakura_char_t str_content[LOG_CONTENT_MAX_LEN + 1] = { 0 };

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_log_mutex);
#endif
    do {
        /* The log level to be printed is below the specified level. */
        if ((sakura_uint32_t)level <= glob_log_handle.default_conf_info.level) {
            va_list args;
            va_start(args, format);

            /* get print content. */
            (sakura_void_t)vsnprintf(str_content, LOG_CONTENT_MAX_LEN, format, args);

            /** There is no line break at the end, Add newlines and terminators. */
            if (strlen(str_content) < (sakura_uint32_t)LOG_CONTENT_MAX_LEN && str_content[strlen(str_content) - (sakura_uint32_t)1] != '\n') {
                str_content[strlen(str_content)] = '\n';
                str_content[strlen(str_content) + (sakura_uint32_t)1] = '\0';
            }

            gen_ts_func_to_output(level, component, func_name, str_content);
            va_end(args);
        }
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_log_mutex);
#endif
}

/**
 * @brief log module init.
 *
 * @return sakura_int32_t Result of init.
 */
sakura_int32_t sakura_log_init(sakura_void_t)
{
    sakura_int32_t ret = LOG_SUCCESS;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_log_mutex);
#endif
    do {
        /* set user conf */
        if(glob_log_handle.user_set_flag == SAKURA_TRUE){
            memset(&glob_log_handle.default_conf_info, 0, sizeof(sakura_sdk_log_conf_t));
            memmove(&glob_log_handle.default_conf_info, &glob_log_handle.user_conf_info, sizeof(sakura_sdk_log_conf_t));
        }
    } while (SAKURA_FALSE);

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_log_mutex);
#endif
    return ret;
}

/**
 * @brief deinit log module.
 *
 * @return sakura_int32_t Result of deinit.
 */
sakura_int32_t sakura_log_deinit(sakura_void_t)
{
    /** Clears global cache variables. */
    sakura_int32_t ret = LOG_SUCCESS;

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_lock(&glob_log_mutex);
#endif
    /* reset log level */
#ifdef SAKURA_RELEASE
    glob_log_handle.default_conf_info.level = LOG_LEVEL_NONE;
#else
    glob_log_handle.default_conf_info.level = LOG_LEVEL_DEBUG;
#endif

    /* reset log is_color_console */
    glob_log_handle.default_conf_info.is_color_console = SAKURA_TRUE;

    /* reset log print_function */
#ifdef CONFIG_SYS_UNIX
    glob_log_handle.default_conf_info.print_function = sakura_unix_printf;
#else
    glob_log_handle.default_conf_info.print_function = NULL;
#endif

#ifdef CONFIG_SYS_UNIX
    (sakura_void_t)pthread_mutex_unlock(&glob_log_mutex);
#endif
    return ret;
}

/**
 * @brief Get current calendar string
 *
 * @param[in]  void
 *
 * @return current calendar string
 */
static const sakura_char_t *sakura_get_calendar_str(void)
{
    static sakura_char_t cal_str[24] = {0};
    sakura_calendar_t calendar = {0};

    /* get calendar in second */
    calendar = sakura_time_utc2calendar(sakura_get_timestamp() / 1000U /* in second */);

    /* get calendar string */
    (sakura_void_t)snprintf(cal_str, sizeof(cal_str), "%04d-%02d-%02d %02d:%02d:%02d",
        calendar.year,
        calendar.month,
        calendar.day,
        calendar.hour,
        calendar.minute,
        calendar.second
    );

    return cal_str;
}


/**
 * @brief convert utc time to calendar
 *
 * @param[in] utc_sec  the utc time
 *
 * @return  the calendar structure
 */
static sakura_calendar_t sakura_time_utc2calendar(sakura_uint64_t utc_sec)
{
    /* init variable */
    sakura_calendar_t calendar = {0};
    sakura_int32_t days = 0, sec = 0, mon = 0, day = 0;
    sakura_uint64_t days_u64 = 0, sec_u64 = 0;

    /* leap year if year%4==0 in 1901-2099 */
    days_u64 = utc_sec / 86400U;
    days = (sakura_int32_t)days_u64;
    /* get remaining seconds */
    sec_u64 = utc_sec - (sakura_uint64_t)days * 86400u;
    sec = (sakura_int32_t)sec_u64;

    /* calculation day and month */
    day = days % 1461;
    mon = 0;
    /* get real month and remaining days */
    while (day >= glob_log_handle.month_day[mon]) {
        day -= glob_log_handle.month_day[mon];
        mon++;
    }

    /* set date to calendar */
    calendar.year   = 1970 + days / 1461 * 4 + mon / 12;
    calendar.month  = mon % 12 + 1;
    calendar.day    = day + 1;
    calendar.hour   = sec / 3600;
    calendar.minute = sec % 3600 / 60;
    calendar.second = sec % 60;

    return calendar;
}

/**
 * @brief Get the timestamp. m0If we could get the time from the system, we should get it from the system.
 * get the timestamp calculated from the reference timestamp and time shift in millisecond.
 *
 * @param[in]  void
 *
 * @return  the timestamp from server.
 */
static sakura_uint64_t sakura_get_timestamp(void)
{
    /* get timestamp in millisecond */
    if( glob_log_handle.timeval.tv_sec == 0 &&
        glob_log_handle.timeval.tv_usec == 0 ) {
        gettimeofday(&glob_log_handle.timeval, NULL);
    }
    return glob_log_handle.timeval.tv_sec * 1000U;
}

/**
 * @brief output log to console.
 *
 * @param level log level.
 * @param str_time log time.
 * @param str_func module/func/line.
 * @param str_content log content.
 * @return sakura_void_t
 */
static sakura_void_t log_output_console(SAKURA_LOG_LEVEL level, sakura_char_t *str_time, sakura_char_t *str_func, const sakura_char_t* func_name, sakura_char_t* str_content)
{
    sakura_char_t str_log[LOG_PRINT_MAX_LEN] = { 0 };

    /* check params. */
    if (NULL != str_time && NULL != str_func && NULL != str_content) {
        /* Console printing requires color. */
        if (glob_log_handle.default_conf_info.is_color_console == SAKURA_TRUE) {
            if(level == LOG_LEVEL_ERROR){
                (sakura_void_t)sprintf(str_log, "%s[%s] %s %s [%s] %s%s", glob_log_handle.level_info[level].color_str,
                str_time, glob_log_handle.level_info[level].level_str,
                str_func, func_name, str_content, glob_log_handle.level_info[LOG_LEVEL_NONE].color_str); 
            } else {
                (sakura_void_t)sprintf(str_log, "%s[%s] %s %s %s%s", glob_log_handle.level_info[level].color_str,
                str_time, glob_log_handle.level_info[level].level_str,
                str_func, str_content, glob_log_handle.level_info[LOG_LEVEL_NONE].color_str); 
            }
        } else {
            if(level == LOG_LEVEL_ERROR){
                (sakura_void_t)sprintf(str_log, "[%s] %s %s [%s] %s", str_time, glob_log_handle.level_info[level].level_str,
                str_func, func_name, str_content);
            } else {
                (sakura_void_t)sprintf(str_log, "[%s] %s %s %s", str_time, glob_log_handle.level_info[level].level_str,
                str_func, str_content);  
            }
        }

        /* print log. */
        if(glob_log_handle.default_conf_info.print_function != NULL){
            (sakura_void_t)glob_log_handle.default_conf_info.print_function(str_log);
        }
    }
}

/**
 * @brief gen timestamp and func, output to file or console.
 *
 * @param level log level.
 * @param component component.
 * @return sakura_void_t
 */
static sakura_void_t gen_ts_func_to_output(SAKURA_LOG_LEVEL level, const sakura_char_t *component, const sakura_char_t* func_name, sakura_char_t* str_content) 
{
    sakura_char_t str_time[LOG_HEADER_MAX_LEN] = { 0 };
    sakura_char_t str_func[LOG_CONTENT_MAX_LEN] = { 0 };
    const sakura_char_t *calendar_str = NULL;

    /* get cur timestamp string */
    calendar_str = sakura_get_calendar_str();
    if(calendar_str != NULL){
        memmove(str_time, calendar_str, strlen(calendar_str));    
    }
    /* get module/func/line string */
    (sakura_void_t)sprintf(str_func, "[%s]", component);
    /* output to console. */
    log_output_console(level, str_time, str_func, func_name, str_content);
}