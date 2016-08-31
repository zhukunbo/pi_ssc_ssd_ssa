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
	u32 age_time;	 /* 老化时间*/
	port_status_t port_status[PORT_NUM];  	/* 每个端口的状态*/
	u32 dyn_addr_count;	    				/* 有效动态地址个数	*/
	u32 static_addr_count;				/* 有效静态地址个数*/
	struct hlist_head static_mac_tbl_head[HASH_SIZE];	/* 动态mac */
	struct hlist_head dyn_mac_tbl_head[HASH_SIZE];		/* 静态mac */
	ssc_conf_func_t ssc_mac_func[PI_MSGID_DS_END];
	pthread_mutex_t  mutex;			/* 用来保护改数据结构*/
}mac_opra_class_t;

extern void ssc_comm_init(void);
extern void ssc_mac_opera_init(void);

#endif    /* _SSC_COMM_PI_H_ */

