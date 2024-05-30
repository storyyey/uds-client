#include "yapi_dm_simple.h"
#include "ydm_api.h"
#include "pthread.h"

YAPI_DM sg_yapi_dm__ = 0;
yint16 sg_udscid__ = -1;

static pthread_mutex_t sapi_mutex_lock = PTHREAD_MUTEX_INITIALIZER;
#define SAPI_LOCK     pthread_mutex_lock(&sapi_mutex_lock)
#define SAPI_UNLOCK   pthread_mutex_unlock(&sapi_mutex_lock)

/* 定义事件 */
void YEVENT(udsc_request_transfer)(YAPI_DM yapi, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype) { }
void YEVENT(udsc_task_end)(YAPI_DM yapi, yuint16 udscid, yuint32 rcode, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl) { }
void YEVENT(udsc_security_access_keygen)(YAPI_DM yapi, yuint16 udscid, yuint8 level, const yuint8 *seed, yuint32 seed_size) { }
void YEVENT(udsc_service_36_transfer_progress)(YAPI_DM yapi, yuint16 udscid, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time) { }
void YEVENT(diag_master_instance_destroy)(YAPI_DM yapi) { }

/* 连接事件和回调函数 */
void yevent_connect(YAPI_DM yapi, yuint16 udscid, void *event, void *callback)
{
    if (event == YEVENT(udsc_request_transfer)) {
        ydm_api_udsc_request_transfer_callback_set(yapi, udscid, callback, yapi);
    }
    else if (event == YEVENT(udsc_task_end)) {
        ydm_api_udsc_task_end_callback_set(yapi, udscid, callback, yapi);
    }
    else if (event == YEVENT(udsc_security_access_keygen)) {
        ydm_api_udsc_security_access_keygen_callback_set(yapi, udscid, callback, yapi);
    }    
    else if (event == YEVENT(udsc_service_36_transfer_progress)) {
        ydm_api_udsc_service_36_transfer_progress_callback_set(yapi, udscid, callback, yapi);
    }
    else if (event == YEVENT(diag_master_instance_destroy)) {
        ydm_api_diag_master_instance_destroy_callback_set(yapi, callback, yapi);
    }
    else {
        /* 未知事件 */
    }
}

/* 连接事件和回调函数 */
void syevent_connect(void *event, void *callback)
{
    if (event == YEVENT(udsc_request_transfer)) {
        ydm_api_udsc_request_transfer_callback_set(sg_yapi_dm__, sg_udscid__, callback, sg_yapi_dm__);
    }
    else if (event == YEVENT(udsc_task_end)) {
        ydm_api_udsc_task_end_callback_set(sg_yapi_dm__, sg_udscid__, callback, sg_yapi_dm__);
    }
    else if (event == YEVENT(udsc_security_access_keygen)) {
        ydm_api_udsc_security_access_keygen_callback_set(sg_yapi_dm__, sg_udscid__, callback, sg_yapi_dm__);
    }
    else if (event == YEVENT(udsc_service_36_transfer_progress)) {
        ydm_api_udsc_service_36_transfer_progress_callback_set(sg_yapi_dm__, sg_udscid__, callback, sg_yapi_dm__);
    }    
    else if (event == YEVENT(diag_master_instance_destroy)) {
        ydm_api_diag_master_instance_destroy_callback_set(sg_yapi_dm__, callback, sg_yapi_dm__);
    }
    else {
        /* 未知事件 */
    }
}

/* 获取api调用出错时的错误码 */
int yapi_dm_errcode(YAPI_DM yapi)
{
    return ydm_api_errcode(yapi);
}

/* 获取错误码描述 */
char *yapi_dm_errcode_str(int errcode)
{
    return ydm_api_errcode_str(errcode);
}

/* 打开或者关闭调试信息默认关闭 */
void yapi_dm_debug_enable(int eb)
{
    return ydm_debug_enable(eb);
}

/* 创建初始化OTA MASTER API */
YAPI_DM yapi_dm_create()
{
    return ydm_api_create();
}

int syapi_dm_create()
{
    SAPI_LOCK;
    if (sg_yapi_dm__ == 0) {
        sg_yapi_dm__ = ydm_api_create();
    }
    SAPI_UNLOCK;

    return sg_yapi_dm__ ? 0 : -1;
}

YAPI_DM syapi_dm()
{
    return sg_yapi_dm__;
}

/* 判断OTA MASTER API是否有效返回true表示有效 */
int yapi_dm_is_valid(YAPI_DM yapi)
{
    return ydm_api_is_valid(yapi);
}

int syapi_dm_is_valid()
{
    return ydm_api_is_valid(sg_yapi_dm__);
}

/* 销毁OTA MASTER API */
void yapi_dm_destroy(YAPI_DM yapi)
{
    return ydm_api_destroy(yapi);
}

void syapi_dm_destroy()
{
    SAPI_LOCK;
    ydm_api_destroy(sg_yapi_dm__);
    sg_yapi_dm__ = 0;
    SAPI_UNLOCK;
}

/* 获取和OTA MASTER进程间通信的socket，可以用于监听是否有进程通信数据 */
int yapi_dm_ipc_channel_fd(YAPI_DM yapi)
{
    return ydm_api_sockfd(yapi);
}

int syapi_dm_ipc_channel_fd()
{
    return ydm_api_sockfd(sg_yapi_dm__);
}

/* 请求OTA MASTER创建一个UDS客户端，成功返回非负数UDS客户端ID，失败返回-1 */
int yapi_dm_udsc_create(YAPI_DM yapi, udsc_create_config *config)
{
    return ydm_api_udsc_create(yapi, config);
}

int syapi_dm_udsc_create(udsc_create_config *config)
{
    SAPI_LOCK;
    if (sg_udscid__ < 0) {
        sg_udscid__ = ydm_api_udsc_create(sg_yapi_dm__, config);
        sg_udscid__ = sg_udscid__ < 0 ? -1 : sg_udscid__;
    }
    SAPI_UNLOCK;

    return sg_udscid__ >= 0 ? 0 : -1;
}

int syapi_dm_udsc()
{
    return sg_udscid__;
}

/* 请求OTA MASTER销毁一个UDS客户端 */
int yapi_dm_udsc_destroy(YAPI_DM yapi, yuint16 udscid)
{
    return ydm_api_udsc_destroy(yapi, udscid);
}

int syapi_dm_udsc_destroy()
{
    int re = 0;

    SAPI_LOCK;
    re = ydm_api_udsc_destroy(sg_yapi_dm__, sg_udscid__);
    sg_udscid__ = -1;
    SAPI_UNLOCK;
        
    return re; 
}

/* 请求OTA MASTER创建一个DOIP客户端，并将UDS客户端与DOIP客户端绑定,
   成功返回非负数UDS客户端ID，UDS客户端销毁的同时将销毁DOIP客户端，失败返回-1 */
int yapi_dm_doipc_create(YAPI_DM yapi, yuint16 udscid, doipc_config_t *config)
{
    return ydm_api_doipc_create(yapi, udscid, config);
}

int syapi_dm_doipc_create(doipc_config_t *config)
{
    return ydm_api_doipc_create(sg_yapi_dm__, sg_udscid__, config);
}

/*  请求OTA MASTER启动UDS 客户端开始执行诊断任务 */
int yapi_dm_udsc_start(YAPI_DM yapi, yuint16 udscid)
{
    return ydm_api_udsc_start(yapi, udscid);
}

int syapi_dm_udsc_start()
{
    return ydm_api_udsc_start(sg_yapi_dm__, sg_udscid__);
}

/* 获取指定UDS客户端是否正在执行任务返回true表示任务正在执行中 */
int yapi_dm_udsc_is_active(YAPI_DM yapi, yuint16 udscid)
{
    return ydm_api_udsc_is_active(yapi, udscid);
}

int syapi_dm_udsc_is_active()
{
    return ydm_api_udsc_is_active(sg_yapi_dm__, sg_udscid__);
}

/*  请求OTA MASTER中止UDS 客户端执行诊断任务 */
int yapi_dm_udsc_stop(YAPI_DM yapi, yuint16 udscid)
{
    return ydm_api_udsc_stop(yapi, udscid);
}

int syapi_dm_udsc_stop()
{
    return ydm_api_udsc_stop(sg_yapi_dm__, sg_udscid__);
}

/* 复位OTA MASTER防止OTA MASTER存在异常数据 */
int yapi_dm_master_reset(YAPI_DM yapi)
{
    return ydm_api_master_reset(yapi);
}

int syapi_dm_master_reset()
{
    return ydm_api_master_reset(sg_yapi_dm__);
}

/* 配置OTA MASTER中的UDS诊断刷写任务 */
int yapi_dm_udsc_service_add(YAPI_DM yapi, yuint16 udscid, service_config *config, uds_service_result_callback call)
{
    if (config) {
        if (call) {
            /* 注册结果处理函数 */
            config->rr_callid = ydm_api_udsc_service_result_callback_set(yapi, udscid, call, yapi);
        }
        else {
            config->rr_callid = 0;
        }
    }

    return ydm_api_udsc_service_config(yapi, udscid, config);
}

int syapi_dm_udsc_service_add(service_config *config, uds_service_result_scallback call)
{
    if (config) {
        if (call) {
            /* 注册结果处理函数 */
            config->rr_callid = ydm_api_udsc_service_result_callback_set(sg_yapi_dm__, sg_udscid__, call, sg_yapi_dm__);
        }
        else {
            config->rr_callid = 0;
        }
    }

    return ydm_api_udsc_service_config(sg_yapi_dm__, sg_udscid__, config);
}

/* UDS客户端的一些通用设置  包括3E服务、任务执行动作等配置 */
int yapi_dm_udsc_runtime_config(YAPI_DM yapi, yuint16 udscid, udsc_runtime_config *config)
{
    return ydm_api_udsc_runtime_config(yapi, udscid, config);
}

int syapi_dm_udsc_runtime_config(udsc_runtime_config *config)
{
    return ydm_api_udsc_runtime_config(sg_yapi_dm__, sg_udscid__, config);
}

/* 将生成的key发送给ota master */
int yapi_dm_udsc_sa_key(YAPI_DM yapi, yuint16 udscid, yuint8 level, const yuint8 *key, yuint32 key_size)
{
    return ydm_api_udsc_sa_key(yapi, udscid, level, key, key_size);
}

int syapi_dm_udsc_sa_key(yuint8 level, const yuint8 *key, yuint32 key_size)
{
    return ydm_api_udsc_sa_key(sg_yapi_dm__, sg_udscid__, level, key, key_size);
}

/* UDS的诊断响应报文发送给OTA MASTER中的UDS客户端处理 */
int yapi_dm_service_response(YAPI_DM yapi, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype)
{
    return ydm_api_service_response(yapi, udscid, data, size, sa, ta, tatype);
}

int syapi_dm_service_response(const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype)
{
    return ydm_api_service_response(sg_yapi_dm__, sg_udscid__, data, size, sa, ta, tatype);
}

/* 设置单个诊断任务请求响应结果的回调处理函数 */  
int yapi_om_udsc_service_result_callback_set(YAPI_DM yapi, yuint16 udscid, uds_service_result_callback call, void *arg)
{
    return ydm_api_udsc_service_result_callback_set(yapi, udscid, call, arg);
} 

int syapi_om_udsc_service_result_callback_set(uds_service_result_callback call, void *arg)
{
    return ydm_api_udsc_service_result_callback_set(sg_yapi_dm__, sg_udscid__, call, arg);
} 

/* 保存诊断请求和响应ID/地址 */  
int yapi_dm_udsc_diag_id_storage(YAPI_DM yapi, yuint16 udscid, yuint32 diag_req_id, yuint32 diag_resp_id)
{
    return ydm_api_udsc_diag_id_storage(yapi, udscid, diag_req_id, diag_resp_id);
}

int syapi_dm_udsc_diag_id_storage(yuint32 diag_req_id, yuint32 diag_resp_id)
{
    return ydm_api_udsc_diag_id_storage(sg_yapi_dm__, sg_udscid__, diag_req_id, diag_resp_id);
}
  
/* 通过诊断响应ID/地址查找UDS客户端ID */  
int yapi_dm_udsc_index_by_resp_id(YAPI_DM yapi, yuint32 diag_resp_id)
{
    return ydm_api_udsc_index_by_resp_id(yapi, diag_resp_id);
}

int syapi_dm_udsc_index_by_resp_id(yuint32 diag_resp_id)
{
    return ydm_api_udsc_index_by_resp_id(sg_yapi_dm__, diag_resp_id);
}
  
/* 通过诊断请求ID/地址查找UDS客户端ID */  
int yapi_dm_udsc_index_by_req_id(YAPI_DM yapi, yuint32 diag_req_id)
{
    return ydm_api_udsc_index_by_req_id(yapi, diag_req_id);
}

int syapi_dm_udsc_index_by_req_id(yuint32 diag_req_id)
{
    return ydm_api_udsc_index_by_req_id(sg_yapi_dm__, diag_req_id);
}
  
/* 通过UDS客户端ID查找诊断响应ID/地址 */  
yuint32 yapi_dm_udsc_resp_id(YAPI_DM yapi, yuint16 udscid)
{
    return ydm_api_udsc_resp_id(yapi, udscid);
}

yuint32 syapi_dm_udsc_resp_id()
{
    return ydm_api_udsc_resp_id(sg_yapi_dm__, sg_udscid__);
}

/* 通过UDS客户端ID查找诊断请求ID/地址 */  
yuint32 yapi_dm_udsc_req_id(YAPI_DM yapi, yuint16 udscid)
{
    return ydm_api_udsc_req_id(yapi, udscid);
}

yuint32 syapi_dm_udsc_req_id()
{
    return ydm_api_udsc_req_id(sg_yapi_dm__, sg_udscid__);
}

/* 获取UDS客户端运行数据 */
int yapi_dm_udsc_rundata_statis(YAPI_DM yapi, yuint16 udscid, runtime_data_statis_t *statis)
{
    return ydm_api_udsc_rundata_statis(yapi, udscid, statis);
}

int syapi_dm_udsc_rundata_statis(runtime_data_statis_t *statis)
{
    return ydm_api_udsc_rundata_statis(sg_yapi_dm__, sg_udscid__, statis);
}

/* 用于和OTA MASTER进行进程间通信建议和yapi_om_sockfd() 和select epoll 函数配合使用 */  
void yapi_dm_event_loop(YAPI_DM yapi)
{
    return ydm_api_request_event_loop(yapi);
}

void syapi_dm_event_loop()
{
    return ydm_api_request_event_loop(sg_yapi_dm__);
}

// 创建一个新的YByteArray对象并返回其指针  
ybyte_array *yapi_byte_array_new()
{
    return y_byte_array_new();
}
  
// 释放YByteArray对象所占用的内存  
void yapi_byte_array_delete(ybyte_array *arr)
{
    return y_byte_array_delete(arr);
}
  
// 清空YByteArray对象的内容  
void yapi_byte_array_clear(ybyte_array *arr)
{
    return y_byte_array_clear(arr);
}
  
// 返回YByteArray对象的常量数据指针  
const yuint8 *yapi_byte_array_const_data(ybyte_array *arr)
{
    return y_byte_array_const_data(arr);
}
  
// 返回YByteArray对象中的元素数量  
int yapi_byte_array_count(ybyte_array *arr)
{
    return y_byte_array_count(arr);
}
  
// 在YByteArray对象末尾添加一个字符  
void yapi_byte_array_append_char(ybyte_array *dest, yuint8 c)
{
    return y_byte_array_append_char(dest, c);
}
  
// 将一个YByteArray对象的内容追加到另一个YByteArray对象中  
void yapi_byte_array_append_array(ybyte_array *dest, ybyte_array *src)  
{
    return y_byte_array_append_array(dest, src);
}
  
// 在YByteArray对象末尾追加一个字符数组的内容  
void yapi_byte_array_append_nchar(ybyte_array *dest, yuint8 *c, yuint32 count)
{
    return y_byte_array_append_nchar(dest, c, count);
}
  
// 比较两个YByteArray对象是否相等  
int yapi_byte_array_equal(ybyte_array *arr1, ybyte_array *arr2)
{
    return y_byte_array_equal(arr1, arr2);
}
  
// 比较一个YByteArray对象和一个字符数组的内容是否相等  
int yapi_byte_array_char_equal(ybyte_array *arr1, yuint8 *c, yuint32 count)
{
    return y_byte_array_char_equal(arr1, c, count);
}

int yapi_om_log_print(enum yapi_log_level level, int (*printf)(const char *format, ...))
{
    switch (level) {
        case YAPI_LOG_DEBUG:
            return ydm_api_logd_print_set(printf);
        case YAPI_LOG_INFO:
            return ydm_api_logi_print_set(printf);
        case YAPI_LOG_WARNING:
            return ydm_api_logw_print_set(printf);
        case YAPI_LOG_ERROR:
            return ydm_api_loge_print_set(printf);
        case YAPI_LOG_ALL:
            ydm_api_logd_print_set(printf);
            ydm_api_logi_print_set(printf);
            ydm_api_logw_print_set(printf);
            ydm_api_loge_print_set(printf);
            break;
        default:
            break;
    }

    return -1;
}

