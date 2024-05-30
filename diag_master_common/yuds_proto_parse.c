#include <stdio.h>
#include <string.h>

#include "cjson.h"
#include "ydm_types.h"
#include "yudsc_types.h"
#include "ydm_stream.h"
#include "yuds_proto_parse.h"

void yproto_uds_negative_response(cJSON *root, yuint8 sid, yuint8 nrc)
{
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    
    snprintf(val, sizeof(val), "Negative response (0x%02X) %s", sid, ydesc_uds_services(sid));    
    cJSON_AddItemToObject(root, "ServiceIdentifier", cJSON_CreateString(val));

    memset(val, 0, sizeof(val));
    snprintf(val, sizeof(val), "(0x%02X) %s", nrc, ydesc_uds_response_codes(nrc));
    cJSON_AddItemToObject(root, "NRC", cJSON_CreateString(val));
}

void yproto_uds_service_identifier(cJSON *root, yuint8 sid)
{
    char val[UDS_PROTOCOL_KEY_VAL] = {0};

    if (sid & UDS_REPLY_MASK) {
        snprintf(val, sizeof(val), "Reply (0x%02X) %s", sid & UDS_SID_MASK, ydesc_uds_services(sid & UDS_SID_MASK));
    }
    else {
        snprintf(val, sizeof(val), "Request (0x%02X) %s", sid, ydesc_uds_services(sid));
    }
    cJSON_AddItemToObject(root, "ServiceIdentifier", cJSON_CreateString(val));
}

void yproto_uds_subfunction(cJSON *root, yuint8 sid, yuint8 sub)
{
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    int sl = 0;

    if (sub & UDS_SUPPRESS_POS_RSP_MSG_IND_MASK) {
        sl += snprintf(val + sl, sizeof(val) - sl, "Suppress reply (0x%02X) ", sub & UDS_SUBFUNCTION_MASK);
    }
    else {
        sl += snprintf(val + sl, sizeof(val) - sl, "(0x%02X) ", sub);
    }

    sub &= UDS_SUBFUNCTION_MASK;
    switch (sid) {
        case UDS_SERVICES_DSC:        
            sl += snprintf(val + sl, sizeof(val) - sl, "%s", ydesc_uds_dsc_types(sub));
            break;
        case UDS_SERVICES_ER:
            sl += snprintf(val + sl, sizeof(val) - sl, "%s", ydesc_uds_er_types(sub));
            break;
        case UDS_SERVICES_RDTCI:            
            sl += snprintf(val + sl, sizeof(val) - sl, "%s", ydesc_uds_rdtci_types(sub));
            break;
        case UDS_SERVICES_SA:            
            if (sub % 2) {
                sl += snprintf(val + sl, sizeof(val) - sl, "%s", "request seed");
            }
            else {
                sl += snprintf(val + sl, sizeof(val) - sl, "%s", "request key");
            }            
            break;
        case UDS_SERVICES_RC:            
            sl += snprintf(val + sl, sizeof(val) - sl, "%s", ydesc_uds_rc_types(sub));            
            break;
        case UDS_SERVICES_CC:            
            sl += snprintf(val + sl, sizeof(val) - sl, "%s", ydesc_uds_cc_types(sub));
            break;
        case UDS_SERVICES_CDTCS:            
            sl += snprintf(val + sl, sizeof(val) - sl, "%s", ydesc_uds_cdtcs_types(sub));            
            break;            
    }    
    cJSON_AddItemToObject(root, "subfunction", cJSON_CreateString(val));
}

void yproto_uds_standard_did(cJSON *root, yuint16 did)
{
    char val[UDS_PROTOCOL_KEY_VAL] = {0};

    snprintf(val, sizeof(val), "(0x%02X) %s", did, ydesc_uds_standard_did_types(did));
    
    cJSON_AddItemToObject(root, "DataIdentifier", cJSON_CreateString(val));
}

void yproto_uds_standard_rid(cJSON *root, yuint16 rid)
{
    char val[UDS_PROTOCOL_KEY_VAL] = {0};

    snprintf(val, sizeof(val), "(0x%02X) %s", rid, ydesc_uds_standard_rid_types(rid));
    
    cJSON_AddItemToObject(root, "DataIdentifier", cJSON_CreateString(val));
}

void yproto_uds_group_of_dtc(cJSON *root, yuint32 group_of_dtc)
{
    char val[UDS_PROTOCOL_KEY_VAL] = {0};

    snprintf(val, sizeof(val), "(0x%06X) %s", group_of_dtc, ydesc_uds_cdtci_group_of_dtc(group_of_dtc));
    
    cJSON_AddItemToObject(root, "GroupOfDTC", cJSON_CreateString(val));
}

void yproto_uds_cc_comm_types(cJSON *root, yuint8 type)
{
    char val[UDS_PROTOCOL_KEY_VAL] = {0};

    snprintf(val, sizeof(val), "(0x%02X) %s", type, ydesc_uds_cc_comm_types(type));
    
    cJSON_AddItemToObject(root, "CommunicationType", cJSON_CreateString(val));
}

void yproto_uds_34_rd(cJSON *root, yuint8 *msg, yuint32 size)
{
    stream_t sp;
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    yuint32 memory_address = 0;
    yuint32 memory_size = 0;
    yuint32 max_number_of_block_length = 0;
    int nbyte = 0;

    stream_init(&sp, msg, size);
    yuint8 sid = stream_byte_read(&sp);
    
    if (sid & UDS_REPLY_MASK) {
        yuint8 lengthFormatIdentifier = stream_byte_read(&sp);           
        memset(val, 0, sizeof(val)); 
        snprintf(val, sizeof(val), "0x%02X", lengthFormatIdentifier);
        cJSON_AddItemToObject(root, "LengthFormatIdentifier", cJSON_CreateString(val));
        for (nbyte = 0; nbyte < (lengthFormatIdentifier >> 4 & 0x0f); nbyte++) {
            yuint8 byte = 0;
        
            byte = stream_byte_read(&sp);
            max_number_of_block_length |=  byte << (((lengthFormatIdentifier >> 4 & 0x0f) - nbyte - 1) * 8);
        }        
        cJSON_AddItemToObject(root, "MaxNumberOfBlockLength", cJSON_CreateNumber(max_number_of_block_length));
    }
    else {
        yuint8 data_format_identifier = stream_byte_read(&sp);
        snprintf(val, sizeof(val), "0x%02X", data_format_identifier);
        cJSON_AddItemToObject(root, "DataFormatIdentifier", cJSON_CreateString(val));
        yuint8 address_and_length_format_identifier = stream_byte_read(&sp);    
        memset(val, 0, sizeof(val));
        snprintf(val, sizeof(val), "0x%02X", address_and_length_format_identifier);
        cJSON_AddItemToObject(root, "AddressAndLengthFormatIdentifier", cJSON_CreateString(val));
        
        for (nbyte = 0; nbyte < (address_and_length_format_identifier >> 4 & 0x0f); nbyte++) {
            yuint8 byte = 0;

            byte = stream_byte_read(&sp);
            memory_address |=  byte << (((address_and_length_format_identifier >> 4 & 0x0f) - nbyte - 1) * 8);
        } 
        memset(val, 0, sizeof(val));
        snprintf(val, sizeof(val), "0x%08X", memory_address);
        cJSON_AddItemToObject(root, "MemoryAddress", cJSON_CreateString(val));

        for (nbyte = 0; nbyte < (address_and_length_format_identifier & 0x0f); nbyte++) {
            yuint8 byte = 0;

            byte = stream_byte_read(&sp);
            memory_size |=  byte << (((address_and_length_format_identifier & 0x0f) - nbyte - 1) * 8);
        } 
        memset(val, 0, sizeof(val));
        snprintf(val, sizeof(val), "0x%08X", memory_size);
        cJSON_AddItemToObject(root, "MemorySize", cJSON_CreateString(val));
    }
}

void yproto_uds_38_rft(cJSON *root, yuint8 *msg, yuint32 size)
{
    int nbyte = 0;
    stream_t sp;    
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    yuint32 file_size_uncompressed = 0;
    yuint32 file_size_compressed = 0;
    yuint32 max_number_of_block_length = 0;
    yuint32 fileSizeUncompressedOrDirInfoLength = 0;
    yuint16 fileSizeOrDirInfoParameterLength = 0;
    yuint8 data_format_identifier = 0;
    yuint8 lengthFormatIdentifier = 0;
    yuint8 mode_of_operation = 0;

    stream_init(&sp, msg, size);
    yuint8 sid = stream_byte_read(&sp);

    if (sid & UDS_REPLY_MASK) {
        mode_of_operation = stream_byte_read(&sp);
        cJSON_AddItemToObject(root, "ModeOfOperation", cJSON_CreateString(ydesc_uds_rft_mode_types(mode_of_operation)));

        if (mode_of_operation != UDS_RFT_MODE_DELETE_FILE) {
            lengthFormatIdentifier = stream_byte_read(&sp);           
            cJSON_AddItemToObject(root, "LengthFormatIdentifier", cJSON_CreateNumber(lengthFormatIdentifier));
        
            for (nbyte = 0; nbyte < lengthFormatIdentifier; nbyte++) {            
                yuint8 byte = stream_byte_read(&sp);
                max_number_of_block_length |=  byte << ((lengthFormatIdentifier - nbyte - 1) * 8);
            }        
            cJSON_AddItemToObject(root, "MaxNumberOfBlockLength", cJSON_CreateNumber(max_number_of_block_length));
        
            data_format_identifier = stream_byte_read(&sp);   
            memset(val, 0, sizeof(val));
            snprintf(val, sizeof(val), "0x%02X", data_format_identifier);      
            cJSON_AddItemToObject(root, "DataFormatIdentifier", cJSON_CreateString(val));
        }

        if (mode_of_operation != UDS_RFT_MODE_ADD_FILE && \
            mode_of_operation != UDS_RFT_MODE_DELETE_FILE && \
            mode_of_operation != UDS_RFT_MODE_REPLACE_FILE) {
            fileSizeOrDirInfoParameterLength = stream_2byte_read(&sp); 
            cJSON_AddItemToObject(root, "FileSizeOrDirInfoParameterLength", cJSON_CreateNumber(fileSizeOrDirInfoParameterLength));
        }
            
        if (mode_of_operation != UDS_RFT_MODE_ADD_FILE && \
            mode_of_operation != UDS_RFT_MODE_DELETE_FILE && \
            mode_of_operation != UDS_RFT_MODE_REPLACE_FILE && \
            mode_of_operation != UDS_RFT_MODE_READ_DIR) {
            for (nbyte = 0; nbyte < fileSizeOrDirInfoParameterLength; nbyte++) {            
                yuint8 byte = stream_byte_read(&sp);
                fileSizeUncompressedOrDirInfoLength |=  byte << ((fileSizeOrDirInfoParameterLength - nbyte - 1) * 8);
            }
            cJSON_AddItemToObject(root, "FileSizeUncompressedOrDirInfoLength", cJSON_CreateNumber(fileSizeUncompressedOrDirInfoLength));

            for (nbyte = 0; nbyte < fileSizeOrDirInfoParameterLength; nbyte++) {            
                yuint8 byte = stream_byte_read(&sp);
                file_size_compressed |=  byte << ((fileSizeOrDirInfoParameterLength - nbyte - 1) * 8);
            }
            cJSON_AddItemToObject(root, "FileSizeCompressed", cJSON_CreateNumber(file_size_compressed));
        }
    }
    else {
        mode_of_operation = stream_byte_read(&sp);
        cJSON_AddItemToObject(root, "ModeOfOperation", cJSON_CreateString(ydesc_uds_rft_mode_types(mode_of_operation)));

        yuint16 file_path_and_name_length = stream_2byte_read(&sp);
        cJSON_AddItemToObject(root, "FilePathAndNameLength", cJSON_CreateNumber(file_path_and_name_length));

        stream_nbyte_read(&sp, val, file_path_and_name_length);    
        cJSON_AddItemToObject(root, "FilePathAndName", cJSON_CreateString(val));
        
        yuint8 data_format_identifier = stream_byte_read(&sp);   
        memset(val, 0, sizeof(val));
        snprintf(val, sizeof(val), "0x%02X", data_format_identifier);      
        cJSON_AddItemToObject(root, "DataFormatIdentifier", cJSON_CreateString(val));
        
        yuint8 file_size_parameter_length = stream_byte_read(&sp); 
        memset(val, 0, sizeof(val));
        snprintf(val, sizeof(val), "0x%02X", file_size_parameter_length);    
        cJSON_AddItemToObject(root, "FileSizeParameterLength", cJSON_CreateString(val));

        for (nbyte = 0; nbyte < file_size_parameter_length; nbyte++) {
            yuint8 byte = stream_byte_read(&sp);
            file_size_uncompressed |=  byte << ((file_size_parameter_length - nbyte - 1) * 8);
        } 
        cJSON_AddItemToObject(root, "FileSizeUnCompressed", cJSON_CreateNumber(file_size_uncompressed));

        for (nbyte = 0; nbyte < file_size_parameter_length; nbyte++) {
            yuint8 byte = stream_byte_read(&sp);
            file_size_compressed |=  byte << ((file_size_parameter_length - nbyte - 1) * 8);
        } 
        cJSON_AddItemToObject(root, "FileSizeCompressed", cJSON_CreateNumber(file_size_compressed));
    }
}

void yproto_uds_27_sa(cJSON *root, yuint8 *msg, yuint32 size)
{
    stream_t sp;    
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    int sl = 0;

    stream_init(&sp, msg, size);
    yuint8 sid = stream_byte_read(&sp);
    yuint8 sub = stream_byte_read(&sp);
    if (sid & UDS_REPLY_MASK) {
        if (sub % 2) {
           while (stream_left_len(&sp) > 0) {
                sl += snprintf(val + sl, sizeof(val) - sl, "%02x ", stream_byte_read(&sp));
           }           
           cJSON_AddItemToObject(root, "Seed", cJSON_CreateString(val));
        }
    }
    else {
        if (sub % 2 == 0) {
           while (stream_left_len(&sp) > 0) {
                sl += snprintf(val + sl, sizeof(val) - sl, "%02x ", stream_byte_read(&sp));
           }           
           cJSON_AddItemToObject(root, "Key", cJSON_CreateString(val));
        }
    }
}

void yproto_uds_10_dsc(cJSON *root, yuint8 *msg, yuint32 size)
{
    stream_t sp;    
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    int sl = 0;

    stream_init(&sp, msg, size);
    yuint8 sid = stream_byte_read(&sp);
    yuint8 sub = stream_byte_read(&sp);
    if (sid & UDS_REPLY_MASK) {
        cJSON_AddItemToObject(root, "P2Server_max", cJSON_CreateNumber(stream_2byte_read(&sp)));
        cJSON_AddItemToObject(root, "P2*Server_max", cJSON_CreateNumber(stream_2byte_read(&sp)));
    }
}

void yproto_uds_36_td(cJSON *root, yuint8 *msg, yuint32 size)
{
    stream_t sp;    
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    int sl = 0;

    stream_init(&sp, msg, size);
    yuint8 sid = stream_byte_read(&sp);
    yuint8 count = stream_byte_read(&sp);
        
    cJSON_AddItemToObject(root, "BlockSequenceCounter", cJSON_CreateNumber(count));
    if (stream_left_len(&sp) > 0) {
        cJSON_AddItemToObject(root, "BlockDataLength", cJSON_CreateNumber(stream_left_len(&sp)));
    }
}

void yproto_uds_record_data(cJSON *root, yuint8 *record, yuint32 size)
{
    stream_t sp;
    char val[UDS_PROTOCOL_KEY_VAL] = {0};
    int sl = 0;

    if (record == 0 || size == 0) return ;

    stream_init(&sp, record, size);
    if (strlen((const char *)record) == size) {
        cJSON_AddItemToObject(root, "DataRecordString", cJSON_CreateString(record));
    }

    while (stream_left_len(&sp) > 0) {
        sl += snprintf(val + sl, sizeof(val) - sl, "%02x ", stream_byte_read(&sp));
    }    
    cJSON_AddItemToObject(root, "DataRecordHex", cJSON_CreateString(val));    
    cJSON_AddItemToObject(root, "DataRecordLength", cJSON_CreateNumber(size));
}

const char *yuds_proto_parse(yuint8 *msg, yuint32 size) 
{
    stream_t sp;

    if (msg == NULL || size == 0) {
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    stream_init(&sp, msg, size);
    yuint8 sid = stream_byte_read(&sp);
    if (sid == 0x7f) {/* 消极响应解析 */
        yuint8 id = stream_byte_read(&sp);
        yuint8 nrc = stream_byte_read(&sp);
        yproto_uds_negative_response(root, id, nrc);
    }
    else {/* 积极响应或者服务请求 */        
        /* 服务ID解析 */
        yproto_uds_service_identifier(root, sid);
    }

    /* 子功能解析 */
    switch (sid & UDS_SID_MASK) {
        case UDS_SERVICES_DSC:
        case UDS_SERVICES_ER:
        case UDS_SERVICES_SA:
        case UDS_SERVICES_CC:
        case UDS_SERVICES_CDTCS:
        case UDS_SERVICES_ROE:
        case UDS_SERVICES_LC:
        case UDS_SERVICES_RDTCI:
        case UDS_SERVICES_DDDI:
        case UDS_SERVICES_RC: 
            yproto_uds_subfunction(root, sid & UDS_SID_MASK, stream_byte_read(&sp));
            break;
    }

    /* 数据标识符解析 */
    switch (sid & UDS_SID_MASK) {
        case UDS_SERVICES_RDBI:
        case UDS_SERVICES_WDBI:
            yproto_uds_standard_did(root, stream_2byte_read(&sp));
            yproto_uds_record_data(root, msg + stream_use_len(&sp), stream_left_len(&sp));
            break;
        case UDS_SERVICES_RC:            
            yproto_uds_standard_rid(root, stream_2byte_read(&sp));    
            yproto_uds_record_data(root, msg + stream_use_len(&sp), stream_left_len(&sp));
            break;
    }

    /* record data解析 */
    switch (sid & UDS_SID_MASK) {
        case UDS_SERVICES_CDTCI:
            if (!(sid & UDS_REPLY_MASK)) {
                yproto_uds_group_of_dtc(root, stream_2byte_read(&sp) << 8 | stream_byte_read(&sp));
            }
            break;
        case UDS_SERVICES_CC:             
            if (!(sid & UDS_REPLY_MASK)) {
                yproto_uds_cc_comm_types(root, stream_byte_read(&sp));   
            }
            break;
        case UDS_SERVICES_RD:            
            yproto_uds_34_rd(root, msg, size);            
            break;
        case UDS_SERVICES_RFT:            
            yproto_uds_38_rft(root, msg, size);            
            break;
        case UDS_SERVICES_SA:
            yproto_uds_27_sa(root, msg, size); 
            break;
        case UDS_SERVICES_DSC:
            yproto_uds_10_dsc(root, msg, size); 
            break;
        case UDS_SERVICES_TD:
            yproto_uds_36_td(root, msg, size);
            break;
    }
    
    const char *str = cJSON_Print(root);
    cJSON_Delete(root);
    
    return str;
}

#if 0
/* 注释掉文件yudsc_types.h内的宏 #define DEBUG 可直接编译无其他依赖 */
/*  gcc cjson.c yudsc_types.c yuds_proto_parse.c yom_stream.c */
void yproto_uds_test()
{
    yuds_protocol_parse("\x10\x03", sizeof("\x10\x03") - 1);
    yuds_protocol_parse("\x50\x03\x00\x32\x01\xf4", sizeof("\x50\x03\x00\x32\x01\xf4") - 1);
    yuds_protocol_parse("\x50\x02\x00\x32\x01\xf4", sizeof("\x50\x02\x00\x32\x01\xf4") - 1);
    yuds_protocol_parse("\x31\x01\xff\x02", sizeof("\x31\x01\xff\x02") - 1);
    yuds_protocol_parse("\x71\x01\xff\x02\x00", sizeof("\x71\x01\xff\x02\x00") - 1);
    yuds_protocol_parse("\x85\x02", sizeof("\x85\x02") - 1);
    yuds_protocol_parse("\xc5\x02", sizeof("\xc5\x02") - 1);
    yuds_protocol_parse("\x27\x09", sizeof("\x27\x09") - 1);
    yuds_protocol_parse("\x67\x09\x8d\xa2\xd1\xe8", sizeof("\x67\x09\x8d\xa2\xd1\xe8") - 1);
    yuds_protocol_parse("\x27\x0a\x07\xfc\x56\x58", sizeof("\x27\x0a\x07\xfc\x56\x58") - 1);
    yuds_protocol_parse("\x67\x0a", sizeof("\x67\x0a") - 1);
    yuds_protocol_parse("\x34\x00\x44\x40\x03\x00\x00\x00\x00\x1c\x58", sizeof("\x34\x00\x44\x40\x03\x00\x00\x00\x00\x1c\x58") - 1);
    yuds_protocol_parse("\x7f\x34\x11", sizeof("\x7f\x34\x11") - 1);
    yuds_protocol_parse("\x2e\xf1\x5a\xff\x17\x09\x06\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x01\x02\x03\x04\x05", sizeof("\x2e\xf1\x5a\xff\x17\x09\x06\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x01\x02\x03\x04\x05") - 1);
    yuds_protocol_parse("\x31\x01\xfd\x07\x5f\x80\x4e\xb2\xf4\x5e\xae\x33\x63\xaf\x24\xc2\x02\x35\x92\x73", sizeof("\x31\x01\xfd\x07\x5f\x80\x4e\xb2\xf4\x5e\xae\x33\x63\xaf\x24\xc2\x02\x35\x92\x73") - 1);
    yuds_protocol_parse("\x71\x01\xfd\x07\x01", sizeof("\x71\x01\xfd\x07\x01") - 1);
    yuds_protocol_parse("\x6e\xf1\x5a", sizeof("\x6e\xf1\x5a") - 1);
    yuds_protocol_parse("\x38\x01\x00\x15\x2f\x74\x6d\x70\x2f\x75\x64\x73\x2f\x74\x6d\x70\x2f\x34\x4d\x42\x2e\x72\x61\x6e\x64\x00\x04\x00\x17\xa4\xaf\x00\x17\xa4\xaf", sizeof("\x38\x01\x00\x15\x2f\x74\x6d\x70\x2f\x75\x64\x73\x2f\x74\x6d\x70\x2f\x34\x4d\x42\x2e\x72\x61\x6e\x64\x00\x04\x00\x17\xa4\xaf\x00\x17\xa4\xaf") - 1);
    yuds_protocol_parse("\x78\x01\x02\x23\x28\x00", sizeof("\x78\x01\x02\x23\x28\x00") - 1);
    yuds_protocol_parse("\x36\x01\x58\x00\x00\x41\x02\x53\x58\x00\x00\x90\x08\x00\x45\x00", sizeof("\x36\x01\x58\x00\x00\x41\x02\x53\x58\x00\x00\x90\x08\x00\x45\x00") - 1);
    yuds_protocol_parse("\x22\xf1\x93", sizeof("\x22\xf1\x93") - 1);
    yuds_protocol_parse("\x62\xf1\x93\x02\x00", sizeof("\x62\xf1\x93\x02\x00") - 1);
}

int main()
{
    yproto_uds_test();
}
#endif 
