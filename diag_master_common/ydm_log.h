#ifndef __YLOG_H__
#define __YLOG_H__

typedef int (*ydm_log_print)(const char *format, ...); 

extern ydm_log_print g_ydm_logd_print;
extern ydm_log_print g_ydm_logw_print;
extern ydm_log_print g_ydm_loge_print;
extern ydm_log_print g_ydm_logi_print;

#define log_d(fmt, ...) do { \
                            if (g_ydm_logd_print) {\
                                g_ydm_logd_print(fmt"\n", ##__VA_ARGS__); \
                            }\
                        }while(0)
#define log_w(fmt, ...) do { \
                            if (g_ydm_logw_print) {\
                                g_ydm_logw_print(fmt"\n", ##__VA_ARGS__); \
                            }\
                        }while(0)
#define log_e(fmt, ...) do { \
                            if (g_ydm_loge_print) {\
                                g_ydm_loge_print(fmt"\n", ##__VA_ARGS__); \
                            }\
                        }while(0)
#define log_i(fmt, ...) do { \
                            if (g_ydm_logi_print) {\
                                g_ydm_logi_print(fmt"\n", ##__VA_ARGS__); \
                            }\
                        }while(0)

#ifdef __HAVE_ZLOG__
#include "zlog.h"

#define DM_LOG_CONF_FILE      "dm_log.conf"
#define DM_LOG_SAVE_DIR_DEF   "./ydm_log/"
#define DM_LOG_FILE_PREFIX    "ydm"
#define DM_LOG_FILE_FORMAT    DM_LOG_FILE_PREFIX"_%p_%d(%Y_%m_%d_%H).log"
#define DM_UDSC_MDC_KEY       "udsc"
#define DM_DIAG_MASTER_MDC_KEY "diag master"

#undef log_e
#define log_e(fmt, ...) dzlog_error(fmt, ##__VA_ARGS__)
#undef log_w
#define log_w(fmt, ...) dzlog_warn(fmt, ##__VA_ARGS__)
#undef log_d
#define log_d(fmt, ...) dzlog_debug(fmt, ##__VA_ARGS__)
#undef log_i
#define log_i(fmt, ...) dzlog_info(fmt, ##__VA_ARGS__)
#undef log_f
#define log_f(fmt, ...) dzlog_fatal(fmt, ##__VA_ARGS__)
#undef log_hex_e
#define log_hex_e(describe, msg, msglen) hdzlog_error(msg, msglen)
#undef log_hex_w
#define log_hex_w(describe, msg, msglen) hdzlog_warn(msg, msglen)
#undef log_hex_d
#define log_hex_d(describe, msg, msglen) hdzlog_debug(msg, msglen)
#undef log_hex_i
#define log_hex_i(describe, msg, msglen) hdzlog_info(msg, msglen)
#undef log_hex_f
#define log_hex_f(describe, msg, msglen) hdzlog_fatal(msg, msglen)

#define log_put_mdc(key, value) zlog_put_mdc(key, value); 

#define log_show(fmt, ...) dzlog_show(fmt, ##__VA_ARGS__)
#endif /* __HAVE_ZLOG__ */

#endif /* __YLOG_H__ */
