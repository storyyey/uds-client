#ifndef __YAPI_DM_SHORTCUT_H__
#define __YAPI_DM_SHORTCUT_H__

#include "yapi_dm_simple.h"

#define UDS_SUBFUNCTION_MASK                    0x7f
#define UDS_SUPPRESS_POS_RSP_MSG_IND_MASK       0x80

#define UDS_DSC_TYPES_DEFAULT_SESSION                   1
#define UDS_DSC_TYPES_PROGRAMMING_SESSION               2
#define UDS_DSC_TYPES_EXTENDED_DIAGNOSTIC_SESSION       3
#define UDS_DSC_TYPES_SAFETY_SYSTEM_DIAGNOSTIC_SESSION  4
int yapi_add_service_diagnostic_session_control(YAPI_DM yapi, yuint16 udscid, yuint32 sa, yuint32 ta, yuint8 sub, yuint32 delay, yuint32 timeout);

#define UDS_ER_TYPES_HARD_RESET                   1
#define UDS_ER_TYPES_KEY_OFF_ON_RESET             2
#define UDS_ER_TYPES_SOFT_RESET                   3
#define UDS_ER_TYPES_ENABLE_RAPID_POWER_SHUTDOWN  4
#define UDS_ER_TYPES_DISABLE_RAPID_POWER_SHUTDOWN 5
int yapi_add_service_ecu_reset(YAPI_DM yapi, yuint16 udscid, yuint32 sa, yuint32 ta, yuint8 sub, yuint32 delay, yuint32 timeout);

typedef int (*udsc_transfer_file_checksum)(const char *filepath, yuint8 *checksum, yuint32 *checksum_size);

/* 内置文件校验算法支持 */
enum {
    FILE_CHECK_UNKNOWN = 0, /* 未知校验方式 */
    FILE_CHECK_CRC16 = 0x16161616, /* CRC16校验 */
    FILE_CHECK_CRC32 = 0x32323232, /* CRC32校验 */
    FILE_CHECK_MD5 = 0x55555555, /* MD5校验 */
    FILE_CHECK_SUM = 0xAAAAAAAA, /* 累加和 */
    FILE_CHECK_HASH_256 = 0x66666666, /* hash校验 */
};

/* 缓存文件块信息 */
typedef struct cache_block_info {
    char filepath[256];
    yuint32 memory_addr;
    yuint32 file_size;
} cache_block_info;

/* 文件请求下载参数 */
typedef struct udsc_request_download {
    yuint32 sa; /* 源地址 */
    yuint32 ta; /* 目的地址 */
    yuint32 memory_addr; /* 擦除地址 */
    yuint16 erase_did; /* 31服务擦除did */
    yuint16 checksum_did; /* 31服务校验did */
    yuint32 block_len_max; /* 36服务最大传输字节 */    
    yuint32 checksum_type; /* 校验和计算类型,优先级最高 */
    udsc_transfer_file_checksum checksum_cb; /* 计算校验和的回调函数,优先级次之 */
#define UDSC_FILE_CHECKSUM_LEN_MAX (128)    
    yuint8 checksum[UDSC_FILE_CHECKSUM_LEN_MAX]; /* 已经计算好的校验和,优先级最低 */
    yuint32 checksum_len; /* 校验和长度 */ 
    char cache_file_dir[256]; /* 缓存文件目录，hex、srecord文件转bin文件的缓存目录 */
} udsc_request_download;

int yapi_file_checksum_crc32(const char *file, yuint8 *checksum, yuint32 *checksum_size); 

int yapi_files_checksum_crc32(const char **files, yuint8 *checksum, yuint32 *checksum_size); 

int yapi_file_checksum_crc16(const char *file, yuint8 *checksum, yuint32 *checksum_size);

int yapi_file_checksum_md5(const char *file, yuint8 *checksum, yuint32 *checksum_size); 

yuint32 yapi_file_size(const char *file);

int yapi_srecord2bin(const char *filepath, cache_block_info **cache_blocks, int capacity);

void yapi_request_download_file_cache_dir_set(const char *dir);

void yapi_request_download_file_cache_clean();

int yapi_request_download_file(YAPI_DM yapi, yuint16 udscid, const char *file, udsc_request_download *rd);



#endif /* __YAPI_DM_SHORTCUT_H__ */
