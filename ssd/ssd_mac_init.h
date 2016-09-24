#ifndef _SSD_MAC_INIT_H
#define _SSD_MAC_INIT_H

/* 配置消息处理函数 */
typedef void (*ssd_mac_func_t)(void *data);

typedef struct ssc_mac_opra_class{
    u32 age_time;                           /* 老化时间*/
    port_status_t port_status[PORT_NUM];    /* 每个端口的状态*/
    u32 dyn_addr_count;                     /* 有效动态地址个数 */
    u32 static_addr_count;                  /* 有效静态地址个数*/
    struct hlist_head static_mac_tbl_head[HASH_SIZE];   /* 动态mac */
    struct hlist_head dyn_mac_tbl_head[HASH_SIZE];      /* 静态mac */
    ssd_mac_func_t ssd_mac_func[SSC_MSGID_MAC_ID_NUM];
    pthread_mutex_t  mutex;                 /* 用来保护改数据结构*/
}mac_opra_class_t;

#endif  /* _SSD_MAC_INIT_H */

