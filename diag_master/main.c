#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "ydm.h"


void g()
{
    /*
                       _ooOoo_
                      o8888888o
                      88" . "88
                      (| -_- |)
                      O\  =  /O
                   ____/`---'\____
                 .'  \\|     |//  `.
                /  \\|||  :  |||//  \
               /  _||||| -:- |||||-  \
               |   | \\\  -  /// |   |
               | \_|  ''\---/''  |   |
               \  .-\__  `-`  ___/-. /
             ___`. .'  /--.--\  `. . __
          ."" '<  `.___\_<|>_/___.'  >'"".
         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
         \  \ `-.   \_ __\ /__ _/   .-` /  /
    ======`-.____`-.___\_____/___.-`____.-'======
                       `=---='
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
             佛祖保佑       永无BUG
    */
    char *image = \
    "                   _ooOoo_                   \n"
    "                  o8888888o                  \n"
    "                  88\" . \"88                  \n"
    "                  (| -_- |)                  \n"
    "                  O\\  =  /O                  \n"
    "               ____/`---'\\____               \n"
    "             .'  \\\\|     |//  `.             \n"
    "            /  \\\\|||  :  |||//  \\            \n"
    "           /  _||||| -:- |||||-  \\           \n"
    "           |   | \\\\\\  -  /// |   |           \n"
    "           | \\_|  ''\\---/''  |   |           \n"
    "           \\  .-\\__  `-`  ___/-. /           \n"
    "         ___`. .'  /--.--\\  `. . __          \n"
    "      ."" '<  `.___\\_<|>_/___.'  >'"".       \n"
    "     | | :  `- \\`.;`\\ _ /`;.`/ - ` : | |     \n"
    "     \\  \\ `-.   \\_ __\\ /__ _/   .-` /  /     \n"
    "======`-.____`-.___\\_____/___.-`____.-'======\n"
    "                   `=---='                   \n"
    "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n"
    "           Buddha bless, never BUG             \n";    

    printf(image);
}

void help_tip()
{
    printf("Info options: \n");
    printf("  -v, --version                   print process version information\n");
    printf("  -h, --help                      output option help information\n");    
    printf("Run options: \n");
    printf("  -d, --debug                     debug logs are generated when the process is running \n");
    printf("  -m, --process-self-montior      the process enables self-monitoring \n");
    printf("  -s, --wait-system-time-sync     wait for the real-time system time synchronization before the process starts \n");
    printf("  -C, --log-configure-file=FILE   log configuration file \n");
    printf("  -D, --log-save-dir=DIR          Directory for saving log files \n");    
    printf("  -g, --                          Buddha bless, never BUG \n");
}

int main(int argc, char *argv[])
{
    int apt = 0;
    int attr = 0; 
    int sync_time = 0;
    
    while ((apt = getopt(argc, argv, "dvhms::C:D:g")) != -1) {
        switch (apt) {
            case 'd':
                /* 启动调试信息输出 */
                ydiag_master_debug_enable(1);
                break;
            case 'v':
                /* 输出版本信息 */
                ydiag_master_ver_info();
                return 0;
            case 's':
                attr |= DM_START_WAIT_SYSTEM_TIME_SYNC_ATTR;
                sync_time = optarg ? atoi(optarg) : 0;
                ydiag_master_sync_time_wait_set(sync_time);
                break;
            case 'm':
                attr |= DM_SELF_MONITOR_ATTR;
                break;
            case 'C':
                ydiag_master_log_config_file_set(optarg);
                break;
            case 'D':
                ydiag_master_log_save_dir_set(optarg);
                break;
            case 'h':
                /* 帮助信息 */
                help_tip();
                return 0;
            case 'g':
                g();
                return 0;
            default:
            
                break;
        }
    }

    if (ydiag_master_start(attr) == 0) {
        /* 防止进程退出 */
        while (1) { sleep(1000); }
    }
    /* 任务启动失败 */
    printf("The diag master failed to start \n");

    return 0;
}
