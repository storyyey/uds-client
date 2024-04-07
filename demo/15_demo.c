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

int main(int argc, char *argv[])
{
    /* 创建UDS客户端API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    /* UDS客户端API创建成功后复位一下UDS客户端，防止UDS客户端API异常 */
    yapi_dm_master_reset(om_api);
    int udscid = -1;    
    int udscid1 = -1;
    while (1) {
    /* UDS客户端配置结构体 */
    ydef_udsc_create(om_api, udscid, \
                     ydef_udsc_cfg_service_item_capacity(1000)); 
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }
    else {        
        demo_printf("UDS client create faile => %d \n", udscid);
        return 0;
    }
    ydef_udsc_create(om_api, udscid1, \
                     ydef_udsc_cfg_service_item_capacity(1000)); 
    if (udscid1 >= 0) {
       demo_printf("UDS client create success => %d \n", udscid1);
    }
    else {        
        demo_printf("UDS client create faile => %d \n", udscid1);
        return 0;
    }

    /* 校验传输结果 */
    ydef_udsc_service_add_statement("Routine Control");    
    ydef_udsc_service_base_set(0x31, 0x11223344, 0x55667788, \
                               ydef_udsc_sub(0x01), \
                               ydef_udsc_did(0x1122),\
                               ydef_udsc_timeout(1000), \
                               ydef_udsc_expect_respon_rule(NOT_SET_RESPONSE_EXPECT), \
                               ydef_udsc_finish_rule(FINISH_UN_EQUAL_TO, 10));  
    /* 填充预期得到的响应报文 */
    ydef_udsc_service_expect_respon_byte_set("710111/710122/710133/710144/710155");            
    ydef_udsc_service_finish_byte_set("710111/710122/710133/710144/710155");            

    /* -----------------------------------------------------------------------------------END */            
    ydef_udsc_service_add(om_api, udscid, 0);


    /* M这个函数是必须调用的 uds客户端通用配置 */
    ydef_udsc_runtime_config(om_api, udscid, \
                            ydef_udsc_runtime_statis_enable(0), \
                            ydef_udsc_runtime_fail_abort(0), \
                            ydef_udsc_runtime_tester_present_enable(0), \
                            ydef_udsc_runtime_td_36_notify_interval(0));
  
        if (yapi_dm_udsc_destroy(om_api, udscid) != 0) {
            demo_printf("yapi_om_udsc_destroy error \n");
            return 0;; 
        }
        

        
        /* 校验传输结果 */
        ydef_udsc_service_add_statement("Routine Control");    
        ydef_udsc_service_base_set(0x31, 0x11223344, 0x55667788, \
                                   ydef_udsc_sub(0x01), \
                                   ydef_udsc_did(0x1122),\
                                   ydef_udsc_timeout(1000), \
                                   ydef_udsc_expect_respon_rule(NOT_SET_RESPONSE_EXPECT), \
                                   ydef_udsc_finish_rule(FINISH_UN_EQUAL_TO, 10));  
        /* 填充预期得到的响应报文 */
        ydef_udsc_service_expect_respon_byte_set("710111/710122/710133/710144/710155");            
        ydef_udsc_service_finish_byte_set("710111/710122/710133/710144/710155");            
        
        /* -----------------------------------------------------------------------------------END */            
        ydef_udsc_service_add(om_api, udscid1, 0);
        
        
        /* M这个函数是必须调用的 uds客户端通用配置 */
        ydef_udsc_runtime_config(om_api, udscid1, \
                                ydef_udsc_runtime_statis_enable(0), \
                                ydef_udsc_runtime_fail_abort(0), \
                                ydef_udsc_runtime_tester_present_enable(0), \
                                ydef_udsc_runtime_td_36_notify_interval(0));
        
            if (yapi_dm_udsc_destroy(om_api, udscid1) != 0) {
                demo_printf("yapi_om_udsc_destroy error \n");
                return 0;; 
            }
    }
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    /* M这个函数是必须调用的 接收OTA MASTER的诊断请求报文并通过doIP或者doCAN发送 */
    yevent_connect(om_api, udscid, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* 使用ota master发送过来的种子生成key */
    yevent_connect(om_api, udscid, YEVENT(udsc_security_access_keygen), YCALLBACK(udsc_security_access_keygen));

    yevent_connect(om_api, udscid, YEVENT(udsc_task_end), YCALLBACK(udsc_task_end));

    yevent_connect(om_api, udscid, YEVENT(udsc_service_36_transfer_progress), YCALLBACK(udsc_service_36_transfer_progress));

    /* M这个函数是必须调用的 启动UDS客户端开始执行诊断任务 */
    yapi_dm_udsc_start(om_api, udscid);
    g_udscid = udscid;

    /* 监听获取OTA MASTER进程消息 */
    while (1) {
        sleep(1);
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
    if (msg[0] == 0x31) {
       yapi_dm_service_response(yapi, udscid, "\x71\x11\x22", 3, sa, ta, 0);
    } 
}

void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl)
{
    demo_printf("------------ all service finish ------------ \n");
    runtime_data_statis_t statis;
    // yapi_dm_udsc_rundata_statis(yapi, udscid, &statis);
    //yapi_dm_udsc_start(yapi, g_udscid);
    yapi_request_download_file_cache_clean();
    
    //yapi_dm_udsc_start(yapi, udscid);
}

void YCALLBACK(udsc_service_36_transfer_progress)(YAPI_DM yapi, unsigned short udscid, unsigned int file_size, unsigned int total_byte, unsigned int elapsed_time)
{
    demo_printf("file_size => %d \n", file_size);
    demo_printf("total_byte => %d \n", total_byte);
    demo_printf("elapsed_time => %d \n", elapsed_time);
}
