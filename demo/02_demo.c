#include <stdio.h>

#include "demo.h"
#include "yapi_dm_simple.h"

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    yapi_dm_master_reset(om_api);

    udsc_create_config config;

    int udscid = yapi_dm_udsc_create(om_api, &config);
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }
    /* ----------------------------------------------------------------- */
    /* ================================================================= */
    
    return 0;
}

