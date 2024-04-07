#include <stdio.h>

#include "demo.h"
#include "yapi_dm_simple.h"

int main(int argc, char *argv[])
{
    udsc_create_config config;
    service_config srv_config;
    udsc_runtime_config gen_config;
    int udscid = -1;
    
    //ydm_debug_enable(1);

    memset(&config, 0, sizeof(config));
    memset(&srv_config, 0, sizeof(srv_config));
    memset(&gen_config, 0, sizeof(gen_config));
    YAPI_DM dmapi = yapi_dm_create();
    if (dmapi) {
        demo_printf("yota_master_api * yapi_dm_create() - PASS \n");
    }
    else {
        demo_printf("yota_master_api * yapi_dm_create() - FAIL \n");
    }

    if (yapi_dm_is_valid(dmapi)) {
        demo_printf("yapi_dm_is_valid() - PASS \n");
    }
    else {
        demo_printf("yapi_dm_is_valid() - FAIL \n");
    }

    if (yapi_dm_is_valid(0)) {
        demo_printf("yapi_dm_is_valid() - FAIL \n");
    }
    else {
        demo_printf("yapi_dm_is_valid() - PASS \n");
    }
    
    YAPI_DM dmapi1 = yapi_dm_create();
    if (dmapi1 == 0) {
        demo_printf("yota_master_api * yapi_dm_create(1) - FAIL \n");
    }
    YAPI_DM dmapi2 = yapi_dm_create();
    if (dmapi2 == 0) {
        demo_printf("yota_master_api * yapi_dm_create(2) - FAIL \n");
    }
    YAPI_DM dmapi3 = yapi_dm_create();    
    if (dmapi2 == 0) {
        demo_printf("yota_master_api * yapi_dm_create(3) - FAIL \n");
    }
    YAPI_DM dmapi4 = yapi_dm_create();
    if (dmapi4 == 0) {
        demo_printf("yota_master_api * yapi_dm_create(4) - FAIL \n");
    }

    if (yapi_dm_ipc_channel_fd(dmapi) > 0) {
        demo_printf("int yapi_dm_sockfd(%p) - PASS \n", dmapi);
    }
    else {
        demo_printf("int yapi_dm_sockfd(%p) - FAIL \n", dmapi);
    }

    if (yapi_dm_master_reset(dmapi) == 0) {
        demo_printf("int yapi_dm_master_reset(%p) - PASS \n", dmapi);
    }
    else {
        demo_printf("int yapi_dm_master_reset(%p) - FAIL \n", dmapi);
    }

    udscid = yapi_dm_udsc_create(dmapi, &config);
    if (udscid >= 0) {
        demo_printf("int yapi_dm_udsc_create(%p, %p) - PASS \n", dmapi, &config);
    }
    else {
        demo_printf("int yapi_dm_udsc_create(%p, %p) - FAIL \n", dmapi, &config);
    }

    if (yapi_dm_udsc_service_add(dmapi, udscid, &srv_config, 0) == 0) {
        demo_printf("int yapi_dm_udsc_service_config(%p, %d, %p) - PASS \n", dmapi, udscid, &srv_config);
    }
    else {
        demo_printf("int yapi_dm_udsc_service_config(%p, %d, %p) - FAIL \n", dmapi, udscid, &srv_config);
    }

    if (yapi_dm_udsc_runtime_config(dmapi, udscid, &gen_config) == 0) {
        demo_printf("int yapi_dm_udsc_runtime_config(%p, %d, %p) - PASS \n", dmapi, udscid, &gen_config);
    }
    else {
        demo_printf("int yapi_dm_udsc_runtime_config(%p, %d, %p) - FAIL \n", dmapi, udscid, &gen_config);
    }

    if (yapi_dm_udsc_is_active(dmapi, udscid) == 0) {
        demo_printf("int yapi_dm_udsc_is_active(%p, %d) - PASS \n", dmapi, udscid);
    }
    else {
        demo_printf("int yapi_dm_udsc_is_active(%p, %d) - FAIL \n", dmapi, udscid);
    }

    if (yapi_dm_udsc_start(dmapi, udscid) == 0) {
        demo_printf("int yapi_dm_udsc_start(%p, %d) - PASS \n", dmapi, udscid);
    }
    else {
        demo_printf("int yapi_dm_udsc_start(%p, %d) - FAIL \n", dmapi, udscid);
    }

    if (yapi_dm_udsc_is_active(dmapi, udscid) == 0) {
        demo_printf("int yapi_dm_udsc_is_active(%p, %d) - FAIL \n", dmapi, udscid);
    }
    else {
        demo_printf("int yapi_dm_udsc_is_active(%p, %d) - PASS \n", dmapi, udscid);
    }

    if (yapi_dm_udsc_sa_key(dmapi, udscid, 0x0a, "\x11\x22\x33\x44", 4) == 0) {
        demo_printf("int yapi_dm_udsc_sa_key(%p, %d, %02x, byte array, %d) - PASS \n", dmapi, udscid, 0x0a, 4);
    }
    else {
        demo_printf("int yapi_dm_udsc_sa_key(%p, %d, %02x, byte array, %d) - FAIL \n", dmapi, udscid, 0x0a, 4);
    }

    if (yapi_dm_udsc_stop(dmapi, udscid) == 0) {
        demo_printf("int yapi_dm_udsc_stop(%p, %d) - PASS \n", dmapi, udscid);
    }
    else {
        demo_printf("int yapi_dm_udsc_stop(%p, %d) - FAIL \n", dmapi, udscid);
    }

    if (yapi_dm_udsc_destroy(dmapi, udscid) == 0) {
        demo_printf("int yapi_dm_udsc_destroy(%p, %d) - PASS \n", dmapi, udscid);
    }
    else {
        demo_printf("int yapi_dm_udsc_destroy(%p, %d) - FAIL \n", dmapi, udscid);
    }

    /* 无效UDS客户端ID测试 */
    if (yapi_dm_udsc_destroy(dmapi, 0xffff) == 0) {
        demo_printf("int yapi_dm_udsc_destroy(%p, %d) - FAIL \n", dmapi, 0xffff);
    }
    else {        
        demo_printf("int yapi_dm_udsc_destroy(%p, %d) - PASS \n", dmapi, 0xffff);
    }
    if (yapi_dm_udsc_service_add(dmapi, 0xffff, &srv_config, 0) == 0) {
        demo_printf("int yapi_dm_udsc_service_config(%p, %d, %p) - FAIL \n", dmapi, 0xffff, &srv_config);
    }
    else {
        demo_printf("int yapi_dm_udsc_service_config(%p, %d, %p) - PASS \n", dmapi, 0xffff, &srv_config);
    }

    if (yapi_dm_udsc_is_active(dmapi, 0xffff) == 0) {
        demo_printf("int yapi_dm_udsc_is_active(%p, %d) - PASS \n", dmapi, 0xffff);
    }
    else {
        demo_printf("int yapi_dm_udsc_is_active(%p, %d) - FAIL \n", dmapi, 0xffff);
    }

    if (yapi_dm_udsc_runtime_config(dmapi, 0xffff, &gen_config) == 0) {
        demo_printf("int yapi_dm_udsc_runtime_config(%p, %d, %p) - FAIL \n", dmapi, 0xffff, &gen_config);
    }
    else {
        demo_printf("int yapi_dm_udsc_runtime_config(%p, %d, %p) - PASS \n", dmapi, 0xffff, &gen_config);
    }

    if (yapi_dm_udsc_start(dmapi, 0xffff) == 0) {
        demo_printf("int yapi_dm_udsc_start(%p, %d) - FAIL \n", dmapi, 0xffff);
    }
    else {
        demo_printf("int yapi_dm_udsc_start(%p, %d) - PASS \n", dmapi, 0xffff);
    }

    if (yapi_dm_udsc_sa_key(dmapi, 0xffff, 0x0a, "\x11\x22\x33\x44", 4) == 0) {
        demo_printf("int yapi_dm_udsc_sa_key(%p, %d, %02x, byte array, %d) - FAIL \n", dmapi, 0xffff, 0x0a, 4);
    }
    else {
        demo_printf("int yapi_dm_udsc_sa_key(%p, %d, %02x, byte array, %d) - PASS \n", dmapi, 0xffff, 0x0a, 4);
    }

    if (yapi_dm_udsc_stop(dmapi, 0xffff) == 0) {
        demo_printf("int yapi_dm_udsc_stop(%p, %d) - FAIL \n", dmapi, 0xffff);
    }
    else {
        demo_printf("int yapi_dm_udsc_stop(%p, %d) - PASS \n", dmapi, 0xffff);
    }

    if (yapi_dm_udsc_destroy(dmapi, 0xffff) == 0) {
        demo_printf("int yapi_dm_udsc_destroy(%p, %d) - FAIL \n", dmapi, 0xffff);
    }
    else {
        demo_printf("int yapi_dm_udsc_destroy(%p, %d) - PASS \n", dmapi, 0xffff);
    }

    //return 0;
    for ( ; ; ) {
        continue;
        YAPI_DM dmapi = yapi_dm_create();
        if (dmapi) {
            demo_printf("FREE ota master api \n");
            yapi_dm_destroy(dmapi);
        }
    }
}

