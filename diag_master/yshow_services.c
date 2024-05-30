#include <stdio.h> 

#include "ev.h"
#include "ycommon_type.h"
#include "ydm_types.h"
#include "ydm_log.h"  
#include "ydm_ipc_common.h"  
#include "yuds_client.h"

static yuint32 sg_sas[8] = {0};
static yuint32 sg_tas[8] = {0};
char sg_line_buf[1024] = {0};
#define ROW_LENGTH (128)

#define T_INDEX_TITLE             "No."
#define C_INDEX_OFFSET            (0)
#define C_INDEX_SIZE              (6)

#define T_SID_TITLE               "SID"
#define C_SID_OFFSET              (C_INDEX_OFFSET + C_INDEX_SIZE)
#define C_SID_SIZE                (4)

#define T_SUB_TITLE               "SUB"
#define C_SUB_OFFSET              (C_SID_OFFSET + C_SID_SIZE)
#define C_SUB_SIZE                (4)

#define T_DELAY_TITLE             "Delay"
#define C_DELAY_OFFSET            (C_SUB_OFFSET + C_SUB_SIZE)
#define C_DELAY_SIZE              (6)

#define T_TIMEOUT_TITLE           "Tout"
#define C_TIMEOUT_OFFSET          (C_DELAY_OFFSET + C_DELAY_SIZE)
#define C_TIMEOUT_SIZE            (6)

#define T_REQUEST_MSG_TITLE       "Request message"
#define C_REQUEST_MSG_OFFSET      (C_TIMEOUT_OFFSET + C_TIMEOUT_SIZE)
#define C_REQUEST_MSG_SIZE        (22)

#define T_EXPECT_RESP_TITLE       "ExpectResp"
#define C_EXPECT_RESP_OFFSET      (C_REQUEST_MSG_OFFSET + C_REQUEST_MSG_SIZE)
#define C_EXPECT_RESP_SIZE        (11)

#define T_EXPECT_RESP_MSG_TITLE   "ExpectRespMsg"
#define C_EXPECT_RESP_MSG_OFFSET  (C_EXPECT_RESP_OFFSET + C_EXPECT_RESP_SIZE)
#define C_EXPECT_RESP_MSG_SIZE    (14)

#define T_RESPONSE_MSG_TITLE      "Response message"
#define C_RESPONSE_MSG_OFFSET     (C_EXPECT_RESP_MSG_OFFSET + C_EXPECT_RESP_MSG_SIZE)
#define C_RESPONSE_MSG_SIZE       (22)

#define T_SERVICE_STATUS_TITLE    "Status"
#define C_SERVICE_STATUS_OFFSET   (C_RESPONSE_MSG_OFFSET + C_RESPONSE_MSG_SIZE)
#define C_SERVICE_STATUS_SIZE     (7)

#define T_REMARK_TITLE            "Remark"
#define C_REMARK_OFFSET           (C_SERVICE_STATUS_OFFSET + C_SERVICE_STATUS_SIZE)
#define C_REMARK_SIZE             (ROW_LENGTH - C_REMARK_OFFSET - 1)

#define HEX_TO_CHAR(h) (((h) >= 10) ? ((h) - 10) + 'A' : (h) + '0')

char *expect_resp_rule_str(int rule)
{
    switch (rule) {
        case NOT_SET_RESPONSE_EXPECT: return "default";
        case POSITIVE_RESPONSE_EXPECT: return "positive";
        case NEGATIVE_RESPONSE_EXPECT: return "negative";
        case MATCH_RESPONSE_EXPECT: return "match";        
        case NO_RESPONSE_EXPECT: return "noresponse";
        default: return "default";
    }
}

char *service_item_status_str(int status)
{
    switch (status) {
        case SERVICE_ITEM_WAIT: return "wait";
        case SERVICE_ITEM_RUN: return "run";        
        case SERVICE_ITEM_TIMEOUT: return "tout";        
        case SERVICE_ITEM_FAILED: return "fail";        
        case SERVICE_ITEM_SUCCESS: return "succ";        
        default: return "/";
    }
}

void show_table_header()
{
    empty_column_init();    
    memcpy(sg_line_buf + C_INDEX_OFFSET + 1, T_INDEX_TITLE, MIN(C_INDEX_SIZE - 1, sizeof(T_INDEX_TITLE) - 1));
    memcpy(sg_line_buf + C_SID_OFFSET + 1, T_SID_TITLE, MIN(C_SID_SIZE - 1, sizeof(T_SID_TITLE) - 1));
    memcpy(sg_line_buf + C_SUB_OFFSET + 1, T_SUB_TITLE, MIN(C_SUB_SIZE - 1, sizeof(T_SUB_TITLE) - 1));    
    memcpy(sg_line_buf + C_DELAY_OFFSET + 1, T_DELAY_TITLE, MIN(C_DELAY_SIZE - 1, sizeof(T_DELAY_TITLE) - 1));
    memcpy(sg_line_buf + C_TIMEOUT_OFFSET + 1, T_TIMEOUT_TITLE, MIN(C_TIMEOUT_SIZE - 1, sizeof(T_TIMEOUT_TITLE) - 1));      
    memcpy(sg_line_buf + C_REQUEST_MSG_OFFSET + 1, T_REQUEST_MSG_TITLE, MIN(C_REQUEST_MSG_SIZE - 1, sizeof(T_REQUEST_MSG_TITLE) - 1));    
    memcpy(sg_line_buf + C_EXPECT_RESP_OFFSET + 1, T_EXPECT_RESP_TITLE, MIN(C_EXPECT_RESP_SIZE - 1, sizeof(T_EXPECT_RESP_TITLE) - 1));       
    memcpy(sg_line_buf + C_EXPECT_RESP_MSG_OFFSET + 1, T_EXPECT_RESP_MSG_TITLE, MIN(C_EXPECT_RESP_MSG_SIZE - 1, sizeof(T_EXPECT_RESP_MSG_TITLE) - 1)); 
    memcpy(sg_line_buf + C_RESPONSE_MSG_OFFSET + 1, T_RESPONSE_MSG_TITLE, MIN(C_RESPONSE_MSG_SIZE - 1, sizeof(T_RESPONSE_MSG_TITLE) - 1)); 
    memcpy(sg_line_buf + C_SERVICE_STATUS_OFFSET + 1, T_SERVICE_STATUS_TITLE, MIN(C_SERVICE_STATUS_SIZE - 1, sizeof(T_SERVICE_STATUS_TITLE) - 1)); 
    memcpy(sg_line_buf + C_REMARK_OFFSET + 1, T_REMARK_TITLE, MIN(C_REMARK_SIZE - 1, sizeof(T_REMARK_TITLE) - 1));     
    log_show("%s", sg_line_buf);
}

void show_row_divide_symbol()
{
    memset(sg_line_buf, '-', ROW_LENGTH);
    sg_line_buf[0] = ' ';    
    sg_line_buf[ROW_LENGTH - 1] = ' ';
    sg_line_buf[ROW_LENGTH] = 0;
    log_show("%s", sg_line_buf);
}

void show_table_name(char *name)
{
    if (name == NULL) {
        return ;
    }

    int sp = ((ROW_LENGTH / 2) - (strlen(name) / 2));

    memset(sg_line_buf, '=', ROW_LENGTH);
    sg_line_buf[0] = ' ';    
    sg_line_buf[ROW_LENGTH - 1] = ' ';
    sg_line_buf[ROW_LENGTH] = 0;
    log_show("%s", sg_line_buf);
    
    memset(sg_line_buf, ' ', ROW_LENGTH);
    memcpy(sg_line_buf + sp, name, MIN(ROW_LENGTH - sp, strlen(name))); 
    sg_line_buf[0] = '|';    
    sg_line_buf[ROW_LENGTH - 1] = '|';
    sg_line_buf[ROW_LENGTH] = 0;
    log_show("%s", sg_line_buf);
}

void empty_column_init()
{
    memset(sg_line_buf, ' ', ROW_LENGTH);     
    sg_line_buf[C_INDEX_OFFSET] = '|';
    sg_line_buf[C_SID_OFFSET] = '|';
    sg_line_buf[C_SUB_OFFSET] = '|';
    sg_line_buf[C_DELAY_OFFSET] = '|';    
    sg_line_buf[C_TIMEOUT_OFFSET] = '|';    
    sg_line_buf[C_REQUEST_MSG_OFFSET] = '|';
    sg_line_buf[C_EXPECT_RESP_OFFSET] = '|';    
    sg_line_buf[C_EXPECT_RESP_MSG_OFFSET] = '|';
    sg_line_buf[C_RESPONSE_MSG_OFFSET] = '|';    
    sg_line_buf[C_SERVICE_STATUS_OFFSET] = '|';
    sg_line_buf[C_REMARK_OFFSET] = '|';
    sg_line_buf[ROW_LENGTH - 1] = '|';
    sg_line_buf[ROW_LENGTH] = 0;
}

void show_row_service(yint32 index, service_item *item)
{
    char tbuf[ROW_LENGTH] = {0};
    yint32 tl = 0;

    empty_column_init();        
    /* --------------------------------------------------------------------------------------- */
    /* 索引列 */
    tl = snprintf(tbuf, sizeof(tbuf), "%d.", index);    
    memcpy(sg_line_buf + C_INDEX_OFFSET + 1, tbuf, MIN(C_INDEX_SIZE - 1, tl));    
    /* --------------------------------------------------------------------------------------- */
    /* service ID列 */
    tl = snprintf(tbuf, sizeof(tbuf), "%02X", item->sid);    
    memcpy(sg_line_buf + C_SID_OFFSET + 1, tbuf, MIN(C_SID_SIZE - 1, tl)); 
    /* --------------------------------------------------------------------------------------- */
    /* 子功能列 */
    if (item->sub != 0) {
        tl = snprintf(tbuf, sizeof(tbuf), "%02X", item->issuppress ? item->sub | 0x40 : item->sub);  
    }
    else {
        tl = snprintf(tbuf, sizeof(tbuf), "/");  
    }
    memcpy(sg_line_buf + C_SUB_OFFSET + 1, tbuf, MIN(C_SUB_SIZE - 1, tl));
    /* --------------------------------------------------------------------------------------- */
    /* 请求前延时时间列 */
    if (item->delay != 0) {
        tl = snprintf(tbuf, sizeof(tbuf), "%d", item->delay);  
    }
    else {
        tl = snprintf(tbuf, sizeof(tbuf), "/");  
    }
    memcpy(sg_line_buf + C_DELAY_OFFSET + 1, tbuf, MIN(C_DELAY_SIZE - 1, tl));    
    /* --------------------------------------------------------------------------------------- */
    /* 响应超时时间列 */
    if (item->timeout != 0) {
        tl = snprintf(tbuf, sizeof(tbuf), "%d", item->timeout);  
    }
    else {
        tl = snprintf(tbuf, sizeof(tbuf), "/");  
    }
    memcpy(sg_line_buf + C_TIMEOUT_OFFSET + 1, tbuf, MIN(C_TIMEOUT_SIZE - 1, tl));    
    /* --------------------------------------------------------------------------------------- */  
    /* service ID列 */
    tl = snprintf(tbuf, sizeof(tbuf), "%s", expect_resp_rule_str(item->response_rule));    
    memcpy(sg_line_buf + C_EXPECT_RESP_OFFSET + 1, tbuf, MIN(C_EXPECT_RESP_SIZE - 1, tl)); 

    tl = snprintf(tbuf, sizeof(tbuf), "%s", service_item_status_str(item->status));    
    memcpy(sg_line_buf + C_SERVICE_STATUS_OFFSET + 1, tbuf, MIN(C_SERVICE_STATUS_SIZE - 1, tl)); 
    
    boolean isbr = false;
    char request_tbuf[1024] = {0};
    yint32 request_tl = 0, request_p = 0; 
    char *expect_resp_tbuf = NULL;
    yint32 expect_resp_tl = 0, expect_resp_p = 0;
    char *remark_tbuf = NULL;
    yint32 remark_tl = 0, remark_p = 0;    
    char response_tbuf[1024] = {0};
    yint32 response_tl = 0, response_p = 0; 
    int ii = 0;
    do {        
        if (isbr) {
            empty_column_init();
        }
        
        if (isbr == false) {
            const yuint8 *data = NULL;
            int count = 0;
        
            /* 请求数据列 */
            if (item->request_byte) {
                data = y_byte_array_const_data(item->request_byte);
                count = y_byte_array_count(item->request_byte);
                for (ii = 0; ii < count; ii++) {
                    if (sizeof(request_tbuf) - request_tl > 3) {
                        request_tbuf[request_tl++] = HEX_TO_CHAR(data[ii] >> 4 & 0x0f);
                        request_tbuf[request_tl++] = HEX_TO_CHAR(data[ii] & 0x0f);
                        request_tbuf[request_tl++] = ' ';
                    }
                }
            }
            /* 预期响应消息列 */
            if (item->expect_byte) {
                expect_resp_tbuf = y_byte_array_const_data(item->expect_byte);
            }
            if (expect_resp_tbuf == NULL) {
                expect_resp_tbuf = "/";
            }
            expect_resp_tl = strlen(expect_resp_tbuf);
            if (expect_resp_tl == 0) {
                expect_resp_tbuf = "/";
                expect_resp_tl = 1;
            }
            /* 备注信息列 */
            remark_tbuf = item->desc;
            if (remark_tbuf == NULL) {
                remark_tbuf = "/";
            }            
            remark_tl = strlen(remark_tbuf);
            if (remark_tl == 0) {
                remark_tbuf = "/";
                remark_tl = 1;
            }
            /* 响应消息列 */
            if (item->response_byte) {
                data = y_byte_array_const_data(item->response_byte);
                count = y_byte_array_count(item->response_byte);
                for (ii = 0; ii < count; ii++) {
                    if (sizeof(response_tbuf) - response_tl > 3) {
                        response_tbuf[response_tl++] = HEX_TO_CHAR(data[ii] >> 4 & 0x0f);
                        response_tbuf[response_tl++] = HEX_TO_CHAR(data[ii] & 0x0f);
                        response_tbuf[response_tl++] = ' ';
                    }
                }
            }
            else {
                response_tbuf[0] = '/';
                response_tl = 1;
            }
        }
 
        if (request_p < request_tl) {
            memcpy(sg_line_buf + C_REQUEST_MSG_OFFSET + 1, request_tbuf + request_p, \
                MIN(C_REQUEST_MSG_SIZE - 1, request_tl - request_p));
            request_p += MIN(C_REQUEST_MSG_SIZE - 1, request_tl - request_p);
  
        }

        if (expect_resp_tbuf && expect_resp_p < expect_resp_tl) {
            memcpy(sg_line_buf + C_EXPECT_RESP_MSG_OFFSET + 1, expect_resp_tbuf + expect_resp_p, \
                MIN(C_EXPECT_RESP_MSG_SIZE - 1, expect_resp_tl - expect_resp_p));
            expect_resp_p += MIN(C_EXPECT_RESP_MSG_SIZE - 1, expect_resp_tl - expect_resp_p);
        }
        
        if (remark_tbuf && remark_p < remark_tl) {
            memcpy(sg_line_buf + C_REMARK_OFFSET + 1, remark_tbuf + remark_p, \
                MIN(C_REMARK_SIZE - 1, remark_tl - remark_p));
            remark_p += MIN(C_REMARK_SIZE - 1, remark_tl - remark_p);
        }

        if (response_p < response_tl) {
            memcpy(sg_line_buf + C_RESPONSE_MSG_OFFSET + 1, response_tbuf + response_p, \
                MIN(C_RESPONSE_MSG_SIZE - 1, response_tl - response_p));
            response_p += MIN(C_RESPONSE_MSG_SIZE - 1, response_tl - response_p);
  
        }

        if (request_p < request_tl ||\
            expect_resp_p < expect_resp_tl || \
            remark_p < remark_tl || \
            response_p < response_tl) {
            isbr = true;
        }
        else {
            isbr = false;
        }
        log_show("%s", sg_line_buf);
    } while (isbr);
}

void show_table_remark()
{
    char tbuf[ROW_LENGTH] = {0};
    yint32 tl = 0;
    int ii = 0;
    
    memset(sg_line_buf, ' ', ROW_LENGTH);
    sg_line_buf[0] = '|';    
    sg_line_buf[ROW_LENGTH - 1] = '|';
    sg_line_buf[ROW_LENGTH] = 0;

    tl += snprintf(tbuf + tl, sizeof(tbuf) - tl, "SA:");
    for (ii = 0; ii < sizeof(sg_sas); ii++) {
        if (sg_sas[ii] != 0) {
            tl += snprintf(tbuf + tl, sizeof(tbuf) - tl, "%X/", sg_sas[ii]);
        }
        else {
            break;
        }
    }    
    tl += snprintf(tbuf + tl, sizeof(tbuf) - tl, "    ");

    tl += snprintf(tbuf + tl, sizeof(tbuf) - tl, "TA:");
    for (ii = 0; ii < sizeof(sg_tas); ii++) {
        if (sg_tas[ii] != 0) {
            tl += snprintf(tbuf + tl, sizeof(tbuf) - tl, "%X/", sg_tas[ii]);
        }
        else {
            break;
        }
    }
    memcpy(sg_line_buf + 1, tbuf, MIN(ROW_LENGTH - 2, tl));
    log_show("%s", sg_line_buf);
}

int yshow_uds_services(char *name, service_item **item, yint32 num)
{
    int ii = 0, rowi = 0;

    if (item == NULL) {
        return -1;
    }
    memset(sg_sas, 0, sizeof(sg_sas));
    memset(sg_tas, 0, sizeof(sg_tas));

    show_table_name(name);
    show_row_divide_symbol();
    show_table_header();
    for (rowi = 0; rowi < num; rowi++) {
        if (item[rowi]) {
            show_row_divide_symbol();
            show_row_service(rowi + 1, item[rowi]);
            for (ii = 0; ii < sizeof(sg_sas); ii++) {
                if (sg_sas[ii] == item[rowi]->sa) {
                    break;
                }
                if (sg_sas[ii] == 0) {
                    sg_sas[ii] = item[rowi]->sa;
                    break;
                }
            }  
            for (ii = 0; ii < sizeof(sg_tas); ii++) {
                if (sg_tas[ii] == item[rowi]->ta) {
                    break;
                }
                if (sg_tas[ii] == 0) {
                    sg_tas[ii] = item[rowi]->ta;
                    break;
                }
            }
        }
    }
    show_row_divide_symbol();
    show_table_remark();
    show_row_divide_symbol();
    
    return 0;
}
