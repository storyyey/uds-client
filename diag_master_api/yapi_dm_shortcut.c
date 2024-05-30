#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "yapi_crc8.h"
#include "yapi_crc16.h"
#include "yapi_crc32.h"
#include "yapi_crc64.h"
#include "yapi_md5.h"
#include "yapi_dm_shortcut.h"

#define CACHE_BLOCK_FILE_PATH "/mnt/sdisk/cache_block"
#define S19_CHECKSUN_BYTE_LEN (1)
#define S1_ADDR_BYTE_LEN (2)
#define S2_ADDR_BYTE_LEN (3)
#define S3_ADDR_BYTE_LEN (4)

#define FFILESUFFIX(x) strrchr(x, '.')?strrchr(x, '.')+1:x
#define FFILENAME(x) strrchr(x, '/')?strrchr(x, '/')+1:x

/* 文件格式类型 */
enum {
    FILE_FORMAT_UNKNOWN = 0,
    FILE_FORMAT_MOTOROLA_S_RECORD = 1,
    FILE_FORMAT_HEX = 2,
    FILE_FORMAT_BIN = 3
};

/* 摩托罗拉格式文件后缀不区分大小写 */
static const char *mot_srecord_suffix[] = {"SRECORD", "SRE", "SREC", "S19", "mot", "S28", "S37", "SX", "S", "S1", "S2", "S3", "mxt", "exo", NULL}; 
/* hex格式文件后缀不区分大小写 */
static const char *hex_suffix[] = {"hex", NULL};
/* bin格式文件后缀不区分大小写 */
static const char *bin_suffix[] = {"bin", NULL};
/* hex/srecord转bin文件的缓存目录 */
static char request_downlod_file_cache_block_dir[128] = {CACHE_BLOCK_FILE_PATH};

int yapi_add_service_diagnostic_session_control(YAPI_DM yapi, yuint16 udscid, yuint32 sa, yuint32 ta, yuint8 sub, yuint32 delay, yuint32 timeout)
{
    service_config service;

    memset(&service, 0, sizeof(service));
    service.sa = sa;
    service.ta = ta;
    service.sid = 0x10;
    service.sub = (sub & UDS_SUBFUNCTION_MASK);
    service.issuppress = (sub & UDS_SUPPRESS_POS_RSP_MSG_IND_MASK) ? 1 : 0;
    service.delay = delay;
    service.timeout = timeout;

    return yapi_dm_udsc_service_add(yapi, udscid, &service, 0);
}

int yapi_add_service_ecu_reset(YAPI_DM yapi, yuint16 udscid, yuint32 sa, yuint32 ta, yuint8 sub, yuint32 delay, yuint32 timeout)
{
    service_config service;

    memset(&service, 0, sizeof(service));
    service.sa = sa;
    service.ta = ta;
    service.sid = 0x11;
    service.sub = (sub & UDS_SUBFUNCTION_MASK); 
    service.issuppress = (sub & UDS_SUPPRESS_POS_RSP_MSG_IND_MASK) ? 1 : 0;
    service.delay = delay;
    service.timeout = timeout;

    return yapi_dm_udsc_service_add(yapi, udscid, &service, 0);
}

int yapi_file_checksum_crc32(const char *file, yuint8 *checksum, yuint32 *checksum_size) 
{
    FILE *fp = 0;
    char buffer[512] = {0};
    int len = 0;
    yuint32 crc = 0xFFFFFFFF;

    fp = fopen(file, "rb");
    if (!fp) {        
        return -1;
    }
    
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        crc = yapi_crc32re(buffer, len, crc);
    }
    fclose(fp);
    crc ^= 0xffffffff;
 
    if (*checksum_size >= 4) {
        *checksum_size = 4;
        checksum[0] = crc >> 24 & 0xff;
        checksum[1] = crc >> 16 & 0xff;
        checksum[2] = crc >> 8 & 0xff;
        checksum[3] = crc >> 0 & 0xff;
    }
    
    return 0;
}

int yapi_files_checksum_crc32(const char **files, yuint8 *checksum, yuint32 *checksum_size) 
{
    FILE *fp = 0;
    char buffer[512] = {0};
    int len = 0;
    int findex = 0;
    yuint32 crc = 0xFFFFFFFF;

    while (files[findex]) {
        fp = fopen(files[findex], "rb");
        if (!fp) {        
            return -1;
        }
        
        while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            crc = yapi_crc32re(buffer, len, crc);
        }
        fclose(fp);
        findex++;
    }
    crc ^= 0xffffffff;
 
    if (*checksum_size >= 4) {
        *checksum_size = 4;
        checksum[0] = crc >> 24 & 0xff;
        checksum[1] = crc >> 16 & 0xff;
        checksum[2] = crc >> 8 & 0xff;
        checksum[3] = crc >> 0 & 0xff;
    }
    
    return 0;
}

int yapi_file_checksum_crc16(const char *file, yuint8 *checksum, yuint32 *checksum_size) 
{
    FILE *fp = 0;
    char buffer[512] = {0};
    int len = 0;
    yuint16 crc = 0;

    fp = fopen(file, "rb");
    if (!fp) {        
        return -1;
    }
    
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        crc = yapi_crc16re(buffer, len, crc);
    }
    fclose(fp);
 
    if (*checksum_size >= 2) {
        *checksum_size = 2;
        checksum[0] = crc >> 8 & 0xff;
        checksum[1] = crc >> 0 & 0xff;
    }
    
    return 0;
}

int yapi_file_checksum_md5(const char *file, yuint8 *checksum, yuint32 *checksum_size) 
{
    FILE *fp = 0;
    char buffer[512] = {0};
    unsigned char digest[16] = {0};
    int len = 0;
    yapi_MD5_CTX context;
    
    fp = fopen(file, "rb");
    if (!fp) {        
        return -1;
    }
    
    yapi_MD5Init(&context);
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        yapi_MD5Update(&context, buffer, len);
    }
    fclose(fp);
    yapi_MD5Final(&context, digest);
    if (*checksum_size >= 16) {
        *checksum_size = 16;
        memcpy(checksum, digest, 16);
    }

    return 0;
}

int yapi_file_checksum_hash_256(const char *file, yuint8 *checksum, yuint32 *checksum_size) 
{


}

yuint32 yapi_file_size(const char *file)
{
    struct stat file_info;

    if (stat(file, &file_info) != 0) {
        return 0;
    }
    
    return file_info.st_size;
}

int yapi_srecord2bin(const char *filepath, cache_block_info **cache_blocks, int capacity)
{
    FILE *s19fp = 0; /* S-RECORD文件描述符 */   
    FILE *binfp = 0; /* bin文件描述符 */  
    char line[512]; /* 读取S-RECORD文件行数据 */
    yuint8 binbuf[256] = {0}; /* 转换的bin文件数据 */
    yuint8 type = 0; /* 行数据类型 */
    yuint8 payloadlen = 0; 
    yuint8 hexlen = 0;
    yuint8 checksum = 0;
    cache_block_info *newbin = 0;
    int block_cnt = 0;
    int isfail = 0;
    yuint32 expect_addr = 0; /* 预期下一个地址 */    
    yuint32 curraddr = 0; /* 记录当前地址 */
    int addr_byte_len = 0;

    s19fp = fopen(filepath, "r");  
    if (s19fp == 0) {  
        printf("Failed to open file => %s. \n", filepath);  
        return 0;  
    }  

    while (fgets(line, sizeof(line), s19fp)) {          
        type = line[1];
        yuint32 binlen = ydm_common_hex2bin(binbuf, line + 2, sizeof(binbuf));        
        payloadlen = binbuf[0];
        switch (type) {
            case '0': /* 程序开始行 */
                /* 不允许没有正确的结束又有了新的开始 */
                if (newbin || binfp) {
                    isfail = 1;                    
                    printf("S19 to bin file failed \n");
                    goto FINISH;
                }
                /* 超过最大存储容量了 */
                if (!(block_cnt < capacity)) {
                    isfail = 1;                    
                    printf("S19 to bin file failed \n");
                    goto FINISH;
                }
                /* 创建新的BIN文件 */
                newbin = malloc(sizeof(*newbin));
                if (newbin == 0) {
                    isfail = 1;                    
                    printf("S19 to bin file failed \n");
                    goto FINISH;
                } 
                memset(newbin, 0, sizeof(*newbin));
            case '1':
                /* 记录bin文件起始地址 */
                if (type == '1') {
                    curraddr = binbuf[1] << 8 | binbuf[2];
                    hexlen = (payloadlen - S1_ADDR_BYTE_LEN - S19_CHECKSUN_BYTE_LEN);            
                    addr_byte_len = S1_ADDR_BYTE_LEN;
                }
            case '2':                     
                /* 记录bin文件起始地址 */
                if (type == '2') {
                    curraddr = binbuf[1] << 16 | binbuf[2] << 8 | binbuf[3];
                    hexlen = (payloadlen - S2_ADDR_BYTE_LEN - S19_CHECKSUN_BYTE_LEN);              
                    addr_byte_len = S2_ADDR_BYTE_LEN;
                }
            case '3':                 
                /* 记录bin文件起始地址 */
                if (type == '3') {
                    curraddr = binbuf[1] << 24 | binbuf[2] << 16 | binbuf[3] << 8 | binbuf[4];
                    hexlen = (payloadlen - S3_ADDR_BYTE_LEN - S19_CHECKSUN_BYTE_LEN);
                    addr_byte_len = S3_ADDR_BYTE_LEN;
                }
                if (newbin) {                    
                    if (newbin->memory_addr == 0) {
                        newbin->memory_addr = curraddr;
                        expect_addr = curraddr + hexlen;
                    }
                    else {
                        if (expect_addr != curraddr) {
                            /* 地址不连续到达新的数据块了 */
                            printf("Close file %s \n", newbin->filepath);
                            printf("Start addr 0x%08x file size %d \n", newbin->memory_addr, newbin->file_size);
                            fclose(binfp);
                            binfp = 0;
                            cache_blocks[block_cnt] = newbin;                
                            block_cnt++;
                            newbin = 0;    
                
                            /* 超过最大存储容量了 */
                            if (!(block_cnt < capacity)) {
                                isfail = 1;                    
                                printf("S19 to bin file failed \n");
                                goto FINISH;
                            }
                            /* 创建新的BIN文件 */
                            newbin = malloc(sizeof(*newbin));
                            if (newbin == 0) {
                                isfail = 1;                    
                                printf("S19 to bin file failed \n");
                                goto FINISH;
                            } 
                            memset(newbin, 0, sizeof(*newbin));
                            newbin->memory_addr = curraddr;
                        }
                        expect_addr = curraddr + hexlen;
                    }
                    if (binfp == 0) {
                        snprintf(newbin->filepath, sizeof(newbin->filepath), \
                            "%s/%s_block_%x.bin", request_downlod_file_cache_block_dir, FFILENAME(filepath), newbin->memory_addr);
                        binfp = fopen(newbin->filepath, "w");  
                        if (binfp == 0) {  
                            printf("Failed to open file => %s. \n", newbin->filepath);                    
                            printf("S19 to bin file failed \n");
                            isfail = 1;
                            goto FINISH;
                        } 
                        printf("Create bin file (%s) \n", newbin->filepath); 
                    } 
                    /* 统计bin文件大小 */
                    newbin->file_size += hexlen;                        
                    /* 保存bin文件 */
                    fwrite(binbuf + addr_byte_len + S19_CHECKSUN_BYTE_LEN, 1, hexlen, binfp);
                }
                break;
            case '4':
                break;
            case '5':
                break;
            case '6':
                break;
            case '7':
            case '8':
            case '9':
                /* 不允许没有开始就结束 */
                if (!binfp || !newbin) {
                    isfail = 1;                    
                    printf("S19 to bin file failed \n");
                    goto FINISH;
                }
                printf("Close file %s \n", newbin->filepath);                
                printf("Start addr 0x%08x file size %d \n", newbin->memory_addr, newbin->file_size);
                fclose(binfp);
                binfp = 0;
                cache_blocks[block_cnt] = newbin;                
                block_cnt++;
                newbin = 0;  
                break;
            default:
                break;
        }
    }  

FINISH:
    if (binfp) {        
        isfail = 1;
        fclose(binfp);     
        printf("S19 to bin file failed \n");
    }
    if (newbin) {        
        isfail = 1;
        free(newbin);
        printf("S19 to bin file failed \n");
        newbin = 0;
    }
    fclose(s19fp);  

    if (isfail) {
        printf("S19 to bin file failed \n");
        while ((--block_cnt) >= 0) {
            if (cache_blocks[block_cnt]) {
                free(cache_blocks[block_cnt]);
                cache_blocks[block_cnt] = 0;
            }
        }
        block_cnt = 0;
    }
    
    return block_cnt; 
}

int yapi_file_format(const char *file)
{
    int ii = 0;

    if (!file) {
        return FILE_FORMAT_UNKNOWN;
    }    

    for (; hex_suffix[ii]; ii++) {
        if (strcasecmp(FFILESUFFIX(file), hex_suffix[ii]) == 0) {
            return FILE_FORMAT_HEX;
        }
    }

    for (; bin_suffix[ii]; ii++) {
        if (strcasecmp(FFILESUFFIX(file), bin_suffix[ii]) == 0) {
            return FILE_FORMAT_BIN;
        }
    }

    for (; mot_srecord_suffix[ii]; ii++) {
        if (strcasecmp(FFILESUFFIX(file), mot_srecord_suffix[ii]) == 0) {
            return FILE_FORMAT_MOTOROLA_S_RECORD;
        }
    }

    return FILE_FORMAT_UNKNOWN;
}

void yapi_request_download_file_cache_clean()
{
    char scmd[256] = {0};

    snprintf(scmd, sizeof(scmd), "rm %s/ -rf", request_downlod_file_cache_block_dir);
    system(scmd);
}

void yapi_request_download_file_cache_dir_set(const char *dir)
{
    dir = (dir && strlen(dir) > 0) ? dir : CACHE_BLOCK_FILE_PATH;
    memset(request_downlod_file_cache_block_dir, 0, sizeof(request_downlod_file_cache_block_dir));
    snprintf(request_downlod_file_cache_block_dir, sizeof(request_downlod_file_cache_block_dir), "%s", dir);    
    mkdir(request_downlod_file_cache_block_dir, 0777);
}

int yapi_request_download_file(YAPI_DM yapi, yuint16 udscid, const char *file, udsc_request_download *rd)
{
    cache_block_info *cache_blocks[128] = {0};
    cache_block_info acache_block = {0};
    char *files[128] = {0};
    ybyte_array *arrbytes = yapi_byte_array_new();    
    yuint8 expect_respon[32] = {0};
    int block_index = 0;
    int file_format = yapi_file_format(file);
    yuint8 checksum[UDSC_FILE_CHECKSUM_LEN_MAX] = {0};
    yuint32 checksum_size = sizeof(checksum);

    if (!arrbytes || !file || !rd) {
        return -1;
    }

    /* 所有格式文件统一转换成bin文件 */
    if (file_format == FILE_FORMAT_MOTOROLA_S_RECORD) {
        /* 设置一下转换后的bin文件缓存目录 */
        yapi_request_download_file_cache_dir_set(rd->cache_file_dir);
        if (!(yapi_srecord2bin(file, &cache_blocks, sizeof(cache_blocks) / sizeof(cache_blocks[0])) > 0)) {
            return -2;
        }
    }
    else if (file_format == FILE_FORMAT_HEX) {        
        /* 设置一下转换后的bin文件缓存目录 */
        yapi_request_download_file_cache_dir_set(rd->cache_file_dir);
    }
    else if (file_format == FILE_FORMAT_BIN) {
        snprintf(acache_block.filepath, sizeof(acache_block.filepath), "%s", file);
        acache_block.memory_addr = rd->memory_addr;
        acache_block.file_size = yapi_file_size(file);
        cache_blocks[0] = &acache_block;
    }
    else {
        snprintf(acache_block.filepath, sizeof(acache_block.filepath), "%s", file);
        acache_block.memory_addr = rd->memory_addr;
        acache_block.file_size = yapi_file_size(file);
        cache_blocks[0] = &acache_block;
    } 

    /* 使用内置校验和计算函数计算文件校验和 */
    switch (rd->checksum_type) {
        case FILE_CHECK_CRC32:
            rd->checksum_cb = yapi_file_checksum_crc32;
            break;
        case FILE_CHECK_CRC16:
            rd->checksum_cb = yapi_file_checksum_crc16;
            break;
        case FILE_CHECK_MD5:
            rd->checksum_cb = yapi_file_checksum_md5;
            break;
        case FILE_CHECK_SUM:
            
            break;
        case FILE_CHECK_HASH_256:
            rd->checksum_cb = yapi_file_checksum_hash_256;
            break;
        default:
            break;
    }
    
    for ( ; cache_blocks[block_index]; block_index++) {         
        /* 内存擦除 */
        if (rd->erase_did > 0) { /* 如果设置了内存擦除DID说明就是有擦除内存操作 */
            ydef_udsc_service_add_statement("Routine Control");    
            ydef_udsc_service_base_set(0x31, rd->sa, rd->ta, \
                                       ydef_udsc_sub(0x01), \
                                       ydef_udsc_did(rd->erase_did),\
                                       ydef_udsc_timeout(1000), \
                                       ydef_udsc_expect_respon_rule(MATCH_RESPONSE_EXPECT));
            /* -----------------------------------------------------------------------------------START */
            /* 填充擦除内存所需要的一些信息 */
            yapi_byte_array_clear(arrbytes);
            yapi_byte_array_append_char(arrbytes, 0x44);
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->memory_addr >> 24 & 0xFF);
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->memory_addr >> 16 & 0xFF);
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->memory_addr >> 8 & 0xFF);    
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->memory_addr >> 0 & 0xFF);
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->file_size >> 24 & 0xFF);
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->file_size >> 16 & 0xFF);
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->file_size >> 8 & 0xFF);    
            yapi_byte_array_append_char(arrbytes, cache_blocks[block_index]->file_size >> 0 & 0xFF);
            ydef_udsc_service_variable_byte_set(yapi_byte_array_const_data(arrbytes), yapi_byte_array_count(arrbytes));
            /* -----------------------------------------------------------------------------------END */            
            /* -----------------------------------------------------------------------------------START */
            /* 填充预期得到的响应报文 */            
            memset(expect_respon, 0, sizeof(expect_respon));
            snprintf(expect_respon, sizeof(expect_respon), "7101%02x%02x00", rd->erase_did >> 8 & 0xFF, rd->erase_did >> 0 & 0xFF);
            ydef_udsc_service_expect_respon_byte_set(expect_respon);            
            /* -----------------------------------------------------------------------------------END */            
            ydef_udsc_service_add(yapi, udscid, 0);
        }

        /* 请求下载 */
        ydef_udsc_service_add_statement("Request Download");    
        ydef_udsc_service_base_set(0x34, rd->sa, rd->ta, \
                                   ydef_udsc_timeout(1000), \
                                   ydef_udsc_34_rd_data_format_identifier(0), \
                                   ydef_udsc_34_rd_address_and_length_format_identifier(0x44), \
                                   ydef_udsc_34_rd_memory_address(cache_blocks[block_index]->memory_addr), \
                                   ydef_udsc_34_rd_memory_size(cache_blocks[block_index]->file_size), \
                                   ydef_udsc_expect_respon_rule(POSITIVE_RESPONSE_EXPECT));
        ydef_udsc_service_add(yapi, udscid, 0);

        /* 数据传输 */
        ydef_udsc_service_add_statement("Transfer Data");    
        ydef_udsc_service_base_set(0x36, rd->sa, rd->ta, \
                                   ydef_udsc_timeout(1000), \
                                   ydef_udsc_max_number_of_block_length(rd->block_len_max), \
                                   ydef_udsc_expect_respon_rule(POSITIVE_RESPONSE_EXPECT));
        ydef_udsc_service_file_local_path_set(cache_blocks[block_index]->filepath);
        ydef_udsc_service_add(yapi, udscid, 0);

        /* 请求传输退出 */
        ydef_udsc_service_add_statement("Request Transfer Exit");    
        ydef_udsc_service_base_set(0x37, rd->sa, rd->ta, \
                                   ydef_udsc_timeout(1000), \
                                   ydef_udsc_expect_respon_rule(POSITIVE_RESPONSE_EXPECT));
        ydef_udsc_service_add(yapi, udscid, 0);

        /* 校验传输结果 */
        ydef_udsc_service_add_statement("Routine Control");    
        ydef_udsc_service_base_set(0x31, rd->sa, rd->ta, \
                                   ydef_udsc_sub(0x01), \
                                   ydef_udsc_did(rd->checksum_did),\
                                   ydef_udsc_timeout(1000), \
                                   ydef_udsc_expect_respon_rule(MATCH_RESPONSE_EXPECT));  
        /* -----------------------------------------------------------------------------------START */                                   
        /* 填充校验写内存是否正确所需要的一些信息 */
        yapi_byte_array_clear(arrbytes);
        if (rd->checksum_cb) {
            /* 优先使用函数计算校验和 */
            rd->checksum_cb(cache_blocks[block_index]->filepath, checksum, &checksum_size);
        }
        else {       
            checksum_size = rd->checksum_len < UDSC_FILE_CHECKSUM_LEN_MAX ? rd->checksum_len : UDSC_FILE_CHECKSUM_LEN_MAX;
            memcpy(checksum, rd->checksum, checksum_size);
        }
        yapi_byte_array_append_nchar(arrbytes, checksum, checksum_size);
        ydef_udsc_service_variable_byte_set(yapi_byte_array_const_data(arrbytes), yapi_byte_array_count(arrbytes));
        /* -----------------------------------------------------------------------------------END */            
        /* -----------------------------------------------------------------------------------START */        
        /* 填充预期得到的响应报文 */
        memset(expect_respon, 0, sizeof(expect_respon));
        snprintf(expect_respon, sizeof(expect_respon), "7101%02x%02x00", rd->checksum_did >> 8 & 0xFF, rd->checksum_did >> 0 & 0xFF);
        ydef_udsc_service_expect_respon_byte_set(expect_respon);            
        /* -----------------------------------------------------------------------------------END */            
        ydef_udsc_service_add(yapi, udscid, 0);

        if (file_format == FILE_FORMAT_MOTOROLA_S_RECORD) {
            /* 释放文件块信息内存 */
            free(cache_blocks[block_index]);
            cache_blocks[block_index] = NULL;
        }
    }

    return 0;
}
