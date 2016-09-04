/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/*
 * File Name: pi_comm_ssc.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2016-8-24
 *
 * function: 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <mng/cli/cli_transtion.h>
#include <mng/cli/cli_append_cmd.h>

#include "ss_public.h"
#include "pi_comm_ssc.h"
#include "hash_tab_oper.h"

#define DEFAULT_NUM		0;

/* PI 本地数据库*/
static pi_db_mac_info_t g_mac_local_db;
static sock_info_t		g_serve_sock;
struct sockaddr_in 	g_client_addr;
static struct pollfd 	g_client[MAX_CLIENT];
static LIST_HEAD(g_msg_head);

pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  msg_cond = PTHREAD_COND_INITIALIZER;
/* 用来标识有无数据*/
static int g_data; 

/* 显示指定vlan 下的mac */
void pi_show_vlan_mac(int num_vlan)
{
	int i, j;
	struct hlist_node *tmp;
	pi_mac_entry_t *assist;  

	pthread_mutex_lock(&g_mac_local_db.mutex);
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.static_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.static_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				if (assist->vlan_id == num_vlan) {
					cli_printf("%d \t",assist->vlan_id);
					for (j = 0; j < MAC_LEN; j += 2) {
						cli_printf("%02x%02x.",assist->mac[j],assist->mac[j + 1]);
					}
					cli_printf("\t\tSTATIC\t GigabitEthernet 0/%d \n", assist->port_id); 	
				}
			}
		}

		if (g_mac_local_db.dyn_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.dyn_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				if (assist->vlan_id == num_vlan) {
					cli_printf("%d \t",assist->vlan_id);
					for (j = 0; j < MAC_LEN; j += 2) {
						cli_printf("%02x%02x.",assist->mac[j],assist->mac[j + 1]);
					}
					cli_printf("\t\tDYNAMIC\t GigabitEthernet 0/%d\n", assist->port_id);
				}
			}
		}
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* 显示mac 地址条目个数*/
void pi_show_mac_count(void)
{
	cli_printf("Dynamic Address Count  : %d\n",g_mac_local_db.dyn_addr_count);
	cli_printf("Static  Address Count  : %d\n",g_mac_local_db.static_addr_count);
	cli_printf("Total Mac Addresses    : %d\n",g_mac_local_db.dyn_addr_count + 
				g_mac_local_db.static_addr_count);
}

/* 显示静态地址条目*/
void pi_show_static_mac(void)
{
	int i, j;
	struct hlist_node *tmp;
	pi_mac_entry_t *assist;    
	
	cli_printf("Vlan\t MAC Address\t\t Type\t Interface \n");

	/* 静态MAC 仅由用户线程操作，无需加锁*/
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.static_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp,&g_mac_local_db.static_mac_tbl_head[i]) {
				assist = hlist_entry(tmp,pi_mac_entry_t,tb_hlist);
				cli_printf("%d \t",assist->vlan_id);
				for (j = 0; j < MAC_LEN; j += 2) {
					cli_printf("%02x%02x.",assist->mac[j],assist->mac[j + 1]);
				}
				cli_printf("\t\tSTATIC\t GigabitEthernet 0/%d \n", assist->port_id); 					
			}
		}
	}
}

/* 显示所有的mac 地址*/
void pi_show_all_mac(void)
{
	int i, j;
	struct hlist_node *tmp;
	pi_mac_entry_t *assist;  
	
	cli_printf("Vlan\t MAC Address\t\t Type\t Interface \n");

	pthread_mutex_lock(&g_mac_local_db.mutex);
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.static_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.static_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				cli_printf("%d \t",assist->vlan_id);
				for (j = 0; j < MAC_LEN; j += 2) {
					cli_printf("%02x%02x.",assist->mac[j],assist->mac[j + 1]);
				}
				cli_printf("\t\tSTATIC\t GigabitEthernet 0/%d \n", assist->port_id);	
			}
		}

		if (g_mac_local_db.dyn_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.dyn_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				cli_printf("%d \t",assist->vlan_id);
				for (j = 0; j < MAC_LEN; j += 2) {
					cli_printf("%02x%02x.",assist->mac[j],assist->mac[j + 1]);
				}
				cli_printf("\t\tDYNAMIC\t GigabitEthernet 0/%d\n", assist->port_id);		
			}
		}
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* 显示老化时间*/
void pi_show_mac_agetime(void)
{
	cli_printf("Aging time	  : %d seconds \n",g_mac_local_db.age_time);
}

/* 删除本地动态mac 地址*/
static int clear_dyn_mac(char *data, pi_mac_entry_t *entry)
{
	 hlist_del(&entry->tb_hlist);				  
	 free(entry); 			   
	 entry = NULL;

	 return 0;
}

/* 清除本地某vlan 下的mac */
static int clear_vlan_mac(char *data, pi_mac_entry_t *entry)
{
	int vlan;
		
	vlan = *((int *)data);
	
	if (entry->vlan_id == vlan) {
		hlist_del(&entry->tb_hlist);				  
		free(entry);			   
		entry = NULL;
		return 0;
	}		
	return -1;
}

/* 删除指定接口下的mac */
static int clear_inter_mac(char *data, pi_mac_entry_t *entry)
{
	int inface;

	inface = *(int*)data;
	if (entry->port_id == inface) {
		hlist_del(&entry->tb_hlist);				  
		free(entry);			   
		entry = NULL;
		return 0;
	}	
	return -1;
}

static void call_clear_local_mac(int type, char *data, clear_mac fun)
{	
	int i;
	struct hlist_node *tmp;
	struct hlist_node *n;
	pi_mac_entry_t *assist;  

	pthread_mutex_lock(&g_mac_local_db.mutex);
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.dyn_mac_tbl_head[i].first != NULL) {
			hlist_for_each_safe(tmp, n, &g_mac_local_db.dyn_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				if ((*fun)(data, assist) == 0) {
					g_mac_local_db.dyn_addr_count--;
				} 
			}
		}
	}
	
	/* 如果仅删除动态地址，则不继续执行*/
	if (type == PI_MSGID_MAC_CLEAR_DYN) {
		pthread_mutex_unlock(&g_mac_local_db.mutex);
		return;
	}
	
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.static_mac_tbl_head[i].first != NULL) {
			hlist_for_each_safe(tmp, n, &g_mac_local_db.static_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				if ((*fun)(data, assist) == 0) {
					g_mac_local_db.static_addr_count--;
				}
			}
		}
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* 删除本地mac */
static void clear_local_mac(int type, char *data, int len)
{
	switch (type) {
	case PI_MSGID_MAC_CLEAR_DYN :
		call_clear_local_mac(type, data, clear_dyn_mac);
		break;
	case PI_MSGID_CLEAR_VLAN_MAC:
		call_clear_local_mac(type, data, clear_vlan_mac);
		break;
	case PI_MSGID_CLEAR_INTER_DYN:
		call_clear_local_mac(type, data, clear_inter_mac);
		break;
	default:
		cli_printf("unknew order! \n");
		break;
	}
}

/* 设置端口全局学习状态*/
static void pi_local_learn_sta(int type, char *str, int len)
{
	int i;
	
	for (i = 0; i < PORT_NUM; i++) {
		if (strncmp(str, "enable", strlen("enable")) == 0) {
			g_mac_local_db.port_status[i] = MAC_FWD_LEARN;
		} else if (strncmp(str, "disable", strlen("disable")) == 0){
			g_mac_local_db.port_status[i] = MAC_UNFWD_UNLEARN;
		} else {
			cli_printf("the args is invliad! \n");
			break;
		}
	}
}

/* 设置老化时间*/
static void pi_local_set_agetime(int type, char *data, int len)
{
	int age_time;

	age_time = *((int *)data);
	g_mac_local_db.age_time = age_time;
}

/* 更新本地静态地址*/
static void pi_local_set_static_addr(int type, char *data, int len)
{
	base_static_mac_t *mac_info;
	pi_mac_entry_t *tmp;

	mac_info = (base_static_mac_t *)data;
	tmp = (pi_mac_entry_t *)malloc(sizeof(pi_mac_entry_t));
	if (tmp == NULL) {
		cli_printf("no mem \n");
		return;
	}
	memset(tmp, 0, sizeof(pi_mac_entry_t));
	
	tmp->port_id = mac_info->port_id;
	tmp->vlan_id = mac_info->vlan_id;
	memcpy(tmp->mac,mac_info->mac,MAC_LEN);

	pthread_mutex_lock(&g_mac_local_db.mutex);
	if (insert_hash_table(tmp, g_mac_local_db.static_mac_tbl_head) == 0) {
		g_mac_local_db.static_addr_count++;
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* 接收处理来自ssc的数据*/
static void pi_recevie_msg_ssc(char *msg_text, int len)
{
	struct msg_queue *entry;

	if (msg_text == NULL) {
		return;
	}
	entry = malloc(sizeof(struct msg_queue));
	if (entry == NULL) {
		printf("no mem \n");
		return;
	}
	entry->data = malloc(len);
	if (entry->data == NULL) {
		free(entry);
		printf("no mem \n");
		return;
	}
	
	memcpy(entry->data, msg_text, len);
	
	pthread_mutex_lock(&msg_mutex);
	insert_msg_to_queue(entry, &g_msg_head);
	g_data = 1;
	pthread_mutex_unlock(&msg_mutex);
	/* 发送信号给主线程处理数据*/
	pthread_cond_signal(&msg_cond);
}

static void* pthread_ds_fun(void *arg)
{
	int i;
	conf_info_t tmp;
	
	(void)pthread_detach(pthread_self());

	tmp.age_time = g_mac_local_db.age_time;
	for (i = 1; i <= PORT_NUM; i++) {
		tmp.port_status[i] = g_mac_local_db.port_status[i];
	}
	/*1、发送数据同步开始信号*/
	pi_send_msg_ssc(PI_MSGID_DS_START, NULL, 0);
	usleep(1000000);
	/*2、仅向SSC 同步基本的配置信息*/
	pi_send_msg_ssc(PI_MSGID_DS_ING, &tmp, sizeof(tmp));
	usleep(1000000);
	/*3、发送数据同步结束信号*/
	pi_send_msg_ssc(PI_MSGID_DS_END, NULL, 0);
    
    return NULL;
}

/* 向SSC下发配置信息*/
static void pi_to_ssc_conf(msg_info_t *rece_msg)
{	
	int ret;
	pthread_t thread_id;

	PRINT_DUG(" pi start to data sync \n");

	/* 创建一个线程向PI 同步数据*/
	ret = pthread_create(&thread_id, NULL, pthread_ds_fun, NULL);
    if (ret < 0) {
        printf("create thread failed \n");
        return ;
    }
}

/* 添加动态地址到本地数据库*/
static void pi_add_local_dyn(pi_mac_entry_t *data)
{
	pi_mac_entry_t *tmp;

	tmp = (pi_mac_entry_t *)malloc(sizeof(pi_mac_entry_t));
	if (tmp == NULL) {
		cli_printf("no mem \n");
		return;
	}
	memset(tmp, 0, sizeof(pi_mac_entry_t));
	memcpy(tmp, data, sizeof(pi_mac_entry_t));

	pthread_mutex_lock(&g_mac_local_db.mutex);
	if (insert_hash_table(tmp, g_mac_local_db.dyn_mac_tbl_head) == 0) {
		g_mac_local_db.dyn_addr_count++;
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* 删除老化地址*/
static void pi_del_local_dyn(pi_mac_entry_t *data)
{
	pthread_mutex_lock(&g_mac_local_db.mutex);
	if (del_hash_entry(data, g_mac_local_db.dyn_mac_tbl_head)  == 0){
		g_mac_local_db.dyn_addr_count--;
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* 处理SSC上传的通告消息*/
static void pi_deal_notify_msg(msg_info_t *rece_msg)
{
    pi_mac_entry_t *data;
    
    PRINT_DUG("***pi to deal with dyn addr**** \n");
    
    data = (pi_mac_entry_t *)rece_msg->msg;
    if (data->flags) {
        /* 更新动态地址库 */
		pi_add_local_dyn(data);        
    } else {
        /* 老化时间到，删除对应的表项 */
		pi_del_local_dyn(data);
    }	
}

static void deal_conf_info(ssc_ds_info_t *data)
{
	int i;
	conf_info_t *tmp;

	tmp = (conf_info_t *)data->context;

	pthread_mutex_lock(&g_mac_local_db.mutex);
	g_mac_local_db.age_time = tmp->age_time;
	for (i = 0; i < PORT_NUM; i++) {
		g_mac_local_db.port_status[i] = tmp->port_status[i];
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

static void deal_static_info(ssc_ds_info_t *data)
{
	pi_mac_entry_t *tmp, *entry;

	tmp = (pi_mac_entry_t *)data->context;
	entry = (pi_mac_entry_t *)malloc(sizeof(pi_mac_entry_t));
	if (entry == NULL) {
		cli_printf("no mem \n");
		return;
	}
	memset(tmp, 0, sizeof(pi_mac_entry_t));
	memcpy(entry, tmp, sizeof(pi_mac_entry_t));

	pthread_mutex_lock(&g_mac_local_db.mutex);
	if (insert_hash_table(entry, g_mac_local_db.static_mac_tbl_head) == 0) {
		g_mac_local_db.static_addr_count++;
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
		
}

static void deal_dyn_info(ssc_ds_info_t *data)
{
	pi_mac_entry_t *entry;

	entry = (pi_mac_entry_t *)data->context;

	pi_add_local_dyn(entry);
}

/* 处理ssc 数据同步开始信号*/
static void pi_deal_ds_start(void)
{
	PRINT_DUG("########pi_deal_ds_start##### \n");
}

/* 处理接收到的数据*/
static void pi_deal_ds_ing(msg_info_t *rece_msg)
{
	ssc_ds_info_t *tmp;

	PRINT_DUG("########pi_deal_ds_ing##### \n");

	tmp = (ssc_ds_info_t *)rece_msg->msg;
	switch (tmp->type) {
	case SSC_CONF:
		deal_conf_info(tmp);
		break;
	case SSC_STATIC:
		deal_static_info(tmp);
		break;
	case SSC_DYN:
		deal_dyn_info(tmp);
		break;
	default:
		printf("no match type \n");
		break;
	}
}

/* 处理ssc 数据同步结束信号*/
static void pi_deal_ds_end(void)
{
	PRINT_DUG("########pi_deal_ds_end##### \n");
}

/*
 * 更新本地数据库
 * @type :消息类型
 * @data :数据 
 * @len: 消息长度
 */
void pi_update_local_db(int type, char *data, int len)
{
	g_mac_local_db.pi_local_func[type](type, data, len) ;
}

/* 主线程处理函数*/
void *deal_info_func(void *arg)
{
	msg_info_t *rece_msg;
	
	while  (1)  {
		pthread_mutex_lock(&msg_mutex);
		while  (g_data  ==  0)  {
			/* 没有数据时一直阻塞*/
			pthread_cond_wait(&msg_cond, &msg_mutex);	
		} 
		/* 获取队列中的数据处理*/
		while  (!list_empty(&g_msg_head)) {
			rece_msg = get_msg_data(&g_msg_head);
			if (rece_msg) {
				switch (rece_msg->msg_type) {
				case SSC_MSGID_MAC_FETCH:
					pi_to_ssc_conf(rece_msg);
					break;
				case SSC_MSGID_MAC_UP_START:
					pi_deal_ds_start();
					break;
				case SSC_MSGID_MAC_UP_ING:
					pi_deal_ds_ing(rece_msg);
					break;	
				case SSC_MSGID_MAC_UP_END:
					pi_deal_ds_end();			
					break;
				case SSC_MSGID_MAC_ADDR_NOTIFY:
					pi_deal_notify_msg(rece_msg);
					break;
				default:
					printf("invalid info !\n");
					break;
				}
				/* 删除处理完的消息*/
				del_dealed_msg(&g_msg_head);
			}else {
				PRINT_DUG("rece_msg is null \n");
			}
		}		
		g_data = 0;
		pthread_mutex_unlock(&msg_mutex);
	}
}

/*
 *发送数据接口
 *@msg_type: 消息类型
 *@msg : 消息内容指针
 *@msg_len: 消息的长度
 */
void pi_send_msg_ssc(int msg_type, void *msg, int msg_len)
{
	int i;
	msg_info_t *tmp;

	pthread_mutex_lock(&send_mutex);
    PRINT_DUG("enter pi_send_msg_ssc \n");
	
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
	
	/* 发送给客户端*/
	for (i = 1; i < MAX_CLIENT; i++) {
		if ((g_client[i].fd < 0) || (g_client[i].fd == g_client[i-1].fd)) {
			continue;
		} 
		if (write(g_client[i].fd , (char *)tmp, tmp->msg_len) < 0) {
			printf("write to ssc is failed \n");
			break;
		}
		printf("pi_send_msg_ssc write sucess %d %d \n", i,g_client[i].fd);
	}
	free(tmp);
	pthread_mutex_unlock(&send_mutex);
}						

static void sig_handler(int id)
{
	int i;
	
	for (i = 0; i < MAX_CLIENT; i++) {
		if (g_client[i].fd < 0) {
			continue;
		}
		close(g_client[i].fd);
	}
	PRINT_DUG("the process was killed \n");
	
	exit(-1);
}

/* 设置默认参数，下发给ssc*/
void pi_default_args_init(void)
{	
	int i;
	
	/* 默认老化时间300s */
	g_mac_local_db.age_time          = 300; 
	g_mac_local_db.dyn_addr_count    = DEFAULT_NUM;
	g_mac_local_db.static_addr_count = DEFAULT_NUM;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_ADD_ADDR]    = pi_local_set_static_addr;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_CLEAR_DYN]   = clear_local_mac;
	g_mac_local_db.pi_local_func[PI_MSGID_CLEAR_VLAN_MAC]  = clear_local_mac;
	g_mac_local_db.pi_local_func[PI_MSGID_CLEAR_INTER_DYN] = clear_local_mac;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_INTER_LEARN] = pi_local_learn_sta;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_AGETIME]     = pi_local_set_agetime;

#if 0
	g_mac_local_db.pi_local_func[PI_MSGID_DS_START] = ;
	g_mac_local_db.pi_local_func[PI_MSGID_DS_END] = ;	
#endif

	/* 注册信号，退出时关闭套接字*/
	signal(SIGKILL, sig_handler);
	signal(SIGTERM, sig_handler);

	/* 默认开启所有的端口学习能力*/
	for (i = 1; i <= PORT_NUM; i++) {
		g_mac_local_db.port_status[i] = MAC_FWD_LEARN;
	}

	/* 初始化动态和静态mac hash 表*/
	for (i = 0; i < HASH_SIZE; i++) {
		INIT_HLIST_HEAD(&g_mac_local_db.static_mac_tbl_head[i]);
		INIT_HLIST_HEAD(&g_mac_local_db.dyn_mac_tbl_head[i]);
	}

	/* 初始化互斥锁*/
	pthread_mutex_init(&g_mac_local_db.mutex,NULL);
	
}

/* pi与ssc通信相关初始化 */
void *cli_comm_ssc_init(void *arg)
{
	int i, maxi, len, n_ready, tmp, n;
	
	char buff[BUF_SIZE];

	(void)pthread_detach(pthread_self());
 
    g_serve_sock.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (g_serve_sock.sock_fd < 0) {
    	printf("socket create failed \n");
        return NULL;
    }

	bzero(buff, BUF_SIZE);
    bzero(&g_serve_sock.server_addr, sizeof(struct sockaddr_in));
    
    g_serve_sock.server_addr.sin_family = AF_INET;
    g_serve_sock.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    g_serve_sock.server_addr.sin_port = htons(SERVER_PORT); 
    
    /* 绑定本地IP 和 端口号 */
	bind(g_serve_sock.sock_fd, (struct sockaddr *)&g_serve_sock.server_addr,
		sizeof(struct sockaddr_in));
        
	listen(g_serve_sock.sock_fd, MAX_CLIENT);
	 
	 /* 将服务器ID加入监听 */
	 g_client[0].fd = g_serve_sock.sock_fd;
	 g_client[0].events = POLLIN;
	 
	 maxi = 0;
	 for (i = 1; i < MAX_CLIENT; i++) {
		 g_client[i].fd = -1;
	 }
	 
	 while (1) {
		 /* 如果没有套接字发生变化，就一直阻塞 */
		 n_ready = poll(g_client, maxi+1 , INFTIM);
		 if (n_ready < 0 ) {
			 printf("poll is failed \n");
		 }

		 len = sizeof(struct sockaddr_in);
		 /* 监听套接字发生变化 */
		 if (g_client[0].revents & POLLIN ) {
			 tmp = accept(g_serve_sock.sock_fd, (struct sockaddr *)&g_client_addr,
                        (socklen_t*)&len);
			 if (tmp < 0) {
				 printf("accept is failed \n");
				 continue;
			 }
			 cli_printf("connect sucess %d \n", tmp);
		 }
		 
		 for (i = 1; i < MAX_CLIENT; i++) {
			 if (g_client[i].fd < 0) {
				 g_client[i].fd = tmp;
				 break;
			 }
		 }
		 /* 将新增的客户端套接字加入监听 */
		 g_client[i].events = POLLIN;
		 if (i > maxi) {
			 maxi = i;
		 }
		 /* 如果没有其他id发生变化，继续监听 */
		 if (--n_ready <= 0) {
			 continue;	  
		 }
		 
		 for (i = 1; i <= maxi; i++) {
			 if (g_client[i].fd < 0) {
				continue;
			 }
			 
			 if (g_client[i].revents & POLLIN) {
				 /* 客户端发送消息过来 */
				 n = read(g_client[i].fd, buff, BUF_SIZE);
				 if (n <= 0) {
					 /* 当做客户端已经退出 */
					 printf("pi read error ret = %d \n", n);
					 close(g_client[i].fd);
					 g_client[i].fd = -1;
					 break;
				 }
				  /* 对读到的数据进行处理*/
				 pi_recevie_msg_ssc(buff,BUF_SIZE);
				 if (--n_ready <= 0) {
					 break;
				 }					 
			 }
		 }
	 }
}

