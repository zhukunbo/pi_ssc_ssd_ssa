/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/*
 * File Name: main.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2013-5-14
 *
 * function: 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <rg_ss/lib/libpub/ss_msg_top_alloc.h>
#include <rg_ss/public/msgdef/switch/ss_msg_switch_mac.h>
#include <rg_ss/lib/libpub/ss_msg_util.h>
#include <rg_ss/lib/libpub/ss_msg_com.h>
#include <rg_ss/public/ss_errno.h>
#include <rg_ss/lib/libssd/dce/ss_dce_cn.h>
#include <rg_ss/public/msgdef/dce/ss_msg_dce_cn.h>
#include <rg_ss/lib/libpub/ss_comm.h>
#include <rg_ss/public/dce/ss_dce_cn_comm.h>
#include <at/at_srv.h>
#include <rg_ss/lib/libpub/ss_public.h>

#include "../include/ssc_comm_pi.h"

//static int phyid[PORT_NUM + 1];
static mac_opra_class_t g_ssc_mac_opera;
static sock_info_t g_sock_info;

/* 向ssd 下发配置信息*/
static void ssc_send_ssd_conf(int node, int vsdid, int msgid, int msg_len, void *msg)
{
	sscmw_mac_msg_t *payload;
    ss_info_t *ss_info;
    int rv;

    ss_info = ss_info_alloc_init(sizeof(sscmw_mac_msg_t) + msg_len);
    if (ss_info == NULL) {
        printf("no mem \n");
        return;
    }
    /* 填充消息结构和参数 */
    ss_info->hdr.msgid = SSC_MSGID_MAC_COMM; 
    ss_info->hdr.msg_ver = 1;
    ss_info->hdr.concurrent = 1;
    ss_info->hdr.sender = MSG_ROLE_SSC;
    ss_info->hdr.reciver = MSG_ROLE_SSD;
    if (node == SSCMW_MAC_DIST_DEF) {
        ss_info->hdr.dst_type = SS_MSG_DST_ALL_DIST_NODE;
    } else {
        ss_info->hdr.dst_type = SS_MSG_DST_SPEC_DIST_NODE;
        ss_info->hdr.dst_dm_node = node;
    }
    payload = (sscmw_mac_msg_t *)ss_info->payload;
    payload->vsdid = vsdid;
    payload->msgid = msgid;
    payload->msg_len = msg_len;
	
	if (msg_len > 0) {
		memcpy(payload->msg, (char *)msg, msg_len);
	} 
    
    /* 发送消息 */
    rv = ss_msg_send(ss_info);
    if (rv != SS_E_NONE) {
        printf("rv: %d", rv);
    }
    /* 释放消息内容 */
    ss_info_free(ss_info);
    
}

/* 更新动态地址*/
void ssc_add_dyn_func(void *msg)
{
	PRINT_DUG("enter ssc_add_static_func \n");
}


/* 删除老化表项*/
void ssc_del_age_time_mac(void *msg)
{
	PRINT_DUG("enter ssc_add_static_func \n");
}

/* 添加静态地址*/
void ssc_add_static_func(void *data)
{
	int phy_id;
	msg_info_t *reve_info;
	base_static_mac_t *static_mac;

	PRINT_DUG("enter ssc_add_static_func \n");

	reve_info   = (msg_info_t *)data;
	static_mac = (base_static_mac_t *)reve_info->msg;
	
	if (ss_comm_ifx_db_get_phyid(0, static_mac->port_id, &phy_id)) {
		printf("ss_comm_ifx_db_get_phyid is failed \n");
		return;
	}
	static_mac->port_id = phy_id;

	ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID, 
		SSC_MSGID_MAC_ADD_ADDR, sizeof(base_static_mac_t), (char *)static_mac);

	/* 更新本地数据库*/
}

/* 清空动态地址表项*/
void ssc_clear_dyn_func(void *data)
{
	PRINT_DUG("enter ssc_clear_dyn_func \n");
	/* 向SSD 传送清空动态地址的消息*/
	ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID, 
		SSC_MSGID_MAC_CLEAR_DYN,0, NULL);
	
}

/* 删除某一vlan下的地址*/
void ssc_clear_vlan_func(void *data)
{
	int vlan_id;
	msg_info_t *reve_info;	

	reve_info   = (msg_info_t *)data;
	vlan_id  = *(int *)reve_info->msg;
		
	PRINT_DUG("enter ssc_clear_vlan_func \n");
	ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID, 
		SSC_MSGID_CLEAR_VLAN_MAC, sizeof(int), &vlan_id);

	/* 更新本地数据库*/
}

/* 删除某一端口下的地址*/
void ssc_clear_inter_func(void *data)
{
	int port_id;
	int phy_id;
	msg_info_t *reve_info;	

	PRINT_DUG("enter ssc_clear_inter_func \n");

	reve_info   = (msg_info_t *)data;
	port_id  = *(int *)reve_info->msg;

	if(ss_comm_ifx_db_get_phyid(0, port_id, &phy_id)){
		printf("ss_comm_ifx_db_get_phyid is failed \n");
		return;
	}
	port_id = phy_id;

	ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID, 
		SSC_MSGID_CLEAR_INTER_DYN, sizeof(int), &port_id);
	/* 更新本地数据库*/	
}

/* 修改老化时间*/
void ssc_modify_age_time(void *data)
{
	int seconds;
	msg_info_t *reve_info;	

	reve_info   = (msg_info_t *)data;
	seconds  = *(int *)reve_info->msg;
	
	PRINT_DUG("enter ssc_modify_age_time \n");
	ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID, 
		SSC_MSGID_MAC_AGETIME,  sizeof(int),  &seconds);
	/* 更新本地数据库*/	
}

/* 修改端口学习状态*/
void ssc_modify_inter_lean_sta(void *data)
{
	char *str;
	msg_info_t *reve_info;	

	reve_info   = (msg_info_t *)data;
	str  = reve_info->msg;
	
	PRINT_DUG("enter ssc_modify_inter_lean_sta \n");
	ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID, 
		SSC_MSGID_MAC_INTER_LEARN, strlen(str), str);
	/* 更新本地数据库*/
}

/*
 *发送数据到PI 接口
 *@msg_type: 消息类型
 *@msg : 消息内容指针
 *@msg_len: 消息的长度
 */
void ssc_send_msg_pi(int msg_type, void *msg, int msg_len)
{
	msg_info_t *tmp;

	PRINT_DUG("enter ssc_send_msg_pi \n");

	tmp = (msg_info_t *)malloc(sizeof(msg_info_t) + msg_len);
	if (tmp == NULL) {
		printf("no mem,send msg is failed \n");
		exit(-1);
	}

	bzero(tmp, (sizeof(msg_info_t) + msg_len));
	
	tmp->msg_type = msg_type;
	tmp->msg_len  = msg_len + sizeof(msg_info_t);

	if (msg_len) {
		memcpy(tmp->msg, (char *)msg, msg_len);
	}

	if (write(g_sock_info.sock_fd, (char *)tmp, tmp->msg_len) < 0) {
		printf("write to ssc is failed \n");
	}	

	free(tmp);
}

/* 接受来自下层ssd的地址信息 */
int ssc_mac_update_recv(ss_rcv_msg_t *rcv_msg, int *ret)
{
	ss_info_t *msg;
	msg_info_t *payload;
	pi_mac_entry_t *data;

	PRINT_DUG("enter ssc_mac_update_recv \n");
	
	msg = (ss_info_t *)rcv_msg->data;
	payload = (msg_info_t *)msg->payload;
    	data = (pi_mac_entry_t *)payload->msg;

	/* 此处要进行端口类型的转换*/
	
	/* 发送数据到PI */
	ssc_send_msg_pi(SSC_MSGID_MAC_ADDR_NOTIFY, (void *)data, sizeof(pi_mac_entry_t));
	
	if (data->flags) {
		ssc_add_dyn_func(payload);
	} else {
		ssc_del_age_time_mac(payload);
	}
	
    return true;
}

/* 接收到开始数据同步信号*/
int ssc_mac_recv_syn_data_begin(ss_rcv_msg_t *rcv_msg, int *ret)
{
	return true;
}

/* 接收到结束数据同步信号*/
int ssc_mac_recv_syn_data_end(ss_rcv_msg_t *rcv_msg, int *ret)
{
	return true;	
}

/* *
 * 接收处理来自PI 的数据
 * @msg_text :接收到的消息内容
 * @len :消息长度
 */
static void ssc_recevie_msg_pi(char *msg_text, int len)
{
	msg_info_t *reve_info;

	reve_info = (msg_info_t *)msg_text;
	PRINT_DUG("ENTER ssc_recevie_msg_pi %d \n", reve_info->msg_type);
	
	g_ssc_mac_opera.ssc_mac_func[reve_info->msg_type](msg_text);
}


static void ssc_send_pi_fetch(void)
{
	ssc_send_msg_pi(SSC_MSGID_MAC_FETCH, NULL, 0);
}

static void *pthread_deal_func(void *arg)
{
	int ret;
	char buff[BUF_SIZE];

    (void)pthread_detach(pthread_self());

RE_CONN:	
	
	g_sock_info.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sock_info.sock_fd < 0) {
    	printf("socket create failed \n");
        return NULL;
    }
	
	bzero(&g_sock_info.server_addr, sizeof(struct sockaddr_in));
    
    g_sock_info.server_addr.sin_family = AF_INET;
    g_sock_info.server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  
    g_sock_info.server_addr.sin_port = htons(SERVER_PORT);
	
    while (1) {
        if (connect(g_sock_info.sock_fd, (struct sockaddr *)&g_sock_info.server_addr, 
                sizeof(struct sockaddr_in)) < 0) {
            printf("ssc connect error \n");
        } else {
        	/* 连接成功，向PI请求配置信息*/
			ssc_send_pi_fetch();
            while (1) {
                ret = read(g_sock_info.sock_fd, buff, BUF_SIZE);
				if (ret <= 0) {
					close(g_sock_info.sock_fd);
					goto RE_CONN;
				}		
				
				PRINT_DUG("read from pi sucess \n");
				ssc_recevie_msg_pi(buff, BUF_SIZE);
            }       
        }
        sleep(2);
    }
}

/* mac处理函数初始化*/
void ssc_mac_opera_init(void)
{	
	g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_ADD_ADDR]=ssc_add_static_func;
	g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_CLEAR_DYN]=ssc_clear_dyn_func;
	g_ssc_mac_opera.ssc_mac_func[PI_MSGID_CLEAR_VLAN_MAC]=ssc_clear_vlan_func;
	g_ssc_mac_opera.ssc_mac_func[PI_MSGID_CLEAR_INTER_DYN]=ssc_clear_inter_func;
	g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_INTER_LEARN]=ssc_modify_inter_lean_sta;
	g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_AGETIME]=ssc_modify_age_time;
				
//	g_ssc_mac_opera.del_age_time_mac = ssc_del_age_time_mac;
//	g_ssc_mac_opera.add_static_func  = ssc_add_static_func;
}

/* ssc与pi组件通信初始化 */
void ssc_comm_init(void)
{
	int ret;
	pthread_t thread_id;

	/* 初始化通信接口*/
	ret = pthread_create(&thread_id, NULL, pthread_deal_func, NULL);
    if (ret < 0) {
        printf("create thread failed \n");
        return ;
    }
}

