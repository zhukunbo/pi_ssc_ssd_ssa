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
#include <mng/cli/cli_transtion.h>
#include <mng/cli/cli_append_cmd.h>

#include "../include/pi_comm_ssc.h"
#include "../include/hash_tab_oper.h"

/* PI �������ݿ�*/
static pi_db_mac_info_t g_mac_local_db;
static sock_info_t		g_serve_sock;
struct sockaddr_in 		g_client_addr;
static struct pollfd 	g_client[MAX_CLIENT];
static LIST_HEAD(g_msg_head);
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  msg_cond = PTHREAD_COND_INITIALIZER;
/* ������ʶ��������*/
static int g_data; 

/* ��ʾָ��vlan �µ�mac */
void pi_show_vlan_mac(int num_vlan)
{
	int i;
	struct hlist_node *tmp;
	pi_mac_entry_t *assist;  

	pthread_mutex_lock(&g_mac_local_db.mutex);
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.static_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.static_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				if (assist->vlan_id == num_vlan) {
					cli_printf("%d \t %s\t\t STATIC\t %d \n",assist->vlan_id,
					assist->mac, assist->port_id);	
				}
			}
		}

		if (g_mac_local_db.dyn_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.dyn_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				if (assist->vlan_id == num_vlan) {
					cli_printf("%d \t %s\t\t STATIC\t %d \n",assist->vlan_id,
					assist->mac, assist->port_id);	
				}
			}
		}
	}
	pthread_mutex_lock(&g_mac_local_db.mutex);
}

/* ��ʾmac ��ַ��Ŀ����*/
void pi_show_mac_count(void)
{
	cli_printf("Dynamic Address Count  : %d\n",g_mac_local_db.dyn_addr_count);
	cli_printf("Static  Address Count  : %d\n",g_mac_local_db.static_addr_count);
	cli_printf("Total Mac Addresses    : %d\n",g_mac_local_db.dyn_addr_count + 
		g_mac_local_db.static_addr_count);
}

/* ��ʾ��̬��ַ��Ŀ*/
void pi_show_static_mac(void)
{
	int i;
	struct hlist_node *tmp;
	pi_mac_entry_t *assist;    
	
	cli_printf("Vlan\t MAC Address\t\t Type\t Interface \n");

	/* ��̬MAC �����û��̲߳������������*/
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.static_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp,&g_mac_local_db.static_mac_tbl_head[i]) {
				assist = hlist_entry(tmp,pi_mac_entry_t,tb_hlist);
				cli_printf("%d \t %s\t\t STATIC\t %d \n",assist->vlan_id,
					assist->mac, assist->port_id);			
			}
		}
	}
}

/* ��ʾ���е�mac ��ַ*/
void pi_show_all_mac(void)
{
	int i;
	struct hlist_node *tmp;
	pi_mac_entry_t *assist;  
	
	cli_printf("Vlan\t MAC Address\t\t Type\t Interface \n");

	pthread_mutex_lock(&g_mac_local_db.mutex);
	for (i = 0; i < HASH_SIZE; i++) {
		if (g_mac_local_db.static_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.static_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				cli_printf("%d \t %s\t\t STATIC\t %d \n",assist->vlan_id,
							assist->mac, assist->port_id);			
			}
		}

		if (g_mac_local_db.dyn_mac_tbl_head[i].first != NULL) {
			hlist_for_each(tmp, &g_mac_local_db.dyn_mac_tbl_head[i]) {
				assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
				cli_printf("%d \t %s\t\t DYNAMIC\t %d \n",assist->vlan_id,
							assist->mac, assist->port_id);			
			}
		}
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* ��ʾ�ϻ�ʱ��*/
void pi_show_mac_agetime(void)
{
	cli_printf("Aging time	  : %d seconds \n",g_mac_local_db.age_time);
}

/* ɾ�����ض�̬mac ��ַ*/
static int clear_dyn_mac(char *data, pi_mac_entry_t *entry)
{
	 hlist_del(&entry->tb_hlist);				  
	 free(entry); 			   
	 entry = NULL;

	 return 0;
}

/* �������ĳvlan �µ�mac */
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

/* ɾ��ָ���ӿ��µ�mac */
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
	
	/* �����ɾ����̬��ַ���򲻼���ִ��*/
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

/* ɾ������mac */
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

/* ���ö˿�ȫ��ѧϰ״̬*/
void pi_local_learn_sta(int type, char *str, int len)
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

/* �����ϻ�ʱ��*/
void pi_local_set_agetime(int type, char *data, int len)
{
	int age_time;

	age_time = *(int *)data;

	g_mac_local_db.age_time = age_time;
}

/* ���±��ؾ�̬��ַ*/
void pi_local_set_static_addr(int type, char *data, int len)
{
	base_static_mac_t *mac_info;
	pi_mac_entry_t *tmp;

	mac_info = (base_static_mac_t *)data;
	tmp = (pi_mac_entry_t *)malloc(sizeof(pi_mac_entry_t));
	if (tmp == NULL) {
		cli_printf("no mem \n");
		return;
	}
	tmp->port_id = mac_info->port_id;
	tmp->vlan_id = mac_info->vlan_id;
	memcpy(tmp->mac,mac_info->mac,MAC_LEN);

	pthread_mutex_lock(&g_mac_local_db.mutex);
	if (insert_hash_table(tmp,g_mac_local_db.static_mac_tbl_head) == 0) {
		g_mac_local_db.static_addr_count++;
	}
	pthread_mutex_unlock(&g_mac_local_db.mutex);
}

/* ���±������ݿ�*/
void pi_update_local_db(int type, char *data, int len)
{
	g_mac_local_db.pi_local_func[type](type, data, len) ;
}

/* ���մ�������ssc������*/
static void pi_recevie_msg_ssc(char *msg_text, int len)
{
	struct msg_queue *entry;

	PRINT_DUG("enter pi_recevie_msg_ssc \n");

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
	
	/* �����յ�����Ϣ���������ض���*/

	memcpy(entry->data, msg_text, len);
	
	PRINT_DUG("pi_recevie_msg_ssc recevie data \n");
	
	pthread_mutex_lock(&msg_mutex);
	insert_msg_to_queue(entry, &g_msg_head);
	g_data = 1;
	pthread_mutex_unlock(&msg_mutex);
	/* �����źŸ����̴߳�������*/
	pthread_cond_signal(&msg_cond);

}

/* ��SSC�·�������Ϣ*/
void pi_to_ssc_conf(msg_info_t *rece_msg)
{
	PRINT_DUG("pi_to_ssc_conf \n");
	/*1����������ͬ����ʼ�ź�*/
	/*2����������*/
	/*3����������ͬ�������ź�*/
}

/* ����SSC�ϴ���ͨ����Ϣ*/
void pi_deal_notify_msg(msg_info_t *rece_msg)
{
	PRINT_DUG("pi_deal_notify_msg \n");
}

/* ���̴߳�������*/
void *deal_info_func(void *arg)
{
	msg_info_t *rece_msg;
	
	while  (1)  {
		pthread_mutex_lock(&msg_mutex);
		while  (g_data  ==  0)  {
			/* û������ʱһֱ����*/
			pthread_cond_wait(&msg_cond,&msg_mutex);	
		} 
		/* ��ȡ�����е����ݴ���*/
		while  (!list_empty(&g_msg_head)) {
			rece_msg = get_msg_data(&g_msg_head);
			if (rece_msg) {
				PRINT_DUG("rece_msg->msg_type = %d \n",rece_msg->msg_type);
				switch (rece_msg->msg_type) {
				case SSC_MSGID_MAC_FETCH:
					pi_to_ssc_conf(rece_msg);
					break;
				case SSC_MSGID_MAC_UP_START:
					/*"����"*/
					break;
				case SSC_MSGID_MAC_UP_END:
					/*"����"*/				
					break;
				case SSC_MSGID_MAC_ADDR_NOTIFY:
					pi_deal_notify_msg(rece_msg);
					break;
				default:
					printf("invalid info !\n");
					break;
				}
				/* ɾ�����������Ϣ*/
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
 *�������ݽӿ�
 *@msg_type: ��Ϣ����
 *@msg : ��Ϣ����ָ��
 *@msg_len: ��Ϣ�ĳ���
 */
void pi_send_msg_ssc(int msg_type, void *msg, int msg_len)
{
	int i;
	msg_info_t *tmp;

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
	
	/* ���͸��ͻ���*/
	for (i = 1; i < MAX_CLIENT; i++) {
		if (g_client[i].fd < 0) {
			continue;
		} 
		if (write(g_client[i].fd , (char *)tmp, tmp->msg_len) < 0) {
			printf("write to ssc is failed \n");
			break;
		}
		PRINT_DUG("pi_send_msg_ssc write sucess \n");
	}
	
    PRINT_DUG("out pi_send_msg_ssc \n");
    
	free(tmp);
}						

/* ����Ĭ�ϲ������·���ssc*/
void pi_default_args_init(void)
{	
	int i;
	
	/* Ĭ���ϻ�ʱ��300s */
	g_mac_local_db.age_time = 300; 
	g_mac_local_db.dyn_addr_count = 0;
	g_mac_local_db.static_addr_count = 0;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_ADD_ADDR] = pi_local_set_static_addr;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_CLEAR_DYN] = clear_local_mac;
	g_mac_local_db.pi_local_func[PI_MSGID_CLEAR_VLAN_MAC] = clear_local_mac;
	g_mac_local_db.pi_local_func[PI_MSGID_CLEAR_INTER_DYN] = clear_local_mac;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_INTER_LEARN] = pi_local_learn_sta;
	g_mac_local_db.pi_local_func[PI_MSGID_MAC_AGETIME] = pi_local_set_agetime;

#if 0
	g_mac_local_db.pi_local_func[PI_MSGID_DS_START] = ;
	g_mac_local_db.pi_local_func[PI_MSGID_DS_END] = ;	
#endif

	/* ע���źţ��˳�ʱ�ر��׽���*/

	/* Ĭ�Ͽ������еĶ˿�ѧϰ����*/
	for (i = 1; i <= PORT_NUM; i++) {
		g_mac_local_db.port_status[i] = MAC_FWD_LEARN;
	}

	/* ��ʼ����̬�;�̬mac hash ��*/
	for (i = 0; i < HASH_SIZE; i++) {
		INIT_HLIST_HEAD(&g_mac_local_db.static_mac_tbl_head[i]);
		INIT_HLIST_HEAD(&g_mac_local_db.dyn_mac_tbl_head[i]);
	}

	/* ��ʼ��������*/
	pthread_mutex_init(&g_mac_local_db.mutex,NULL);
	
}

/* pi��sscͨ����س�ʼ�� */
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
    
    /* �󶨱���IP �� �˿ں� */
	bind(g_serve_sock.sock_fd,(struct sockaddr *)&g_serve_sock.server_addr,
        sizeof(struct sockaddr_in));
        
	listen(g_serve_sock.sock_fd,20);
	 
	 /* ��������ID������� */
	 g_client[0].fd = g_serve_sock.sock_fd;
	 g_client[0].events = POLLIN;
	 
	 maxi = 0;
	 for (i = 1; i < MAX_CLIENT; i++) {
		 g_client[i].fd = -1;
	 }
	 
	 while (1) {
		 /* ���û���׽��ַ����仯����һֱ���� */
		 n_ready = poll(g_client, maxi+1 ,INFTIM);
		 if (n_ready < 0 ) {
			 printf("poll is failed \n");
		 }

		 len = sizeof(struct sockaddr_in);
		 /* �����׽��ַ����仯 */
		 if (g_client[0].revents & POLLIN ) {
			 tmp = accept(g_serve_sock.sock_fd, (struct sockaddr *)&g_client_addr, &len);
			 if (tmp < 0) {
				 printf("accept is failed \n");
				 continue;
			 }
			 PRINT_DUG("connect sucess \n");
		 }
		 
		 for (i = 1; i < MAX_CLIENT; i++) {
			 if (g_client[i].fd < 0) {
				 g_client[i].fd = tmp;
				 break;
			 }
		 }
		 /* �������Ŀͻ����׽��ּ������ */
		 g_client[i].events = POLLIN;
		 if (i > maxi) {
			 maxi = i;
		 }
		 /* ���û������id�����仯���������� */
		 if (--n_ready <= 0) {
			 continue;	  
		 }
		 
		 for (i = 1; i <= maxi; i++) {
			 if (g_client[i].fd < 0)
				 continue;
			 
			 if (g_client[i].revents & POLLIN) {
				 /* �ͻ��˷�����Ϣ���� */
				 n = read(g_client[i].fd, buff, BUF_SIZE);
				 if (n <= 0) {
					 /* �����ͻ����Ѿ��˳� */
					 printf("read error \n");
					 close(g_client[i].fd);
					 g_client[i].fd = -1;
				 }
				  /* �Զ��������ݽ��д���*/
				 pi_recevie_msg_ssc(buff,BUF_SIZE);
				 if (--n_ready <= 0) {
					 break;
				 }					 
			 }
		 }
	 }
}
