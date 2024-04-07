#ifndef __YUDS_CLIENT_H__
#define __YUDS_CLIENT_H__
/*
  1.添加的诊断服务项将按序保存在数组内 | 1.启动uds客户端执行任务                                                         |
    yudsc_service_item_add()         |   yudsc_services_start()                            |
    [ service 1 ]                      |                                                       |
          |                            | 2.uds客户端的运行主要依靠以下几个定时器                                |
          V                            |   request_watcher                                     |
    [ service 2 ]                      |   用于控制诊断请求消息的发送，定时器触发                                 |
          |                            |   回调函数内将发送诊断请求消息，定时器时                                 |
          V                            |   间是用于在发送请求前等待多久，设置0就是                                |
    [ service 3 ]                      |   立即发送。
          |                            |   
          V                            |   response_watcher
    [ service 4 ]                      |   诊断响应超时定时器，在成功发送请求响应后
          |                            |   将启动这个定时器，yudsc_service_response_finish()
          V                            |   收到请求响应后将关闭定时器，或者超时标记当
    [ service 5 ]                      |   前诊断服务超时，然后搜索下一个诊断服务，
          |                            |   启动request_watcher定时器，
          V                            |   如此反复直至所有诊断服务执行结束。
    [ service 6 ]                      |   
                                       |   yudsc_services_start()
                                       |   request_watcher定时器启动
                                       |              |
                                       |              V
                                       |   request_watcher超时触发    
                                       |              |
                                       |              V
                                       |          发送诊断请求                                            
                                       |              |成功
                                       |              V
                                       |   response_watcher定时器启动 ->超时标记响应超时
                                       |              |收到响应              |
                                       |              V                  V
                                       |       下一个诊断服务项          <--------
                                       |    request_watcher定时器启动
*/

/* 27服务种子key buff最大长度 */
#define SA_KEY_SEED_SIZE (64)

/* 36服务最大重传次数 */
#define TD_RETRANS_NUM_MAX (2)

#define DSC_10_RESPONSE_FIX_LEN (6) 

#define NEGATIVE_RESPONSE_FIX_SIZE (3)
#define NEGATIVE_RESPONSE_ID (0x7f)

/* 在文件小于一定的时候直接全部读出文件到内存中 */
#define FIRMWARE_FILE_CACHE_BUFF_MAX (1024 * 1024)

typedef enum service_response_stat {
    SERVICE_RESPONSE_NORMAL,
    SERVICE_RESPONSE_TIMEOUT,
} service_response_stat;

typedef struct service_36_td {
#define MAX_NUMBER_OF_BLOCK_LENGTH (10000) /* 设备支持的最大传输数据块长度 */
    yuint32 maxNumberOfBlockLength; /* 最大传输块长度，由外部设置的 */    
    char *local_path;
    yuint8 rt_cnt; /* 重传次数计次 */
    BOOLEAN is_retrans; /* 当前是否是重传 */
} service_36_td;

typedef struct service_34_rd {
    yuint8 dataFormatIdentifier;
    yuint8 addressAndLengthFormatIdentifier;
    yuint32 memoryAddress;
    yuint32 memorySize;
} service_34_rd;

typedef struct service_38_rft {
    yuint8 modeOfOperation;
    yuint16 filePathAndNameLength;
    char *filePathAndName;
    yuint8 dataFormatIdentifier;
    yuint8 fileSizeParameterLength;
    yuint32 fileSizeUnCompressed;
    yuint32 fileSizeCompressed;
} service_38_rft;

typedef int (*YKey)(
    const unsigned char*    iSeedArray,        // seed数组
    unsigned short          iSeedArraySize,    // seed数组大小
    unsigned int            iSecurityLevel,    // 安全级别 1,3,5...
    const char*             iVariant,          // 其他数据, 可设置为空
    unsigned char*          iKeyArray,         // key数组, 空间由用户构造
    unsigned short*         iKeyArraySize      // key数组大小, 函数返回时为key数组实际大小
);

typedef struct service_27_ac {
    YKey key_callback;
} service_27_ac;

typedef void (*service_variable_byte_callback)(void *item, YByteArray *variable_byte, void *arg);

typedef void (*service_response_callback)(void *item, const yuint8 *data, yuint32 len, void *arg);

enum service_item_status {
    SERVICE_ITEM_WAIT, /* 等待执行 */
    SERVICE_ITEM_RUN, /* 服务执行中 */
    SERVICE_ITEM_TIMEOUT, /* 响应超时 */
    SERVICE_ITEM_FAILED, /* 出错 */
    SERVICE_ITEM_SUCCESS, /* 按预期执行结束 */
};

/* 这个结构体内的变量值都是在添加诊断服务项时初始化赋值的，后续执行过程中不修改其中变量值 */
typedef struct service_item {
    enum service_item_status status;
    BOOLEAN isenable;
    yuint8 sid; /* 诊断服务ID */
    yuint8 sub; /* 诊断服务子功能，option */
    yuint16 did; /* 数据标识符，option */
#define UDS_SERVICE_DELAY_MIN (0) /* unit ms */
#define UDS_SERVICE_DELAY_MAX (60000) /* unit ms */
    yint32 delay; /* 服务处理延时时间，延时时间到达后才会处理这条服务项 unit ms */
#define UDS_SERVICE_TIMEOUT_MIN (10) /* unit ms */
#define UDS_SERVICE_TIMEOUT_TYPICAL_VAL (50) /* unit ms */
#define UDS_SERVICE_TIMEOUT_MAX (20000) /* unit ms */
    yint32 timeout; /* 应答等待超时时间 unit ms */
    BOOLEAN issuppress; /* 是否是抑制响应 */
    yuint32 tatype; /* 目的地址类型 */
    yuint32 ta; /* 诊断目的地址 */
    yuint32 sa; /* 诊断源地址 */
    YByteArray *request_byte; /* 请求数据 */
    YByteArray *variable_byte; /* 非固定格式的数据，比如31 2e服务后面不固定的数据 */
    service_variable_byte_callback vb_callback; /* 在构建请求数据时候调用 */
    void *vb_callback_arg;
    service_response_callback response_callback; /* 接收到应答的时候调用 */
    void *response_callback_arg;
    service_36_td td_36; /* 36服务配置,可选数据 */
    service_34_rd rd_34; /* 34服务配置,可选数据 */
    service_38_rft *rft_38; /* 38服务配置,可选数据 */
    service_27_ac ac_27;
    
    serviceResponseExpect response_rule; /* 预期响应规则 */ 
    YByteArray *expect_byte; /* 预期响应字符 */
    YByteArray *expect_bytes[64]; /* 支持多个预期响应字符 */

    serviceFinishCondition finish_rule; /* 结束规则 */
    YByteArray *finish_byte; /* 结束响应字符 */    
    YByteArray *finish_bytes[64]; /* 支持多个结束响应字符 */
    
    yuint32 finish_try_num; /* 结束条件检测次数 */
    yuint32 finish_num_max; /* 结束最大尝试次数，到了这个次数不论是否符合结束条件都会结束这个诊断项 */
    yuint32 finish_time_max; /* 结束最大等待时间，到了这个时间不论是否符合结束条件都会结束这个诊断项 */

    YByteArray *response_byte; /* 记录一下响应数据 */
        
    yuint32 rr_callid; /* ota master api 端的请求结果回调函数ID，大于0才有效 */
    char *desc; /* 诊断服务项描述信息 */
} service_item;

typedef enum udsc_finish_stat {
    UDSC_NORMAL_FINISH = 0, /* 正常结束 */
    UDSC_SENT_ERROR_FINISH, /* 发送请求错误结束 */
    UDSC_UNEXPECT_RESPONSE_FINISH, /* 非预期应答错误结束 */
    UDSC_TIMEOUT_RESPONSE_FINISH, /* 超时应答错误结束 */
} udsc_finish_stat;

/* 诊断发送请求回调函数 */
typedef yint32 (*udsc_request_transfer_callback)(yuint16 id, void *arg, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);

/* 诊断任务结束后调用 */
typedef void (*udsc_task_end_callback)(void *udsc, udsc_finish_stat stat, void *arg/* 用户数据指针 */, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl);

/* 用于生成种子 */
typedef void (*security_access_keygen_callback)(void *arg/* 用户数据指针 */, yuint16 id, yuint8 level, yuint8 *seed, yuint16 seed_size);

/* 反馈36服务传输进度 */
typedef void (*service_36_transfer_progress_callback)(void *arg/* 用户数据指针 */, yuint16 id, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time);

typedef struct uds_client {
    BOOLEAN isidle; /* 是否被使用 */
    yuint16 id; /* uds客户端ID */
    char *udsc_name; /* uds客户端名 */
    char om_name[8]; /* ota master名字 */
    struct ev_loop *loop; /* 主事件循环结构体 */
    ev_timer request_watcher; /* 诊断任务请求处理定时器 */
    ev_timer response_watcher; /* 诊断任务应答超时定时器 */
    ev_timer testerPresent_watcher; /* 诊断仪在线请求定时器 */
    ev_timer td_36_progress_watcher; /* 传输进度通告定时器 */
    BOOLEAN isFailAbort; /* 诊断项任务处理发生错误后时候中止执行服务表项任务 */
    BOOLEAN response_meet_expect; /* 运行过程中标志位，应答符合预期要求 */
    yint32 sindex; /* 当前进行处理的诊断服务项索引 service_items[sindex] */
    BOOLEAN iskeep; /* 是否继续执行当前诊断服务项，默认不继续支持当前项，在运行过程中确定 */
    yuint32 service_cnt; /* 诊断服务数量 */
    yuint32 service_size; /* service_items 的大小 */
    service_item **service_items; /* 诊断服务处理列表 */
    void *udsc_request_transfer_arg;
    udsc_request_transfer_callback udsc_request_transfer_cb; /* 诊断消息发送函数，在需要发送诊断请求的时候调用 */
    void *udsc_task_end_arg;
    udsc_task_end_callback udsc_task_end_cb; /* 诊断客户端结束调用 */    
    void *security_access_keygen_arg;    
    security_access_keygen_callback security_access_keygen_cb; /* 在需要生成key的时候回调这个函数 */    
    void *service_36_transfer_progress_arg;    
    service_36_transfer_progress_callback service_36_transfer_progress_cb;    
    /* 用于诊断服务项之间共享数据 */
    struct common_variable {
        yuint8 fileTransferCount; /* 36服务传输序列号 */
        yuint32 fileTransferTotalSize; /* 36服务传输文件总字节数 */
        yuint32 fileSize; /* 文件大小 */
        yuint8 *td_buff; /* 36服务传输buff */
        yuint8 *ff_cache_buff; /* 36服务读取文件使用的缓存大小 */
        void *filefd; /* 打开的文件描述符34 36 37 38 服务使用 */
        yuint32 maxNumberOfBlockLength; /* 最大传输块长度，由34,38服务的响应得到，实际36传输时使用这个变量的长度值，会根据需求设置 */
        YByteArray *seed_byte;
        YByteArray *key_byte; /* 保存由seed的生成的key */
        yuint16 p2_server_max;
        yuint16 p2_e_server_max;
    } common_var;
    BOOLEAN runtime_statis_enable; /* 使能数据统计 */
    /* ------------一些需要记录的中间变量start-------------- */
    yuint32 run_start_time;
    yuint32 recv_pretime;
    int recv_total_byte;
    int recv_average_byte;
    yuint32 sent_pretime;
    int sent_total_byte;
    int sent_average_byte;
    /* ------------一些需要记录的中间变量end-------------- */
    /* 运行时数据统计 */
    struct runtime_data_statis_s rt_data_statis;
    /* 诊断仪在线请求 */
    BOOLEAN tpEnable; /* 使能诊断仪在线请求报文 */
    BOOLEAN isTpRefresh; /* 是否被UDS报文刷新定时器 */
    yuint32 tpInterval; /* 发送间隔 unit ms */
    yuint32 tpta; /* 诊断目的地址 */
    yuint32 tpsa; /* 诊断源地址 */ 
    void *doipc_channel; /* doip传输通道 */

    yuint32 td_36_start_time; 
 #define TD_36_PROGRESS_NOTIFY_INTERVAL_MIN (10) /* ms */
 #define TD_36_PROGRESS_NOTIFY_INTERVAL_MAX (10000) /* ms */
    yuint32 td_36_progress_interval;

    BOOLEAN uds_msg_parse_enable; /* UDS消息解析使能，开启后有些许性能损耗 */
    BOOLEAN uds_asc_record_enable; /* UDS消息记录，开启后有些许性能损耗 */
} uds_client;

/*
    函数内部不检查 uds_client * 和 service_item * 指针是否为空，在传参前检查
*/

/*
    创建一个UDS 客户端，多ECU交互时可以创建多个ECU客户端
*/
extern uds_client *yudsc_create();

/*
    调用这个函数前，确保没有事件循环和没有诊断业务在执行
    销毁UDS 客户端，在有诊断任务执行或者事件循环没有停止的时候返回错误
*/
extern BOOLEAN yudsc_destroy(uds_client *udsc);

/*
    事件循环，需要循环调用这个函数才能进行事件循环
    和 yudsc_thread_loop_start 二选一即可
*/
extern BOOLEAN yudsc_event_loop_start(uds_client *udsc);

/*
    在线程内进行事件循环，只用调用一次就行，需要关注线程资源互斥问题
    和 yudsc_event_loop_start 二选一即可
*/
extern BOOLEAN yudsc_thread_loop_start(uds_client *udsc);

/*
    停止事件循环
*/
extern BOOLEAN yudsc_loop_stop(uds_client *udsc);

/*
    是否有诊断任务在执行中
*/
extern BOOLEAN yudsc_service_isactive(uds_client *udsc);

/*
    开始执行诊断任务
*/
extern BOOLEAN yudsc_services_start(uds_client *udsc);

/*
    停止执行诊断任务
*/
extern BOOLEAN yudsc_services_stop(uds_client *udsc);

/*
    设置诊断项执行出错是否终止执行诊断任务
*/
extern void yudsc_service_fail_abort(uds_client *udsc, BOOLEAN b);

/*
    增加一个诊断服务项，已经加入到诊断服务执行表内
*/
extern service_item *yudsc_service_item_add(uds_client *udsc, char *desc);

/*
    从诊断服务执行表内删除一个诊断服务项，后续不能再使用了
*/
extern void yudsc_service_item_del(uds_client *udsc, service_item *item);

/*
    根据结构体内的配置生成请求报文
*/
extern BOOLEAN yudsc_service_request_build(service_item *sitem);

/*
    设置诊断请求发送回调函数
*/
extern void yudsc_request_transfer_callback_set(uds_client *udsc, udsc_request_transfer_callback call, void *arg);

/*
    设置所有诊断服务执行结束后的回调处理函数
*/
extern void yudsc_task_end_callback_set(uds_client *udsc, udsc_task_end_callback call, void *arg);

extern void yudsc_service_variable_byte_callback_set(service_item *sitem, service_variable_byte_callback vb_callback, void *arg);

extern void yudsc_service_response_callback_set(service_item *sitem, service_response_callback response_callback, void *arg);

extern void yudsc_service_response_finish(uds_client *udsc, service_response_stat stat, yuint32 sa, yuint32 ta, yuint8 *data, yuint32 size);

extern service_item *yudsc_curr_service_item(uds_client *udsc);

extern BOOLEAN yudsc_reset(uds_client *udsc); 

extern void yudsc_service_sid_set(service_item *sitem, yuint8 sid);

extern yuint8 yudsc_service_sid_get(service_item *sitem);

extern void yudsc_service_sub_set(service_item *sitem, yuint8 sub);

extern yuint8 yudsc_service_sub_get(service_item *sitem);

extern void yudsc_service_did_set(service_item *sitem, yuint16 did);

extern yuint16 yudsc_service_did_get(service_item *sitem);

extern void yudsc_service_delay_set(service_item *sitem, yint32 delay);

extern void yudsc_service_timeout_set(service_item *sitem, yint32 timeout);

extern void yudsc_service_suppress_set(service_item *sitem, BOOLEAN b);

extern void yudsc_service_enable_set(service_item *sitem, BOOLEAN b);

extern void yudsc_service_expect_response_set(service_item *sitem, serviceResponseExpect rule, yuint8 *data, yuint32 size);

extern void yudsc_service_finish_byte_set(service_item *sitem, serviceFinishCondition rule, yuint8 *data, yuint32 size);

extern void yudsc_service_key_set(service_item *sitem, YKey key_callback);

extern void yudsc_ev_loop_set(uds_client* udsc, struct ev_loop* loop);

extern void yudsc_service_key_generate(uds_client *udsc, yuint8 *key, yuint16 key_size);

extern void yudsc_security_access_keygen_callback_set(uds_client *udsc, security_access_keygen_callback call, void *arg);

extern void yudsc_36_transfer_progress_callback_set(uds_client *udsc, service_36_transfer_progress_callback call, void *arg);

extern void yudsc_doip_channel_bind(uds_client *udsc, void *doip_channel);

extern void yudsc_doip_channel_unbind(uds_client *udsc);

extern void *yudsc_doip_channel(uds_client *udsc);

extern BOOLEAN yudsc_runtime_data_statis_get(uds_client *udsc, runtime_data_statis_t *rt_data_statis);

extern void yudsc_runtime_data_statis_enable(uds_client *udsc, BOOLEAN en);

extern BOOLEAN yudsc_service_item_capacity_init(uds_client *udsc, int size);

extern int yudsc_service_item_capacity(uds_client *udsc);

extern int yudsc_security_access_key_set(uds_client *udsc, yuint8 *key, yuint16 key_size);

extern int yudsc_tester_present_config(uds_client *udsc, yuint8 tpEnable, yuint8 isTpRefresh, yuint32 tpInterval, yuint32 tpta, yuint32 tpsa);

extern int yudsc_fail_abort(uds_client *udsc, BOOLEAN b);

extern int yudsc_id_set(uds_client *udsc, int id);

extern int yudsc_id(uds_client *udsc);

extern int yudsc_idle_set(uds_client *udsc, BOOLEAN b);

extern BOOLEAN yudsc_is_idle(uds_client *udsc);

extern int yudsc_td_36_progress_interval_set(uds_client *udsc, yuint32 interval);

extern int yudsc_name_set(uds_client *udsc, char *name);

extern BOOLEAN yudsc_asc_record_enable(uds_client *udsc);

extern void yudsc_asc_record_enable_set(uds_client *udsc, BOOLEAN b);

extern void yudsc_msg_parse_enable_set(uds_client *udsc, BOOLEAN b);

#endif /* __YUDS_CLIENT_H__ */
