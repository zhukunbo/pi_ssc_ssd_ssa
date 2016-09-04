#ifndef _SSC_COMM_PI_H_
#define _SSC_COMM_PI_H_

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT  9527
#define BUF_SIZE		  1024

typedef struct _socket_info{    
	int sock_fd;    
	struct sockaddr_in server_addr;
}sock_info_t;

typedef void (*ssc_conf_func_t)( void *data);

typedef struct ssc_mac_opra_class{
	u32 age_time;	 /* �ϻ�ʱ��*/
	port_status_t port_status[PORT_NUM + 1];  	/* ÿ���˿ڵ�״̬*/
	u32 dyn_addr_count;	    				/* ��Ч��̬��ַ����	*/
	u32 static_addr_count;				/* ��Ч��̬��ַ����*/
	struct hlist_head static_mac_tbl_head[HASH_SIZE];	/* ��̬mac */
	struct hlist_head dyn_mac_tbl_head[HASH_SIZE];		/* ��̬mac */
	ssc_conf_func_t ssc_mac_func[PI_MSGID_MAC_ID_NUM];
	pthread_mutex_t mutex;			/* �������������ݽṹ*/
}mac_opra_class_t;

extern void ssc_comm_init(void);
extern void ssc_mac_opera_init(void);
extern bool ssc_mac_update_recv(ss_rcv_msg_t *rcv_msg, int *ret);
extern bool ssc_mac_recv_syn_data_begin(ss_rcv_msg_t *rcv_msg, int *ret);
extern bool ssc_mac_recv_syn_data_end(ss_rcv_msg_t *rcv_msg, int *ret);

#endif    /* _SSC_COMM_PI_H_ */

