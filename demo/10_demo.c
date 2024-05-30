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

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }
    
    // ydm_debug_enable(1);

    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    yapi_dm_master_reset(om_api);

    while (1) {
        /* UDS客户端配置结构体 */
        udsc_create_config config;

        config.service_item_capacity = 100000;
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
        service.sid = 0x38;
        service.service_38_rft.mode_of_operation = 0x01;
        service.service_38_rft.file_path_and_name_length = 10;
        snprintf(service.service_38_rft.file_path_and_name, \
            sizeof(service.service_38_rft.file_path_and_name), "%s", "1111111111111111111111111");
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
        yapi_dm_udsc_destroy(om_api, udscid);
    }
    
    /* 监听获取OTA MASTER进程消息 */
    while (1) {
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
