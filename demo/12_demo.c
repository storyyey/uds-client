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

static g_udscid = 0;

void YCALLBACK(udsc_security_access_keygen)(YAPI_DM yapi, unsigned short udscid, unsigned char level, const unsigned char *data, unsigned int size);
void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype);
void yuds_request_seed_result_callback(void *arg, unsigned short id, const unsigned char *data, unsigned int size);
void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl);
void YCALLBACK(udsc_service_36_transfer_progress)(YAPI_DM yapi, unsigned short udscid, unsigned int file_size, unsigned int total_byte, unsigned int elapsed_time);
void YCALLBACK(diag_master_instance_destroy)(YAPI_DM yapi);

int main(int argc, char *argv[])
{
    YAPI_DM om_api;


    /* 创建UDS客户端API */
     om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    // ydm_debug_enable(1);

    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    yapi_dm_master_reset(om_api);

    /* UDS客户端配置结构体 */
    int udscid = -1;
    ydef_udsc_create(om_api, udscid, \
                     ydef_udsc_cfg_service_item_capacity(1000), ydef_udsc_cfg_udsc_name("VCU Flash")); 
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }

#if 0
    ydef_udsc_service_add_statement("Request Download");    
    ydef_udsc_service_base_set(0x34, 0x11223344, 0x55667788, ydef_udsc_timeout(100), \
                        ydef_udsc_34_rd_data_format_identifier(0), \
                        ydef_udsc_34_rd_address_and_length_format_identifier(0x44), \
                        ydef_udsc_34_rd_memory_address(1), \
                        ydef_udsc_34_rd_memory_size(1));
    ydef_udsc_service_file_local_path_set("./diag_master");
    ydef_udsc_service_add(om_api, udscid, 0);

    ydef_udsc_service_add_statement("Transfer Data");    
    ydef_udsc_service_base_set(0x36, 0x11223344, 0x55667788, ydef_udsc_timeout(100), ydef_udsc_max_number_of_block_length(300));
    ydef_udsc_service_file_local_path_set("./diag_master");
    ydef_udsc_service_add(om_api, udscid, 0);
#endif
    udsc_request_download udsc_rd;

    memset(&udsc_rd, 0, sizeof(udsc_rd));
    udsc_rd.block_len_max = 300;
    udsc_rd.checksum_type = FILE_CHECK_CRC32;
    udsc_rd.erase_did = 0x1122;
    udsc_rd.checksum_did = 0x3344;
    udsc_rd.sa = 0x1F834000;
    udsc_rd.ta = 0x1F834111;
    snprintf(udsc_rd.cache_file_dir, sizeof(udsc_rd.cache_file_dir), "%s", "./cache");
    yapi_request_download_file(om_api, udscid, "./diag_master", &udsc_rd);

    /* M这个函数是必须调用的 uds客户端通用配置 */
    ydef_udsc_runtime_config(om_api, udscid, \
                            ydef_udsc_runtime_statis_enable(1), \
                            ydef_udsc_runtime_fail_abort(0), \
                            ydef_udsc_runtime_tester_present_enable(1), \
                            ydef_udsc_runtime_td_36_notify_interval(1000), \
                            ydef_udsc_runtime_uds_msg_parse_enable(1), \
                            ydef_udsc_runtime_uds_asc_record_enable(1));
    
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    yevent_connect(om_api, udscid, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* 使用ota master发送过来的种子生成key */
    yevent_connect(om_api, udscid, YEVENT(udsc_security_access_keygen), YCALLBACK(udsc_security_access_keygen));

    yevent_connect(om_api, udscid, YEVENT(udsc_task_end), YCALLBACK(udsc_task_end));

    yevent_connect(om_api, udscid, YEVENT(udsc_service_36_transfer_progress), YCALLBACK(udsc_service_36_transfer_progress));

    yevent_connect(om_api, udscid, YEVENT(diag_master_instance_destroy), YCALLBACK(diag_master_instance_destroy));

    /* M这个函数是必须调用的 启动UDS客户端开始执行诊断任务 */
    yapi_dm_udsc_start(om_api, udscid);
    g_udscid = udscid;

    /* 监听获取OTA MASTER进程消息 */
    while (1) {
        sleep(1);
    }
    
    return 0;
}

void YCALLBACK(diag_master_instance_destroy)(YAPI_DM yapi)
{
    demo_printf(" ---------- diag_master_instance_destroy ---------------\n");
    yapi_dm_destroy(yapi);

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
   // demo_printf("UDS SA: 0x%08X TA: 0x%08X \n", sa, ta);
    demo_hex_printf("UDS Service Request", msg, msg_len);
    /* 将诊断请求通过doCAN或者doIP发送send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */

    /* 这里只是为了方便演示 yapi_om_service_response应该在收到诊断应答的时候调用 */
    /* 接收到诊断响应了，将响应数据发送给OTA MASTER */
    if (msg[0] == 0x34) {
       yapi_dm_service_response(yapi, udscid, "\x74\x44\x11\x22\x33\x44", sizeof("\x74\x44\x11\x22\x33\x44") - 1, sa, ta, 0);
    }

    if (msg[0] == 0x36) {
       char remsg[2] = {0};
       
       remsg[0] = 0x36 | 0x40;
       remsg[1] = msg[1]; 
       yapi_dm_service_response(yapi, udscid, remsg, 2, sa, ta, 0);
    }  
}

void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl)
{
    demo_printf("------------ all service finish ------------ \n");
    runtime_data_statis_t statis;
    yapi_dm_udsc_rundata_statis(yapi, udscid, &statis);
    //yapi_dm_udsc_start(yapi, g_udscid);
    //yapi_request_download_file_cache_clean();
    
    yapi_dm_udsc_start(yapi, udscid);
}

void YCALLBACK(udsc_service_36_transfer_progress)(YAPI_DM yapi, unsigned short udscid, unsigned int file_size, unsigned int total_byte, unsigned int elapsed_time)
{
    demo_printf("file_size => %d \n", file_size);
    demo_printf("total_byte => %d \n", total_byte);
    demo_printf("elapsed_time => %d \n", elapsed_time);
}

