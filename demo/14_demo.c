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

void YCALLBACK(udsc_security_access_keygen)(YAPI_DM yapi, unsigned short udscid, unsigned char level, const unsigned char *data, unsigned int size);
void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype);
void yuds_request_seed_result_callback(void *arg, unsigned short id, const unsigned char *data, unsigned int size);
void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl);

int g_udscid = -1;

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
    int udscid = -1;
    ydef_udsc_create(om_api, udscid, ydef_udsc_cfg_service_item_capacity(1000)); 
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }

    ydef_udsc_service_add_statement("FINISH_DEFAULT_SETTING");    
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x09), \
                        ydef_udsc_delay(500), ydef_udsc_timeout(100), \
                        ydef_udsc_finish_rule(FINISH_DEFAULT_SETTING, 10));
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);

    ydef_udsc_service_add_statement("FINISH_EQUAL_TO");    
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x09), \
                        ydef_udsc_delay(500), ydef_udsc_timeout(100), \
                        ydef_udsc_finish_rule(FINISH_EQUAL_TO, 10));
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);

    ydef_udsc_service_add_statement("FINISH_UN_EQUAL_TO");    
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x09), \
                        ydef_udsc_delay(500), ydef_udsc_timeout(100), \
                        ydef_udsc_finish_rule(FINISH_UN_EQUAL_TO, 10));
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);

    ydef_udsc_service_add_statement("FINISH_EQUAL_TO_POSITIVE_RESPONSE");    
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x09), \
                        ydef_udsc_delay(500), ydef_udsc_timeout(100), \
                        ydef_udsc_finish_rule(FINISH_EQUAL_TO_POSITIVE_RESPONSE, 10));
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);

    ydef_udsc_service_add_statement("FINISH_EQUAL_TO_NEGATIVE_RESPONSE");    
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x09), \
                        ydef_udsc_delay(500), ydef_udsc_timeout(100), \
                        ydef_udsc_finish_rule(FINISH_EQUAL_TO_NEGATIVE_RESPONSE, 10));
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);

    ydef_udsc_service_add_statement("FINISH_EQUAL_TO_NO_RESPONSE");    
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x09), \
                        ydef_udsc_delay(500), ydef_udsc_timeout(100), \
                        ydef_udsc_finish_rule(FINISH_EQUAL_TO_NO_RESPONSE, 10));
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);
    
    ydef_udsc_service_add_statement("FINISH_EQUAL_TO_NORMAL_RESPONSE");    
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x09), \
                        ydef_udsc_delay(500), ydef_udsc_timeout(100), \
                        ydef_udsc_finish_rule(FINISH_EQUAL_TO_NORMAL_RESPONSE, 10));
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);
    
    ydef_udsc_service_add_statement("222");
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x0a), \
                        ydef_udsc_delay(1), ydef_udsc_timeout(100), ydef_udsc_38_rft_mode_of_operation(1), \
                        ydef_udsc_38_rft_file_path_and_name_length(1), \
                        ydef_udsc_38_rft_data_format_identifier(1), \
                        ydef_udsc_38_rft_file_size_parameter_length(1), \
                        ydef_udsc_38_rft_file_size_un_compressed(1), \
                        ydef_udsc_38_rft_file_size_compressed(1), \
                        ydef_udsc_max_number_of_block_length(2));
    ydef_udsc_service_38_rft_file_path_and_name_set("1111111");
    ydef_udsc_service_file_local_path_set("222222222222222");
    ydef_udsc_service_variable_byte_set("11111", 2);
    ydef_udsc_service_add(om_api, udscid, yuds_request_seed_result_callback);

    ydef_udsc_service_add_statement("3333");
    ydef_udsc_service_base_set(0x27, 0x11223344, 0x55667788, ydef_udsc_sub(0x0a), \
                         ydef_udsc_delay(1), ydef_udsc_timeout(100), ydef_udsc_38_rft_mode_of_operation(1), \
                         ydef_udsc_38_rft_file_path_and_name_length(1), \
                         ydef_udsc_38_rft_data_format_identifier(1), \
                         ydef_udsc_38_rft_file_size_parameter_length(1), \
                         ydef_udsc_38_rft_file_size_un_compressed(1), \
                         ydef_udsc_38_rft_file_size_compressed(1), \
                         ydef_udsc_max_number_of_block_length(2));
    ydef_udsc_service_38_rft_file_path_and_name_set("1111111");
    ydef_udsc_service_file_local_path_set("222222222222222");
    ydef_udsc_service_variable_byte_set("11111", 2);
    ydef_udsc_service_add(syapi_dm(), syapi_dm_udsc(), 0);

    g_udscid = udscid;
    
    /* M这个函数是必须调用的 uds客户端通用配置 */
    ydef_udsc_runtime_config(om_api, udscid, ydef_udsc_runtime_fail_abort(1), ydef_udsc_runtime_tester_present_enable(1), ydef_udsc_runtime_statis_enable(1));
    
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    yevent_connect(om_api, udscid, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* 使用ota master发送过来的种子生成key */
    yevent_connect(om_api, udscid, YEVENT(udsc_security_access_keygen), YCALLBACK(udsc_security_access_keygen));

    yevent_connect(om_api, udscid, YEVENT(udsc_task_end), YCALLBACK(udsc_task_end));
    /* ----------------------------------------------------------------- */
    /* ================================================================= */    
    /* M这个函数是必须调用的 启动UDS客户端开始执行诊断任务 */
    yapi_dm_udsc_start(om_api, udscid);

    /* 监听获取OTA MASTER进程消息 */
    while (1) {
        sleep(1);
    }
    
    return 0;
}

void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl)
{
    demo_printf("------------ all service finish ------------ \n");
    runtime_data_statis_t statis;
    yapi_dm_udsc_rundata_statis(yapi, udscid, &statis);
    yapi_dm_udsc_start(yapi, g_udscid);
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


