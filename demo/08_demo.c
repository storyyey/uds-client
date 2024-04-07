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

    /* UDS客户端配置结构体 */
    udsc_create_config config;

    while (1) {
        int udscid = yapi_dm_udsc_create(om_api, 0);
        if (udscid >= 0) {
           demo_printf("UDS client create success => %d \n", udscid);
        }
        
        doipc_config_t dcfg;

        dcfg.ver = 2;
        dcfg.sa_addr = 0x1122;
        dcfg.src_ip = 1111111111;
        dcfg.src_port = 13400;    
        dcfg.dst_ip = 2222222222;
        dcfg.dst_port = 13400;
        dcfg.rxlen = 1024;
        dcfg.txlen = 1024;
        yapi_dm_doipc_create(om_api, udscid, &dcfg);        
        usleep(1);        
        yapi_dm_doipc_create(om_api, udscid, &dcfg);        
        usleep(1);
        yapi_dm_udsc_destroy(om_api, udscid);
        usleep(1);
    }
    
    return 0;
}
