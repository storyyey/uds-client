#ifndef __YUDS_PROTO_PARSE_H__
#define __YUDS_PROTO_PARSE_H__

#define UDS_PROTOCOL_KEY_VAL (512)

#ifdef _WIN32
extern const char *yuds_proto_parse(yuint8 *msg, yuint32 size); 
#define yuds_protocol_parse(m, s) do{ \
        const char *str = yuds_proto_parse((yuint8 *)m, (yuint32)s); \
        if (str) { \
            char *context = 0; \
            const char *pstr = strtok_s((char *)str, "\r\n", &context); \
            while (pstr) {\
                log_d("%s", pstr); \
                pstr = strtok_s(0, "\r\n", &context); \
            } \
            free((void *)str);\
        }\
    } while(0) 
#else /* #ifdef _WIN32 */
extern const char* yuds_proto_parse(yuint8 * msg, yuint32 size);
#define yuds_protocol_parse(m, s) do{ \
        const char *str = yuds_proto_parse((yuint8 *)m, (yuint32)s); \
        if (str) { \
            const char *pstr = strtok((char *)str, "\r\n"); \
            while (pstr) {\
                log_d("%s", pstr); \
                pstr = strtok(0, "\r\n"); \
            } \
            free((void *)str);\
        }\
    } while(0) 
#endif /* #ifdef _WIN32 */
#endif /* __YUDS_PROTO_PARSE_H__ */
