#ifndef _SSD_MAC_INIT_H
#define _SSD_MAC_INIT_H

/* ������Ϣ������ */
typedef void (*ssd_mac_func_t)(void *data);

typedef struct ssc_mac_opra_class{
    u32 age_time;                           /* �ϻ�ʱ��*/
    port_status_t port_status[PORT_NUM];    /* ÿ���˿ڵ�״̬*/
    u32 dyn_addr_count;                     /* ��Ч��̬��ַ���� */
    u32 static_addr_count;                  /* ��Ч��̬��ַ����*/
    struct hlist_head static_mac_tbl_head[HASH_SIZE];   /* ��̬mac */
    struct hlist_head dyn_mac_tbl_head[HASH_SIZE];      /* ��̬mac */
    ssd_mac_func_t ssd_mac_func[SSC_MSGID_MAC_ID_NUM];
    pthread_mutex_t  mutex;                 /* �������������ݽṹ*/
}mac_opra_class_t;

#endif  /* _SSD_MAC_INIT_H */

