#ifndef __YREMOTE_COMMAND_H__
#define __YREMOTE_COMMAND_H__

struct terminal_control_service_s;
typedef struct terminal_control_service_s terminal_control_service_t;

#define TRC_CHECK_VERSION_T (0x9991) /* 版本检查 */

/* 平台远程指令处理回调函数 */
typedef void (*premote_cmd_handler)(void *argv, yuint16 msgid, void *data);

int yterminal_control_service_start();
terminal_control_service_t *yterminal_control_service_create();
int yterminal_control_service_destroy(terminal_control_service_t *tcs);
int yterminal_control_service_connect(terminal_control_service_t *tcs);
int yterminal_control_service_handler_set(terminal_control_service_t *tcs, yuint16 msgid, premote_cmd_handler call, void *argv);
int yterminal_control_service_info_set(terminal_control_service_t *tcs, terminal_control_service_info_t *tcs_config);
int yterminal_control_service_info_get(terminal_control_service_t *tcs, terminal_control_service_info_t *tcs_config);
boolean yterminal_control_service_info_equal(terminal_control_service_t *tcs, terminal_control_service_info_t *tcs_config);
#endif /* __YREMOTE_COMMAND_H__ */
