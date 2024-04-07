#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>  
#include <ws2tcpip.h>
#endif /* _WIN32 */
#include <sys/types.h>
#ifdef __linux__
#include <sys/select.h>
#include <sys/socket.h>
#endif /* __linux__ */

#include "demo.h"
#include "yapi_dm_simple.h"

void YCALLBACK(udsc_security_access_keygen)(YAPI_DM yapi, unsigned short udscid, unsigned char level, const unsigned char *data, unsigned int size);
void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype);
void yuds_request_seed_result_callback(void *arg, unsigned short id, const unsigned char *data, unsigned int size);
void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl);

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    sleep(1);

    // ydm_debug_enable(1);

    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    yapi_dm_master_reset(om_api);

    int udscid = 0;
    udsc_create_config config;

    int ii = 0;
    for (ii = 0; ii < 20; ii++) {
        /* UDS客户端配置结构体 */
        config.service_item_capacity = 200;
        udscid = yapi_dm_udsc_create(om_api, &config);
        if (udscid >= 0) {
           demo_printf("UDS client create success => %d \n", udscid);
        }
    }
    for (ii = 0; ii < 20; ii++) {
        if (yapi_dm_udsc_destroy(om_api, ii) == 0) {
           demo_printf("UDS client destroy success => %d \n", ii);
        }
    }
    
    udscid = yapi_dm_udsc_create(om_api, &config);
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }

    /* UDS服务配置结构体 */
    service_config service;

    memset(&service, 0, sizeof(service));
    /* 诊断服务中的可变数据，UDS 客户端将根据 sid sub did和这个自动构建UDS请求数据 */
    service.variableByte = yapi_byte_array_new();
    /* 预期诊断响应数据，用于判断当前诊断服务执行是否符合预期 */
    service.expectResponByte = yapi_byte_array_new();
    /* 响应结束匹配数据，用于判断当前诊断服务是否需要重复执行 */
    service.finishByte = yapi_byte_array_new();
    /* 诊断请求源地址 */
    service.sa = 0x11223344;
    /* 诊断请求目的地址 */
    service.ta = 0x55667788;
    /* 诊断服务ID */
    service.sid = 0x27;
    service.sub = 0x09;
    /* 延时多长时间后执行这个诊断服务unit ms */
    service.delay = 500;
    /* 服务响应超时时间unit ms */
    service.timeout = 100;
    /* 注册诊断响应处理函数，注册了这个函数后OTA MASTER会将收到的诊断响应转发给这个函数处理 */
    /* 添加一个诊断请求 */
    if (yapi_dm_udsc_service_add(om_api, udscid, &service, yuds_request_seed_result_callback) == 0) {
        demo_printf("UDS service config success. \n");
    }

    /* 诊断请求源地址 */
    service.sa = 0x11223344;
    /* 诊断请求目的地址 */
    service.ta = 0x55667788;
    /* 诊断服务ID */
    service.sid = 0x27;
    service.sub = 0x0a;
    /* 延时多长时间后执行这个诊断服务unit ms */
    service.delay = 500;
    /* 服务响应超时时间unit ms */
    service.timeout = 100;
    service.rr_callid = 0;
    /* 添加一个诊断请求 */
    if (yapi_dm_udsc_service_add(om_api, udscid, &service, 0) == 0) {
        demo_printf("UDS service config success. \n");
    }
    
    /* UDS服务项添加结束，释放内存 */
    yapi_byte_array_delete(service.variableByte);
    yapi_byte_array_delete(service.expectResponByte);
    yapi_byte_array_delete(service.finishByte);
    
    udsc_runtime_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M这个函数是必须调用的 uds客户端通用配置 */
    yapi_dm_udsc_runtime_config(om_api, udscid, &gconfig);
    
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    yevent_connect(om_api, udscid, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* 使用ota master发送过来的种子生成key */
    yevent_connect(om_api, udscid, YEVENT(udsc_security_access_keygen), YCALLBACK(udsc_security_access_keygen));
    /* ----------------------------------------------------------------- */
    /* ================================================================= */    
    /* M这个函数是必须调用的 启动UDS客户端开始执行诊断任务 */
    yapi_dm_udsc_start(om_api, udscid);

    /* 监听获取OTA MASTER进程消息 */
    while (1) {
        continue;
        fd_set readfds;
        struct timeval timeout;
        int sret = 0;
        int sockfd = yapi_dm_ipc_channel_fd(om_api);
    
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 100;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        if ((sret = select(sockfd + 1, &readfds, NULL, NULL, &timeout)) > 0 &&\
            FD_ISSET(sockfd, &readfds)) {
            /* M这个函数是必须调用的接收OTA MASTER进程间通信数据 */
            yapi_dm_event_loop(om_api);
        }
    }
    
    return 0;
}

void YCALLBACK(udsc_security_access_keygen)(YAPI_DM yapi, unsigned short udscid, unsigned char level, const unsigned char *seed, unsigned int seed_size)
{
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    demo_printf("Seed level => %02X \n", level);
    demo_hex_printf("UDS Service seed", seed, seed_size);
    /* seed_generation_key(seed, seed_size) */
    /* 这里通过种子生成KEY,再将KEY发送给ota master */
    yapi_dm_udsc_sa_key(yapi, udscid, level, "\x11\x22\x33\x44", 4);    
    /* ----------------------------------------------------------------- */
    /* ================================================================= */    
}

void yuds_request_seed_result_callback(void *arg, unsigned short id, const unsigned char *data, unsigned int size)
{
    /* 这里会收到完整的UDS响应报文，如果没有诊断请求的响应报文的话这里不会触发，比如请求超时之类的 */
    demo_hex_printf("UDS Service Response", data, size);    
}

void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype)
{
    demo_printf("UDS SA: 0x%08X TA: 0x%08X \n", sa, ta);
    demo_hex_printf("UDS Service Request", msg, msg_len);
    /* 将诊断请求通过doCAN或者doIP发送send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */

    /* 这里只是为了方便演示 yapi_om_service_response应该在收到诊断应答的时候调用 */
    /* 接收到诊断响应了，将响应数据发送给OTA MASTER */
    if (msg[0] == 0x27 && msg[1] == 0x09) {
       yapi_dm_service_response(yapi, udscid, "\x67\x09\x93\x02\x00\x11", sizeof("\x67\x09\x93\x02\x00\x11") - 1, sa, ta, 0);
    }
    else if (msg[0] == 0x27 && msg[1] == 0x0a) {
       yapi_dm_service_response(yapi, udscid, "\x67\x0a", sizeof("\x67\x0a") - 1, sa, ta, 0);
    }
}


