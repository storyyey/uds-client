#ifndef _YAPI_CRC_CRC32_H
#define _YAPI_CRC_CRC32_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus  */

unsigned int yapi_crc32(const char* s, int len);

unsigned int yapi_crc32re(const char* s, int len, unsigned int crc32val);

#ifdef __cplusplus
}
#endif /* __cplusplus  */
#endif
