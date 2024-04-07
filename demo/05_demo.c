#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "demo.h"
#include "yapi_dm_simple.h"

void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype);

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    yapi_dm_master_reset(om_api);

    /* UDS客户端配置结构体 */
    udsc_create_config config;

    int udscid = yapi_dm_udsc_create(om_api, &config);
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
    service.sid = 0x22;
    /* DID $22 $2e $31服务会使用 */
    service.did = 0x1122;
    /* 延时多长时间后执行这个诊断服务unit ms */
    service.delay = 500;
    /* 服务响应超时时间unit ms */
    service.timeout = 100;
    /* 添加一个诊断请求 */
    if (yapi_dm_udsc_service_add(om_api, udscid, &service, 0) == 0) {
        demo_printf("UDS service config success. \n");
    }
    /* UDS服务项添加结束，释放内存 */
    yapi_byte_array_delete(service.variableByte);
    yapi_byte_array_delete(service.expectResponByte);
    yapi_byte_array_delete(service.finishByte);

    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    int udscid2 = yapi_dm_udsc_create(om_api, &config);
    if (udscid2 >= 0) {
       demo_printf("UDS client create success => %d \n", udscid2);
    }

    memset(&service, 0, sizeof(service));
    /* 诊断服务中的可变数据，UDS 客户端将根据 sid sub did和这个自动构建UDS请求数据 */
    service.variableByte = yapi_byte_array_new();
    /* 预期诊断响应数据，用于判断当前诊断服务执行是否符合预期 */
    service.expectResponByte = yapi_byte_array_new();
    /* 响应结束匹配数据，用于判断当前诊断服务是否需要重复执行 */
    service.finishByte = yapi_byte_array_new();
    /* 诊断请求源地址 */
    service.sa = 0x77777777;
    /* 诊断请求目的地址 */
    service.ta = 0x88888888;
    /* 诊断服务ID */
    service.sid = 0x22;
    /* DID $22 $2e $31服务会使用 */
    service.did = 0x1122;
    /* 延时多长时间后执行这个诊断服务unit ms */
    service.delay = 500;
    /* 服务响应超时时间unit ms */
    service.timeout = 100;
    /* 添加一个诊断请求 */
    if (yapi_dm_udsc_service_add(om_api, udscid2, &service, 0) == 0) {
        demo_printf("UDS service config success. \n");
    }
    /* UDS服务项添加结束，释放内存 */
    yapi_byte_array_delete(service.variableByte);
    yapi_byte_array_delete(service.expectResponByte);
    yapi_byte_array_delete(service.finishByte);

    udsc_runtime_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M这个函数是必须调用的 uds客户端通用配置 */
    yapi_dm_udsc_runtime_config(om_api, udscid2, &gconfig);
    
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */    
    yevent_connect(om_api, udscid2, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* M这个函数是必须调用的 启动UDS客户端开始执行诊断任务 */
    yapi_dm_udsc_start(om_api, udscid2);
    
    /* ----------------------------------------------------------------- */
    /* ================================================================= */

    //udsc_runtime_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M这个函数是必须调用的 uds客户端通用配置 */
    yapi_dm_udsc_runtime_config(om_api, udscid, &gconfig);
    
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    yevent_connect(om_api, udscid, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    
    /* M这个函数是必须调用的 启动UDS客户端开始执行诊断任务 */
    yapi_dm_udsc_start(om_api, udscid);

    /* 监听获取OTA MASTER进程消息 */
    while (1) {
        fd_set readfds;
        struct timeval timeout;
        int sret = 0;
        int sockfd = yapi_dm_ipc_channel_fd(om_api);
    
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
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

void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype)
{
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    demo_printf("udscid => %d \n", udscid);
    /* ----------------------------------------------------------------- */
    /* ================================================================= */
    demo_printf("UDS SA: 0x%08X TA: 0x%08X \n", sa, ta);
    demo_hex_printf("UDS Service Request", msg, msg_len);
    /* 将诊断请求通过doCAN或者doIP发送send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */
}
