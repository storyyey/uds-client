#ifndef __CONFIG_H__
#define __CONFIG_H__

/* 开启UDS 协议解析,有些许性能影响 */
#define __HAVE_UDS_PROTOCOL_ANALYSIS__

/* ota master是开启多线程 */
// #define __HAVE_DIAG_MASTER_PTHREAD__

/* 开启进程监控 */
#define __HAVE_SELF_MONITOR__

#endif /* __CONFIG_H__ */
