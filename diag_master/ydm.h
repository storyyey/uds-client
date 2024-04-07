#ifndef __DM_H__
#define __DM_H__

/* 将fork一个子进程处理主业务，父进程监控子进程运行状态，设置这个参数后续代码将不会运行 */
#define DM_SELF_MONITOR_ATTR                (0x00000001) 
/* 设置后会等待系统完成时间同步后运行主业务,五分钟未完成同步依然运行主业务 */
#define DM_START_WAIT_SYSTEM_TIME_SYNC_ATTR (0x00000010) 

/* 启动一个线程处理主业务，主线程需要保持进程不退出 */
int ydiag_master_start(int attr);

/* 停止所有业务 */
int ydiag_master_stop();

void ydiag_master_ver_info();

void ydiag_master_debug_enable(int b);

void ydiag_master_log_config_file_set(const char *file);

void ydiag_master_log_save_dir_set(const char *dir);

void ydiag_master_sync_time_wait_set(int s);

#endif /* __DM_H__ */
