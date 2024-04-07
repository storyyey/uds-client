#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else /* #ifdef _WIN32 */
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* #ifdef _WIN32 */

#include "cjson.h"
#include "ydm_log.h"
#include "ydm_types.h"
#include "ydm_stream.h"
#include "ydm_ipc_common.h"

#define HEX_INVALID_VALUE (0xff)
static yuint8 yom_common_hexchar_to_int(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    
    return HEX_INVALID_VALUE;
}

yuint32 ydm_common_hex2bin(yuint8 *bin, char *hex, yuint32 binlength) 
{
    yuint32 i = 0, binnn = 0;
    yuint32 hexlen = strlen(hex);
    yuint8 hbit = HEX_INVALID_VALUE, lbit = HEX_INVALID_VALUE; 

    if (hexlen % 2) {
        hbit = 0;
    }
    
    for (i = 0; i < hexlen && binnn < binlength; i++) {
        if (hbit == HEX_INVALID_VALUE) {
            hbit = yom_common_hexchar_to_int(hex[i]);
        }
        else if (lbit == HEX_INVALID_VALUE) {
            lbit = yom_common_hexchar_to_int(hex[i]);        
        }
        
        if (hbit != HEX_INVALID_VALUE &&\
            lbit != HEX_INVALID_VALUE) {
            bin[binnn] = (char)(hbit << 4 | lbit);
            binnn++;
            hbit = HEX_INVALID_VALUE, lbit = HEX_INVALID_VALUE; 
        }
    }
        
    return binnn;
}

yuint32 ydm_common_nhex2bin(yuint8 *bin, char *hex, yuint32 hexlen, yuint32 binlength) 
{
    yuint32 i = 0, binnn = 0;
    yuint8 hbit = HEX_INVALID_VALUE, lbit = HEX_INVALID_VALUE; 

    if (hexlen % 2) {
        hbit = 0;
    }
    
    for (i = 0; i < hexlen && binnn < binlength; i++) {
        if (hbit == HEX_INVALID_VALUE) {
            hbit = yom_common_hexchar_to_int(hex[i]);
        }
        else if (lbit == HEX_INVALID_VALUE) {
            lbit = yom_common_hexchar_to_int(hex[i]);        
        }
        
        if (hbit != HEX_INVALID_VALUE &&\
            lbit != HEX_INVALID_VALUE) {
            bin[binnn] = (char)(hbit << 4 | lbit);
            binnn++;
            hbit = HEX_INVALID_VALUE, lbit = HEX_INVALID_VALUE; 
        }
    }
        
    return binnn;
}

int ydm_common_is_hex_digit(char c) 
{  
    if ((c >= '0' && c <= '9') || 
        (c >= 'a' && c <= 'f') ||   
        (c >= 'A' && c <= 'F')) {
        return 1;  
    }  

    return 0;  
}

int ydm_recvfrom(int sockfd, yuint8 *buff, yuint32 size, yuint32 toms)
{
#ifdef __linux__
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
#endif /* __linux__ */
    int bytesRecv = -1;
    fd_set readfds;
    struct timeval timeout;
    int sret = 0;
    
    if (sockfd < 0) return 0;

    timeout.tv_sec = toms / 1000;
    timeout.tv_usec = (toms % 1000) * 1000;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    if ((sret = select(sockfd + 1, &readfds, NULL, NULL, &timeout)) > 0 &&\
        FD_ISSET(sockfd, &readfds)) {
#ifdef __linux__
        bytesRecv = recvfrom(sockfd, buff, size, 0, \
                                (struct sockaddr*)&un, (socklen_t *)&len);
#endif /* __linux__ */
#ifdef _WIN32
        SOCKADDR_IN fromAddress;
        int fromLength = sizeof(fromAddress);
        bytesRecv = recvfrom(sockfd, buff, size, 0, (SOCKADDR*)&fromAddress, &fromLength);
#endif /* _WIN32 */
    } 
    // log_hex_d("recv", buff, bytesRecv);
    return bytesRecv;
}

int yom_command_reply(int sockfd, yuint32 *evtype, yuint32 *recode)
{
#ifdef __linux__
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
#endif /* __linux__ */
    yuint8 recvbuf[64] = {0};
    int bytesRecv = -1;

    if (!evtype || !recode) return 0;
    if (sockfd < 0) return 0;
#ifdef __linux__
    bytesRecv = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, \
                            (struct sockaddr*)&un, (socklen_t *)&len);
#endif /* __linux__ */
#ifdef _WIN32
    SOCKADDR_IN fromAddress;
    int fromLength = sizeof(fromAddress);
    bytesRecv = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (SOCKADDR*)&fromAddress, &fromLength);
#endif /* _WIN32 */
    if (bytesRecv > 0) {        
        *evtype = TYPE_CAST_UINT(recvbuf + DM_IPC_EVENT_TYPE_OFFSET);
        *recode = TYPE_CAST_UINT(recvbuf + DM_IPC_EVENT_RECODE_OFFSET);
    }

    return bytesRecv;
}

int ydm_tv_get(yuint32 *tv_sec, yuint32 *tv_usec)
{
    if (!tv_sec || !tv_usec) return -1;
#ifdef _WIN32

#else /* #ifdef _WIN32 */
    struct timespec time = {0, 0};

    // gettimeofday(tv, NULL);
    clock_gettime(CLOCK_MONOTONIC, &time);
    *tv_sec = time.tv_sec;
    *tv_usec = time.tv_nsec / 1000;
     
    return 0;
#endif /* #ifdef _WIN32 */
}

int ydm_tv_currtoms(yuint32 tv_sec, yuint32 tv_usec)
{
    yuint32 curr_tv_sec = 0; yuint32 curr_tv_usec = 0;
    /* 不考虑时间溢出的风险 */
    ydm_tv_get(&curr_tv_sec, &curr_tv_usec);
    
    return (curr_tv_sec * 1000 + curr_tv_usec / 1000) - (tv_sec * 1000 + tv_usec / 1000); 
}

int ydm_ipc_channel_create(void *addr, int rbuf_size, int sbuf_size)
{
    int channel_fd = -1;
    int flag = 0;
    struct sockaddr_un *un = addr;
    socklen_t opt_len = 0;
    int rbs = 0, sbs = 0;

    if (!un) {
        return channel_fd;
    } 

    /* 创建域套接字socket */
    channel_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (channel_fd < 0) {
        goto CREATE_FAIL;    
    }

    /* 设置非阻塞的socket */
    flag = fcntl(channel_fd, F_GETFL, 0);
    fcntl(channel_fd, F_SETFL, flag | O_NONBLOCK);

    /* 移除已经存在的域套接字地址 */
    unlink(un->sun_path);
    /* 绑定域套接字地址 */
    if (bind(channel_fd, (struct sockaddr *)un, sizeof(struct sockaddr_un)) < 0) {
        goto CREATE_FAIL;
    }
    
    /* 获取发送缓冲区大小 */
    opt_len = sizeof(sbs);
    if (!(getsockopt(channel_fd, SOL_SOCKET, SO_SNDBUF, &sbs, &opt_len) < 0)) {
        log_i("The IPC send buffer size is %d", sbs);
        if (sbs < sbuf_size) {
            log_i("Reset the IPC send buffer size(%d), original send buffer size(%d)", sbuf_size, sbs);
            opt_len = sizeof(sbuf_size);
            setsockopt(channel_fd, SOL_SOCKET, SO_SNDBUF, &sbuf_size, opt_len);
        }
    }

    /* 获取接收缓冲区大小 */
    opt_len = sizeof(rbs);
    if (!(getsockopt(channel_fd, SOL_SOCKET, SO_RCVBUF, &rbs, &opt_len) < 0)) {    
        log_i("The IPC recv buffer size is %d", sbs);
        if (rbs < rbuf_size) {            
            log_i("Reset the IPC send buffer size(%d), original send buffer size(%d)", rbuf_size, rbs);
            opt_len = sizeof(rbuf_size);
            setsockopt(channel_fd, SOL_SOCKET, SO_SNDBUF, &rbuf_size, opt_len);
        }
    }

    return channel_fd;
CREATE_FAIL:
    if (channel_fd > 0) {
        close(channel_fd);
    }

    return -1;
}

/* 重置ipc消息时间参数，设置为当前时间 */
int ydm_common_ipc_tv_reset(yuint8 *buff, yuint32 size)
{
    yuint32 tv_sec = 0;
    yuint32 tv_usec = 0;

    if (!buff || size < DM_IPC_EVENT_MSG_SIZE) {
        return -1;
    }
    
    ydm_tv_get(&tv_sec, &tv_usec);
    memcpy(buff + DM_IPC_EVENT_TV_SEC_OFFSET, &tv_sec, DM_IPC_CEVENT_TV_SEC_SIZE);
    memcpy(buff + DM_IPC_EVENT_TV_USEC_OFFSET, &tv_usec, DM_IPC_EVENT_TV_USEC_SIZE);

    return 0;
}

int ydm_common_is_system_time_stamp(yuint32 tv_sec, yuint32 tv_usec)
{
    if (tv_sec == 0x12345678 && tv_usec == 0x12345678) {
        return 0;
    }

    return 1;
}


int ydm_common_encode(yuint8 *buff, yuint32 size, yuint32 evtype, yuint32 recode, yuint16 id, yuint32 tv_sec, yuint32 tv_usec)
{
    if (!buff || size < DM_IPC_EVENT_MSG_SIZE) {
        return -1;
    }
    
    memset(buff, 0, DM_IPC_EVENT_MSG_SIZE);
    memcpy(buff + DM_IPC_EVENT_TYPE_OFFSET, &evtype, DM_IPC_EVENT_TYPE_SIZE);
    memcpy(buff + DM_IPC_EVENT_RECODE_OFFSET, &recode, DM_IPC_EVENT_RECODE_SIZE);
    memcpy(buff + DM_IPC_EVENT_UDSC_ID_OFFSET, &id, DM_IPC_EVENT_UDSC_ID_SIZE); 
    if (ydm_common_is_system_time_stamp(tv_sec, tv_usec)) {
        /* 使用系统时间戳 */
        ydm_tv_get(&tv_sec, &tv_usec);
    }
    memcpy(buff + DM_IPC_EVENT_TV_SEC_OFFSET, &tv_sec, DM_IPC_CEVENT_TV_SEC_SIZE);
    memcpy(buff + DM_IPC_EVENT_TV_USEC_OFFSET, &tv_usec, DM_IPC_EVENT_TV_USEC_SIZE);

    return 0;
}

int ydm_common_decode(yuint8 *buff, yuint32 size, yuint32 *evtype, yuint32 *recode, yuint16 *id, yuint32 *tv_sec, yuint32 *tv_usec)
{
    if (!buff || size < DM_IPC_EVENT_MSG_SIZE) {
        return -1;
    }
    
    *evtype = TYPE_CAST_UINT(buff + DM_IPC_EVENT_TYPE_OFFSET);
    *recode = TYPE_CAST_UINT(buff + DM_IPC_EVENT_RECODE_OFFSET);
    *id = TYPE_CAST_USHORT(buff + DM_IPC_EVENT_UDSC_ID_OFFSET); 
    *tv_sec = TYPE_CAST_UINT(buff + DM_IPC_EVENT_TV_SEC_OFFSET);
    *tv_usec = TYPE_CAST_UINT(buff + DM_IPC_EVENT_TV_USEC_OFFSET);

    return 0;
}

int ydm_service_config_encode(yuint8 *buff, yuint32 size, service_config *config)
{
    int ii = 0;
    int rlen = 0;

    if (config == 0) return -1;

    cJSON *root = cJSON_CreateObject();
    if (root == 0) return -1;
    if (config->sid > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_ID, cJSON_CreateNumber(config->sid));
    }
    if (config->sub > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_SUB, cJSON_CreateNumber(config->sub));
    }    
    if (config->did > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_DID, cJSON_CreateNumber(config->did));    
    }    
    if (config->delay > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_ITEM_DELAY_TIME, cJSON_CreateNumber(config->delay));
    }    
    if (config->timeout > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_TIMEOUT, cJSON_CreateNumber(config->timeout));   
    }
    cJSON_AddItemToObject(root, C_KEY_SI_IS_SUPPRESS, cJSON_CreateBool(config->issuppress));
    if (config->tatype > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_TA_TYPE, cJSON_CreateNumber(config->tatype));
    }
    if (config->ta > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_TA, cJSON_CreateNumber(config->ta));
    }
    if (config->sa > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_SA, cJSON_CreateNumber(config->sa));
    }

    cJSON_AddItemToObject(root, C_KEY_SI_EXPECT_RESPONSE_RULE, cJSON_CreateNumber(config->expectRespon_rule));
    cJSON_AddItemToObject(root, C_KEY_SI_FINISH_RESPONSE_RULE, cJSON_CreateNumber(config->finish_rule));
    if (config->finish_num_max > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_FINISH_RESPONSE_TRYMAX, cJSON_CreateNumber(config->finish_num_max));
    }
    if (config->rr_callid > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_REQUEST_RESULT_CALL_ID, cJSON_CreateNumber(config->rr_callid));
    }
    if (strlen(config->desc) > 0) {
        cJSON_AddItemToObject(root, C_KEY_SI_DESC, cJSON_CreateString(config->desc));
    }
    if (config->variableByte && YByteArrayCount(config->variableByte) > 0) {
        cJSON *arrobj = cJSON_CreateArray();
        if (arrobj) {
            for (ii = 0; ii < YByteArrayCount(config->variableByte); ii++) {
                cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(YByteArrayConstData(config->variableByte)[ii]));
            }
            cJSON_AddItemToObject(root, C_KEY_SI_VARIABLE_BYTE, arrobj);
        }
    }
#if 0    
    if (config->expectResponByte && YByteArrayCount(config->expectResponByte) > 0) {
        cJSON *arrobj = cJSON_CreateArray();        
        if (arrobj) {
            for (ii = 0; ii < YByteArrayCount(config->expectResponByte); ii++) {
                cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(YByteArrayConstData(config->expectResponByte)[ii]));
            }
            cJSON_AddItemToObject(root, C_KEY_SI_EXPECT_RESPONSE_BYTE, arrobj);
        }
    }
    if (config->finishByte && YByteArrayCount(config->finishByte) > 0) {
        cJSON *arrobj = cJSON_CreateArray();
        if (arrobj) {
            for (ii = 0; ii < YByteArrayCount(config->finishByte); ii++) {
                cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(YByteArrayConstData(config->finishByte)[ii]));
            }
            cJSON_AddItemToObject(root, C_KEY_SI_FINISH_RESPONSE_BYTE, arrobj);
        }
    }
#endif    
    if (config->expectResponByte && strlen(YByteArrayConstData(config->expectResponByte))) {
        cJSON_AddItemToObject(root, C_KEY_SI_EXPECT_RESPONSE_BYTE, cJSON_CreateString(YByteArrayConstData(config->expectResponByte)));
    }
    if (config->finishByte && strlen(YByteArrayConstData(config->finishByte))) {
        cJSON_AddItemToObject(root, C_KEY_SI_FINISH_RESPONSE_BYTE, cJSON_CreateString(YByteArrayConstData(config->finishByte)));
    }
    if (config->sid == 0x34) {
        cJSON *rdobj = cJSON_CreateObject();
        if (rdobj) {
            cJSON_AddItemToObject(root, C_KEY_SI_ID_34_RD, rdobj);
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_DFI, cJSON_CreateNumber(config->service_34_rd.dataFormatIdentifier));
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_AALFI, cJSON_CreateNumber(config->service_34_rd.addressAndLengthFormatIdentifier));
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_MA, cJSON_CreateNumber(config->service_34_rd.memoryAddress));
            cJSON_AddItemToObject(rdobj, C_KEY_SI_ID_34_MS, cJSON_CreateNumber(config->service_34_rd.memorySize));             
        }
    }
    if (config->sid == 0x38) {
        cJSON *rftobj = cJSON_CreateObject();
        if (rftobj) {
            cJSON_AddItemToObject(root, C_KEY_SI_ID_38_FRT, rftobj);
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_MOO, cJSON_CreateNumber(config->service_38_rft.modeOfOperation));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FPANL, cJSON_CreateNumber(config->service_38_rft.filePathAndNameLength));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FPAN, cJSON_CreateString(config->service_38_rft.filePathAndName));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_DFI, cJSON_CreateNumber(config->service_38_rft.dataFormatIdentifier));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FSPL, cJSON_CreateNumber(config->service_38_rft.fileSizeParameterLength));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FSUC, cJSON_CreateNumber(config->service_38_rft.fileSizeUnCompressed));
            cJSON_AddItemToObject(rftobj, C_KEY_SI_ID_38_FSC, cJSON_CreateNumber(config->service_38_rft.fileSizeCompressed)); 
        }
    }
    if (config->sid == 0x36) {
        cJSON *rftobj = cJSON_CreateObject();
        if (rftobj) {            
            cJSON_AddItemToObject(root, C_KEY_SI_ID_36_TD, rftobj);
            if (strlen(config->local_path) > 0) {
                cJSON_AddItemToObject(rftobj, C_KEY_SI_LOCAL_FILE_PATH, cJSON_CreateString(config->local_path));
            }
            if (config->maxNumberOfBlockLength > 0) {        
                cJSON_AddItemToObject(rftobj, C_KEY_SI_MAX_NUMBER_OF_BLOCK_LEN, cJSON_CreateNumber(config->maxNumberOfBlockLength));
            }
        }
    }

    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s", str);
        free((void *)str);
    }
    cJSON_Delete(root);

    return rlen;
}

int ydm_service_config_decode(yuint8 *buff, yuint32 size, service_config *config)
{
    int arrcount = 0;
    cJSON *vobj = 0;

    if (config == 0) return -1;

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) return -2;

    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ID);
    if (vobj) {
        config->sid = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_SUB);
    if (vobj) {
        config->sub = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_DID);
    if (vobj) {
        config->did = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ITEM_DELAY_TIME);
    if (vobj) {
        config->delay = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_TIMEOUT);
    if (vobj) {
        config->timeout = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_IS_SUPPRESS);
    if (vobj) {
        config->issuppress = cJSON_IsFalse(vobj) ? 0 : 1;        
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_TA_TYPE);    
    if (vobj) {
        config->tatype = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_TA);
    if (vobj) {
        config->ta = cJSON_GetNumberValue(vobj);        
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_SA);
    if (vobj) {
        config->sa = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_DESC);
    if (vobj) {
        snprintf(config->desc, sizeof(config->desc), "%s", cJSON_GetStringValue(vobj));       
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_EXPECT_RESPONSE_RULE);
    if (vobj) {
        config->expectRespon_rule = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_FINISH_RESPONSE_RULE);
    if (vobj) {
        config->finish_rule = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_FINISH_RESPONSE_TRYMAX);
    if (vobj) {
        config->finish_num_max = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_REQUEST_RESULT_CALL_ID);
    if (vobj) {
        config->rr_callid = cJSON_GetNumberValue(vobj);        
    } 
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_VARIABLE_BYTE); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        int index = 0;
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem && config->variableByte) {
                YByteArrayAppendChar(config->variableByte, cJSON_GetNumberValue(arrayItem));                
            }            
        }         
    }
#if 0    
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_EXPECT_RESPONSE_BYTE); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        int index = 0;
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem && config->expectResponByte) {
                YByteArrayAppendChar(config->expectResponByte, cJSON_GetNumberValue(arrayItem));                
            }            
        }         
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_FINISH_RESPONSE_BYTE); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        int index = 0;
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem && config->finishByte) {
                YByteArrayAppendChar(config->finishByte, cJSON_GetNumberValue(arrayItem));
            }
        }         
    }
#endif 
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_EXPECT_RESPONSE_BYTE);
    if (vobj) {
        char *str = cJSON_GetStringValue(vobj);
        if (str && config->expectResponByte) {
            YByteArrayAppendNChar(config->expectResponByte, str, strlen(str));
        }
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_FINISH_RESPONSE_BYTE);
    if (vobj) {
        char *str = cJSON_GetStringValue(vobj);
        if (str && config->finishByte) {
            YByteArrayAppendNChar(config->finishByte, str, strlen(str));       
        }
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ID_34_RD);
    if (vobj) {
        cJSON *tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_DFI);
        if (tobj) {
            config->service_34_rd.dataFormatIdentifier = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_AALFI);
        if (tobj) {
            config->service_34_rd.addressAndLengthFormatIdentifier = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_MA);
        if (tobj) {
            config->service_34_rd.memoryAddress = cJSON_GetNumberValue(tobj);        
        }        
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_34_MS);
        if (tobj) {
            config->service_34_rd.memorySize = cJSON_GetNumberValue(tobj);        
        }
    }

    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ID_38_FRT);
    if (vobj) {
        cJSON *tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_MOO);
        if (tobj) {
            config->service_38_rft.modeOfOperation = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FPANL);
        if (tobj) {
            config->service_38_rft.filePathAndNameLength = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FPAN);
        if (tobj) {
            snprintf(config->service_38_rft.filePathAndName, \
                sizeof(config->service_38_rft.filePathAndName), "%s", cJSON_GetStringValue(tobj));       
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_DFI);
        if (tobj) {
            config->service_38_rft.dataFormatIdentifier = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FSPL);
        if (tobj) {
            config->service_38_rft.fileSizeParameterLength = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FSUC);
        if (tobj) {
            config->service_38_rft.fileSizeUnCompressed = cJSON_GetNumberValue(tobj);        
        }
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_ID_38_FSC);
        if (tobj) {
            config->service_38_rft.fileSizeCompressed = cJSON_GetNumberValue(tobj);        
        }
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SI_ID_36_TD);
    if (vobj) {
        cJSON *tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_LOCAL_FILE_PATH);
        if (tobj) {
            snprintf(config->local_path, \
                sizeof(config->local_path), "%s", cJSON_GetStringValue(tobj));       
        }        
        tobj = cJSON_GetObjectItem(vobj, C_KEY_SI_MAX_NUMBER_OF_BLOCK_LEN);
        if (tobj) {
            config->maxNumberOfBlockLength = cJSON_GetNumberValue(tobj);        
        }
    }
    cJSON_Delete(root);
    
    return 0;
}

int ydm_general_config_encode(yuint8 *buff, yuint32 size, udsc_runtime_config *config)
{
    int rlen = 0;

    if (config == 0) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == 0) return -1;

    cJSON_AddItemToObject(root, C_KEY_GC_IS_FAILABORT, cJSON_CreateBool(config->isFailAbort));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_ENABLE, cJSON_CreateBool(config->tpEnable));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_IS_REFRESH, cJSON_CreateBool(config->isTpRefresh));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_INTERVAL, cJSON_CreateNumber(config->tpInterval));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_TA, cJSON_CreateNumber(config->tpta));
    cJSON_AddItemToObject(root, C_KEY_GC_TP_SA, cJSON_CreateNumber(config->tpsa));    
    cJSON_AddItemToObject(root, C_KEY_GC_TD_36_NOTIFY_INTERVAL, cJSON_CreateNumber(config->td_36_notify_interval));    
    cJSON_AddItemToObject(root, C_KEY_RUN_STATIS_ENABLE, cJSON_CreateBool(config->runtime_statis_enable));
    cJSON_AddItemToObject(root, C_KEY_UDS_MSG_PARSE, cJSON_CreateBool(config->uds_msg_parse_enable));
    cJSON_AddItemToObject(root, C_KEY_UDS_ASC_RECORD, cJSON_CreateBool(config->uds_asc_record_enable));
    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);        
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s", str);
        free((void *)str);
    }
    cJSON_Delete(root);

    return rlen;
}

int ydm_general_config_decode(yuint8 *buff, yuint32 size, udsc_runtime_config *config)
{
    cJSON *vobj = 0;

    if (config == 0) {
        return -1;
    }

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) return -2;
    
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_IS_FAILABORT);
    if (vobj) {
        config->isFailAbort = cJSON_IsFalse(vobj) ? 0 : 1;        
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_ENABLE);
    if (vobj) {
        config->tpEnable = cJSON_IsFalse(vobj) ? 0 : 1;        
    }  
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_IS_REFRESH);
    if (vobj) {
        config->isTpRefresh = cJSON_IsFalse(vobj) ? 0 : 1;        
    }  
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_INTERVAL);
    if (vobj) {
        config->tpInterval = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_TA);
    if (vobj) {
        config->tpta = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TP_SA);
    if (vobj) {
        config->tpsa = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_GC_TD_36_NOTIFY_INTERVAL);
    if (vobj) {
        config->td_36_notify_interval = cJSON_GetNumberValue(vobj);        
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_RUN_STATIS_ENABLE);
    if (vobj) {
        config->runtime_statis_enable = cJSON_IsFalse(vobj) ? 0 : 1;        
    }  
    vobj = cJSON_GetObjectItem(root, C_KEY_UDS_MSG_PARSE);
    if (vobj) {
        config->uds_msg_parse_enable = cJSON_IsFalse(vobj) ? 0 : 1;        
    } 
    vobj = cJSON_GetObjectItem(root, C_KEY_UDS_ASC_RECORD);
    if (vobj) {
        config->uds_asc_record_enable = cJSON_IsFalse(vobj) ? 0 : 1;        
    } 

    cJSON_Delete(root);

    return 0;
}

int ydm_udsc_create_config_encode(yuint8 *buff, yuint32 size, udsc_create_config *config)
{
    int rlen = 0;

    if (config == 0) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == 0) {
        return -2;
    }

    cJSON_AddItemToObject(root, C_KEY_SERVICE_ITEM_CAPACITY, cJSON_CreateNumber(config->service_item_capacity));    
    cJSON_AddItemToObject(root, C_KEY_UDSC_NAME, cJSON_CreateString(config->udsc_name));    
    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);        
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s", str);
        free((void *)str);
    }
    cJSON_Delete(root);
    
    return rlen;
}

int ydm_udsc_create_config_decode(yuint8 *buff, yuint32 size, udsc_create_config *config)
{
    cJSON *vobj = 0;

    if (config == 0) {
        return -1;
    }

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) {
        return -2;
    }
          
    vobj = cJSON_GetObjectItem(root, C_KEY_SERVICE_ITEM_CAPACITY);
    if (vobj) {
        config->service_item_capacity = cJSON_GetNumberValue(vobj);      
    }  
    vobj = cJSON_GetObjectItem(root, C_KEY_UDSC_NAME);
    if (vobj) {
        snprintf(config->udsc_name, \
            sizeof(config->udsc_name), "%s", cJSON_GetStringValue(vobj));       
    }  
    cJSON_Delete(root);

    return 0;
}

int ydm_doipc_create_config_encode(yuint8 *buff, yuint32 size, doipc_config_t *config)
{
    int rlen = 0;

    if (config == 0) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == 0) {
        return -2;
    }
    cJSON_AddItemToObject(root, C_KEY_DOIPC_VERSION, cJSON_CreateNumber(config->ver));
    cJSON_AddItemToObject(root, C_KEY_DOIPC_SA, cJSON_CreateNumber(config->sa_addr));
    cJSON_AddItemToObject(root, C_KEY_DOIPC_SRC_IP, cJSON_CreateNumber(config->src_ip));
    cJSON_AddItemToObject(root, C_KEY_DOIPC_SRC_PORT, cJSON_CreateNumber(config->src_port));
    cJSON_AddItemToObject(root, C_KEY_DOIPC_DST_IP, cJSON_CreateNumber(config->dst_ip));    
    cJSON_AddItemToObject(root, C_KEY_DOIPC_DST_PORT, cJSON_CreateNumber(config->dst_port));
    cJSON_AddItemToObject(root, C_KEY_DOIPC_RX_LEN, cJSON_CreateNumber(config->rxlen));    
    cJSON_AddItemToObject(root, C_KEY_DOIPC_TX_LEN, cJSON_CreateNumber(config->txlen));
    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);        
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s", str);
        free((void *)str);
    }
    cJSON_Delete(root);
    log_d("ver        : %d", config->ver);
    log_d("sa_addr    : %d", config->sa_addr);
    log_d("src_ip     : %d", config->src_ip);
    log_d("src_port   : %d", config->src_port);
    log_d("dst_ip     : %d", config->dst_ip);
    log_d("dst_port   : %d", config->dst_port);
    log_d("rxlen      : %d", config->rxlen);
    log_d("txlen      : %d", config->txlen);
    
    return rlen;
}

int ydm_doipc_create_config_decode(yuint8 *buff, yuint32 size, doipc_config_t *config)
{
    cJSON *vobj = 0;

    if (config == 0) {
        return -1;
    }

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) {
        return -2;
    }
    
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_VERSION);
    if (vobj) {
        config->ver = cJSON_GetNumberValue(vobj);    
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_SA);
    if (vobj) {
        config->sa_addr = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_SRC_IP);
    if (vobj) {
        config->src_ip = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_SRC_PORT);
    if (vobj) {
        config->src_port = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_DST_IP);
    if (vobj) {
        config->dst_ip = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_DST_PORT);
    if (vobj) {
        config->dst_port = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_RX_LEN);
    if (vobj) {
        config->rxlen = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_DOIPC_TX_LEN);
    if (vobj) {
        config->txlen = cJSON_GetNumberValue(vobj);    
    }
    cJSON_Delete(root);
    log_d("ver        : %d", config->ver);
    log_d("sa_addr    : %d", config->sa_addr);
    log_d("src_ip     : %d", config->src_ip);
    log_d("src_port   : %d", config->src_port);
    log_d("dst_ip     : %d", config->dst_ip);
    log_d("dst_port   : %d", config->dst_port);
    log_d("rxlen      : %d", config->rxlen);
    log_d("txlen      : %d", config->txlen);

    return 0;
}

int ydm_udsc_rundata_statis_encode(yuint8 *buff, yuint32 size, runtime_data_statis_t *statis)
{
    int rlen = 0;

    if (statis == 0) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == 0) {
        return -2;
    }
    cJSON_AddItemToObject(root, C_KEY_SENT_TOTAL_BYTES, cJSON_CreateNumber(statis->sent_total_byte));
    cJSON_AddItemToObject(root, C_KEY_RECV_TOTAL_BYTES, cJSON_CreateNumber(statis->recv_total_byte));
    cJSON_AddItemToObject(root, C_KEY_SENT_PEAK_SPEED, cJSON_CreateNumber(statis->sent_peak_speed));
    cJSON_AddItemToObject(root, C_KEY_RECV_PEAK_SPEED, cJSON_CreateNumber(statis->recv_peak_speed));
    cJSON_AddItemToObject(root, C_KEY_SENT_VALLEY_SPEED, cJSON_CreateNumber(statis->sent_valley_speed));    
    cJSON_AddItemToObject(root, C_KEY_RECV_VALLEY_SPEED, cJSON_CreateNumber(statis->recv_valley_speed));
    cJSON_AddItemToObject(root, C_KEY_RUN_TIME_TOTAL, cJSON_CreateNumber(statis->run_time_total));    
    cJSON_AddItemToObject(root, C_KEY_SENT_PACKET_NUM, cJSON_CreateNumber(statis->sent_num));
    cJSON_AddItemToObject(root, C_KEY_RECV_PACKET_NUM, cJSON_CreateNumber(statis->recv_num));
    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);        
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s", str);
        free((void *)str);
    }
    cJSON_Delete(root);
    log_d("sent_total_byte    : %d", statis->sent_total_byte);
    log_d("recv_total_byte    : %d", statis->recv_total_byte);
    log_d("sent_peak_speed    : %d", statis->sent_peak_speed);
    log_d("recv_peak_speed    : %d", statis->recv_peak_speed);
    log_d("sent_valley_speed  : %d", statis->sent_valley_speed);
    log_d("recv_valley_speed  : %d", statis->recv_valley_speed);
    log_d("run_time_total     : %d", statis->run_time_total);
    log_d("sent_num           : %d", statis->sent_num);
    log_d("recv_num           : %d", statis->recv_num);
    
    return rlen;

}

int ydm_udsc_rundata_statis_decode(yuint8 *buff, yuint32 size, runtime_data_statis_t *statis)
{
    cJSON *vobj = 0;

    if (statis == 0) {
        return -1;
    }

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) {
        return -2;
    }
    
    vobj = cJSON_GetObjectItem(root, C_KEY_SENT_TOTAL_BYTES);
    if (vobj) {
        statis->sent_total_byte = cJSON_GetNumberValue(vobj);    
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_RECV_TOTAL_BYTES);
    if (vobj) {
        statis->recv_total_byte = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SENT_PEAK_SPEED);
    if (vobj) {
        statis->sent_peak_speed = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_RECV_PEAK_SPEED);
    if (vobj) {
        statis->recv_peak_speed = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SENT_VALLEY_SPEED);
    if (vobj) {
        statis->sent_valley_speed = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_RECV_VALLEY_SPEED);
    if (vobj) {
        statis->recv_valley_speed = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_RUN_TIME_TOTAL);
    if (vobj) {
        statis->run_time_total = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_SENT_PACKET_NUM);
    if (vobj) {
        statis->sent_num = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, C_KEY_RECV_PACKET_NUM);
    if (vobj) {
        statis->recv_num = cJSON_GetNumberValue(vobj);    
    }
    cJSON_Delete(root);
    log_d("sent_total_byte    : %d", statis->sent_total_byte);
    log_d("recv_total_byte    : %d", statis->recv_total_byte);
    log_d("sent_peak_speed    : %d", statis->sent_peak_speed);
    log_d("recv_peak_speed    : %d", statis->recv_peak_speed);
    log_d("sent_valley_speed  : %d", statis->sent_valley_speed);
    log_d("recv_valley_speed  : %d", statis->recv_valley_speed);
    log_d("run_time_total     : %d", statis->run_time_total);
    log_d("sent_num           : %d", statis->sent_num);
    log_d("recv_num           : %d", statis->recv_num);

    return 0;
}

int ydm_omapi_connect_master_encode(yuint8 *buff, yuint32 size, omapi_connect_master_t *cf)
{
    int rlen = 0;

    if (cf == 0) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == 0) {
        return -2;
    }
    cJSON_AddItemToObject(root, C_KEY_CM_KEEPALIVE_INTER, cJSON_CreateNumber(cf->keepalive_inter));
    cJSON_AddItemToObject(root, C_KEY_CM_CALL_PATH, cJSON_CreateString(cf->event_listen_path));
    cJSON_AddItemToObject(root, C_KEY_CM_INFO_PATH, cJSON_CreateString(cf->event_emit_path));
    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);        
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s", str);
        free((void *)str);
    }
    cJSON_Delete(root);
    log_d("cf->keepalive_inter    : %d", cf->keepalive_inter);
    log_d("cf->call_path    : %s", cf->event_listen_path);
    log_d("cf->info_path    : %s", cf->event_emit_path);
    
    return rlen;
}

int ydm_omapi_connect_master_decode(yuint8 *buff, yuint32 size, omapi_connect_master_t *cf)
{
    cJSON *vobj = 0;

    if (cf == 0) {
        return -1;
    }

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) {
        return -2;
    }

    vobj = cJSON_GetObjectItem(root, C_KEY_CM_KEEPALIVE_INTER);
    if (vobj) {
        cf->keepalive_inter = cJSON_GetNumberValue(vobj);    
    }    
    vobj = cJSON_GetObjectItem(root, C_KEY_CM_CALL_PATH);
    if (vobj) {        
        snprintf(cf->event_listen_path, \
            sizeof(cf->event_listen_path), "%s", cJSON_GetStringValue(vobj));       
    } 
    vobj = cJSON_GetObjectItem(root, C_KEY_CM_INFO_PATH);
    if (vobj) {
        snprintf(cf->event_emit_path, \
            sizeof(cf->event_emit_path), "%s", cJSON_GetStringValue(vobj));       
    } 

    cJSON_Delete(root);
    log_d("cf->keepalive_inter    : %d", cf->keepalive_inter);
    log_d("cf->call_path    : %s", cf->event_listen_path);
    log_d("cf->info_path    : %s", cf->event_emit_path);

    return 0;
}

void yom_terminal_control_service_info_dump(terminal_control_service_info_t *trinfo)
{
    log_d("terminal_control_service_info_t dump =>");
    log_d("provinceid => %d", trinfo->provinceid);
    log_d("cityid => %d", trinfo->cityid);
    log_d("color => %d", trinfo->color);
    log_d("vin => %s", trinfo->vin);
    log_d("srv_addr => %s", trinfo->srv_addr);
    log_d("srv_port => %d", trinfo->srv_port);
}

int ydm_terminal_control_service_info_encode(yuint8 *buff, yuint32 size, terminal_control_service_info_t *trinfo)
{
    int ii = 0;
    int rlen = 0;
    cJSON *arrobj = NULL;

    if (trinfo == 0) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == 0) {
        return -2;
    }

    arrobj = cJSON_CreateArray();
    if (arrobj) {
        for (ii = 0; ii < sizeof(trinfo->devid); ii++) {
            cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(trinfo->devid[ii]));
        }
        cJSON_AddItemToObject(root, KEY_TR_DEV_ID, arrobj);
    }
    cJSON_AddItemToObject(root, KEY_TR_PROVINCE, cJSON_CreateNumber(trinfo->provinceid));
    cJSON_AddItemToObject(root, KEY_TR_CITY, cJSON_CreateNumber(trinfo->cityid));
    arrobj = cJSON_CreateArray();
    if (arrobj) {
        for (ii = 0; ii < sizeof(trinfo->manufacturer); ii++) {
            cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(trinfo->manufacturer[ii]));
        }
        cJSON_AddItemToObject(root, KEY_TR_MANUFACTURER, arrobj);
    }
    arrobj = cJSON_CreateArray();
    if (arrobj) {
        for (ii = 0; ii < sizeof(trinfo->model); ii++) {
            cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(trinfo->model[ii]));
        }
        cJSON_AddItemToObject(root, KEY_TR_MODEL, arrobj);
    }
    arrobj = cJSON_CreateArray();
    if (arrobj) {
        for (ii = 0; ii < sizeof(trinfo->terid); ii++) {
            cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(trinfo->terid[ii]));
        }
        cJSON_AddItemToObject(root, KEY_TR_TER_ID, arrobj);
    }
    cJSON_AddItemToObject(root, KEY_TR_COLOR, cJSON_CreateNumber(trinfo->color));    
    cJSON_AddItemToObject(root, KEY_TR_VIN, cJSON_CreateString(trinfo->vin));
    cJSON_AddItemToObject(root, KEY_TR_IMEI, cJSON_CreateString(trinfo->imei));
    cJSON_AddItemToObject(root, KEY_TR_SRV_ADDR, cJSON_CreateString(trinfo->srv_addr));
    cJSON_AddItemToObject(root, KEY_TR_SRV_PORT, cJSON_CreateNumber(trinfo->srv_port));
    arrobj = cJSON_CreateArray();
    if (arrobj) {
        for (ii = 0; ii < sizeof(trinfo->app_version); ii++) {
            cJSON_AddItemToArray(arrobj, cJSON_CreateNumber(trinfo->app_version[ii]));
        }
        cJSON_AddItemToObject(root, KEY_TR_APP_VERSION, arrobj);
    }
    
    const char *str = cJSON_PrintUnformatted(root);
    if (str) {
        rlen = snprintf((char *)buff, size, "%s", str);        
        if (rlen < size) {
            buff[rlen] = 0;
        }
        //log_d("%s", str);
        free((void *)str);
    }
    cJSON_Delete(root);
    yom_terminal_control_service_info_dump(trinfo);
    
    return rlen;
}

int ydm_terminal_control_service_info_decode(yuint8 *buff, yuint32 size, terminal_control_service_info_t *trinfo)
{
    cJSON *vobj = 0;
    int arrcount = 0;
    int index = 0;

    if (trinfo == 0) {
        return -1;
    }

    cJSON *root = cJSON_Parse((const char *)buff);
    if (root == 0) {
        return -2;
    }
    vobj = cJSON_GetObjectItem(root, KEY_TR_DEV_ID); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        arrcount = MIN(arrcount, sizeof(trinfo->devid));
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem) {
                trinfo->devid[index] = cJSON_GetNumberValue(arrayItem);
            }
        }         
    }
    vobj = cJSON_GetObjectItem(root, KEY_TR_PROVINCE);
    if (vobj) {
        trinfo->provinceid = cJSON_GetNumberValue(vobj);    
    }    
    vobj = cJSON_GetObjectItem(root, KEY_TR_CITY);
    if (vobj) {
        trinfo->cityid = cJSON_GetNumberValue(vobj);    
    }   
    vobj = cJSON_GetObjectItem(root, KEY_TR_MANUFACTURER); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        arrcount = MIN(arrcount, sizeof(trinfo->manufacturer));
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem) {
                trinfo->manufacturer[index] = cJSON_GetNumberValue(arrayItem);
            }
        }         
    }
    vobj = cJSON_GetObjectItem(root, KEY_TR_MODEL); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        arrcount = MIN(arrcount, sizeof(trinfo->model));
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem) {
                trinfo->model[index] = cJSON_GetNumberValue(arrayItem);
            }
        }         
    }
    vobj = cJSON_GetObjectItem(root, KEY_TR_TER_ID); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        arrcount = MIN(arrcount, sizeof(trinfo->terid));
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem) {
                trinfo->terid[index] = cJSON_GetNumberValue(arrayItem);
            }
        }         
    }
    vobj = cJSON_GetObjectItem(root, KEY_TR_COLOR);
    if (vobj) {
        trinfo->color = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, KEY_TR_VIN);
    if (vobj) {        
        snprintf(trinfo->vin, \
            sizeof(trinfo->vin), "%s", cJSON_GetStringValue(vobj));       
    }     
    vobj = cJSON_GetObjectItem(root, KEY_TR_IMEI);
    if (vobj) {        
        snprintf(trinfo->imei, \
            sizeof(trinfo->imei), "%s", cJSON_GetStringValue(vobj));       
    } 
    vobj = cJSON_GetObjectItem(root, KEY_TR_SRV_ADDR);
    if (vobj) {        
        snprintf(trinfo->srv_addr, \
            sizeof(trinfo->srv_addr), "%s", cJSON_GetStringValue(vobj));       
    } 
    vobj = cJSON_GetObjectItem(root, KEY_TR_SRV_PORT);
    if (vobj) {
        trinfo->srv_port = cJSON_GetNumberValue(vobj);    
    }
    vobj = cJSON_GetObjectItem(root, KEY_TR_APP_VERSION); 
    if (vobj) {
        arrcount = cJSON_GetArraySize(vobj);
        arrcount = MIN(arrcount, sizeof(trinfo->app_version));
        for (index = 0; index < arrcount; index++) {
            cJSON *arrayItem = cJSON_GetArrayItem(vobj, index);
            if (arrayItem) {
                trinfo->app_version[index] = cJSON_GetNumberValue(arrayItem);
            }
        }         
    }
    
    cJSON_Delete(root);
    yom_terminal_control_service_info_dump(trinfo);

    return 0;
}

char *ydm_ipc_event_str(yuint32 evtype)
{
    switch (evtype) {
        case DM_EVENT_SERVICE_INDICATION_EMIT: 
             return "(event emit) <service indication>";
        case DM_EVENT_SERVICE_INDICATION_ACCEPT:   
             return "[event accept] <service indication>";
        /* ota_master_api 发送诊断应答请求 */
        case DM_EVENT_SERVICE_RESPONSE_EMIT:            
             return "(event emit>) <service rsponse>";
        case DM_EVENT_SERVICE_RESPONSE_ACCEPT:     
             return "[event accept] <service rsponse>";
        /* ota_master_api 控制 ota_master 启动诊断脚本 */
        case DM_EVENT_START_UDS_CLIENT_EMIT:            
             return "(event emit) <start uds client>";
        case DM_EVENT_START_UDS_CLIENT_ACCEPT:          
             return "[event accept] <start uds client>";
        /* ota_master_api 控制 ota_master 停止诊断脚本 */
        case DM_EVENT_STOP_UDS_CLIENT_EMIT:           
             return "(event emit) <stop uds client>";
        case DM_EVENT_STOP_UDS_CLIENT_ACCEPT:           
             return "[event accept] <stop uds client>";
        /* ota_master 通知 ota_master_api 诊断任务执行结果 */
        case DM_EVENT_UDS_CLIENT_RESULT_EMIT:            
             return "(event emit) <uds client service result>";
        case DM_EVENT_UDS_CLIENT_RESULT_ACCEPT:
             return "[event accept] <uds client service result>";
        /* ota_master_api 配置诊断脚本 */
        case DM_EVENT_UDS_SERVICE_ITEM_ADD_EMIT:            
             return "(event emit) <uds service item add>";
        case DM_EVENT_UDS_SERVICE_ITEM_ADD_ACCEPT:
             return "[event accept] <uds service item add>";
        /* ota_master_api 让 ota_master直接读脚本文件 */
        case DM_EVENT_READ_UDS_SERVICE_CFG_FILE_EMIT:            
             return "(event emit) <uds service config file read>";
        case DM_EVENT_READ_UDS_SERVICE_CFG_FILE_ACCEPT:
             return "[event accept] <uds service config file read>";
        /* ota_master_api 让 ota_master 创建 uds客户端 */
        case DM_EVENT_UDS_CLIENT_CREATE_EMIT:            
             return "(event emit) <uds client create>";
        case DM_EVENT_UDS_CLIENT_CREATE_ACCEPT:
             return "[event accept] <uds client create>";
        /* ota_master_api 让 ota_master 销毁 uds客户端 */
        case DM_EVENT_UDS_CLIENT_DESTORY_EMIT:            
             return "(event emit) <uds client destory>";
        case DM_EVENT_UDS_CLIENT_DESTORY_ACCEPT:
             return "[event accept] <uds client destory>";
        /* ota_master_api 请求 ota_master 复位 */
        case DM_EVENT_DIAG_MASTER_RESET_EMIT:            
             return "(event emit) <ota master reset>";
        case DM_EVENT_DIAG_MASTER_RESET_ACCEPT:
             return "[event accept] <ota master reset>";
        /* ota_master_api 请求 ota_master 配置通用项 */
        case DM_EVENT_UDS_CLIENT_GENERAL_CFG_EMIT:            
             return "(event emit) <uds client general config>";
        case DM_EVENT_UDS_CLIENT_GENERAL_CFG_ACCEPT:
             return "[event accept] <uds client general config>";
        /* ota_master 通知 ota_master_api 诊断请求结果 */
        case DM_EVENT_UDS_SERVICE_RESULT_EMIT:
             return "(event emit) <service request result>";
        case DM_EVENT_UDS_SERVICE_RESULT_ACCEPT:
             return "[event accept] <service request result>";
        case DM_EVENT_UDS_SERVICE_27_SA_SEED_EMIT:
             return "(event emit) <service request SA seed>";
        case DM_EVENT_UDS_SERVICE_27_SA_SEED_ACCEPT:
             return "[event accept] <service request SA seed>";             
        case DM_EVENT_UDS_SERVICE_27_SA_KEY_EMIT:
             return "(event emit) <service request SA key>";
        case DM_EVENT_UDS_SERVICE_27_SA_KEY_ACCEPT:
             return "[event accept] <service request SA key>";
        case DM_EVENT_CONNECT_DIAG_MASTER_EMIT:            
             return "(event emit) <connect ota master>";
        case DM_EVENT_CONNECT_DIAG_MASTER_ACCEPT:
             return "[event accept] <connect ota master>";
        case DM_EVENT_OMAPI_KEEPALIVE_EMIT:
             return "(event emit) <keepalive>";
        case DM_EVENT_OMAPI_KEEPALIVE_ACCEPT:
             return "[event accept] <keepalive>";
        case DM_EVENT_DOIP_CLIENT_CREATE_EMIT:
             return "(event emit) <create doip client>";
        case DM_EVENT_DOIP_CLIENT_CREATE_ACCEPT:
             return "[event accept] <create doip client>";
        case DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_EMIT:
             return "(event emit) <uds client rundata statis>";             
        case DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_ACCEPT:
             return "[event accept] <uds client rundata statis>";
        case DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_EMIT:
             return "(event emit) <uds client check active>";
        case DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_ACCEPT:
             return "[event accept] <uds client check active>";
        case DM_EVENT_OMAPI_CHECK_VALID_EMIT:
             return "(event emit) <ota master api check valid>";
        case DM_EVENT_OMAPI_CHECK_VALID_ACCEPT:
             return "[event accept] <ota master api check valid>";
        case DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_EMIT:
             return "(event emit) <terminal control service create>";
        case DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_ACCEPT:
             return "[event accept] <terminal control service create>";
        case DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_EMIT:
             return "(event emit) <terminal control service destory>";
        case DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_ACCEPT:           
             return "[event accept] <terminal control service destory>";
        default:
             return "unknown";
    }
    
    return "unknown";
}

char *ydm_event_rcode_str(int rcode)
{
    switch (rcode) {
        case DM_ERR_NO:
            return "no error";
        case DM_ERR_SERVICE_REQUEST:
            return "UDS request sent failed";
        case DM_ERR_START_SCRIPT_FAILED:
            return "UDS client start failed";
        case DM_ERR_NOT_FOUND_FILE:
            return "UDS services script file not found";
        case DM_ERR_UNABLE_PARSE_FILE:
            return "UDS services script file not parse";
        case DM_ERR_UNABLE_PARSE_CONFIG:
            return "UDS services config not parse";
        case DM_ERR_UDSC_CREATE:
            return "UDS client create failed";
        case DM_ERR_UDSC_INVALID:
            return "UDS client id invalid";
        case DM_ERR_UDSC_RUNING:
            return "UDS client runing not destory";
        case DM_ERR_UDS_RESPONSE_TIMEOUT:
            return "UDS service response timeout";
        case DM_ERR_UDS_RESPONSE_UNEXPECT:
            return "UDS service response unexpect";
        case DM_ERR_DIAG_MASTER_MAX:
            return "ota master out max";
        case DM_ERR_OMAPI_UNKNOWN:
            return "ota master api unknown";
        case DM_ERR_DIAG_MASTER_CREATE:        
            return "ota master create failed";
        case DM_ERR_DOIPC_CREATE_FAILED:
            return "doip client create failed";
        case DM_ERR_UDSC_RUNDATA_STATIS:
            return "uds client run data statis disable";
        default:
            return "unknown";
    }

    return "unknown";
}

#define YBYTE_ARRAY_RESERVED_SIZE (32)

YByteArray *YByteArrayNew()
{
    YByteArray *arr = calloc(sizeof(YByteArray), 1);
    if (arr) {
        arr->size = 1;
        arr->data = calloc(arr->size, 1);
        if (arr->data) {
            arr->dlen = 0;
        }
    }
    
    return arr;
}

void YByteArrayDelete(YByteArray *arr)
{
    if (arr) {
        if (arr->data) { 
            free(arr->data);
        }
        free(arr);
    }
}

void YByteArrayClear(YByteArray *arr)
{
    arr->dlen = 0;
    memset(arr->data, 0, arr->size);
}

const yuint8 *YByteArrayConstData(YByteArray *arr)
{
    return arr->data;
}

int YByteArrayCount(YByteArray *arr)
{
    return arr->dlen;
}

void YByteArrayAppendChar(YByteArray *dest, yuint8 c)
{
    if (!(dest->dlen + 1 < dest->size)) {
        dest->size = YBYTE_ARRAY_RESERVED_SIZE + dest->dlen + 1;
        yuint8 *data = calloc(dest->size, 1);
        if (data) {
            memcpy(data, dest->data, dest->dlen);
            data[dest->dlen] = c;
            dest->dlen++;
            free(dest->data);
            dest->data = data;
        }
    }
    else {
        dest->data[dest->dlen] = c;
        dest->dlen++;
    }
}

void YByteArrayAppendArray(YByteArray *dest, YByteArray *src)
{
    if (!(dest->dlen + src->dlen < dest->size)) {
        dest->size = YBYTE_ARRAY_RESERVED_SIZE + dest->dlen + src->dlen;
        yuint8 *data = calloc(dest->size, 1);
        if (data) {
            memcpy(data, dest->data, dest->dlen);
            memcpy(data + dest->dlen, src->data, src->dlen);
            dest->dlen += src->dlen;
            free(dest->data);
            dest->data = data;
        }
    }
    else {
        memcpy(dest->data + dest->dlen, src->data, src->dlen);
        dest->dlen += src->dlen;
    }
}

void YByteArrayAppendNChar(YByteArray *dest, yuint8 *c, yuint32 count)
{
    if (!(dest->dlen + count < dest->size)) {
        dest->size = YBYTE_ARRAY_RESERVED_SIZE + dest->dlen + count;
        yuint8 *data = calloc(dest->size, 1);
        if (data) {
            memcpy(data, dest->data, dest->dlen);
            memcpy(data + dest->dlen, c, count);
            dest->dlen += count;
            free(dest->data);
            dest->data = data;
        }
    }
    else {
        memcpy(dest->data + dest->dlen, c, count);
        dest->dlen += count;
    }    
}

int YByteArrayContainsNChar(YByteArray *dest, yuint8 *c, yuint32 count)
{


}

int YByteArrayEqual(YByteArray *arr1, YByteArray *arr2)
{
    if (arr1->dlen == arr2->dlen && \
        memcmp(arr1->data, arr2->data, arr2->dlen) == 0) {
        return 1;
    }

    return 0;
}

int YByteArrayCharEqual(YByteArray *arr1, yuint8 *c, yuint32 count)
{
    if (arr1->dlen == count && \
        memcmp(arr1->data, c, count) == 0) {
        return 1;
    }

    return 0;
}

char YByteArrayAt(YByteArray *arr, int pos)
{
    return 0;
}
