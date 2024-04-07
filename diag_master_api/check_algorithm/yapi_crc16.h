#ifndef _YAPI_CRC_CRC16_H
#define _YAPI_CRC_CRC16_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus  */

unsigned short yapi_crc16(const char *buf, int len);

unsigned short yapi_crc16re(const char *buf, int len, unsigned short crc);

#ifdef __cplusplus
}
#endif /* __cplusplus  */
#endif
