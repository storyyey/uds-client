#ifndef __YDM_API_H__
#define __YDM_API_H__

#include "ycommon_type.h"

struct ydiag_master_api;
typedef struct ydiag_master_api ydiag_master_api;

/*  
 * 当UDS客户端需要发送诊断请求数据时，它会调用此回调函数。  
 * 这个回调函数由ota master api调用者使用，用于通过onip或oncan发送诊断请求。  
 * @param arg 用户数据
 * @param udscid uds客户端ID
 * @param data 诊断消息
 * @param size 诊断消息长度
 * @param sa 诊断消息源地址
 * @param ta 诊断消息目的地址
 * @param tatype 目的地址类型 phy/func
 */
typedef void (*udsc_request_transfer_callback)(void *arg, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);

/*  
 * 当所有UDS任务执行结束或者因出错导致的任务中断，会调用此回调函数。  
 * 此回调函数用于通知任务执行的结果状态。 
 * @param arg 用户数据
 * @param udscid uds客户端ID
 * @param rcode 结果状态码 0正常，其他错误
 * @param ind 诊断请求消息
 * @param indl 诊断请求消息长度
 * @param resp 诊断响应消息
 * @param respl 诊断响应消息长度
 */
typedef void (*udsc_task_end_callback)(void *arg, yuint16 udscid, yuint32 rcode, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl);

/*  
 * 单个UDS任务执行结束后，会调用此回调函数。  
 * 此回调函数用于通知单个UDS任务执行的结果状态。 
 * @param arg 用户数据
 * @param udscid uds客户端ID
 * @param data 诊断响应消息
 * @param size 诊断响应消息长度
 */
typedef void (*uds_service_result_callback)(void *arg, yuint16 udscid, const yuint8 *data, yuint32 size);

/*  
 * 当27服务获取到seed后，会回调此函数。  
 * ota master api调用者使用此函数生成密钥，并使用yom_api_udsc_sa_key函数设置相应的密钥。  
 * @param arg 用户数据
 * @param udscid uds客户端ID
 * @param level 安全等级，也就是请求种子时的子功能
 * @param seed 种子
 * @param seed_size 种子长度
 */
typedef void (*udsc_security_access_keygen_callback)(void *arg, yuint16 udscid, yuint8 level, const yuint8 *seed, yuint32 seed_size);

typedef void (*uds_service_36_transfer_progress_callback)(void *arg, yuint16 udscid, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time);

typedef void (*diag_master_instance_destroy_callback)(void *arg/* 用户数据指针 */);

/*
    这些接口用于控制OTA Master 做出相应动作
*/
/* ---------- DIAG master api 所有的接口均为同步接口，会阻塞 --------- */

/* 获取api调用出错时的错误码 */
extern int ydm_api_errcode(ydiag_master_api *dm_api);

/* 获取错误码描述 */
extern char *ydm_api_errcode_str(int errcode);

/* 打开或者关闭调试信息默认关闭 */
extern void ydm_debug_enable(int eb);

/* 创建初始化DIAG MASTER API */
extern ydiag_master_api *ydm_api_create();

/* 判断DIAG MASTER API是否有效返回true表示有效 */
extern int ydm_api_is_valid(ydiag_master_api *dm_api);

/* 销毁DIAG MASTER API */
extern int ydm_api_destroy(ydiag_master_api *dm_api);

/* 获取和DIAG MASTER进程间通信的socket，可以用于监听是否有进程通信数据 */
extern int ydm_api_sockfd(ydiag_master_api *oa_api);

/* 请求DIAG MASTER创建一个UDS客户端，成功返回非负数UDS客户端ID，失败返回-1 */
extern int ydm_api_udsc_create(ydiag_master_api *dm_api, udsc_create_config *config);

/* 请求DIAG MASTER销毁一个UDS客户端 */
extern int ydm_api_udsc_destroy(ydiag_master_api *dm_api, yuint16 udscid);

/* 请求DIAG MASTER创建一个DOIP客户端，并将UDS客户端与DOIP客户端绑定,
   成功返回非负数UDS客户端ID，UDS客户端销毁的同时将销毁DOIP客户端，失败返回-1 */
extern int ydm_api_doipc_create(ydiag_master_api *dm_api, yuint16 udscid, doipc_config_t *config);

/* 请求DIAG MASTER启动UDS 客户端开始执行诊断任务 */
extern int ydm_api_udsc_start(ydiag_master_api *dm_api, yuint16 udscid);

/* 获取指定UDS客户端是否正在执行任务返回true表示任务正在执行中 */
extern int ydm_api_udsc_is_active(ydiag_master_api *dm_api, yuint16 udscid);

/*  请求DIAG MASTER中止UDS 客户端执行诊断任务 */
extern int ydm_api_udsc_stop(ydiag_master_api *dm_api, yuint16 udscid);    

/* 复位DIAG MASTER防止DIAG MASTER存在异常数据 */
extern int ydm_api_master_reset(ydiag_master_api *dm_api);

/* 配置DIAG MASTER中的UDS诊断刷写任务 */
extern int ydm_api_udsc_service_config(ydiag_master_api *dm_api, yuint16 udscid, service_config *config);

/* UDS客户端的一些通用设置  包括3E服务、任务执行动作等配置 */
extern int ydm_api_udsc_runtime_config(ydiag_master_api *dm_api, yuint16 udscid, udsc_runtime_config *config);

/* 将生成的key发送给ota master */
extern int ydm_api_udsc_sa_key(ydiag_master_api *dm_api, yuint16 udscid, yuint8 level, const yuint8 *key, yuint32 key_size);

/* UDS的诊断响应报文发送给DIAG MASTER中的UDS客户端处理 */
extern int ydm_api_service_response(ydiag_master_api *dm_api, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);

/* 设置DIAG MASTER有诊断请求需要发送时候的诊断请求发送回调函数 */
extern int ydm_api_udsc_request_transfer_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_request_transfer_callback call, void *arg);

/* 设置DIAG MASTER中诊断客户端所有请求任务都执行完成后结果处理回调函数 */  
extern int ydm_api_udsc_task_end_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_task_end_callback call, void *arg);  
  
/* 设置单个诊断任务请求响应结果的回调处理函数 */  
extern int ydm_api_udsc_service_result_callback_set(ydiag_master_api *dm_api, yuint16 udscid, uds_service_result_callback call, void *arg);  
  
/* 通过27服务获取的种子生成key的回调函数设置 */  
extern int ydm_api_udsc_security_access_keygen_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_security_access_keygen_callback call, void *arg);  

extern int ydm_api_udsc_service_36_transfer_progress_callback_set(ydiag_master_api *dm_api, yuint16 udscid, uds_service_36_transfer_progress_callback call, void *arg);

extern int ydm_api_diag_master_instance_destroy_callback_set(ydiag_master_api *dm_api, diag_master_instance_destroy_callback call, void *arg);

/* 保存诊断请求和响应ID/地址 */  
extern int ydm_api_udsc_diag_id_storage(ydiag_master_api *dm_api, yuint16 udscid, yuint32 diag_req_id, yuint32 diag_resp_id);  
  
/* 通过诊断响应ID/地址查找UDS客户端ID */  
extern int ydm_api_udsc_index_by_resp_id(ydiag_master_api *dm_api, yuint32 diag_resp_id);  
  
/* 通过诊断请求ID/地址查找UDS客户端ID */  
extern int ydm_api_udsc_index_by_req_id(ydiag_master_api *dm_api, yuint32 diag_req_id);  
  
/* 通过UDS客户端ID查找诊断响应ID/地址 */  
extern yuint32 ydm_api_udsc_resp_id(ydiag_master_api *dm_api, yuint16 udscid);  
  
/* 通过UDS客户端ID查找诊断请求ID/地址 */  
extern yuint32 ydm_api_udsc_req_id(ydiag_master_api *dm_api, yuint16 udscid);  

/* 获取UDS客户端运行数据 */
extern int ydm_api_udsc_rundata_statis(ydiag_master_api *dm_api, yuint16 udscid, runtime_data_statis_t *statis);

/* 用于和DIAG MASTER进行进程间通信建议和yom_api_sockfd() 和select epoll 函数配合使用 */  
extern void ydm_api_request_event_loop(ydiag_master_api *dm_api);

/* 创建终端连接远程指令服务器 */
extern int ydm_api_terminal_control_service_create(ydiag_master_api *dm_api, terminal_control_service_info_t *trinfo);

// 创建一个新的YByteArray对象并返回其指针  
extern ybyte_array *y_byte_array_new();  
  
// 释放YByteArray对象所占用的内存  
extern void y_byte_array_delete(ybyte_array *arr);  
  
// 清空YByteArray对象的内容  
extern void y_byte_array_clear(ybyte_array *arr);  
  
// 返回YByteArray对象的常量数据指针  
extern const yuint8 *y_byte_array_const_data(ybyte_array *arr);  
  
// 返回YByteArray对象中的元素数量  
extern int y_byte_array_count(ybyte_array *arr);  
  
// 在YByteArray对象末尾添加一个字符  
extern void y_byte_array_append_char(ybyte_array *dest, yuint8 c);  
  
// 将一个YByteArray对象的内容追加到另一个YByteArray对象中  
extern void y_byte_array_append_array(ybyte_array *dest, ybyte_array *src);  
  
// 在YByteArray对象末尾追加一个字符数组的内容  
extern void y_byte_array_append_nchar(ybyte_array *dest, yuint8 *c, yuint32 count);  
  
// 比较两个YByteArray对象是否相等  
extern int y_byte_array_equal(ybyte_array *arr1, ybyte_array *arr2);  
  
// 比较一个YByteArray对象和一个字符数组的内容是否相等  
extern int y_byte_array_char_equal(ybyte_array *arr1, yuint8 *c, yuint32 count);

extern int ydm_api_logd_print_set(int (*printf)(const char *format, ...));

extern int ydm_api_loge_print_set(int (*printf)(const char *format, ...));

extern int ydm_api_logw_print_set(int (*printf)(const char *format, ...));

extern int ydm_api_logi_print_set(int (*printf)(const char *format, ...));

#endif /* __YDM_API_H__ */
