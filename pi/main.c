/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/*
 * File Name: main.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2016-09-04
 *
 * function:  主函数入口
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <mng/cli/cli_transtion.h>
#include <mng/cli/cli_append_cmd.h>

#include "ss_public.h"
#include "pi_comm_ssc.h"

static struct rg_thread_master *g_cli_master;
static int pi_exit_flags = 0;

int main(int argc, char *argv[])
{
    int ret;
    pthread_t thread_id;
    pthread_t main_thread_id;
    struct rg_thread thread;
    
    signal(SIGPIPE, SIG_IGN);
    /* 创建一个伪线程 */
    g_cli_master = rg_thread_master_create();
    if (g_cli_master == NULL) {
        cli_printf("rg_thread_master_create to fail \n");
        return ERR;
    }
    (void)cli_client_init("cli_mac", g_cli_master);
    
    /* 初始化命令挂节点 */
    cli_mac_manager_init();
    /* 默认参数初始化*/
    pi_default_args_init();
    /* 主线程处理函数*/
    ret = pthread_create(&main_thread_id, NULL, deal_info_func, NULL);
    if (ret < 0) {
        printf("create thread failed \n");
        return ret;
    }

    /* 初始化通信接口*/
    ret = pthread_create(&thread_id, NULL, cli_comm_ssc_init, NULL);
    if (ret < 0) {
        printf("create thread failed \n");
        return ret;
    }

    ret = cli_client_ready();
    if (ret != 0) {
        cli_printf("cli_client_ready error");
        return ret;
    }
    
    while (rg_thread_fetch(g_cli_master, &thread)) {
        if (pi_exit_flags) {
            break;
        }
        rg_thread_call(&thread);
    }
    
    return 0;
}

