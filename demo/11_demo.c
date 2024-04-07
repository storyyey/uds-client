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
#include "yapi_dm_shortcut.h"

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }
    terminal_control_service_info_t trinfo;

    int tcsid = -1;
    int nn = 0;
    int xx = 14;
    for (;;) {
        
    int xx = 14;
    while (xx --> 0) {
        sleep(2);
        terminal_control_service_info_t trinfo;

        memset(&trinfo, 0, sizeof(trinfo));
        
        memcpy(trinfo.devid, "\x00\x00\x00\x00\x01\x23\x45\x67\x89\x01", 10);
        trinfo.provinceid = 2;
        trinfo.cityid = 3;
        // memcpy(trinfo.manufacturer, "\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22", 11);
        //memcpy(trinfo.model, "\x33\x33\x33\x33\x33\x33\x33\x33\x33\x33\x33", 11);    
        memcpy(trinfo.terid, "\x00\x00\x00\x00\x01\x23\x45\x67\x89\x01", 10);
        trinfo.color = xx;
        memcpy(trinfo.vin, "12345670123456789", 17);
        snprintf(trinfo.srv_addr, sizeof(trinfo.srv_addr), "%s", "127.0.0.1");
        trinfo.srv_port = 12345;

        tcsid = ydm_api_terminal_control_service_create(om_api, &trinfo);
        demo_printf("Create id => %d \n", tcsid);
   }
     xx = 14;
     while (xx --> 0) {
         sleep(2);
         terminal_control_service_info_t trinfo;
    
         memset(&trinfo, 0, sizeof(trinfo));
         memcpy(trinfo.devid, "\x00\x00\x00\x00\x01\x23\x45\x67\x89\x01", 10);
         trinfo.provinceid = 2;
         trinfo.cityid = 3;
         // memcpy(trinfo.manufacturer, "\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22", 11);
         //memcpy(trinfo.model, "\x33\x33\x33\x33\x33\x33\x33\x33\x33\x33\x33", 11);    
         memcpy(trinfo.terid, "\x00\x00\x00\x00\x01\x23\x45\x67\x89\x01", 10);
         trinfo.color = nn++;
         memcpy(trinfo.vin, "12345670123456789", 17);
         snprintf(trinfo.srv_addr, sizeof(trinfo.srv_addr), "%s", "127.0.0.1");
         trinfo.srv_port = 12345;
    
         ydm_api_terminal_control_service_destroy(om_api, xx / 2);
         demo_printf("destroy id => %d \n",  xx / 2);
    }
    }

    
    return 0;
}

