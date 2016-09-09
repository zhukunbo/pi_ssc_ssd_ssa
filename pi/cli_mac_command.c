/*
 * Copyright(C) 2013 Ruijie Network. All rights reserved.
 */
/*
 * File Name: cli_mac_command.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2013-5-14
 *
 * function: CLI命令树函数
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mng/cli/cli_transtion.h>
#include <mng/cli/cli_kernel.h>
#include <mng/cli/cli_append_cmd.h>
#include <libpub/rg_thread/rg_thread.h>
#include <rg_dev/dm_lib/rg_dm_spf.h> 
#include <rg_dev/dm_lib/rg_dm.h>

#include "ss_public.h"
#include "cli_mac_command.h"

extern void pi_show_all_mac(void);
extern void pi_show_vlan_mac(int num_vlan);
extern void pi_show_static_mac(void);
extern void pi_show_all_mac(void);
extern void pi_show_mac_agetime(void);
extern void pi_local_learn_sta(char *str, int len);
extern void pi_local_set_agetime(char *data, int len);
extern void pi_local_set_static_addr(char *data, int len);
extern void pi_update_local_db(int type, char *data, int len);
extern void pi_send_msg_ssc(int msg_type, void *msg, int msg_len);
extern void pi_show_mac_count(void);

APPEND_POINT(exec_show_mac_commands, exec_show_rookie);
APPEND_POINT(exec_clear_mac_commands, exec_clear_rookie);
APPEND_POINT(cfg_mac_commands, cfg_rookie);

/* 显示mac表项的内容 */
void exec_show_rookie_mac_table_cmd(struct_command_data_block *pcdb)
{
    pi_show_all_mac();
}

/* 显示指定vlan下的mac表信息 */
void exec_show_rookie_mac_vlan_num_cmd(struct_command_data_block *pcdb)
{
    int vlan_num;

    vlan_num = GETCDBVAR(int, FIRST_ARG);
         
    if (vlan_num < 1 || vlan_num > VLAN_MAX_SIZE) {
        printf("the vlan num is invliad \n");
        return;
    }
    pi_show_vlan_mac(vlan_num);
}

/* 显示mac条目数目 */
void exec_show_rookie_mac_count_cmd(struct_command_data_block *pcdb)
{
    pi_show_mac_count();
}

/* 显示静态路由 */
void exec_show_rookie_mac_static_cmd(struct_command_data_block *pcdb)
{
    pi_show_static_mac();
}

/* 显示设置的老化时间 */
void exec_show_rookie_mac_agetime_cmd(struct_command_data_block *pcdb)
{
    pi_show_mac_agetime();
}

/* 清除动态mac表项 */
void exec_clear_rookie_mac_dynamic_cmd(struct_command_data_block *pcdb)
{   
    pi_send_msg_ssc(PI_MSGID_MAC_CLEAR_DYN, NULL, 0);
    pi_update_local_db(PI_MSGID_MAC_CLEAR_DYN, NULL, 0);
}

/* 清除指定vlan下的表项 */
void exec_clear_rookie_mac_vlan_num_cmd(struct_command_data_block *pcdb)
{
    int vlan_num;

    vlan_num = GETCDBVAR(int, FIRST_ARG);
    if (vlan_num < VLAN_MIN_SIZE || vlan_num > VLAN_MAX_SIZE) {
        printf("the vlan num is invliad \n");
        return;
    }    
        
    pi_send_msg_ssc(PI_MSGID_CLEAR_VLAN_MAC, (void *)&vlan_num, sizeof(int));
    pi_update_local_db(PI_MSGID_CLEAR_VLAN_MAC, (void *)&vlan_num, sizeof(int));
}

/* 清除指定接口下的表项 */
void exec_clear_rookie_mac_interface_num_cmd(struct_command_data_block *pcdb)
{
    int num_inter;
    
    num_inter = GETCDBVAR(int, FIRST_ARG);
    /* 本程序默认使用0 插槽，放高16位*/
    num_inter = num_inter | (0 << 16);
    
    pi_send_msg_ssc(PI_MSGID_CLEAR_INTER_DYN, (void *)&num_inter, sizeof(int));
    pi_update_local_db(PI_MSGID_CLEAR_INTER_DYN, (void *)&num_inter, sizeof(int));
}

/* 是否启动全局mac 地址学习*/
void cfg_rookie_mac_learn_cmd(struct_command_data_block *pcdb)
{
    int status;
    char buff[32];

    memcpy(buff,GETCDBVAR(string, FIRST_ARG),sizeof(buff));
    buff[31] = '\0';

    if (strncmp(buff, "enable", strlen("enable")) == 0) {
        status = MAC_FWD_LEARN;
    } else if (strncmp(buff, "disable", strlen("disable")) == 0){
        status = MAC_UNFWD_UNLEARN;
    } else {
        cli_printf("the args is invliad! \n");
    }
        
    pi_send_msg_ssc(PI_MSGID_MAC_INTER_LEARN, (char *)&status, sizeof(int));
    pi_update_local_db(PI_MSGID_MAC_INTER_LEARN, buff, strlen(buff) + 1);
}

/* 配置老化时间 */
void cfg_rookie_mac_table_agetime_num_cmd(struct_command_data_block *pcdb)
{
    int age_time;

    age_time = GETCDBVAR(int, FIRST_ARG);
    
    pi_send_msg_ssc(PI_MSGID_MAC_AGETIME, (void *)&age_time, sizeof(int));
    pi_update_local_db(PI_MSGID_MAC_AGETIME, (void *)&age_time, sizeof(int));
}

/* 配置静态mac */
void cfg_rookie_mac_table_static__cmd(struct_command_data_block *pcdb)
{
    base_static_mac_t tmp;

    memcpy(tmp.mac,(uchar *)GETCDBVAR(paddr, FIRST_ARG), MAC_LEN);
    tmp.vlan_id = GETCDBVAR(int, SECOND_ARG);
    tmp.port_id = GETCDBVAR(int, 3);  
   
    pi_send_msg_ssc(PI_MSGID_MAC_ADD_ADDR, (void *)&tmp, sizeof(base_static_mac_t));
    pi_update_local_db(PI_MSGID_MAC_ADD_ADDR, (void *)&tmp, sizeof(base_static_mac_t));
}

void cli_mac_manager_init(void)
{
    
    cli_add_command(PARSE_ADD_SHOW_CMD, &TNAME(exec_show_mac_commands),
        "Routing related commands");
    
    cli_add_command(PARSE_ADD_CLEAR_CMD, &TNAME(exec_clear_mac_commands),
        "show mmu information");

    cli_add_command(PARSE_ADD_CFG_TOP_CMD, &TNAME(cfg_mac_commands),
        "show mmu information");
}

