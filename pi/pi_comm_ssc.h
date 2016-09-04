#ifndef _PI_COMM_SSC_H_
#define _PI_COMM_SSC_H_

#include <rg_sys/list.h>

#define SERVER_IP      	 "127.0.0.1"	
#define SERVER_PORT      9527
#define BUF_SIZE		 1024
#define MAX_CLIENT		 5
#define INFTIM			 -1

/* 配置消息处理函数 */
typedef void (*pi_mac_conf_func_t)(int type, char *data, int len);

typedef struct pi_db_mac_info {
	u32 age_time;	 			/* 老化时间*/
	u32 dyn_addr_count;	    	/* 有效动态地址个数*/
	u32 static_addr_count;		/* 有效静态地址个数*/
	port_status_t port_status[PORT_NUM + 1];  				/* 每个端口的状态*/
	struct hlist_head static_mac_tbl_head[HASH_SIZE];	/* 动态mac表头*/
	struct hlist_head dyn_mac_tbl_head[HASH_SIZE];		/* 静态mac表头*/
	pi_mac_conf_func_t pi_local_func[PI_MSGID_MAC_ID_NUM];
	pthread_mutex_t mutex;			/* 用来保护改数据结构*/
} pi_db_mac_info_t;

/* 套接字相关结构体*/
typedef struct _socket_info{
    int sock_fd;
    struct sockaddr_in server_addr;
}sock_info_t;

/* 消息队列*/
struct msg_queue{
	struct list_head list;
	char *data;
};

typedef int (* clear_mac)(char * data, pi_mac_entry_t *entry);

extern void cli_mac_manager_init(void);
extern void pi_default_args_init(void);
extern void *deal_info_func(void *arg);
extern void *cli_comm_ssc_init(void *arg);
extern void pi_send_msg_ssc(int msg_type, void *msg, int msg_len);


#endif  /* _PI_COMM_SSC_H_ */

