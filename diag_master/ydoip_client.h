#ifndef __YDOIP_CLIENT_H__
#define __YDOIP_CLIENT_H__

typedef void (*doipc_diagnostic_response)(void *arg, yuint16 sa, yuint16 ta, yuint8 *msg, yuint32 len);

typedef enum doipc_tcp_status_e {
    DOIPC_TCP_DISCONNECT,
    DOIPC_TCP_CONNECT_PROCESS,
    DOIPC_TCP_CONNECT_SUCCESS,
} doipc_tcp_status_e;

typedef struct doip_client_s {
    struct ev_loop *loop; /* 主事件循环结构体 */
    ev_io tcprecv_watcher; /* 监听TCP读事件 */
    ev_io tcpsend_watcher; /* 监听TCP写事件 */
    int index; /* 编号索引 */
    int udp_sfd; /* UDP socket */
    int tcp_sfd; /* TCP socket */
    yuint8 *rxbuf; /* 接收Buff */
    yuint32 rxlen; /* 接收Buff长度 */
    yuint8 *txbuf; /* 发送Buff */
    yuint32 txlen; /* 发送buff长度 */
    doipc_tcp_status_e con_stat; /* tcp连接状态 */
    int isactive; /* 是否已经路由激活 */
    doipc_diagnostic_response response_call;
    void *ddr_arg;
    doipc_config_t config;
} doip_client_t;

int ydoipc_diagnostic_request(doip_client_t *doipc, yuint16 sa, yuint16 ta, const yuint8 *msg, yuint32 len, doipc_diagnostic_response call, void *arg);

int ydoipc_disconnect_server(doip_client_t *doipc);

int ydoipc_connect_active_server(doip_client_t *doipc, doipc_config_t *config);

doip_client_t *ydoipc_create(struct ev_loop *loop);

int ydoipc_destroy(doip_client_t *doipc);


#endif /* __YDOIP_CLIENT_H__ */
