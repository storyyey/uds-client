#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <pthread.h>

#include "config.h"
#include "ev.h"
#include "ydm_types.h"
#include "ydm_log.h"
#include "yudsc_types.h"
#include "ydm_ipc_common.h"       
#include "ydoip_client.h"

#define DOIP_PORT                                  13400
#define DOIP_TLS_PORT                               3496

#define DOIP_GENERIC_NACK                          0x0000
#define DOIP_VEHICLE_IDENTIFICATION_REQ            0x0001
#define DOIP_VEHICLE_IDENTIFICATION_REQ_EID        0x0002
#define DOIP_VEHICLE_IDENTIFICATION_REQ_VIN        0x0003
#define DOIP_VEHICLE_ANNOUNCEMENT_MESSAGE          0x0004
#define DOIP_ROUTING_ACTIVATION_REQUEST            0x0005
#define DOIP_ROUTING_ACTIVATION_RESPONSE           0x0006
#define DOIP_ALIVE_CHECK_REQUEST                   0x0007
#define DOIP_ALIVE_CHECK_RESPONSE                  0x0008
#define DOIP_ENTITY_STATUS_REQUEST                 0x4001
#define DOIP_ENTITY_STATUS_RESPONSE                0x4002
#define DOIP_POWER_INFORMATION_REQUEST             0x4003
#define DOIP_POWER_INFORMATION_RESPONSE            0x4004
#define DOIP_DIAGNOSTIC_MESSAGE                    0x8001
#define DOIP_DIAGNOSTIC_MESSAGE_ACK                0x8002
#define DOIP_DIAGNOSTIC_MESSAGE_NACK               0x8003


/* Header */
#define DOIP_VERSION_OFFSET                        0
#define DOIP_VERSION_LEN                           1
#define DOIP_INV_VERSION_OFFSET                    (DOIP_VERSION_OFFSET + DOIP_VERSION_LEN)
#define DOIP_INV_VERSION_LEN                       1
#define DOIP_TYPE_OFFSET                           (DOIP_INV_VERSION_OFFSET + DOIP_INV_VERSION_LEN)
#define DOIP_TYPE_LEN                              2
#define DOIP_LENGTH_OFFSET                         (DOIP_TYPE_OFFSET + DOIP_TYPE_LEN)
#define DOIP_LENGTH_LEN                            4
#define DOIP_HEADER_LEN                            (DOIP_LENGTH_OFFSET + DOIP_LENGTH_LEN)

#define RESERVED_VER                               0x00
#define ISO13400_2010                              0x01
#define ISO13400_2012                              0x02
#define ISO13400_2019                              0x03
#define ISO13400_2019_AMD1                         0x04
#define DEFAULT_VALUE                              0xFF


/* Generic NACK */
#define DOIP_GENERIC_NACK_OFFSET                   DOIP_HEADER_LEN
#define DOIP_GENERIC_NACK_LEN                      1


/* Common */
#define DOIP_COMMON_VIN_LEN                        17
#define DOIP_COMMON_EID_LEN                        6


/*  Vehicle identifcation request */
#define DOIP_VEHICLE_IDENTIFICATION_EID_OFFSET     DOIP_HEADER_LEN
#define DOIP_VEHICLE_IDENTIFICATION_VIN_OFFSET     DOIP_HEADER_LEN


/* Routing activation request */
#define DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET     DOIP_HEADER_LEN
#define DOIP_ROUTING_ACTIVATION_REQ_SRC_LEN        2
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET    (DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_SRC_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V1    2
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V2    1
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V1  (DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V1)
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2  (DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V2)
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN        4
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V1  (DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V1 + DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2  (DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_LEN        4


/* Routing activation response */
#define DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET  DOIP_HEADER_LEN
#define DOIP_ROUTING_ACTIVATION_RES_TESTER_LEN     2
#define DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET  (DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET + DOIP_ROUTING_ACTIVATION_RES_TESTER_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_ENTITY_LEN     2
#define DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET    (DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET + DOIP_ROUTING_ACTIVATION_RES_ENTITY_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_CODE_LEN       1
#define DOIP_ROUTING_ACTIVATION_RES_ISO_OFFSET     (DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET + DOIP_ROUTING_ACTIVATION_RES_CODE_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_ISO_LEN        4
#define DOIP_ROUTING_ACTIVATION_RES_OEM_OFFSET     (DOIP_ROUTING_ACTIVATION_RES_ISO_OFFSET + DOIP_ROUTING_ACTIVATION_RES_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_OEM_LEN        4


/* Vehicle announcement message */
#define DOIP_VEHICLE_ANNOUNCEMENT_VIN_OFFSET       DOIP_HEADER_LEN
#define DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_OFFSET   (DOIP_VEHICLE_ANNOUNCEMENT_VIN_OFFSET + DOIP_COMMON_VIN_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_LEN      2
#define DOIP_VEHICLE_ANNOUNCEMENT_EID_OFFSET       (DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_GID_OFFSET       (DOIP_VEHICLE_ANNOUNCEMENT_EID_OFFSET + DOIP_COMMON_EID_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_GID_LEN          6
#define DOIP_VEHICLE_ANNOUNCEMENT_ACTION_OFFSET    (DOIP_VEHICLE_ANNOUNCEMENT_GID_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_GID_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_ACTION_LEN       1
#define DOIP_VEHICLE_ANNOUNCEMENT_SYNC_OFFSET      (DOIP_VEHICLE_ANNOUNCEMENT_ACTION_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_ACTION_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_SYNC_LEN         1


/* Alive check response */
#define DOIP_ALIVE_CHECK_RESPONSE_SOURCE_OFFSET    DOIP_HEADER_LEN
#define DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN       2


/* Entity status response */
#define DOIP_ENTITY_STATUS_RESPONSE_NODE_OFFSET    DOIP_HEADER_LEN
#define DOIP_ENTITY_STATUS_RESPONSE_NODE_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_MCTS_OFFSET    (DOIP_ENTITY_STATUS_RESPONSE_NODE_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_NODE_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_MCTS_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_NCTS_OFFSET    (DOIP_ENTITY_STATUS_RESPONSE_MCTS_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_MCTS_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_NCTS_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_MDS_OFFSET     (DOIP_ENTITY_STATUS_RESPONSE_NCTS_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_NCTS_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_MDS_LEN        4


/* Diagnostic power mode information response */
#define DOIP_POWER_MODE_OFFSET                     DOIP_HEADER_LEN
#define DOIP_POWER_MODE_LEN                        1


/* Common */
#define DOIP_DIAG_COMMON_SOURCE_OFFSET             DOIP_HEADER_LEN
#define DOIP_DIAG_COMMON_SOURCE_LEN                2
#define DOIP_DIAG_COMMON_TARGET_OFFSET             (DOIP_DIAG_COMMON_SOURCE_OFFSET + DOIP_DIAG_COMMON_SOURCE_LEN)
#define DOIP_DIAG_COMMON_TARGET_LEN                2


/* Diagnostic message */
#define DOIP_DIAG_MESSAGE_DATA_OFFSET              (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)


/* Diagnostic message ACK */
#define DOIP_DIAG_MESSAGE_ACK_CODE_OFFSET          (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)
#define DOIP_DIAG_MESSAGE_ACK_CODE_LEN             1
#define DOIP_DIAG_MESSAGE_ACK_PREVIOUS_OFFSET      (DOIP_DIAG_MESSAGE_ACK_CODE_OFFSET + DOIP_DIAG_MESSAGE_ACK_CODE_LEN)


/* Diagnostic message NACK */
#define DOIP_DIAG_MESSAGE_NACK_CODE_OFFSET         (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)
#define DOIP_DIAG_MESSAGE_NACK_CODE_LEN            1
#define DOIP_DIAG_MESSAGE_NACK_PREVIOUS_OFFSET     (DOIP_DIAG_MESSAGE_NACK_CODE_OFFSET + DOIP_DIAG_MESSAGE_NACK_CODE_LEN)



/*
 * Enums
 */

/* Header */
/* Protocol version */
static const value_string doip_versions[] = {
    { RESERVED_VER,         "Reserved" },
    { ISO13400_2010,        "DoIP ISO/DIS 13400-2:2010" },
    { ISO13400_2012,        "DoIP ISO 13400-2:2012" },
    { ISO13400_2019,        "DoIP ISO 13400-2:2019" },
    { ISO13400_2019_AMD1,   "DoIP ISO 13400-2:2019 Amd1 (experimental)" },
    { DEFAULT_VALUE,        "Default value for vehicle identification request messages" },
    { 0, 0 }
};

const char *ydesc_doip_versions(yuint32 value)
{
    int i = 0;

    for (i = 0; doip_versions[i].strptr; i++) {
        if (doip_versions[i].value == value) {
            return doip_versions[i].strptr;
        }
    }

    return "Unknown";
}

/* Payload type */
static const value_string doip_payloads[] = {
    { DOIP_GENERIC_NACK,                    "Generic DoIP header NACK" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ,      "Vehicle identification request" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ_EID,  "Vehicle identification request with EID" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ_VIN,  "Vehicle identification request with VIN" },
    { DOIP_VEHICLE_ANNOUNCEMENT_MESSAGE,     "Vehicle announcement message/vehicle identification response message" },
    { DOIP_ROUTING_ACTIVATION_REQUEST, "Routing activation request" },
    { DOIP_ROUTING_ACTIVATION_RESPONSE, "Routing activation response" },
    { DOIP_ALIVE_CHECK_REQUEST, "Alive check request" },
    { DOIP_ALIVE_CHECK_RESPONSE, "Alive check response" },
    { DOIP_ENTITY_STATUS_REQUEST, "DoIP entity status request" },
    { DOIP_ENTITY_STATUS_RESPONSE, "DoIP entity status response" },
    { DOIP_POWER_INFORMATION_REQUEST, "Diagnostic power mode information request" },
    { DOIP_POWER_INFORMATION_RESPONSE, "Diagnostic power mode information response" },
    { DOIP_DIAGNOSTIC_MESSAGE, "Diagnostic message" },
    { DOIP_DIAGNOSTIC_MESSAGE_ACK, "Diagnostic message ACK" },
    { DOIP_DIAGNOSTIC_MESSAGE_NACK, "Diagnostic message NACK" },
    { 0, 0 }
};
    
const char *ydesc_doip_payloads(yuint32 value)
{
    int i = 0;

    for (i = 0; doip_payloads[i].strptr; i++) {
        if (doip_payloads[i].value == value) {
            return doip_payloads[i].strptr;
        }
    }

    return "Unknown";
}

/* Generic NACK */
static const value_string nack_codes[] = {
    { 0x00, "Incorrect pattern format" },
    { 0x01, "Unknown payload type" },
    { 0x02, "Message too large" },
    { 0x03, "Out of memory" },
    { 0x04, "Invalid payload length" },
    { 0, 0 }
};

const char *ydesc_nack_codes(yuint32 value)
{
    int i = 0;

    for (i = 0; nack_codes[i].strptr; i++) {
        if (nack_codes[i].value == value) {
            return nack_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Routing activation request */
static const value_string activation_types[] = {
    { 0x00, "Default" },
    { 0x01, "WWH-OBD" },
    { 0xE0, "Central security" },
    { 0, 0 }
};
    
const char *ydesc_activation_types(yuint32 value)
{
    int i = 0;

    for (i = 0; activation_types[i].strptr; i++) {
        if (activation_types[i].value == value) {
            return activation_types[i].strptr;
        }
    }

    return "Unknown";
}

/* Routing activation response */
static const value_string activation_codes[] = {
    { 0x00, "Routing activation denied due to unknown source address." },
    { 0x01, "Routing activation denied because all concurrently supported TCP_DATA sockets are registered and active." },
    { 0x02, "Routing activation denied because an SA different from the table connection entry was received on the already activated TCP_DATA socket." },
    { 0x03, "Routing activation denied because the SA is already registered and active on a different TCP_DATA socket." },
    { 0x04, "Routing activation denied due to missing authentication." },
    { 0x05, "Routing activation denied due to rejected confirmation." },
    { 0x06, "Routing activation denied due to unsupported routing activation type." },
    { 0x07, "Routing activation denied due to request for encrypted connection via TLS." },
    { 0x08, "Reserved by ISO 13400." },
    { 0x09, "Reserved by ISO 13400." },
    { 0x0A, "Reserved by ISO 13400." },
    { 0x0B, "Reserved by ISO 13400." },
    { 0x0C, "Reserved by ISO 13400." },
    { 0x0D, "Reserved by ISO 13400." },
    { 0x0E, "Reserved by ISO 13400." },
    { 0x0F, "Reserved by ISO 13400." },
    { 0x10, "Routing successfully activated." },
    { 0x11, "Routing will be activated; confirmation required." },
    { 0, 0 }
};

const char *ydesc_activation_codes(yuint32 value)
{
    int i = 0;

    for (i = 0; activation_codes[i].strptr; i++) {
        if (activation_codes[i].value == value) {
            return activation_codes[i].strptr;
        }
    }

    return "Unknown";
}


/* Vehicle announcement message */
/* Action code */
static const value_string action_codes[] = {
    { 0x00, "No further action required" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Reserved by ISO 13400" },
    { 0x03, "Reserved by ISO 13400" },
    { 0x04, "Reserved by ISO 13400" },
    { 0x05, "Reserved by ISO 13400" },
    { 0x06, "Reserved by ISO 13400" },
    { 0x07, "Reserved by ISO 13400" },
    { 0x08, "Reserved by ISO 13400" },
    { 0x09, "Reserved by ISO 13400" },
    { 0x0A, "Reserved by ISO 13400" },
    { 0x0B, "Reserved by ISO 13400" },
    { 0x0C, "Reserved by ISO 13400" },
    { 0x0D, "Reserved by ISO 13400" },
    { 0x0E, "Reserved by ISO 13400" },
    { 0x0F, "Reserved by ISO 13400" },
    { 0x10, "Routing activation required to initiate central security" },
    { 0, 0 }
};

const char *ydesc_action_codes(yuint32 value)
{
    int i = 0;

    for (i = 0; action_codes[i].strptr; i++) {
        if (action_codes[i].value == value) {
            return action_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Sync status */
static const value_string sync_status[] = {
    { 0x00, "VIN and/or GID are synchronized" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Reserved by ISO 13400" },
    { 0x03, "Reserved by ISO 13400" },
    { 0x04, "Reserved by ISO 13400" },
    { 0x05, "Reserved by ISO 13400" },
    { 0x06, "Reserved by ISO 13400" },
    { 0x07, "Reserved by ISO 13400" },
    { 0x08, "Reserved by ISO 13400" },
    { 0x09, "Reserved by ISO 13400" },
    { 0x0A, "Reserved by ISO 13400" },
    { 0x0B, "Reserved by ISO 13400" },
    { 0x0C, "Reserved by ISO 13400" },
    { 0x0D, "Reserved by ISO 13400" },
    { 0x0E, "Reserved by ISO 13400" },
    { 0x0F, "Reserved by ISO 13400" },
    { 0x10, "Incomplete: VIN and GID are NOT synchronized" },
    { 0, 0 }
};

const char *ydesc_sync_status(yuint32 value)
{
    int i = 0;

    for (i = 0; action_codes[i].strptr; i++) {
        if (action_codes[i].value == value) {
            return action_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Entity status response */
/* Node type */
static const value_string node_types[] = {
    { 0x00, "DoIP gateway" },
    { 0x01, "DoIp node" },
    { 0, 0 }
};

const char *ydesc_node_types(yuint32 value)
{
    int i = 0;

    for (i = 0; node_types[i].strptr; i++) {
        if (node_types[i].value == value) {
            return node_types[i].strptr;
        }
    }

    return "Unknown";
}

/* Diagnostic power mode information response */
/* Power mode */
static const value_string power_modes[] = {
    { 0x00, "not ready" },
    { 0x01, "ready" },
    { 0x02, "not supported" },
    { 0, 0 }
};

const char *ydesc_power_modes(yuint32 value)
{
    int i = 0;

    for (i = 0; power_modes[i].strptr; i++) {
        if (power_modes[i].value == value) {
            return power_modes[i].strptr;
        }
    }

    return "Unknown";
}

/* Diagnostic message ACK */
static const value_string diag_ack_codes[] = {
    { 0x00, "ACK" },
    { 0, 0 }
};

const char *ydesc_diag_ack_codes(yuint32 value)
{
    int i = 0;

    for (i = 0; diag_ack_codes[i].strptr; i++) {
        if (diag_ack_codes[i].value == value) {
            return diag_ack_codes[i].strptr;
        }
    }

    return "Unknown";
}

/* Diagnostic message NACK */
static const value_string diag_nack_codes[] = {\
    { 0x00, "Reserved by ISO 13400" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Invalid source address" },
    { 0x03, "Unknown target address" },
    { 0x04, "Diagnostic message too large" },
    { 0x05, "Out of memory" },
    { 0x06, "Target unreachable" },
    { 0x07, "Unknown network" },
    { 0x08, "Transport protocol error" },
    { 0, 0 }
};

const char *ydesc_diag_nack_codes(yuint32 value)
{
    int i = 0;

    for (i = 0; diag_nack_codes[i].strptr; i++) {
        if (diag_nack_codes[i].value == value) {
            return diag_nack_codes[i].strptr;
        }
    }

    return "Unknown";
}

static int doipc_disconnect_server(doip_client_t *doipc);
int doipc_routing_activation_request(doip_client_t *doipc);

static int doipc_udp_create(unsigned int ip, short port)
{
    int sockfd = -1;
    int flag = 0;
    
#ifdef __linux__    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#endif /* __linux__ */
    if (!(sockfd > 0)) {
        return sockfd;
    }
    flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
       
    return sockfd;
}

static int doipc_tcp_create(unsigned int ip, short port)
{
    int sockfd = -1;
    int flag = 0;

#ifdef __linux__    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#endif /* __linux__ */
    if (!(sockfd > 0)) {
        return -1;
    }
    flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
       
    return sockfd;
}

static int doipc_is_active(doip_client_t *doipc)
{
    return doipc->isactive;
}

int doipc_tcp_send(doip_client_t *doipc, yuint8 *msg, yuint32 size)
{
    int bytesSent = -1;
    fd_set writefds;
    struct timeval timeout;
    int sret = 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(doipc->tcp_sfd, &writefds);
    if ((sret = select(doipc->tcp_sfd + 1, NULL, &writefds, NULL, &timeout)) > 0 &&\
        FD_ISSET(doipc->tcp_sfd, &writefds)) {
        bytesSent = send(doipc->tcp_sfd, msg, size, 0);
    }

    return bytesSent;
}

int doipc_header_encode(yuint8 *buf, yuint32 buf_size, yuint8 ver, yuint16 type, yuint32 len)
{
    if (buf_size < DOIP_HEADER_LEN) {
        return -1;
    }
    buf[DOIP_VERSION_OFFSET] = ver;
    buf[DOIP_INV_VERSION_OFFSET] = ~ver;
    buf[DOIP_TYPE_OFFSET] = ((type >> 8) & 0xff);
    buf[DOIP_TYPE_OFFSET + 1] = ((type >> 0) & 0xff);
    buf[DOIP_LENGTH_OFFSET] = ((len >> 24) & 0xff);
    buf[DOIP_LENGTH_OFFSET + 1] = ((len >> 16) & 0xff);
    buf[DOIP_LENGTH_OFFSET + 2] = ((len >> 8) & 0xff);
    buf[DOIP_LENGTH_OFFSET + 3] = ((len >> 0) & 0xff);

    return 0;
}

int doipc_header_decode(yuint8 *buf, yuint32 buf_size, yuint8 *ver, yuint16 *type, yuint32 *len)
{
    if (buf_size < DOIP_HEADER_LEN) {
        return -1;
    }
    *ver = buf[DOIP_VERSION_OFFSET];
    *type = (*type & ~(0xff << 8)) | ((buf[DOIP_TYPE_OFFSET] & 0xff) << 8);
    *type = (*type & ~(0xff << 0)) | ((buf[DOIP_TYPE_OFFSET + 1] & 0xff) << 0);
    *len = (*len & ~(0xff << 24)) | ((buf[DOIP_LENGTH_OFFSET] & 0xff) << 24);
    *len = (*len & ~(0xff << 16)) | ((buf[DOIP_LENGTH_OFFSET + 1] & 0xff) << 16);
    *len = (*len & ~(0xff << 8)) | ((buf[DOIP_LENGTH_OFFSET + 2] & 0xff) << 8);
    *len = (*len & ~(0xff << 0)) | ((buf[DOIP_LENGTH_OFFSET + 3] & 0xff) << 0);

    return 0;
}

static boolean doipc_tcp_connect_check(doip_client_t *doipc)
{
    fd_set writefds;
    fd_set readfds;
    struct timeval timeout;
    int sret = 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(doipc->tcp_sfd, &writefds);
    FD_ZERO(&readfds);
    FD_SET(doipc->tcp_sfd, &readfds);
    if ((sret = select(doipc->tcp_sfd + 1, &readfds, &writefds, NULL, &timeout)) > 0) {
        int error = 0;
        socklen_t len = sizeof(error);
    
        if (FD_ISSET(doipc->tcp_sfd, &writefds) &&\
            FD_ISSET(doipc->tcp_sfd, &readfds)) {
            if (getsockopt(doipc->tcp_sfd, SOL_SOCKET, SO_ERROR, (void *)&error, &len) < 0 || \
                error != 0) {
                return false;
            }
            else {                
                return true;
            }
        }
        else if (FD_ISSET(doipc->tcp_sfd, &writefds)) {            
            return true;
        }
        else if (FD_ISSET(doipc->tcp_sfd, &readfds)) {        
            return false;
        }
    }

    return false;
}

/* 连接成功后做一些处理 */
static void doipc_tcp_connect_success_handler(doip_client_t *doipc)
{
    doipc->con_stat = DOIPC_TCP_CONNECT_SUCCESS;
    /* TCP连接后立即进行路由激活 */
    doipc_routing_activation_request(doipc);   
}

/* 断开连接和连接失败做一些处理 */
static void doipc_tcp_connect_failed_handler(doip_client_t *doipc)
{
    /* 断开TCP连接，重置一些标志位 */
    doipc_disconnect_server(doipc); 
}

static void doipc_tcp_connect_process_handler(doip_client_t *doipc)
{
    if (doipc_tcp_connect_check(doipc)) {
        log_d("doip tcp connect success");
        doipc_tcp_connect_success_handler(doipc);   
    }
    else {        
        log_d("doip tcp connect failed");
        doipc_tcp_connect_failed_handler(doipc);
    }
    ev_io_stop(doipc->loop, &doipc->tcpsend_watcher);

    return ;
}

static void doipc_ev_io_tcp_write_handler(struct ev_loop* loop, ev_io* w, int e)
{
    doip_client_t* doipc = container_of(w, doip_client_t, tcpsend_watcher);

    /* 检查是否连接上 */
    if (doipc->con_stat == DOIPC_TCP_CONNECT_PROCESS) {
        doipc_tcp_connect_process_handler(doipc);
        return ;
    }

    return ;
}

static void doipc_routing_activation_response_handler(doip_client_t *doipc)
{
    yuint16 tester = 0, entity = 0;
    yuint8 rcode = 0;

    tester |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET] & 0xff) << 8;
    tester |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET + 1] & 0xff) << 0;
    entity |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET] & 0xff) << 8;
    entity |= (doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET + 1] & 0xff) << 0;
    rcode = doipc->rxbuf[DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET];
    log_d("tester => %04x", tester);
    log_d("entity => %04x", entity);
    log_d("rcode  => %02x", rcode);
    log_d("%s", ydesc_activation_codes(rcode));    
    if (rcode == 0x10) {
        doipc->isactive = 1;
    }
    else {
        doipc->isactive = 0;
    }

    return ;
}

static void doipc_diagnostic_response_handler(doip_client_t *doipc, yuint32 len)
{
    yuint16 sa = 0, ta = 0;

    sa |= (doipc->rxbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET] & 0xff) << 8;
    sa |= (doipc->rxbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET + 1] & 0xff) << 0;
    ta |= (doipc->rxbuf[DOIP_DIAG_COMMON_TARGET_OFFSET] & 0xff) << 8;
    ta |= (doipc->rxbuf[DOIP_DIAG_COMMON_TARGET_OFFSET + 1] & 0xff) << 0;
    if (doipc->response_call) {
        doipc->response_call(doipc->ddr_arg, sa, ta, doipc->rxbuf + DOIP_DIAG_MESSAGE_DATA_OFFSET, len);
    }

    return ;
}

static void doipc_alive_check_request_handler(doip_client_t *doipc)
{
    if (!doipc_is_active(doipc)) {
        return ;
    }

    /* encode 8byte fix header */
    doipc_header_encode(doipc->txbuf, doipc->txlen, doipc->config.ver, DOIP_ALIVE_CHECK_RESPONSE, \
                                DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN);
    /* encode 2byte source addr */
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET] = ((doipc->config.sa_addr >> 8) & 0xff);
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET + 1] = ((doipc->config.sa_addr >> 0) & 0xff);
    doipc_tcp_send(doipc, doipc->txbuf, DOIP_HEADER_LEN + DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN);

    return;
}

static void doipc_ev_io_tcp_read_handler(struct ev_loop* loop, ev_io* w, int e)
{
    int bytesRecv = -1;
    doip_client_t* doipc = container_of(w, doip_client_t, tcprecv_watcher);
    yuint8 ver = 0; yuint16 type = 0; yuint32 len = 0;

    /* 检查是否连接上 */
    if (doipc->con_stat == DOIPC_TCP_CONNECT_PROCESS) {
        doipc_tcp_connect_process_handler(doipc);
        return ;
    }

    /* 尝试接收完整的doip头 */
    bytesRecv = recv(doipc->tcp_sfd, doipc->rxbuf, DOIP_HEADER_LEN, MSG_PEEK);
    if (!(bytesRecv > 0)) {
        log_e("Failed to receive the doip header");
        doipc_tcp_connect_failed_handler(doipc);
        return ;
    }
    if (bytesRecv != DOIP_HEADER_LEN) {
        return ;
    }
    /* 解析doip头 */
    doipc_header_decode(doipc->rxbuf, DOIP_HEADER_LEN, &ver, &type, &len);
    if (DOIP_HEADER_LEN + len > doipc->rxlen) {          
        log_e("The receiving buff length is insufficient");
        doipc_tcp_connect_failed_handler(doipc);
        return ;
    }
    /* 尝试接收完整的doip数据，数据还存储在接收缓冲区中 */
    bytesRecv = recv(doipc->tcp_sfd, doipc->rxbuf, doipc->rxlen, MSG_PEEK);
    if (!(bytesRecv > 0)) {
        log_e("Failed to receive full doip data");
        doipc_tcp_connect_failed_handler(doipc);
        return ;
    }
    /* doip数据不完整等待完整的doip数据到来 */
    if (bytesRecv != DOIP_HEADER_LEN + len) {
        return ;
    }
    /* doip数据已经完整，从接收缓冲区一次读出 */
    bytesRecv = recv(doipc->tcp_sfd, doipc->rxbuf, doipc->rxlen, 0);
    if (!(bytesRecv > 0)) {
        log_e("ailed to receive complete doip data");
        doipc_tcp_connect_failed_handler(doipc);
        return ;
    }
    log_hex_d("DOIP Response", doipc->rxbuf, bytesRecv);    
    switch (type) {
        case DOIP_ROUTING_ACTIVATION_RESPONSE:
            doipc_routing_activation_response_handler(doipc);
            break;
        case DOIP_ALIVE_CHECK_REQUEST:
            doipc_alive_check_request_handler(doipc);
            break;
        case DOIP_DIAGNOSTIC_MESSAGE:
            doipc_diagnostic_response_handler(doipc, len);
            break;
        default:

            break;
    }

    return ;
}

static int doipc_event_loop_set(doip_client_t *doipc, struct ev_loop *loop)
{
    if (doipc == 0 || loop == 0) {
        return -1;
    }
    
    doipc->loop = loop;
    
    return 0;
}

int doipc_routing_activation_request(doip_client_t *doipc)
{
    if (doipc_is_active(doipc)) {        
        log_w("The doip client is not connected to the doip server");
        return -1;
    }

    if (doipc->txlen < DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2) {        
        log_e("The message length(%d) to be sent is greater than the doip client send buff length(%d)",\
            DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2, doipc->txlen);
        return -2;
    }

    /* encode 8byte fix header */
    doipc_header_encode(doipc->txbuf, doipc->txlen, doipc->config.ver, DOIP_ROUTING_ACTIVATION_REQUEST, \
                                DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2 - DOIP_HEADER_LEN);
    /* encode 2byte source addr */
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET] = ((doipc->config.sa_addr >> 8) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET + 1] = ((doipc->config.sa_addr >> 0) & 0xff);
    /* encode 1byte req type */
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET] = 0;
    /* encode 4byte iso */
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2] = ((0 >> 24) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + 1] = ((0 >> 16) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + 2] = ((0 >> 8) & 0xff);
    doipc->txbuf[DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + 3] = ((0 >> 0) & 0xff);
    log_hex_d("DOIP rout active request:", doipc->txbuf, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2);
    
    return doipc_tcp_send(doipc, doipc->txbuf, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2);

}

static int doipc_disconnect_server(doip_client_t *doipc)
{
    log_d("doip client disconnect");
    doipc->isactive = 0;
    doipc->con_stat = DOIPC_TCP_DISCONNECT;
    ev_io_stop(doipc->loop, &doipc->tcprecv_watcher);
    ev_io_stop(doipc->loop, &doipc->tcpsend_watcher);
    if (doipc->tcp_sfd > 0) {
        close(doipc->tcp_sfd);
        doipc->tcp_sfd = -1;
    }

    if (doipc->udp_sfd > 0) {
        close(doipc->udp_sfd);
        doipc->udp_sfd = -1;
    }

    return 0;
}

static int doipc_connect_server(doip_client_t *doipc)
{
    int ret = 0;
    struct sockaddr_in toAddr;

    if (!(doipc->tcp_sfd > 0)) {
        /* 创建TCP */
        doipc->tcp_sfd = doipc_tcp_create(doipc->config.src_ip, doipc->config.src_port);
        if (doipc->tcp_sfd < 0) {
            log_e("TCP Socket create error");
            return -1;
        }
    }
    if (!(doipc->udp_sfd > 0)) {
        /* 创建一个UDP的socket */
        doipc->udp_sfd = doipc_udp_create(doipc->config.src_ip, doipc->config.src_port);
        if (doipc->udp_sfd < 0) {
            log_e("UDP Socket create error");
            return -2;
        }
    }

    /* 开始连接server */
    toAddr.sin_family = AF_INET;
    toAddr.sin_addr.s_addr = htonl(doipc->config.dst_ip);
    toAddr.sin_port = htons(doipc->config.dst_port);
    ret = connect(doipc->tcp_sfd, (struct sockaddr *)&toAddr, sizeof(struct sockaddr));
    if (ret == -1 && errno == EINPROGRESS) {
        /* 标记一下正在连接中 */
        doipc->con_stat = DOIPC_TCP_CONNECT_PROCESS; 
        /* 非阻塞的SOCKET需要另行判断 */
        ev_io_init(&doipc->tcprecv_watcher, doipc_ev_io_tcp_read_handler, doipc->tcp_sfd, EV_READ);
        ev_io_start(doipc->loop, &doipc->tcprecv_watcher);
        ev_io_init(&doipc->tcpsend_watcher, doipc_ev_io_tcp_write_handler, doipc->tcp_sfd, EV_WRITE);
        ev_io_start(doipc->loop, &doipc->tcpsend_watcher);
    }
    else if (ret == 0) {        
        log_w("TCP connect success");
        /* 非阻塞的SOCKET需要另行判断 */
        ev_io_init(&doipc->tcprecv_watcher, doipc_ev_io_tcp_read_handler, doipc->tcp_sfd, EV_READ);
        ev_io_start(doipc->loop, &doipc->tcprecv_watcher);        
        doipc_tcp_connect_success_handler(doipc);   
    }
    else {
        /* 连接失败 */
        log_w("TCP connect failed");
        doipc_tcp_connect_failed_handler(doipc);
    }

    return 0;
}

int ydoipc_diagnostic_request(doip_client_t *doipc, yuint16 sa, yuint16 ta, const yuint8 *msg, yuint32 len, doipc_diagnostic_response call, void *arg)
{
    if (!doipc_is_active(doipc)) {
        log_w("The doip client is not connected to the doip server");
        return -1;
    }

    if (len > (doipc->txlen - DOIP_DIAG_COMMON_TARGET_OFFSET)) {
        log_e("The message length(%d) to be sent is greater than the doip client send buff length(%d)",\
            len, (doipc->txlen - DOIP_DIAG_COMMON_TARGET_OFFSET));
        return -2;
    }
    doipc->response_call = call;
    doipc->ddr_arg = arg;

    /* encode 8byte fix header */
    doipc_header_encode(doipc->txbuf, doipc->txlen, doipc->config.ver, DOIP_DIAGNOSTIC_MESSAGE, \
                                len + DOIP_DIAG_COMMON_TARGET_OFFSET - DOIP_HEADER_LEN);
    /* encode 2byte source addr */
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET] = ((doipc->config.sa_addr >> 8) & 0xff);
    doipc->txbuf[DOIP_DIAG_COMMON_SOURCE_OFFSET + 1] = ((doipc->config.sa_addr >> 0) & 0xff);
    /* encode 2byte target addr */
    doipc->txbuf[DOIP_DIAG_COMMON_TARGET_OFFSET] = ((ta >> 8) & 0xff);
    doipc->txbuf[DOIP_DIAG_COMMON_TARGET_OFFSET + 1] = ((ta >> 0) & 0xff);
    /* encode nbyte uds diag msg */
    memcpy(doipc->txbuf + DOIP_DIAG_COMMON_TARGET_OFFSET, msg, len);
    log_hex_d("DOIP Diag request:", doipc->txbuf, len + DOIP_DIAG_COMMON_TARGET_OFFSET);
    
    return doipc_tcp_send(doipc, doipc->txbuf, len + DOIP_DIAG_COMMON_TARGET_OFFSET);
}

int ydoipc_disconnect_server(doip_client_t *doipc)
{
    doipc_disconnect_server(doipc);

    return 0;
}

int ydoipc_connect_active_server(doip_client_t *doipc, doipc_config_t *config)
{
    memcpy(&doipc->config, config, sizeof(*config));
    doipc->config.ver = ISO13400_2012;
    
    /* 断开后重新连接激活 */
    doipc_disconnect_server(doipc);

    /* 重新分配 rx tx buff */
    if (config->rxlen > 0 && \
        config->rxlen != doipc->rxlen) {
        if (doipc->rxbuf) {
            yfree(doipc->rxbuf);
            doipc->rxbuf = NULL;
            doipc->rxlen = 0;
        }
        doipc->rxlen = config->rxlen;
        doipc->rxbuf = ymalloc(doipc->rxlen);
        if (doipc->rxbuf == NULL) {
            return -1;
        }
        memset(doipc->rxbuf, 0, doipc->rxlen);
        log_d("remalloc doip rx buff %d", doipc->rxlen);
    }
    if (config->txlen > 0 && \
        config->txlen != doipc->txlen) {
        if (doipc->txbuf) {
            yfree(doipc->txbuf);
            doipc->txbuf = NULL;
            doipc->txlen = 0;
        }
        doipc->txlen = config->txlen;
        doipc->txbuf = ymalloc(doipc->txlen);
        if (doipc->txbuf == NULL) {
            return -2;
        }
        memset(doipc->txbuf, 0, doipc->txlen);
        log_d("remalloc doip tx buff %d", doipc->txlen);
    }

    /* 连接doip server */
    doipc_connect_server(doipc);

    return 0;
}

doip_client_t *ydoipc_create(struct ev_loop *loop)
{
    doip_client_t *doipc = ymalloc(sizeof(*doipc));
    if (doipc == 0) {
        return 0;
    }
    memset(doipc, 0, sizeof(*doipc));
    doipc->tcp_sfd = -1;
    doipc->udp_sfd = -1;
    doipc->con_stat = DOIPC_TCP_DISCONNECT;
    doipc->isactive = 0;    

    doipc->rxlen = IPC_RX_BUFF_SIZE;
    doipc->rxbuf = ymalloc(doipc->rxlen);
    if (doipc->rxbuf == 0) {
        log_e("malloc rxbuff error");
        goto CREATE_FAILED;
    }
    memset(doipc->rxbuf, 0, doipc->rxlen);

    doipc->txlen = IPC_TX_BUFF_SIZE;
    doipc->txbuf = ymalloc(doipc->txlen);
    if (doipc->txbuf == 0) {
        log_e("malloc txbuff error");
        goto CREATE_FAILED;
    }
    memset(doipc->txbuf, 0, doipc->txlen);
    
    doipc_event_loop_set(doipc, loop);

    return doipc;
CREATE_FAILED:
    ydoipc_destroy(doipc);

    return 0;
}

int ydoipc_destroy(doip_client_t *doipc)
{
    if (doipc == 0) {
        return -1;
    }
    doipc_disconnect_server(doipc);
    if (doipc->rxbuf) {
        yfree(doipc->rxbuf);
        doipc->rxbuf = 0;
        doipc->rxlen = 0;
    }
    if (doipc->txbuf) {
        yfree(doipc->txbuf);
        doipc->txbuf = 0;
        doipc->txlen = 0;
    }

    yfree(doipc);

    return 0;
}
