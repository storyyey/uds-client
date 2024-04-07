#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/timeb.h>

#ifdef _WIN32
#include <winsock2.h>  
#include <ws2tcpip.h>
#endif /* _WIN32 */

#ifdef __linux__
#include <sys/select.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* __linux__ */

#include "ydm_log.h"
#include "ydm_ipc_common.h"
#include "ydm_api.h"

static int __dbg_info_enable__ = 1;

ydm_log_print g_ydm_logd_print = printf;
ydm_log_print g_ydm_logw_print = printf;
ydm_log_print g_ydm_loge_print = printf;
ydm_log_print g_ydm_logi_print = printf;

#define DM_IPC_SYNC_RECV_EXPECT           (0)
#define DM_IPC_SYNC_RECV_TIMEOUT          (-1111)
#define DM_IPC_SYNC_RECV_UNEXPECT         (-2222)
#define DM_IPC_SYNC_RECV_EVENT_HANDLE_ERR (-3333)

#define DM_API_VALID_FLAG (0x12344321)

#define MUTEX_LOCK(mutex)     pthread_mutex_lock(&mutex)
#define MUTEX_UNLOCK(mutex)   pthread_mutex_unlock(&mutex)

#define DMAPI_TX_BUFF(api) dma_txbuf(api),dma_txbuf_size(api)
#define DMAPI_RX_BUFF(api) dma_rxbuf(api),dma_rxbuf_size(api)

struct dm_api_udscs{
    yuint32 isvalid; /* 当前uds客户端的信息是否有效 */
    yuint16 udsc_id; /* uds客户端id */      
    udsc_request_transfer_callback udsc_request_transfer_cb; /* UDS客户端需要发送诊断请求的时候会回调这个函数 */
    void *udsc_request_transfer_arg; /* udsc_request_transfer_callback回调函数注册时传入的用户数据指针，在触发回调函数时会传入该指针 */
    udsc_task_end_callback udsc_task_end_cb; /* 所有的诊断服务结束后会回调这个函数 */
    void *udsc_task_end_arg; /* udsc_task_end_callback 回调函数注册时传入的用户数据指针，在触发回调函数时会传入该指针 */
    udsc_security_access_keygen_callback security_access_keygen_cb;
    void *security_access_keygen_arg;
    uds_service_36_transfer_progress_callback transfer_progress_cb;
    void *transfer_progress_arg;
    udsc_create_config config; /* uds客户端的基本配置信息 */
    struct uds_service_result_handler_s {
        uds_service_result_callback service_result; /* 单个诊断服务结束后会回调这个函数 */
        void *rr_arg; /* uds_request_result 回调函数注册时传入的用户数据指针，在触发回调函数时会传入该指针 */
    } *rr_handler;
    yuint32 rr_size; /* rr_handler 大小  */
    yuint32 rr_cnt; /* rr_handler 总数 */
    yuint32 diag_req_id; /* 诊断请求ID */
    yuint32 diag_resp_id; /* 诊断应答ID */
};

#define DM_API_THREAD_ENABLE (0x01 << 0)
#define DM_API_THREAD_RUNING (0x01 << 1)
typedef struct ydiag_master_api {
    pthread_mutex_t mutex;
    int thread_flag;
#ifdef _WIN32
    HANDLE el_tidp;
#else /* _WIN32 */
    pthread_t el_tidp;
#endif /* _WIN32 */
    yuint32 vf;
    int errcode;
#ifdef __linux__
    int event_emit_sockfd; /* 与ota_master通信使用 */
    int event_listen_sockfd; /* 与ota_master通信使用回调信息 */
#endif /* __linux__ */
#ifdef _WIN32
    SOCKET event_emit_sockfd;
    SOCKET event_listen_sockfd; /* 与ota_master通信使用回调信息 */
#endif /* _WIN32 */ 
    yuint32 event_valid_time; /* 接收等待时间 */
    struct dm_api_udscs udscs[DM_UDSC_CAPACITY_MAX];
    yuint32 udsc_cnt;
    yuint8 event_emit_txbuf[IPC_TX_BUFF_SIZE]; /* socket 发送buff */
    yuint8 event_emit_rxbuf[IPC_RX_BUFF_SIZE]; /* socket 接收buff */
    yuint8 event_listen_rxbuf[IPC_RX_BUFF_SIZE]; /* socket 发送buff */
#ifdef __linux__
    char dm_path[IPC_PATH_LEN]; /* OTA MASTER unix socket 路径 */
    char event_emit_path[IPC_PATH_LEN]; /* 向diag master发送事件路径 */
    char event_listen_path[IPC_PATH_LEN]; /* 监听IPC事件路径 */
#endif /* __linux__ */
#ifdef _WIN32
    char ip[64];
    yuint16 port;
    int dm_ip;
    yuint16 dm_port;
#endif /* _WIN32 */
    diag_master_instance_destroy_callback instance_destroy_cb;    
    void *instance_destroy_arg;
} ydiag_master_api;
static int dm_api_ipc_create(struct sockaddr_un *un);
static int dm_api_event_accept_handler(ydiag_master_api *dm_api, yuint8 *msg, yuint32 msglen);
static int dm_api_recv_event_message_handler(ydiag_master_api *dm_api, yuint16 *id, yuint32 expect_reply, yuint32 expect_code);
static struct dm_api_udscs *dm_api_udsc_get(ydiag_master_api *dm_api, yuint16 udscid);
static int dm_api_udsc_reset(ydiag_master_api *dm_api, yuint16 udscid);
static void dm_api_event_udsc_request_transfer_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen);
static void dm_api_event_udsc_task_end_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen);
static void dm_api_event_service_result_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen);
static void dm_api_event_security_access_keygen_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen);
static void dm_api_event_keepalive_request_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen);
static void dm_api_event_36_transfer_progress_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen);
static void dm_api_event_instance_destroy_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen);

static yuint8 *dma_rxbuf(ydiag_master_api *dm_api)
{
    return dm_api->event_emit_rxbuf;
}

static int dma_rxbuf_size(ydiag_master_api *dm_api)
{
    return sizeof(dm_api->event_emit_rxbuf);
}

static yuint8 *dma_txbuf(ydiag_master_api *dm_api)
{
    return dm_api->event_emit_txbuf;
}

static int dma_txbuf_size(ydiag_master_api *dm_api)
{
    return sizeof(dm_api->event_emit_txbuf);
}

static void dm_api_errcode_set(ydiag_master_api *dm_api, int errcode)
{
    dm_api->errcode = errcode;
}

static int dm_api_sendto_diag_master(ydiag_master_api *dm_api, yuint8 *buff, yuint32 size)
{
#ifdef __linux__
    struct sockaddr_un un;
    ssize_t bytesSent = -1;
    
    if (dm_api->event_emit_sockfd < 0) {
        return -1;
    }
    
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    snprintf(un.sun_path, sizeof(un.sun_path), dm_api->dm_path, strlen(dm_api->dm_path));    
    bytesSent = sendto(dm_api->event_emit_sockfd, buff, size, 0,\
                        (struct sockaddr *)&un, sizeof(struct sockaddr_un));
    // log_hex_d("send", buff, bytesSent);
    return bytesSent;
#endif /* __linux__ */
#ifdef _WIN32
    int bytes = -1;

    SOCKADDR_IN remoteAddress;
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(dm_api->dm_port);
    remoteAddress.sin_addr.S_un.S_addr = htonl(dm_api->dm_ip);
    bytes = sendto(dm_api->event_emit_sockfd, buff, size, 0, \
        (SOCKADDR*)&remoteAddress, sizeof(remoteAddress));
    return bytes;
#endif /* _WIN32 */
}
/*
    diag master api创建和diag master通信的SOCKET
*/
#ifdef _WIN32
static SOCKET dm_api_ipc_create(char *ip, short port)
{
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) <= 0) {
        log_e("inet_pton() error");
        return -1;
    }

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!(udpSocket > 0)) {
        log_e("socket() error");
        return -2;
    }

    SOCKADDR_IN localAddress;
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(port); // 本地端口号  
    localAddress.sin_addr.S_un.S_addr = addr.s_addr;
    if (bind(udpSocket, (SOCKADDR*)&localAddress, sizeof(localAddress)) != 0) {
        closesocket(udpSocket);
        log_e("bind() error");
        return -3;
    }
    log_d("diag master api udpSocket = %d", udpSocket);
    
    return udpSocket;
}
#endif /* _WIN32 */

#ifdef __linux__
/* 暂时不再使用这个函数 */
static int dm_api_ipc_create(struct sockaddr_un *un)
{
    int handle = -1;
    int flag = 0;

    if (!un) {
        goto CREATE_FALIED;
    }

    handle = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (handle < 0) {
        goto CREATE_FALIED;    
    }
    flag = fcntl(handle, F_GETFL, 0);
    fcntl(handle, F_SETFL, flag | O_NONBLOCK);

    unlink(un->sun_path);
    if (bind(handle, (struct sockaddr *)un, sizeof(struct sockaddr_un)) < 0) {
        goto CREATE_FALIED;
    }

    return handle;
CREATE_FALIED:
    if (handle > 0) {
        close(handle);
    }

    return -1;
}
#endif /* #ifdef __linux_ */

static int dm_api_event_accept_handler(ydiag_master_api *dm_api, yuint8 *msg, yuint32 msglen)
{
    yuint32 evtype = 0;
    yuint32 recode = 0;
    yuint32 tv_sec = 0; yuint32 tv_usec = 0;
    yuint16 udscid = 0;
    struct dm_api_udscs *dm_udsc = 0;

    ydm_common_decode(msg, msglen, &evtype, &recode, &udscid, &tv_sec, &tv_usec);
    if (evtype != DM_EVENT_OMAPI_KEEPALIVE_EMIT) {
        log_d("Recv event: 0x%02X(%s) Recode: %d(%s) udsc ID: %d tv_sec: %d tv_usec: %d", \
            evtype, ydm_ipc_event_str(evtype), recode, ydm_event_rcode_str(recode), udscid, tv_sec, tv_usec);
    }
    if (DM_EVENT_ACCEPT_MASK & evtype) {
        /* 只处理请求消息，应答消息直接忽略 */
        return -1;
    }
    if (evtype != DM_EVENT_OMAPI_KEEPALIVE_EMIT) {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            return -2;
        }

    }
    switch (evtype) {
        case DM_EVENT_SERVICE_INDICATION_EMIT:
            dm_api_event_udsc_request_transfer_handler(dm_api, recode, udscid, msg, msglen);
            break;
        case DM_EVENT_UDS_CLIENT_RESULT_EMIT:                
            dm_api_event_udsc_task_end_handler(dm_api, recode, udscid, msg, msglen);
            break;
        case DM_EVENT_UDS_SERVICE_RESULT_EMIT:
            dm_api_event_service_result_handler(dm_api, recode, udscid, msg, msglen);
            break;
        case DM_EVENT_UDS_SERVICE_27_SA_SEED_EMIT:
            dm_api_event_security_access_keygen_handler(dm_api, recode, udscid, msg, msglen);
            break;
        case DM_EVENT_OMAPI_KEEPALIVE_EMIT:
            dm_api_event_keepalive_request_handler(dm_api, recode, udscid, msg, msglen);
            break;
        case DM_EVENT_36_TRANSFER_PROGRESS_EMIT:
            dm_api_event_36_transfer_progress_handler(dm_api, recode, udscid, msg, msglen);
            break;
        case DM_EVENT_INSTANCE_DESTORY_EMIT:
            dm_api_event_instance_destroy_handler(dm_api, recode, udscid, msg, msglen);
            break;
        default:
            log_w("OTA Master API IPC unknown event => %d", evtype);    
            break;
    }

    return 0;
}

/* 接收来自OTA MASTER的预期响应，接收到预期值返回0，其他值非预期 */
static int dm_api_recv_event_message_handler(ydiag_master_api *dm_api, yuint16 *id, yuint32 expect_event, yuint32 expect_code)
{
    yuint32 reply_event = 0;
    yuint32 recode = 0;
    yuint32 tv_sec = 0; yuint32 tv_usec = 0;
    yuint16 udscid = 0;
    int rbytes = -1;

    if (!(expect_event & DM_EVENT_ACCEPT_MASK)) {
        return -1;
    }

    /* 这里有一个接收超时时间就不判断命令有效时间了 */
    for ( ;(rbytes = ydm_recvfrom(dm_api->event_emit_sockfd, DMAPI_RX_BUFF(dm_api), dm_api->event_valid_time)) > 0; ) {
        ydm_common_decode(DMAPI_RX_BUFF(dm_api), &reply_event, &recode, &udscid, &tv_sec, &tv_usec);
        log_d("Recv event: %s Recode: %d(%s) udsc ID: %d", \
            ydm_ipc_event_str(reply_event), recode, ydm_event_rcode_str(recode), udscid);
        if (reply_event & DM_EVENT_ACCEPT_MASK) {            
            /* 如果是响应消息 */
            if (reply_event == expect_event && recode == expect_code) {
                /* 响应符合预期 */
                if (id) {
                    *id = udscid;
                }
                return DM_IPC_SYNC_RECV_EXPECT;
            }
            else {
                log_w("expect reply: %d expect code: %d reply: %d recode: %d\n", expect_event, expect_code, reply_event, recode);
                if (reply_event != expect_event) {
                    continue;
                }
                dm_api_errcode_set(dm_api, recode);
                /* 响应不及预期 */
                return DM_IPC_SYNC_RECV_UNEXPECT;
            }
        }
        else {
            /* 这里防止遗漏otamaster发起的请求消息 */
            if (dm_api_event_accept_handler(dm_api, dma_rxbuf(dm_api), rbytes) != 0) {
                return DM_IPC_SYNC_RECV_EVENT_HANDLE_ERR;
            }
        }
    }

    return DM_IPC_SYNC_RECV_TIMEOUT;
}

/*
    获取diag master api端保存的UDS客户端信息
*/
static struct dm_api_udscs *dm_api_udsc_get(ydiag_master_api *dm_api, yuint16 udscid)
{
    /* UDS客户端ID不能为无效值，也不能未被diag master api记录 */
    if (!(udscid < DM_UDSC_CAPACITY_MAX) || \
        dm_api->udscs[udscid].isvalid == 0) {        
        dm_api_errcode_set(dm_api, DM_ERR_UDSC_INVALID);
        return 0;
    }
        
    return &dm_api->udscs[udscid];
}


int dm_api_connect_ota_master(ydiag_master_api *dm_api)
{
    yuint32 evtype = DM_EVENT_CONNECT_DIAG_MASTER_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint32 tv_sec = 0; yuint32 tv_usec = 0; yuint16 id = 0;
#ifdef __linux__
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
    ssize_t bytes = -1;
#endif /* __linux__ */
    char dm_path[256] = {0};
#ifdef _WIN32
    char dm_addr_info[256] = { 0 };
#endif /* _WIN32 */
    yuint32 keepalive_inter = 1000;
    omapi_connect_master_t cf;
    int rlen = 0;
    
    assert(dm_api);   
    memset(&cf,  0, sizeof(cf));

    cf.keepalive_inter = keepalive_inter;
#ifdef __linux__
    snprintf(cf.event_emit_path, sizeof(cf.event_emit_path), "%s", dm_api->event_emit_path);
    snprintf(cf.event_listen_path, sizeof(cf.event_listen_path), "%s", dm_api->event_listen_path);
#endif /* __linux__ */
#ifdef _WIN32
    snprintf(dm_addr_info, sizeof(dm_addr_info), "%s:%d", dm_api->ip, dm_api->port);
    snprintf(cf.event_emit_path, sizeof(cf.event_emit_path), "%s", dm_addr_info);
#endif /* _WIN32 */
    rlen = ydm_omapi_connect_master_encode(dma_txbuf(dm_api) + DM_IPC_EVENT_MSG_SIZE, \
        dma_txbuf_size(dm_api) - DM_IPC_EVENT_MSG_SIZE, &cf);
    ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, 0, IPC_USE_USER_TIME_STAMP);
#ifdef __linux__
    /* 域套接字通信 */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, OMS_UNIX_SOCKET_PATH, sizeof(OMS_UNIX_SOCKET_PATH));    
    bytes = sendto(dm_api->event_emit_sockfd, dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE + rlen, 0,\
                        (struct sockaddr *)&un, sizeof(struct sockaddr_un));
#endif /* __linux__ */
#ifdef _WIN32
    /* UDP通信 */
    int bytes = -1;
    struct in_addr addr;
    if (inet_pton(AF_INET, DMS_UDS_SOCKET_PATH, &addr) <= 0) {
        return -1;
    }
    SOCKADDR_IN remoteAddress;
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(DMS_UDS_SOCKET_PORT);
    remoteAddress.sin_addr.S_un.S_addr = addr.s_addr;
    bytes = sendto(dm_api->event_emit_sockfd, dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE + rlen, 0, \
                        (SOCKADDR*)&remoteAddress, sizeof(remoteAddress));
#endif /* _WIN32 */
    if (bytes == (DM_IPC_EVENT_MSG_SIZE + rlen)) {        
        fd_set readfds;
        struct timeval timeout;
        int sret = 0;
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 1000; /* 1000ms */
        FD_ZERO(&readfds);
        FD_SET(dm_api->event_emit_sockfd, &readfds);
        if ((sret = select(dm_api->event_emit_sockfd + 1, &readfds, NULL, NULL, &timeout)) > 0 &&\
            FD_ISSET(dm_api->event_emit_sockfd, &readfds)) {
#ifdef __linux__
            bytes = recvfrom(dm_api->event_emit_sockfd, DMAPI_RX_BUFF(dm_api), 0, \
                                    (struct sockaddr*)&un, (socklen_t *)&len);
#endif /* __linux__ */
#ifdef _WIN32
            SOCKADDR_IN fromAddress;
            int fromLength = sizeof(fromAddress);
            int bytes = recvfrom(dm_api->event_emit_sockfd, DMAPI_RX_BUFF(dm_api), 0, (SOCKADDR*)&fromAddress, &fromLength);
#endif /* _WIN32 */
            if (bytes > 0) {     
                ydm_common_decode(DMAPI_RX_BUFF(dm_api), &evtype, &recode, &id, &tv_sec, &tv_usec);
                log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d tv_sec: %d tv_usec: %d", \
                    evtype, ydm_ipc_event_str(evtype), recode, ydm_event_rcode_str(recode), id, tv_sec, tv_usec);
                if (evtype == DM_EVENT_CONNECT_DIAG_MASTER_ACCEPT && recode == DM_ERR_NO) {
                    yuint32 path_len = TYPE_CAST_UINT(pointer_offset_nbyte(dma_rxbuf(dm_api), DMREP_CONNECT_DM_PATH_LEN_OFFSET));
                    snprintf(dm_path, sizeof(dm_path), "%s", pointer_offset_nbyte(dma_rxbuf(dm_api), DMREP_CONNECT_DM_PATH_OFFSET));
                    log_d("diag master path => %s", dm_path);
#ifdef __linux__
                    if (access(dm_path, F_OK) != 0) {
                        /* 无法获取diag master的存在 */
                        log_e("Not found diag master unix socket path (%s)", dm_path);
                        return -1;
                    }
                    snprintf(dm_api->dm_path, sizeof(dm_api->dm_path), "%s", dm_path);
#endif /* __linux__ */
#ifdef _WIN32
                    char* dm_ip = NULL;
                    char* dm_port = NULL;
                    char* context = NULL;
                    dm_ip = strtok_s(dm_path, ":", &context);
                    dm_port = strtok_s(NULL, "", &context);
                    if (dm_ip == NULL || dm_port == NULL) {
                        log_e("diag master address format error");
                        return -2;
                    }

                    struct in_addr addr;
                    if (inet_pton(AF_INET, dm_ip, &addr) <= 0) {
                        log_e("inet_pton() error");
                        return -3;
                    }
                    dm_api->dm_ip = ntohl(addr.s_addr);
                    dm_api->dm_port = atoi(dm_port);
#endif /* _WIN32 */
                    return 0;
                }
            }
        } 
    }

    return -4;      
}

/* 与diag master IPC 同步请求通信,0表示请求结果符合预期 */
static int dm_api_sync_event_emit_to_master(ydiag_master_api *dm_api, int rlen, yuint16 *udscid, yuint32 expect_event, yuint32 expect_code)
{
    int recode = 0;
    int retrynum = IPC_SYNC_MSG_TRYNUM;
    int sret = 0;

    do {
        if ((sret = dm_api_sendto_diag_master(dm_api, dma_txbuf(dm_api), rlen)) == rlen) {
            recode = dm_api_recv_event_message_handler(dm_api, udscid, expect_event, expect_code);
            switch (recode) {            
                case DM_IPC_SYNC_RECV_EXPECT:
                    return 0; /* IPC通信正常 */
                case DM_IPC_SYNC_RECV_UNEXPECT:
                case DM_IPC_SYNC_RECV_EVENT_HANDLE_ERR:
                case DM_IPC_SYNC_RECV_TIMEOUT:
                default:    
                    log_d("IPC sync recode %d", recode);
                    /* 重试 */
                    break;
            }
        }
        else {
            log_d("IPC send error %d", sret);
        }
        /* 重发的消息需要重置，时间参数 */
        ydm_common_ipc_tv_reset(dma_txbuf(dm_api), rlen);
    } while ((--retrynum) > 0);

    return -1;
}

#ifdef _WIN32
DWORD WINAPI dm_api_event_accept_loop_thread_run(LPVOID* arg)
#else /* _WIN32 */
void *dm_api_event_accept_loop_thread_run(void *arg)
#endif /* _WIN32 */
{
    ydiag_master_api *dm_api = arg;
    int recv_bytes = -1;

    if (dm_api == 0) {
        dm_api->thread_flag = 0;
        return 0;
    }

    log_d("diag master api thread runing");
    while (dm_api->thread_flag & DM_API_THREAD_ENABLE) {
        dm_api->thread_flag |= DM_API_THREAD_RUNING;
        recv_bytes = ydm_recvfrom(dm_api->event_listen_sockfd, dm_api->event_listen_rxbuf, sizeof(dm_api->event_listen_rxbuf), 1);
        if (recv_bytes > 0) {
            MUTEX_LOCK(dm_api->mutex); 
            dm_api_event_accept_handler(dm_api, dm_api->event_listen_rxbuf, recv_bytes);
            memset(dm_api->event_listen_rxbuf, 0, recv_bytes);
            MUTEX_UNLOCK(dm_api->mutex);
        }
    }
    dm_api->thread_flag &= ~DM_API_THREAD_RUNING;
    dm_api->thread_flag = 0;
    log_d("diag master api thread exit");

    return 0;
}
/* 
    diag master api创建
*/
ydiag_master_api *ydm_api_create()
{
    int udsc_ii = 0;
    int tcnt = 0;

    log_d("diag master api start create.");
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log_e("WSAStartup() error");
        return NULL;
    }
#endif /* _WIN32 */
    ydiag_master_api *dm_api = malloc(sizeof(*dm_api));
    if (dm_api == 0) {
        log_e("fatal error malloc failed.");
        return 0;
    }
    memset(dm_api, 0, sizeof(*dm_api));
    dm_api->event_emit_sockfd = -1;
    dm_api->event_listen_sockfd = -1;
    /* API同步接口的超时时间 */
    dm_api->event_valid_time = IPC_MSG_VALID_TIME;
#ifdef __linux__
    /* 生成unix socket 路径     */
    snprintf(dm_api->event_emit_path, sizeof(dm_api->event_emit_path), DM_API_UNIX_SOCKET_PATH_PREFIX"%d_%d", \
        getpid() % 0xffff, rand() % 0xffff);
    if (access(dm_api->event_emit_path, F_OK) == 0) {
        log_e("fatal error (%s) already exist.", dm_api->event_emit_path);
        /* 直接退出防止干扰其他正常工作的API */
        free(dm_api);
        return 0;
    }
    /* 生成unix socket 路径callback信息使用     */
    snprintf(dm_api->event_listen_path, sizeof(dm_api->event_listen_path), DM_API_UNIX_SOCKET_CALL_PATH_PREFIX"%d_%d", \
        getpid() % 0xffff, rand() % 0xffff);
    if (access(dm_api->event_listen_path, F_OK) == 0) {
        log_e("fatal error (%s) already exist.", dm_api->event_listen_path);
        /* 直接退出防止干扰其他正常工作的API */
        free(dm_api);
        return 0;
    }
#endif /* #ifdef __linux__ */

#ifdef _WIN32
    snprintf(dm_api->ip, sizeof(dm_api->ip), "%s", DM_UDS_SOCKET_PATH);
    dm_api->port = DM_UDS_SOCKET_PORT_BASE;
    srand(time(0));
    dm_api->port = rand() % 1000 + DM_UDS_SOCKET_PORT_BASE; /* 随机生成一个端口号 */
    dm_api->event_emit_sockfd = dm_api_ipc_create(dm_api->ip, dm_api->port);
    if (!(dm_api->event_emit_sockfd > 0)) {
        log_e("fatal error dm_api_ipc_create failed");
        goto CREAT_FAILED;
    }
#endif /* _WIN32  */
#ifdef __linux__   
    struct sockaddr_un un;
    memset(&un, 0, sizeof(struct sockaddr_un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, dm_api->event_emit_path, strlen(dm_api->event_emit_path));

    /* 创建域套接字用于和OTA MASTER通信 */
    dm_api->event_emit_sockfd = ydm_ipc_channel_create(&un, dma_rxbuf_size(dm_api), dma_txbuf_size(dm_api));
    if (dm_api->event_emit_sockfd < 0) {
        log_e("fatal error dm_api_ipc_create failed");
        goto CREAT_FAILED;
    }
    log_d("OM API Unix path => %s", dm_api->event_emit_path);

    memcpy(un.sun_path, dm_api->event_listen_path, strlen(dm_api->event_listen_path));
    /* 创建域套接字用于和OTA MASTER通信 */
    dm_api->event_listen_sockfd = dm_api_ipc_create(&un);
    if (dm_api->event_listen_sockfd < 0) {
        log_e("fatal error dm_api_ipc_create failed");
        goto CREAT_FAILED;
    }
    log_d("OM API Unix path => %s", dm_api->event_listen_path);
#endif /* #ifdef __linux__ */
    /* 连接OTAM ASTER */
    if (dm_api_connect_ota_master(dm_api) < 0) {
        log_e("fatal error connect diag master failed.");
        goto CREAT_FAILED;
    }

    /* OTA MASTER API这边的UDS客户端信息初始化 */
    dm_api->udsc_cnt = 0;
    for (udsc_ii = 0; udsc_ii < DM_UDSC_CAPACITY_MAX; udsc_ii++) {
        dm_api->udscs[udsc_ii].isvalid = 0;
        dm_api->udscs[udsc_ii].udsc_id = udsc_ii;            
    }
    memset(dma_txbuf(dm_api), 0, dma_txbuf_size(dm_api));
    memset(dma_rxbuf(dm_api), 0, dma_rxbuf_size(dm_api));    
    memset(dm_api->event_listen_rxbuf, 0, sizeof(dm_api->event_listen_rxbuf));
    
    /* 创建线程用于事件循环接收OTA MASTER主动发送的数据 */
    log_d("diag master api thread enable");
    dm_api->thread_flag |= DM_API_THREAD_ENABLE;
    log_d("dm_api->thread_flag => %d", dm_api->thread_flag);
#ifdef _WIN32
    if ((dm_api->el_tidp = CreateThread(NULL, 0, dm_api_event_accept_loop_thread_run, dm_api, 0, NULL)) == NULL) {
        log_w(" diag master api event lopp thread create faile.");
    }
#else /* _WIN32 */
    if (pthread_create(&dm_api->el_tidp, 0, dm_api_event_accept_loop_thread_run, dm_api) != 0) {
        log_w(" diag master api event lopp thread create faile."); 
    }
#endif /* _WIN32 */
    if (!(dm_api->el_tidp > 0)) {
        log_d("diag master api thread disable");
        dm_api->thread_flag = 0;
    }

    /* 等待线程正常开始工作，具有超时时间 */
    if (dm_api->thread_flag & DM_API_THREAD_ENABLE) {
        tcnt = 50;
        while (tcnt --> 0) {
            if (dm_api->thread_flag & DM_API_THREAD_RUNING) {
                log_d("diag master api thread runing");
                break;
            }
            struct timeval timeout;

            timeout.tv_sec = 0;
            timeout.tv_usec = 1000;
            select(0, 0, 0, 0, &timeout);
        }
    }
    log_d("diag master api success create.");
    
    pthread_mutex_init(&dm_api->mutex, 0);
    dm_api->vf = DM_API_VALID_FLAG;
    return dm_api;
CREAT_FAILED:    
    log_e(" diag master create error");
    if (dm_api->event_emit_sockfd > 0) {
#ifdef _WIN32
        closesocket(dm_api->event_emit_sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(dm_api->event_emit_sockfd);
#endif /* __linux__ */
    }
    
    if (dm_api->event_listen_sockfd > 0) {
#ifdef _WIN32
        closesocket(dm_api->event_listen_sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(dm_api->event_listen_sockfd);
#endif /* __linux__ */
    }
    
#ifdef __linux__ 
    unlink(dm_api->event_emit_path);
    unlink(dm_api->event_listen_path);
#endif /* __linux__ */
    free(dm_api);
#ifdef _WIN32
    WSACleanup();
#endif /* _WIN32 */

    return 0;
}

/* 判断OTA MASTER API是否有效 */
int ydm_api_is_valid(ydiag_master_api *dm_api)
{
    yuint32 evtype = DM_EVENT_OMAPI_CHECK_VALID_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint16 udscid = 0;
    int rv = 0;

    if (dm_api == 0 || dm_api->vf != DM_API_VALID_FLAG) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);
    if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE, \
            &udscid, DM_EVENT_OMAPI_CHECK_VALID_ACCEPT, DM_ERR_NO) == 0) {    
        rv = 1;
    }       
    MUTEX_UNLOCK(dm_api->mutex);
        
    return rv; 
}

int ydm_api_destroy(ydiag_master_api *dm_api)
{
    int udsc_ii = 0;

    if (dm_api == 0) {
        return -1;
    }

    if (dm_api->el_tidp == pthread_self()) {
        log_w("Unable to destroy while running");
        /* 不能在运行时销毁 */
        dm_api->thread_flag = 0; /* 线程可以先退出 */
        return -2;
    }

    /* 通知销毁 */
    if (dm_api->vf == DM_API_VALID_FLAG) {
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), DM_EVENT_INSTANCE_DESTORY_EMIT, 0, 0, IPC_USE_USER_TIME_STAMP);
        dm_api_sendto_diag_master(dm_api, dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE);
    }

    /* 线程退出 */
    dm_api->thread_flag = 0;
    if (dm_api->event_emit_sockfd > 0) {
#ifdef _WIN32
        closesocket(dm_api->event_emit_sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(dm_api->event_emit_sockfd);
#endif /* __linux__ */
    }
    
    if (dm_api->event_listen_sockfd > 0) {
#ifdef _WIN32
        closesocket(dm_api->event_listen_sockfd);
#endif /* _WIN32 */
#ifdef __linux__
        close(dm_api->event_listen_sockfd);
#endif /* __linux__ */
    }
    
#ifdef __linux__ 
    unlink(dm_api->event_emit_path);
    unlink(dm_api->event_listen_path);
#endif /* __linux__ */
    for (udsc_ii = 0; udsc_ii < DM_UDSC_CAPACITY_MAX; udsc_ii++) {
        dm_api_udsc_reset(dm_api, udsc_ii);         
    }
    pthread_mutex_destroy(&dm_api->mutex);
    memset(dm_api, 0, sizeof(*dm_api));
    free(dm_api);
#ifdef _WIN32
    WSACleanup();
#endif /* _WIN32 */

    return 0;
}

int ydm_api_sockfd(ydiag_master_api *oa_api)
{
    if (oa_api == 0) {
        return -1;
    }
    return oa_api->event_emit_sockfd;
}

void ydm_debug_enable(int eb)
{
    __dbg_info_enable__ = eb;
}

/*
    重置diag master api端的UDS客户端信息
*/
static int dm_api_udsc_reset(ydiag_master_api *dm_api, yuint16 udscid)
{
    if (udscid < DM_UDSC_CAPACITY_MAX) {
        struct dm_api_udscs *dm_udsc = &dm_api->udscs[udscid];
        if (dm_udsc->isvalid) {
            dm_api->udsc_cnt--;
            if (dm_api->udsc_cnt > DM_UDSC_CAPACITY_MAX || \
                dm_api->udsc_cnt < 0) {
                dm_api->udsc_cnt = 0;
            }
        }
        dm_udsc->isvalid = 0;     
        memset(&dm_udsc->config, 0, sizeof(dm_udsc->config));
        if (dm_udsc->rr_handler) {
            free(dm_udsc->rr_handler);
            dm_udsc->rr_handler = 0;
        }
        dm_udsc->rr_size = 0;
        dm_udsc->rr_cnt = 0;
        
        dm_udsc->udsc_request_transfer_cb = 0;
        dm_udsc->udsc_request_transfer_arg = 0;
        dm_udsc->udsc_task_end_cb = 0;
        dm_udsc->udsc_task_end_arg = 0;
        dm_udsc->security_access_keygen_cb = 0;
        dm_udsc->security_access_keygen_arg = 0;
        dm_udsc->transfer_progress_cb = 0;
        dm_udsc->transfer_progress_arg = 0;
        dm_udsc->diag_req_id = 0;
        dm_udsc->diag_resp_id = 0;
    }

    return 0;
}

/* 创建uds客户端 */
int ydm_api_udsc_create(ydiag_master_api *dm_api, udsc_create_config *config)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_CREATE_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint16 udscid = 0;
    struct dm_api_udscs *dm_udsc = 0;
    udsc_create_config def_config;
    int clen = 0;
    int rv = -1;

    if (dm_api == 0) {
        return rv;
    }
    
    if (config == 0) {
        memset(&def_config, 0, sizeof(def_config));
        config = &def_config;
    }

    config->service_item_capacity = config->service_item_capacity < SERVICE_ITEM_SIZE_DEF ? \
                             SERVICE_ITEM_SIZE_DEF : config->service_item_capacity; 
    config->service_item_capacity = config->service_item_capacity > SERVICE_ITEM_SIZE_MAX ? \
                             SERVICE_ITEM_SIZE_MAX : config->service_item_capacity; 
    MUTEX_LOCK(dm_api->mutex);
    do {
        /* 请求数据编码 */
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);
        clen = ydm_udsc_create_config_encode(pointer_offset_nbyte(dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE), \
                    dma_txbuf_size(dm_api) - DM_IPC_EVENT_MSG_SIZE, config);    
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE + clen, \
                &udscid, DM_EVENT_UDS_CLIENT_CREATE_ACCEPT, DM_ERR_NO) == 0)  {
            if (udscid < DM_UDSC_CAPACITY_MAX) {
                dm_api_udsc_reset(dm_api, udscid);
                dm_udsc = &dm_api->udscs[udscid];
                dm_udsc->rr_size = config->service_item_capacity;
                log_d("dm_udsc->rr_size => %d", dm_udsc->rr_size);
                dm_udsc->rr_handler = malloc(sizeof(struct uds_service_result_handler_s) * dm_udsc->rr_size);
                if (dm_udsc->rr_handler == 0) {
                    rv = -1;
                    break; /* while() */
                }
                memset(dm_udsc->rr_handler, 0, sizeof(struct uds_service_result_handler_s) * dm_udsc->rr_size);
                dm_udsc->isvalid = 1;
                dm_udsc->udsc_id = udscid;                       
                memcpy(&dm_udsc->config, config, sizeof(*config));
                dm_api->udsc_cnt++; 
                rv = udscid;
                break; /* while() */
            }
        }   
    }while (0);
    MUTEX_UNLOCK(dm_api->mutex);
    if (rv >= 0) {
        log_d("UDS client[%d] created successfully", udscid);
    }
    else {
        log_e("UDS client created failed");   
    }

    return rv;
}

/*
    销毁diag master端创建的UDS客户端
*/
int ydm_api_udsc_destroy(ydiag_master_api *dm_api, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_DESTORY_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;
    
    if (dm_api == 0) {
        return rv;
    }

    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -1;
            break; /* while */
        }
        /* 移除OTA MASTER API端的uds客户端信息 */                
        dm_api_udsc_reset(dm_api, udscid);
        /* 移除OTA MASTER 端的uds客户端信息 */
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE, \
                &udscid, DM_EVENT_UDS_CLIENT_DESTORY_ACCEPT, DM_ERR_NO) == 0) {    
            rv = 0;
            break; /* while */
        }          
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
        
    return rv;      
}

int ydm_api_doipc_create(ydiag_master_api *dm_api, yuint16 udscid, doipc_config_t *config)
{
    yuint32 evtype = DM_EVENT_DOIP_CLIENT_CREATE_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    int clen = 0;
    int rv = -1;
    
    if (dm_api == 0) {
        return rv;
    }

    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -1;
            break; /* while */
        }
        clen = ydm_doipc_create_config_encode(pointer_offset_nbyte(dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE), \
                                dma_txbuf_size(dm_api) - DM_IPC_EVENT_MSG_SIZE, config);
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE + clen, \
                &udscid, DM_EVENT_DOIP_CLIENT_CREATE_ACCEPT, DM_ERR_NO) == 0) {    
            rv = 0;            
            break; /* while */
        }
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
        
    return rv;
}

/*  请求OTA MASTER启动UDS 客户端开始执行诊断任务 */
int ydm_api_udsc_start(ydiag_master_api *dm_api, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_START_UDS_CLIENT_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;
    
    if (dm_api == 0) {
        return rv;
    }

    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {        
            log_e("The udsc id is invalid => %d", udscid);
            rv = -1;
            break; /* while */
        }

        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE, \
                &udscid, DM_EVENT_START_UDS_CLIENT_ACCEPT, DM_ERR_NO) ==  0) {    
            rv = 0;            
            break; /* while */
        }            
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
    
    return rv;      
}

/* 获取指定UDS客户端是否正在执行任务返回true表示任务正在执行中 */
int ydm_api_udsc_is_active(ydiag_master_api *dm_api, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    yuint32 isactive = 0;
    int rv = 0;
    
    if (dm_api == 0) {
        return rv;
    }

    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {        
            log_e("The udsc id is invalid => %d", udscid);
            rv = 0;
            break; /* while */
        }
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE, \
                &udscid, DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_ACCEPT, DM_ERR_NO) == 0) {    
            isactive = TYPE_CAST_UINT(pointer_offset_nbyte(dma_rxbuf(dm_api), UDSC_IS_ACTIVE_OFFSET));
            log_d("UDS client isactive => %d", isactive);
            if (isactive) {
                rv = 1;
            }
            else {
                rv = 0;
            }               
            break; /* while */
        }                
    }while (0);
    MUTEX_UNLOCK(dm_api->mutex); 
    
    return rv; 
}

/*  请求OTA MASTER中止UDS 客户端执行诊断任务 */
int ydm_api_udsc_stop(ydiag_master_api *dm_api, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_STOP_UDS_CLIENT_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;

    if (dm_api == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -1;    
            break; /* while */
        }
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);    
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE, \
                &udscid, DM_EVENT_STOP_UDS_CLIENT_ACCEPT, DM_ERR_NO) == 0) {    
            rv = 0;        
            break; /* while */
        }
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex); 
    
    return rv; 
}

/*
    请求diag master重置，将导致所有的UDS任务停止并且销毁所有的UDS客户端
*/
int ydm_api_master_reset(ydiag_master_api *dm_api)
{
    yuint32 evtype = DM_EVENT_DIAG_MASTER_RESET_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint16 udscid = 0;
    int rv = -1;

    if (dm_api == 0) {
        return rv;
    }
    MUTEX_LOCK(dm_api->mutex);
    ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, 0, IPC_USE_SYSTEM_TIME_STAMP);    
    if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE, \
        &udscid, DM_EVENT_DIAG_MASTER_RESET_ACCEPT, DM_ERR_NO) == 0) {    
        rv = 0;
    }
    MUTEX_UNLOCK(dm_api->mutex); 
    
    return rv; 
}

/*
    在指定的UDS客户端上配置增加一个诊断服务项
*/
int ydm_api_udsc_service_config(ydiag_master_api *dm_api, yuint16 udscid, service_config *config)
{
    yuint32 evtype = DM_EVENT_UDS_SERVICE_ITEM_ADD_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    yuint8 *dp = 0;
    int clen = 0;
    int rv = -1;

    if (dm_api == 0 || config == 0) {
        return rv;
    }
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -1;
            break; /* while */
        }
        dp = pointer_offset_nbyte(dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE);
        clen = ydm_service_config_encode(dp, dma_txbuf_size(dm_api) - DM_IPC_EVENT_MSG_SIZE, config);
        /* 请求数据编码 */
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);    
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE + clen, \
                &udscid, DM_EVENT_UDS_SERVICE_ITEM_ADD_ACCEPT, DM_ERR_NO) == 0) {    
            rv = 0;            
            break; /* while */
        }  
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex); 
        
    return rv;      
}

int ydm_api_udsc_sa_key(ydiag_master_api *dm_api, yuint16 udscid, yuint8 level, const yuint8 *key, yuint32 key_size)
{
    yuint32 evtype = DM_EVENT_UDS_SERVICE_27_SA_KEY_EMIT;
    yuint32 recode = DM_ERR_NO;
    int rv = -1;

    if (dm_api == 0 || key == 0) {
        return rv;
    }
    if (dma_txbuf_size(dm_api) < SERVICE_SA_KEY_OFFSET + key_size) {
        log_w("excessive data length the maximum support %d byte", \
            dma_txbuf_size(dm_api) - SERVICE_SA_KEY_OFFSET);
        return rv;
    }
    MUTEX_LOCK(dm_api->mutex);
    /* encode 1 byte level */
    memappend(pointer_offset_nbyte(dma_txbuf(dm_api), SERVICE_SA_KEY_LEVEL_OFFSET), &level, SERVICE_SA_KEY_LEVEL_SIZE);    
    /* encode 2 byte key长度 */
    memappend(pointer_offset_nbyte(dma_txbuf(dm_api), SERVICE_SA_KEY_SIZE_OFFSET), &key_size, SERVICE_SA_KEY_SIZE_SIZE);    
    /* encode n byte key数据 */
    memappend(pointer_offset_nbyte(dma_txbuf(dm_api), SERVICE_SA_KEY_OFFSET), key, key_size);    
    /* encode 通用的头部固定数据 */
    ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);    
    if (dm_api_sync_event_emit_to_master(dm_api, SERVICE_SA_KEY_OFFSET + key_size, \
        &udscid, DM_EVENT_UDS_SERVICE_27_SA_KEY_ACCEPT, DM_ERR_NO) == 0) {
        rv = 0;
    }
    MUTEX_UNLOCK(dm_api->mutex); 
        
    return rv;      
}

/*
    设置UDS诊断请求处理回调函数
*/
int ydm_api_udsc_service_result_callback_set(ydiag_master_api *dm_api, yuint16 udscid, uds_service_result_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;

    if (dm_api == 0 || call == 0) {
        return rv;
    }

    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -2;
            break; /* while */
        }

        if (!(dm_udsc->rr_cnt < SERVICE_ITEM_SIZE_DEF)) {
            rv = -3;
            break; /* while */
        }
        dm_udsc->rr_handler[dm_udsc->rr_cnt].service_result = call;
        dm_udsc->rr_handler[dm_udsc->rr_cnt].rr_arg = arg;
        dm_udsc->rr_cnt++;
        rv = dm_udsc->rr_cnt;
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
    
    return rv;
}

/*
    UDS客户端的通用配置
*/
int ydm_api_udsc_runtime_config(ydiag_master_api *dm_api, yuint16 udscid, udsc_runtime_config *config)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_GENERAL_CFG_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    yuint8 *dp = 0;
    int clen = 0;    
    int rv = -1;

    if (dm_api == 0 || config == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -1; 
            break; /* while */
        }
        dp = pointer_offset_nbyte(dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE);
        clen = ydm_general_config_encode(dp, dma_txbuf_size(dm_api) - DM_IPC_EVENT_MSG_SIZE, config);
        /* 请求数据编码 */
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);    
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE + clen, \
                &udscid, DM_EVENT_UDS_CLIENT_GENERAL_CFG_ACCEPT, DM_ERR_NO) == 0) {    
            rv = 0;
            break; /* while */
        }        
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
        
    return rv;      
}

/*
    设置UDS服务请求处理的回调函数
*/
int ydm_api_udsc_request_transfer_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_request_transfer_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;

    if (dm_api == 0 || call == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -2;
            break; /* while */
        }
        dm_udsc->udsc_request_transfer_cb = call;
        dm_udsc->udsc_request_transfer_arg = arg;        
        rv = 0;
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);

    return rv;
}

static void dm_api_event_udsc_request_transfer_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen)
{
    yuint32 A_SA = 0, A_TA = 0, payload_length = 0, TA_TYPE = 0; 
    yuint8 *dp = 0;
    yuint8 *payload = 0;
    struct dm_api_udscs *dm_udsc = 0;
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    dp = pointer_offset_nbyte(msg, DM_IPC_EVENT_MSG_SIZE);
    A_SA = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_INDICATION_A_SA_OFFSET));
    A_TA = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_INDICATION_A_TA_OFFSET));
    payload_length = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_INDICATION_PAYLOAD_LEN_OFFSET));
    TA_TYPE = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_INDICATION_TA_TYPE_OFFSET));
    payload = pointer_offset_nbyte(dp, SERVICE_INDICATION_PAYLOAD_OFFSET);
    log_d("Mtype: %d A_SA: 0x%8X A_TA: 0x%8X TA_TYPE: %d pl: %d", 0, A_SA, A_TA, TA_TYPE, payload_length);
    if (dm_udsc->udsc_request_transfer_cb) {
        MUTEX_UNLOCK(dm_api->mutex);        
        /* 回调函数内可能调用API先unlock */
        dm_udsc->udsc_request_transfer_cb(dm_udsc->udsc_request_transfer_arg, udscid, payload, payload_length, A_SA, A_TA, TA_TYPE);
        MUTEX_LOCK(dm_api->mutex);
    }
}

/*
    设置UDS客户端所有诊断服务任务结束后的回调函数
*/
int ydm_api_udsc_task_end_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_task_end_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;

    if (dm_api == 0 || call == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -2;
            break; /* while */
        }
        dm_udsc->udsc_task_end_cb = call;
        dm_udsc->udsc_task_end_arg = arg; 
        rv = 0;
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);

    return rv;
}

int ydm_api_udsc_security_access_keygen_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_security_access_keygen_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;

    if (dm_api == 0 || call == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -2;
            break; /* while */
        }
        dm_udsc->security_access_keygen_cb = call;
        dm_udsc->security_access_keygen_arg = arg;        
        rv = 0;
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);

    return rv;
}

int ydm_api_udsc_service_36_transfer_progress_callback_set(ydiag_master_api *dm_api, yuint16 udscid, uds_service_36_transfer_progress_callback call, void *arg)
{
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;

    if (dm_api == 0 || call == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -2;
            break; /* while */
        }
        dm_udsc->transfer_progress_cb = call;
        dm_udsc->transfer_progress_arg = arg;        
        rv = 0;
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);

    return rv;
}

int ydm_api_diag_master_instance_destroy_callback_set(ydiag_master_api *dm_api, diag_master_instance_destroy_callback call, void *arg)
{
    int rv = -1;

    if (dm_api == 0 || call == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    dm_api->instance_destroy_cb = call;
    dm_api->instance_destroy_arg = arg;   
    MUTEX_UNLOCK(dm_api->mutex);

    return rv;
}

/*
    UDS客户端所有诊断服务任务结束后回调函数
*/
static void dm_api_event_udsc_task_end_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen)
{
    yuint8 *ind = NULL, *resp = NULL;
    yuint32 indl = 0, respl = 0;
    struct dm_api_udscs *dm_udsc = 0;
    
    indl = TYPE_CAST_UINT(pointer_offset_nbyte(msg, SERVICE_FINISH_RESULT_IND_LEN_OFFSET));
    if (indl > 0) {
        ind = pointer_offset_nbyte(msg, SERVICE_FINISH_RESULT_IND_OFFSET);
    }    
    respl = TYPE_CAST_UINT(pointer_offset_nbyte(msg, SERVICE_FINISH_RESULT_IND_OFFSET + indl));
    if (respl > 0) {
        resp = pointer_offset_nbyte(msg, SERVICE_FINISH_RESULT_IND_OFFSET + indl + SERVICE_FINISH_RESULT_RESP_LEN_SIZE);
    }    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);    
    if (dm_udsc->udsc_task_end_cb) {        
        MUTEX_UNLOCK(dm_api->mutex);
        /* 回调函数内可能调用API先unlock */
        dm_udsc->udsc_task_end_cb(dm_udsc->udsc_task_end_arg, udscid, recode, ind, indl, resp, respl);
        MUTEX_LOCK(dm_api->mutex);
    }
}

static void dm_api_event_service_result_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen)
{
    struct dm_api_udscs *dm_udsc = 0;
    yuint32 rr_callid = 0;
    yuint32 dlen = 0;
    yuint8 *payload = 0;
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    rr_callid = TYPE_CAST_UINT(pointer_offset_nbyte(msg, SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET));
    dlen = TYPE_CAST_UINT(pointer_offset_nbyte(msg, SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET));
    payload = pointer_offset_nbyte(msg, SERVICE_REQUEST_RESULT_DATA_OFFSET);    
    if (rr_callid > 0 && rr_callid < SERVICE_ITEM_SIZE_DEF) {
        if (dm_udsc->rr_handler[rr_callid - 1].service_result) {            
            MUTEX_UNLOCK(dm_api->mutex);
            /* 回调函数内可能调用API先unlock */
            dm_udsc->rr_handler[rr_callid - 1].service_result(dm_udsc->rr_handler[rr_callid - 1].rr_arg, udscid, payload, dlen);
            MUTEX_LOCK(dm_api->mutex);
        }
    }  
}

static void dm_api_event_security_access_keygen_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen)
{
    struct dm_api_udscs *dm_udsc = 0;
    yuint16 seed_size = 0;
    yuint8 *seed = 0;
    yuint8 level = 0;
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    level = TYPE_CAST_UCHAR(pointer_offset_nbyte(msg, SERVICE_SA_SEED_LEVEL_OFFSET));
    seed_size = TYPE_CAST_USHORT(pointer_offset_nbyte(msg, SERVICE_SA_SEED_SIZE_OFFSET));
    seed = pointer_offset_nbyte(msg, SERVICE_SA_SEED_OFFSET);    
    if (dm_udsc->security_access_keygen_cb) {
        MUTEX_UNLOCK(dm_api->mutex);
        /* 回调函数内可能调用API先unlock */
        dm_udsc->security_access_keygen_cb(dm_udsc->security_access_keygen_arg, udscid, level, seed, seed_size);
        MUTEX_LOCK(dm_api->mutex);
    }
}

static void dm_api_event_keepalive_request_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen)
{
    yuint32 to_cmd = DM_EVENT_OMAPI_KEEPALIVE_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;

    // log_d("Send command: %s Recode: %d udsc ID: %d", ydm_ipc_event_str(to_cmd), to_recode, 0);
    ydm_common_encode(DMAPI_TX_BUFF(dm_api), to_cmd, to_recode, 0, IPC_USE_USER_TIME_STAMP);
    dm_api_sendto_diag_master(dm_api, dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE);  
}

static void dm_api_event_instance_destroy_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen)
{
    log_d("diag master instance destroy");
    dm_api->vf = 0;
    if (dm_api->instance_destroy_cb) {
        MUTEX_UNLOCK(dm_api->mutex);
        /* 回调函数内可能调用API先unlock */
        dm_api->instance_destroy_cb(dm_api->instance_destroy_arg);
        MUTEX_LOCK(dm_api->mutex);
    }
}

static void dm_api_event_36_transfer_progress_handler(ydiag_master_api *dm_api, yuint32 recode, yuint16 udscid, yuint8 *msg, yuint32 msglen)
{
    struct dm_api_udscs *dm_udsc = 0;
    yuint32 file_size = 0; 
    yuint32 total_byte = 0;
    yuint32 elapsed_time = 0;
    
    dm_udsc = dm_api_udsc_get(dm_api, udscid);
    file_size = TYPE_CAST_UINT(pointer_offset_nbyte(msg, IPC_36_TRANSFER_FILE_SIZE_OFFSET));
    total_byte = TYPE_CAST_UINT(pointer_offset_nbyte(msg, IPC_36_TRANSFER_TOTAL_BYTE_OFFSET));
    elapsed_time = TYPE_CAST_UINT(pointer_offset_nbyte(msg, IPC_36_TRANSFER_ELAPSED_TIME_OFFSET));
    if (dm_udsc->transfer_progress_cb) {
        MUTEX_UNLOCK(dm_api->mutex);
        /* 回调函数内可能调用API先unlock */
        dm_udsc->transfer_progress_cb(dm_udsc->transfer_progress_arg, udscid, file_size, total_byte, elapsed_time);
        MUTEX_LOCK(dm_api->mutex);
    }
}

int ydm_api_udsc_diag_id_storage(ydiag_master_api *dm_api, yuint16 udscid, yuint32 diag_req_id, yuint32 diag_resp_id)
{
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;
    
    if (dm_api == 0) {
        return 0;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            break; /* while */
        }
        dm_udsc->diag_req_id = diag_req_id;
        dm_udsc->diag_resp_id = diag_resp_id;
        rv = 0;
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
    
    return rv;
}

int ydm_api_udsc_index_by_resp_id(ydiag_master_api *dm_api, yuint32 diag_resp_id)
{
    int index = -1;

    if (dm_api == 0) {
        return -1;
    }

    MUTEX_LOCK(dm_api->mutex);    
    for (index = 0; index < DM_UDSC_CAPACITY_MAX; index++) {
        struct dm_api_udscs *dm_udsc = dm_api_udsc_get(dm_api, index);
        if (dm_udsc && \
            dm_udsc->diag_resp_id == diag_resp_id) {
            break; /* for */
        }
    }
    MUTEX_UNLOCK(dm_api->mutex);
    
    return index;
}

int ydm_api_udsc_index_by_req_id(ydiag_master_api *dm_api, yuint32 diag_req_id)
{
    int index = -1;

    if (dm_api == 0) {
        return -1;
    }

    MUTEX_LOCK(dm_api->mutex);    
    for (index = 0; index < DM_UDSC_CAPACITY_MAX; index++) {
        struct dm_api_udscs *dm_udsc = dm_api_udsc_get(dm_api, index);
        if (dm_udsc && \
            dm_udsc->diag_req_id == diag_req_id) {
            break; /* for */
        }
    }
    MUTEX_UNLOCK(dm_api->mutex);
    
    return index;
}

yuint32 ydm_api_udsc_resp_id(ydiag_master_api *dm_api, yuint16 udscid)
{
    struct dm_api_udscs *dm_udsc = 0;
    yuint32 diag_resp_id = 0;

    if (dm_api == 0) {
        return 0;
    }
    
    MUTEX_LOCK(dm_api->mutex);    
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_d("The udsc id is invalid => %d", udscid);
            break; /* while */
        }     
        diag_resp_id = dm_udsc->diag_resp_id;    
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
    
    return diag_resp_id;
}

yuint32 ydm_api_udsc_req_id(ydiag_master_api *dm_api, yuint16 udscid)
{
    struct dm_api_udscs *dm_udsc = 0;
    yuint32 diag_req_id = 0;
    
    if (dm_api == 0) {
        return 0;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            break; /* while */
        }    
        diag_req_id = dm_udsc->diag_req_id;
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);

    return diag_req_id;
}

static int dm_api_service_response_filter(const yuint8 *msg)
{
    if (!(msg[0] & 0x40)) {
        /* 非诊断响应消息 */
        return 0;
    }

    return 1;
}

/*
    诊断应答给UDS客户端处理
*/
int ydm_api_service_response(ydiag_master_api *dm_api, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype)
{
    yuint8 *dp = pointer_offset_nbyte(dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE);
    yuint32 evtype = DM_EVENT_SERVICE_RESPONSE_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint32 A_SA = 0, A_TA = 0, payload_length = 0, TA_TYPE = 0; 
    int rv = -1;

    if (dm_api == 0 || data == 0) {
        return rv;
    }

    /* 过滤掉非诊断响应消息 */
    if (!dm_api_service_response_filter(data)) {
        return rv;
    }
    
    if (dma_txbuf_size(dm_api) < DM_IPC_EVENT_MSG_SIZE + \
        SERVICE_RESPONSE_PAYLOAD_OFFSET + size) {
        log_w("excessive data length the maximum support %d byte", \
            dma_txbuf_size(dm_api) - SERVICE_RESPONSE_PAYLOAD_OFFSET - DM_IPC_EVENT_MSG_SIZE);
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    /* encode 4byte UDS响应源地址 */
    memappend(pointer_offset_nbyte(dp, SERVICE_RESPONSE_A_SA_OFFSET), &sa, SERVICE_RESPONSE_A_SA_SIZE);
    /* encode 4byte UDS响应目的地址 */
    memappend(pointer_offset_nbyte(dp, SERVICE_RESPONSE_A_TA_OFFSET), &ta, SERVICE_RESPONSE_A_TA_SIZE);
    /* encode 4byte UDS响应目的地址类型 */
    memappend(pointer_offset_nbyte(dp, SERVICE_RESPONSE_TA_TYPE_OFFSET), &tatype, SERVICE_RESPONSE_TA_TYPE_SIZE);
    /* encode 4byte UDS响应数据长度 */
    memappend(pointer_offset_nbyte(dp, SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET), &size, SERVICE_RESPONSE_PAYLOAD_LEN_SIZE);
    /* encode nbyte UDS响应数据 */
    memappend(pointer_offset_nbyte(dp, SERVICE_RESPONSE_PAYLOAD_OFFSET), data, size);
    A_SA = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_A_SA_OFFSET));
    A_TA = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_A_TA_OFFSET));
    payload_length = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET));
    TA_TYPE = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_TA_TYPE_OFFSET));
    dp = pointer_offset_nbyte(dp, SERVICE_RESPONSE_PAYLOAD_OFFSET);    
    log_d("Mtype: %d A_SA: 0x%08X A_TA: 0x%08X TA_TYPE: %d pl: %d", 0, A_SA, A_TA, TA_TYPE, payload_length);
    ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_USER_TIME_STAMP);
    if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE + SERVICE_RESPONSE_PAYLOAD_OFFSET + size, \
            &udscid, DM_EVENT_SERVICE_RESPONSE_ACCEPT, DM_ERR_NO) == 0) {    
        rv = 0;
    }            
    MUTEX_UNLOCK(dm_api->mutex);
        
    return rv;      
}

int ydm_api_udsc_rundata_statis(ydiag_master_api *dm_api, yuint16 udscid, runtime_data_statis_t *statis)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;

    if (dm_api == 0 || statis == 0) {
        return rv;
    }
    
    MUTEX_LOCK(dm_api->mutex);
    do {
        dm_udsc = dm_api_udsc_get(dm_api, udscid);
        if (dm_udsc == 0) {
            log_e("The udsc id is invalid => %d", udscid);
            rv = -1;
            break; /* while */
        }
        /* 请求数据编码 */
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, udscid, IPC_USE_SYSTEM_TIME_STAMP);    
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE, \
            &udscid, DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_ACCEPT, DM_ERR_NO) == 0) {    
            ydm_udsc_rundata_statis_decode(pointer_offset_nbyte(dma_rxbuf(dm_api), DM_IPC_EVENT_MSG_SIZE), \
                dma_rxbuf_size(dm_api) - DM_IPC_EVENT_MSG_SIZE, statis);
            rv = 0;            
            break; /* while */
        }
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);

    return rv;   
}

void ydm_api_request_event_loop(ydiag_master_api *dm_api)
{
    int recv_bytes = -1;

    if (dm_api == 0) {
        return ;
    }

    /* 默认创建的线程事件循坏还在执行中，等待线程退出 */
    dm_api->thread_flag &= ~DM_API_THREAD_ENABLE;
    if (dm_api->thread_flag & DM_API_THREAD_RUNING) {
        log_d("The thread is still running");
        return ;
    }
    /* 接收事件轮询 */
    recv_bytes = ydm_recvfrom(dm_api->event_listen_sockfd, dm_api->event_listen_rxbuf, sizeof(dm_api->event_listen_rxbuf), 0);
    if (recv_bytes > 0) {        
        MUTEX_LOCK(dm_api->mutex);
        dm_api_event_accept_handler(dm_api, dm_api->event_listen_rxbuf, recv_bytes);    
        memset(dm_api->event_listen_rxbuf, 0, recv_bytes);        
        MUTEX_UNLOCK(dm_api->mutex);
    } 

    return ;
}

int ydm_api_errcode(ydiag_master_api *dm_api)
{
    return dm_api->errcode;
}

char *ydm_api_errcode_str(int errcode)
{
    return ydm_event_rcode_str(errcode);
}

int ydm_api_terminal_control_service_create(ydiag_master_api *dm_api, terminal_control_service_info_t *trinfo)
{
    yuint32 evtype = DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    int clen = 0;
    int tcsid = -1;

    if (dm_api == 0) {
        return tcsid;
    }

    MUTEX_LOCK(dm_api->mutex);
    do {
        clen = ydm_terminal_control_service_info_encode(pointer_offset_nbyte(dma_txbuf(dm_api), DM_IPC_EVENT_MSG_SIZE), \
                                dma_txbuf_size(dm_api) - DM_IPC_EVENT_MSG_SIZE, trinfo);
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, 0, IPC_USE_SYSTEM_TIME_STAMP);
        if (dm_api_sync_event_emit_to_master(dm_api, DM_IPC_EVENT_MSG_SIZE + clen, \
                0, DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_ACCEPT, DM_ERR_NO) == 0) {    
            tcsid = TYPE_CAST_UINT(pointer_offset_nbyte(dma_rxbuf(dm_api), TERMINAL_CONTROL_SERVICE_ID_OFFSET));         
            break; /* while */
        }
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
        
    return tcsid;
}

int ydm_api_terminal_control_service_destroy(ydiag_master_api *dm_api, yuint32 tcsid)
{
    yuint32 evtype = DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_EMIT;
    yuint32 recode = DM_ERR_NO;
    struct dm_api_udscs *dm_udsc = 0;
    int rv = -1;
    
    if (dm_api == 0) {
        return rv;
    }

    MUTEX_LOCK(dm_api->mutex);
    do {        
        memappend(pointer_offset_nbyte(dma_txbuf(dm_api), TERMINAL_CONTROL_SERVICE_ID_OFFSET), \
                                                                &tcsid, TERMINAL_CONTROL_SERVICE_ID_SIZE);
        ydm_common_encode(DMAPI_TX_BUFF(dm_api), evtype, recode, 0, IPC_USE_SYSTEM_TIME_STAMP);
        if (dm_api_sync_event_emit_to_master(dm_api, TERMINAL_CONTROL_SERVICE_SIZE, \
                0, DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_ACCEPT, DM_ERR_NO) == 0) {    
            rv = 0;
            break; /* while */
        }          
    } while (0);
    MUTEX_UNLOCK(dm_api->mutex);
        
    return rv;      
}

int ydm_api_logd_print_set(int (*printf)(const char *format, ...))
{
    g_ydm_logd_print = printf;
    return 0;
}

int ydm_api_loge_print_set(int (*printf)(const char *format, ...))
{
    g_ydm_loge_print = printf;
    return 0;
}

int ydm_api_logw_print_set(int (*printf)(const char *format, ...))
{
    g_ydm_logw_print = printf;
    return 0;
}

int ydm_api_logi_print_set(int (*printf)(const char *format, ...))
{
    g_ydm_logi_print = printf;
    return 0;
}
