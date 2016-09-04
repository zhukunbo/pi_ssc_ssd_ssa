#ifndef _SS_PUBLIC_H_
#define _SS_PUBLIC_H_

#include <rg_sys/list.h>

#define SSCMW_MAC_DIST_DEF    0x0FFFFFFF

#define DEFAULT_VSD_ID		     0		/* 默认VSDID 设置为0*/
#define PORT_NUM			     52		/* 最大的端口数*/
#define MAC_LEN				     6		/* MAC 地址长度*/
#define VLAN_MAX_SIZE		     4094	/* 最大vlan 数*/ 
#define HASH_SIZE			     1024	/* hash 表大小*/ 
#define ERROR  				     -1

#define SS_TOP_MOD_OFFSET       (16)  
#define SS_SUB_MOD_OFFSET       (8) 
#define MAC_FLAG_DYN        	(0<<6)

#define SSC_CONF		1	/* 同步时标识配置信息*/
#define SSC_STATIC  	2	/* 同步时标识静态信息*/
#define SSC_DYN			3	/* 同步时标识动态信息*/

#define	SS_MSGID_TOP_MOD_UNDEF      	(0x01 << SS_TOP_MOD_OFFSET)	/* 标识UNDEF 子系统*/
#define	SS_MSGID_UNDEF_MAC	 			(SS_MSGID_TOP_MOD_UNDEF | (0x01 << SS_SUB_MOD_OFFSET))
/* 消息分类，第4字节的高两位表示 */
#define	SSC_ROOKIE_MSG_MAC				(SS_MSGID_UNDEF_MAC | 0x00)		
#define	SSD_ROOKIE_MSG_MAC				(SS_MSGID_UNDEF_MAC | 0x40)		
#define	SSA_ROOKIE_MSG_MAC      		(SS_MSGID_UNDEF_MAC | 0x80)  	

/* SSD发往SSC */
#define	SSD_MSGID_MAC_UPDATE			(SSD_ROOKIE_MSG_MAC | 0X21)
#define	SSD_MSGID_MAC_DATA_SYNC_UP_BEGIN	(SSD_ROOKIE_MSG_MAC | 0X22)
#define	SSD_MSGID_MAC_DATA_SYNC_UP_END	(SSD_ROOKIE_MSG_MAC | 0X23)
/* SSC发往SSD的消息 */
#define	SSC_MSGID_MAC_COMM				(SSC_ROOKIE_MSG_MAC | 0x01)
#define	SSC_MSGID_MAC_DATA_SYN_BEGIN	(SSC_ROOKIE_MSG_MAC | 0x02)            
#define	SSC_MSGID_MAC_DATA_SYNC_END     (SSC_ROOKIE_MSG_MAC | 0x03)

/* SSA发往SSD的消息 */
#define SSA_MSGID_MAC_UPDATE            (SSA_ROOKIE_MSG_MAC | 0x21)
#define SSA_MSGID_MAC_DATA_SYNC_BEGIN   (SSA_ROOKIE_MSG_MAC | 0x22)
#define SSA_MSGID_MAC_DATA_SYNC_END     (SSA_ROOKIE_MSG_MAC | 0x23)
/* SSD发往SSA的消息*/
#define SSD_MSGID_MAC_COMM              (SSD_ROOKIE_MSG_MAC | 0x01)

/* 调试信息宏开关*/
#define PRINT_DUG	printf
/* #define PRINT_DUG	printf(...)  */
typedef unsigned int u32;
typedef unsigned char u8;

/* PI消息类型*/
typedef enum {
	PI_MSGID_MAC_ADD_ADDR,	
    PI_MSGID_MAC_CLEAR_DYN,
	PI_MSGID_CLEAR_VLAN_MAC,
	PI_MSGID_CLEAR_INTER_DYN,
    PI_MSGID_MAC_INTER_LEARN,
    PI_MSGID_MAC_AGETIME,
	PI_MSGID_DS_START,
	PI_MSGID_DS_ING,
	PI_MSGID_DS_END,
	PI_MSGID_MAC_ID_NUM,
}pi_mac_msgid_t;

/* ssc 消息类型*/
typedef enum {
	SSC_MSGID_MAC_ADD_ADDR,	
    SSC_MSGID_MAC_CLEAR_DYN,
	SSC_MSGID_CLEAR_VLAN_MAC,
	SSC_MSGID_CLEAR_INTER_DYN,
    SSC_MSGID_MAC_INTER_LEARN,
    SSC_MSGID_MAC_AGETIME,
	SSC_MSGID_MAC_UP_START,
	SSC_MSGID_MAC_UP_ING,
	SSC_MSGID_MAC_UP_END,
	SSC_MSGID_MAC_ADDR_NOTIFY,
	SSC_MSGID_MAC_FETCH,	/* 向PI请求数据的信号*/
    SSC_MSGID_MAC_ID_NUM,
} ssc_mac_msgid_t;

/*ssd 消息类型*/
typedef enum {
    SSD_MSGID_MAC_ADD_ADDR,
	SSD_MSGID_MAC_CLEAR_DYN,
	SSD_MSGID_CLEAR_VLAN_MAC,
	SSD_MSGID_CLEAR_INTER_DYN,
	SSD_MSGID_MAC_INTER_LEARN,
	SSD_MSGID_MAC_AGETIME,
    SSD_MSGID_MAC_ID_NUM,
} ssd_mac_msgid_t;

typedef struct msg_info {
	int msg_type;
	int msg_len;
	char msg[0];
}msg_info_t;

/*mac和vlan作为关键字*/	
typedef struct pi_mac_entry {
	u8 mac[MAC_LEN];	
	u8	flags;			/* 添加或者老化标志*/
	u32 vlan_id;		
	u32 port_id;	
	u32 addr_type;		/* 地址类型*/
	struct hlist_node tb_hlist;	
} pi_mac_entry_t;

/*端口的状态*/
typedef enum {
    MAC_FWD_LEARN = 3,       /* 转发学习 */
    MAC_FWD_UNLEARN = 2,     /* 转发不学习 */
    MAC_UNFWD_LEARN = 1,     /* 不转发但学习 */
    MAC_UNFWD_UNLEARN = 0,   /* 不转发不学习 */
} port_status_t;

typedef struct base_static_mac {
	u8 mac[MAC_LEN];	//mac地址	
	u32 vlan_id;		//vlan号
	u32 port_id;		//端口号
}base_static_mac_t;

typedef struct conf_info {
	u32 age_time;	/* 老化时间*/
	port_status_t port_status[PORT_NUM + 1];  /* 每个端口的状态*/
}conf_info_t;

/* SSC 向PI 同步所使用的结构体*/
typedef struct ssc_ds_info {
	int type;
	char context[0];
}ssc_ds_info_t;

typedef struct inter_learn_sta {
	int port_id[PORT_NUM + 1];
	port_status_t status;
}inter_learn_sta_t;

#endif    /* _SSC_COMM_PI_H_ */

