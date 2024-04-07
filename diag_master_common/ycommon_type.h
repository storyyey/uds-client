#ifndef __YCOMMON_TYPE_H__
#define __YCOMMON_TYPE_H__

#include "ydm_types.h"

/* ---------------------------------------------响应错误码------------------------------------------------ */
#define DM_ERR_NO                   (0)
#define DM_ERR_SERVICE_REQUEST      (1) /* 诊断请求发送失败 */
#define DM_ERR_START_SCRIPT_FAILED  (2) /* 启动诊断服务脚本失败 */
#define DM_ERR_NOT_FOUND_FILE       (3) /* 未发现脚本文件 */
#define DM_ERR_UNABLE_PARSE_FILE    (4) /* 无法解析脚本文件 */
#define DM_ERR_UNABLE_PARSE_CONFIG  (5) /* 无法解析脚本配置 */
#define DM_ERR_NOT_FOUND_SCRIPT     (6) /* 未发现脚本 */
#define DM_ERR_UDSC_CREATE          (7) /* uds客户端创建失败 */
#define DM_ERR_UDSC_INVALID         (8) /* 诊断客户端ID无效 */
#define DM_ERR_UDSC_RUNING          (9) /* udsc客户端运行中无法销毁 */
#define DM_ERR_SHM_OUTOF            (10) /* 共享内存空间不足 */
#define DM_ERR_REPLY_TIMEOUT        (11) /* 应答超时 */
#define DM_ERR_UDS_RESPONSE_TIMEOUT (12) /* 诊断应答超时 */
#define DM_ERR_UDS_RESPONSE_UNEXPECT (13) /* 诊断非预期响应 */
#define DM_ERR_UDS_RESPONSE_OVERLEN  (14) /* 诊断结果长度过长，超过Buff长度 */
#define DM_ERR_UDS_SERVICE_ADD       (15) /* UDS服务项添加失败 */
#define DM_ERR_DIAG_MASTER_MAX        (16) /* 连接OTA MASTER错误，ota master达到最大接入数量 */
#define DM_ERR_OMAPI_UNKNOWN         (17) /* 连接OTA MASTER错误，未知ota master api */
#define DM_ERR_DIAG_MASTER_CREATE     (18) /* 连接OTA MASTER错误，ota master创建失败 */
#define DM_ERR_DOIPC_INVALID         (19) /* doip客户端ID无效 */
#define DM_ERR_DOIPC_CREATE_FAILED   (20) /* doip客户端创建失败 */
#define DM_ERR_UDSC_RUNDATA_STATIS   (21) /* UDS客户端运行统计数据未开启 */
#define DM_ERR_TCS_OUT_NUM_MAX       (22) /* 终端控制服务创建超过最大值 */
#define DM_ERR_TCS_EXIST             (23) /* 相同的终端控制服务 */ 
#define DM_ERR_TCS_CREATE_FAILED     (24) /* 终端控制服务创建失败 */ 
#define DM_ERR_TCS_INVALID           (25) /* 终端控制服务无效 */   

/* ------------------------------------------------------------------------------------------------------- */

struct YByteArray;
typedef struct YByteArray YByteArray;

typedef enum serviceResponseExpect {
    NOT_SET_RESPONSE_EXPECT = 0, /* 未设置预期响应 */
    POSITIVE_RESPONSE_EXPECT, /* 预期正响应 */
    NEGATIVE_RESPONSE_EXPECT, /* 预期负响应 */
    MATCH_RESPONSE_EXPECT, /* 校验预期响应 */
    NO_RESPONSE_EXPECT, /* 预期无响应 */
} serviceResponseExpect;

typedef enum serviceFinishCondition {
    FINISH_DEFAULT_SETTING = 0, /* 默认设置 */
    FINISH_EQUAL_TO, /* 等于结束响应字符 */
    FINISH_UN_EQUAL_TO, /* 不等于结束响应字符 */
    FINISH_EQUAL_TO_POSITIVE_RESPONSE, /* 等于正响应结束 */
    FINISH_EQUAL_TO_NEGATIVE_RESPONSE, /* 等于负响应结束 */
    FINISH_EQUAL_TO_NO_RESPONSE, /* 无响应结束 */
    FINISH_EQUAL_TO_NORMAL_RESPONSE, /* 有响应结束 */
} serviceFinishCondition;
    
typedef struct service_config {
    /* 诊断服务ID */
    yuint8 sid;
    /* 诊断子功能 */
    yuint8 sub; 
    /* 诊断数据标识符 */
    yuint16 did; 
    /* 诊断服务请求前的延时时间 */
    yuint32 delay; 
    /* 诊断服务响应超时时间 */
    yuint32 timeout; 
    /* 是否是抑制响应 */
    yuint8 issuppress;
    /* 目标地址类型 */
    yuint32 tatype;
    /* 目标地址 */
    yuint32 ta; 
    /* 源地址 */
    yuint32 sa; 
    /* 诊断服务中的可变数据，UDS 客户端将根据 sid sub did和这个自动构建UDS请求数据 */
    YByteArray *variableByte; 
    /* 预期诊断响应数据，用于判断当前诊断服务执行是否符合预期 */
    YByteArray *expectResponByte; 
    /* 预期响应规则 */
    yuint32 expectRespon_rule;
    /* 响应结束匹配数据，用于判断当前诊断服务是否需要重复执行 */
    YByteArray *finishByte;  
    /* 响应结束规则 */
    yuint32 finish_rule; 
    /* 响应结束最大匹配次数 */
    yuint32 finish_num_max;
    /* 请求结果回调函数的ID, 大于0才是有效值 */
    yuint32 rr_callid; 
    struct  {
        yuint8 dataFormatIdentifier;
        yuint8 addressAndLengthFormatIdentifier;
        yuint32 memoryAddress;
        yuint32 memorySize;
    } service_34_rd;
    struct  {
        yuint8 modeOfOperation;
        yuint16 filePathAndNameLength;
        char filePathAndName[256];
        yuint8 dataFormatIdentifier;
        yuint8 fileSizeParameterLength;
        yuint32 fileSizeUnCompressed;
        yuint32 fileSizeCompressed;
    } service_38_rft;
    /* 36服务传输数据块最大长度 */
    yuint32 maxNumberOfBlockLength;
    /* 本地刷写文件路径 */
    char local_path[256]; 
    /* 诊断服务项的描述信息 */
    char desc[256]; 
} service_config; 

typedef struct udsc_runtime_config {    
    int padding; /*  */
    /* 诊断项任务处理发生错误后时候中止执行服务表项任务 */
    yuint8 isFailAbort;
    /* 使能诊断仪在线请求报文 */
    yuint8 tpEnable; 
    /* 是否被UDS报文刷新定时器 */
    yuint8 isTpRefresh;
    /* 发送间隔 unit ms */
    yuint32 tpInterval;
    /* 诊断目的地址 */
    yuint32 tpta;
    /* 诊断源地址 */
    yuint32 tpsa;
    /* 36服务传输进度通告间隔 */
    yuint32 td_36_notify_interval;    
    /* UDS客户端运行时数据统计 */ 
    yuint8 runtime_statis_enable;    
    BOOLEAN uds_msg_parse_enable; /* UDS消息解析使能，开启后有些许性能损耗 */
    BOOLEAN uds_asc_record_enable; /* UDS消息记录，开启后有些许性能损耗 */
} udsc_runtime_config;

typedef struct udsc_create_config {
    int padding; /*  */
    /* 支持容纳最多多少条诊断服务项 */
    yuint32  service_item_capacity;
    char udsc_name[256];    
} udsc_create_config;

typedef struct doipc_config_s {
    /* doip版本号 */
    yuint8 ver;
    /* doip客户端源地址 */
    yuint16 sa_addr;
    /* doip客户端IP地址 */
    yuint32 src_ip;
    /* doip客户端端口号 */
    yuint16 src_port;
    /* doip服务器IP地址 */
    yuint32 dst_ip;
    /* doip服务器端口号 */
    yuint16 dst_port;
    /* 接收缓冲区长度 */
    yuint32 rxlen;
    /* 发送缓冲区长度 */
    yuint32 txlen;
} doipc_config_t;

/* 运行时数据统计 */
typedef struct runtime_data_statis_s {
    yuint32 sent_total_byte; /* 总共发送了多少字节数据 */
    yuint32 recv_total_byte; /* 总共接收了多少字节数据 */
    yuint32 sent_peak_speed; /* 发送最高速度 byte/s */
    yuint32 recv_peak_speed; /* 接收最高速度 byte/s */
    yuint32 sent_valley_speed; /* 发送最低速度 byte/s */
    yuint32 recv_valley_speed; /* 接收最低速度 byte/s */
    yuint32 run_time_total; /* 总共的运行时间单位秒 */
    yuint32 sent_num; /* 总共发送了多少条诊断服务消息 */
    yuint32 recv_num; /* 总共接收了多少条诊断服务消息 */
} runtime_data_statis_t;

typedef struct terminal_control_service_info_s {
    /*
       设备标识 BCD[10] 数据不足，则在前补充数字0
    */
    yuint8 devid[10];
    /*
       标示终端安装车辆所在的省域，0保留，由平台取默认值。
       省域ID采用GB/T2260规定的行政区域代码六位中的前两位
    */
    yuint16 provinceid; /* 省域ID */
    /*
       标示终端安装车辆所在的市域和县域，0保留，
       由平台取默认值。省域ID采用GB/T2260规定的行政区域代码六位中的后四位
    */
    yuint16 cityid; /* 市县域ID */
    /*
       TBOX制造商
    */
    yuint8 manufacturer[11]; /* 制造商ID */
    /*
       TBOX型号
    */
    yuint8 model[30]; /* 终端型号 */
    /*
       终端ID
    */
    yuint8 terid[30]; /* 终端ID */
    /*
        车牌颜色
    */
    yuint8 color;
    /*
        VIN
    */
    char vin[17]; 
    /*
        IMEI
    */
    char imei[15];
    /*
        服务器ip地址或者域名
    */
    char srv_addr[256];
    /*
        服务器端口
    */
    short srv_port;

    /* 软件版本 */
    yuint8 app_version[20]; 
} terminal_control_service_info_t;

#endif /* __YCOMMON_TYPE_H__ */
