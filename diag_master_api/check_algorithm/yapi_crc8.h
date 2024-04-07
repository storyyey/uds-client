#ifndef _YAPI_CRC_CRC8_H
#define _YAPI_CRC_CRC8_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus  */

// LSB-first
unsigned char yapi_crc8_lsb(const char *buf, int len);

// MSB-first
unsigned char yapi_crc8_msb(const char *buf, int len);

#ifdef __cplusplus
}
#endif /* __cplusplus  */
#endif
