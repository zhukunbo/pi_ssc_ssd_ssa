/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/*
 * File Name: main.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2016-9-04
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
#include <rg_ss/lib/libpub/ssc_comm_phyid.h>
#include <rg_ss/lib/libpub/ss_msg_util.h>
#include <rg_ss/lib/libpub/ss_msg_com.h>
#include <rg_ss/public/ss_errno.h>
#include <rg_ss/lib/libssd/dce/ss_dce_cn.h>
#include <rg_ss/public/msgdef/dce/ss_msg_dce_cn.h>
#include <rg_ss/lib/libpub/ss_comm.h>
#include <rg_ss/public/dce/ss_dce_cn_comm.h>
#include <at/at_srv.h>

#include "../include/ss_public.h"
#include "../include/ssc_comm_pi.h"
#include "../include/ssc_db_manger.h"

static mac_opra_class_t g_ssc_mac_opera;
static sock_info_t g_sock_info;
/* ���ڱ�ʶ�����Ƿ�ͬ������ */
static int ds_is_end;   
static pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ��ssd �·�������Ϣ */
static void ssc_send_ssd_conf(int node, int vsdid, int msgid, int msg_len, void *msg)
{
    int rv;
    msg_info_t *tmp;
    ss_info_t *ss_info;

    ss_info = ss_info_alloc_init(sizeof(msg_info_t) + msg_len);
    if (ss_info == NULL) {
        printf("no mem \n");
        return;
    }
    /* �����Ϣ�ṹ�Ͳ��� */
    ss_info->hdr.msgid      = SSC_MSGID_MAC_COMM; 
    ss_info->hdr.msg_ver    = 1;
    ss_info->hdr.concurrent = 1;
    ss_info->hdr.sender     = MSG_ROLE_SSC;
    ss_info->hdr.reciver    = MSG_ROLE_SSD;
    ss_info->hdr.dst_type   = SS_MSG_DST_ALL_DIST_NODE;

    tmp = (msg_info_t *)ss_info->payload;
    tmp->msg_type = msgid;
    tmp->msg_len  = msg_len + sizeof(msg_info_t);
    
    if (msg_len > 0) {
        memcpy(tmp->msg, (char *)msg, msg_len);
    }
    
    /* ������Ϣ */
    rv = ss_msg_send(ss_info);
    if (rv != SS_E_NONE) {
        printf("rv: %d", rv);
    }
    /* �ͷ���Ϣ���� */
    ss_info_free(ss_info);
}

static void ssc_add_dyn_func(void *msg)
{
    msg_info_t *data;
    pi_mac_entry_t *entry;

    data  = (msg_info_t *)msg;
    entry = (pi_mac_entry_t *)data->msg;
    /* ���´������Ķ�̬��ַ */
    ssc_add_dyn_addr(&g_ssc_mac_opera, entry);
}

static void ssc_del_age_time_mac(void *msg)
{
    msg_info_t *data;
    pi_mac_entry_t *entry;
    
    data  = (msg_info_t *)msg;
    entry = (pi_mac_entry_t *)data->msg;
    
    /* ɾ���ϻ����� */
    ssc_del_age_entry(&g_ssc_mac_opera, entry);
}

/* ��Ӿ�̬��ַ */
void ssc_add_static_func(void *data)
{
    int phy_id;
    msg_info_t *reve_info;
    base_static_mac_t *static_mac;

    reve_info  = (msg_info_t *)data;
    static_mac = (base_static_mac_t *)reve_info->msg;
    ss_comm_ifx_db_get_phyid(0, static_mac->port_id, &phy_id);
    static_mac->port_id = phy_id;
    ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID,
        SSC_MSGID_MAC_ADD_ADDR, sizeof(base_static_mac_t), (char *)static_mac);
    ssc_local_add_addr(&g_ssc_mac_opera, static_mac);
}

/* ��ն�̬��ַ���� */
void ssc_clear_dyn_func(void *data)
{

    ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID,
        SSC_MSGID_MAC_CLEAR_DYN,0, NULL);
    ssc_del_dyn_addr(&g_ssc_mac_opera);
}

/* ɾ��ĳһvlan�µĵ�ַ */
void ssc_clear_vlan_func(void *data)
{
    int vlan_id;
    msg_info_t *reve_info;
    
    reve_info = (msg_info_t *)data;
    vlan_id   = *((int *)reve_info->msg);
    ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID,
        SSC_MSGID_CLEAR_VLAN_MAC, sizeof(int), &vlan_id);
    /* ɾ��ĳVLAN �µĵ�ַ */
    ssc_del_vlan_addr(&g_ssc_mac_opera, vlan_id);
}

/* ɾ��ĳһ�˿��µĵ�ַ */
void ssc_clear_inter_func(void *data)
{
    int port_id;
    int phy_id;
    msg_info_t *reve_info;
    
    reve_info   = (msg_info_t *)data;
    port_id  = *((int *)reve_info->msg);
    ss_comm_ifx_db_get_phyid(0, port_id, &phy_id);
    port_id = phy_id;
    ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID,
        SSC_MSGID_CLEAR_INTER_DYN, sizeof(int), &port_id);
    /* ɾ���������ݿ��е�ĳ�˿��µĵ�ַ */   
    (void)ssc_del_int_addr(&g_ssc_mac_opera, port_id);
}

/* �޸��ϻ�ʱ�� */
void ssc_modify_age_time(void *data)
{
    int seconds;
    msg_info_t *reve_info;  

    reve_info   = (msg_info_t *)data;
    seconds  = *(int *)reve_info->msg;
    ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID, 
        SSC_MSGID_MAC_AGETIME, sizeof(int), (void *)&seconds);
    /* �޸ı����ϻ�ʱ�� */
    pthread_mutex_lock(&g_ssc_mac_opera.mutex);
    g_ssc_mac_opera.age_time = seconds;
    pthread_mutex_unlock(&g_ssc_mac_opera.mutex);
}

/* �޸Ķ˿�ѧϰ״̬ */
void ssc_modify_inter_lean_sta(void *data)
{
    int i, phy_id;
    msg_info_t *reve_info;
    inter_learn_sta_t tmp;
    
    reve_info  = (msg_info_t *)data;
    tmp.status = *((int *)reve_info->msg);
    for (i = 1; i <= PORT_NUM; i++) {
        ss_comm_ifx_db_get_phyid(0, i, &phy_id);
        tmp.port_id[i] = phy_id;    
    }
    ssc_send_ssd_conf(SSCMW_MAC_DIST_DEF, DEFAULT_VSD_ID,
        SSC_MSGID_MAC_INTER_LEARN, sizeof(tmp), (char *)&tmp);
    /* �޸ı��صĶ˿�ѧϰ״̬ */
    pthread_mutex_lock(&g_ssc_mac_opera.mutex);
    for (i = 0; i <= PORT_NUM; i++) {
        g_ssc_mac_opera.port_status[i]= tmp.status; 
    }
    pthread_mutex_unlock(&g_ssc_mac_opera.mutex);
}

static void *ssc_deal_ds(void *arg)
{
    (void)pthread_detach(pthread_self());
    
    while (!ds_is_end) {
        sleep(5);
    }
    /* ������һ���Լ�� */
    /* ��ssd��ͬ����δʵ�� */
    return &ds_is_end;
}

/* �յ�����PI�Ŀ�ʼ�������� */
static void ssc_rece_pi_ds_start(void *data)
{
    int ret;
    pthread_t thread_id;

    ds_is_end = 0;
    ret = pthread_create(&thread_id, NULL, ssc_deal_ds, NULL);
    if (ret < 0) {
        printf("create thread failed \n");
        return;
    }
}

/* ����PIͬ������������*/
static void ssc_rece_pi_ds_data(void *data)
{
    int i;
    msg_info_t *reve_info;
    conf_info_t *tmp;
    
    reve_info = (msg_info_t *)data;
    tmp = (conf_info_t *)reve_info->msg;
    pthread_mutex_lock(&g_ssc_mac_opera.mutex);
    g_ssc_mac_opera.age_time = tmp->age_time;
    for (i = 1; i < PORT_NUM; i++) {
        g_ssc_mac_opera.port_status[i] = tmp->port_status[i];
    }
    pthread_mutex_unlock(&g_ssc_mac_opera.mutex);
}

/* ���ܵ�����PI�Ľ����ź�*/
static void ssc_rece_pi_ds_end(void *data)
{
    ds_is_end = 1;
}

/*
 *�������ݵ�PI �ӿ�
 *@msg_type: ��Ϣ����
 *@msg : ��Ϣ����ָ��
 *@msg_len: ��Ϣ�ĳ���
 */
void ssc_send_msg_pi(int msg_type, void *msg, int msg_len)
{
    msg_info_t *tmp;

    pthread_mutex_lock(&send_mutex);
    tmp = (msg_info_t *)malloc(sizeof(msg_info_t) + msg_len);
    if (tmp == NULL) {
        printf("no mem,send msg is failed \n");
        pthread_mutex_unlock(&send_mutex);
        return;
    }
    bzero(tmp, (sizeof(msg_info_t) + msg_len));
    tmp->msg_type = msg_type;
    tmp->msg_len  = msg_len + sizeof(msg_info_t);
    if (msg_len) {
        memcpy(tmp->msg, (char *)msg, msg_len);
    }
    if (write(g_sock_info.sock_fd, (char *)tmp, tmp->msg_len) < 0) {
        printf("write to ssc is failed \n");
        close(g_sock_info.sock_fd);
    }
    free(tmp);
    pthread_mutex_unlock(&send_mutex);
}

/* *
 * ���������²�ssd�ĵ�ַ��Ϣ 
 * @rcv_msg :���յ�����Ϣ����
 */
bool ssc_mac_update_recv(ss_rcv_msg_t *rcv_msg, int *ret)
{
    u32 ifx;
    ss_info_t *msg;
    msg_info_t *payload;
    pi_mac_entry_t *data;
    
    msg = (ss_info_t *)rcv_msg->data;
    payload = (msg_info_t *)msg->payload;
    data = (pi_mac_entry_t *)payload->msg;

    /* �˴�Ҫ���ж˿����͵�ת��*/
    ss_comm_get_phyid_user_ifx(data->port_id, 0 ,&ifx);
    data->port_id = ifx;
    /* �������ݵ�PI */
    ssc_send_msg_pi(SSC_MSGID_MAC_ADDR_NOTIFY, (void *)data, sizeof(pi_mac_entry_t));
    if (data->flags) {
        ssc_add_dyn_func(payload);
    } else {
        ssc_del_age_time_mac(payload);
    }
    
    return true;
}

/* *
 * ���մ�������PI ������
 * @msg_text :���յ�����Ϣ����
 * @len :��Ϣ����
 */
static void ssc_recevie_msg_pi(char *msg_text, int len)
{
    msg_info_t *reve_info;

    reve_info = (msg_info_t *)msg_text;
    g_ssc_mac_opera.ssc_mac_func[reve_info->msg_type](msg_text);
}

/* ����PI �ɹ�ʱ�����������·� */
static void ssc_send_pi_fetch(void)
{
    ssc_send_msg_pi(SSC_MSGID_MAC_FETCH, NULL, 0);
}

/* ͬ��������Ϣ */
static void ssc_ds_conf_info(void)
{
    int i, len;
    conf_info_t conf_info;
    ssc_ds_info_t *tmp;
    
    len = sizeof(conf_info_t);
    tmp = (ssc_ds_info_t *)malloc(sizeof(ssc_ds_info_t) + len);
    if (tmp == NULL) {
        printf("no mem \n");
        return;
    }
    tmp->type = SSC_CONF;
    pthread_mutex_lock(&g_ssc_mac_opera.mutex);
    conf_info.age_time = g_ssc_mac_opera.age_time;
    for (i = 1; i <= PORT_NUM; i++) {
        conf_info.port_status[i] = g_ssc_mac_opera.port_status[i];
    }
    pthread_mutex_unlock(&g_ssc_mac_opera.mutex);
    memcpy(tmp->context, &conf_info, len);
    ssc_send_msg_pi(SSC_MSGID_MAC_UP_ING, tmp, sizeof(ssc_ds_info_t) + len);
    
    free(tmp);
}

/* SSC ͬ����̬��ַ */
static void ssc_ds_static_info(void)
{
    int i, len;
    ssc_ds_info_t *tmp;
    pi_mac_entry_t *assist;
    struct hlist_node *element;
    
    len = sizeof(pi_mac_entry_t);
    tmp = (ssc_ds_info_t *)malloc(sizeof(ssc_ds_info_t) + len);
    if (tmp == NULL) {
        printf("no mem \n");
        return;
    }
    tmp->type = SSC_STATIC;
    pthread_mutex_lock(&g_ssc_mac_opera.mutex);
    for (i = 0; i < HASH_SIZE; i++) {
        if (g_ssc_mac_opera.static_mac_tbl_head[i].first == NULL) {
            continue;
        } else {
            hlist_for_each(element, &g_ssc_mac_opera.static_mac_tbl_head[i]) {
                assist = hlist_entry(element, pi_mac_entry_t, tb_hlist);
                memcpy(tmp->context, assist, len);
                ssc_send_msg_pi(SSC_MSGID_MAC_UP_ING, tmp, sizeof(ssc_ds_info_t) + len);
                usleep(100);
            }
        }
    }
    free(tmp);
    pthread_mutex_unlock(&g_ssc_mac_opera.mutex);
}

/* SSC ͬ����̬��ַ */
static void ssc_ds_dyn_info(void)
{
    int i, len;
    ssc_ds_info_t *tmp;
    pi_mac_entry_t *assist;
    struct hlist_node *element;

    len = sizeof(pi_mac_entry_t);
    tmp = (ssc_ds_info_t *)malloc(sizeof(ssc_ds_info_t) + len);
    if (tmp == NULL) {
        printf("no mem \n");
        return;
    }
    tmp->type = SSC_DYN;

    pthread_mutex_lock(&g_ssc_mac_opera.mutex);
    for (i = 0; i < HASH_SIZE; i++) {
        if (g_ssc_mac_opera.dyn_mac_tbl_head[i].first == NULL) {
            continue;
        } else {
            hlist_for_each(element, &g_ssc_mac_opera.dyn_mac_tbl_head[i]) {
                assist = hlist_entry(element, pi_mac_entry_t, tb_hlist);
                memcpy(tmp->context, assist, len);
                ssc_send_msg_pi(SSC_MSGID_MAC_UP_ING, tmp, sizeof(ssc_ds_info_t) + len);
                usleep(100);
            }
        }
    }
    pthread_mutex_unlock(&g_ssc_mac_opera.mutex);
    free(tmp);
}

/* ����SSC ��PI  �������� */
static void *ssc_ds_data_pi(void *arg)
{
    (void)pthread_detach(pthread_self());
    
    /* 1����PI ��������ͬ����ʼ���� */
    ssc_send_msg_pi(SSC_MSGID_MAC_UP_START, NULL, 0);
    usleep(100);
    /* 2����ʼ������������ */
    ssc_ds_conf_info();
    usleep(100);
    ssc_ds_static_info();
    usleep(100);
    ssc_ds_dyn_info();
    usleep(100);
    /* 3����PI ��������ͬ���������� */
    ssc_send_msg_pi(SSC_MSGID_MAC_UP_END, NULL, 0);

    return NULL;
}

static void ssc_sync_data_pi(void)
{
    int ret;
    pthread_t thread_id;

    PRINT_DUG(" ssc sync date pi \n");
    /* ����һ���߳���PI ͬ������*/
    ret = pthread_create(&thread_id, NULL, ssc_ds_data_pi, NULL);
    if (ret < 0) {
        printf("create thread failed \n");
        return ;
    }
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
    g_sock_info.server_addr.sin_family      = AF_INET;
    g_sock_info.server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    g_sock_info.server_addr.sin_port        = htons(SERVER_PORT);
    
    while (1) {
        if (connect(g_sock_info.sock_fd, (struct sockaddr *)&g_sock_info.server_addr,
            sizeof(struct sockaddr_in)) < 0) {
            printf("ssc connect error \n");
        } else {
            /* ���ӳɹ�����PI����������Ϣ */
            ssc_send_pi_fetch();
            /* �����̣߳�ͬ��SSC �е����ݵ�PI */
            ssc_sync_data_pi();
            while (1) {
                ret = read(g_sock_info.sock_fd, buff, BUF_SIZE);
                if (ret <= 0) {
                    close(g_sock_info.sock_fd);
                    goto RE_CONN;
                }
                ssc_recevie_msg_pi(buff, BUF_SIZE);
            }
        }
        sleep(2);
    }
}

/* mac��������ʼ�� */
void ssc_mac_opera_init(void)
{   
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_ADD_ADDR]    = ssc_add_static_func;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_CLEAR_DYN]   = ssc_clear_dyn_func;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_CLEAR_VLAN_MAC]  = ssc_clear_vlan_func;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_CLEAR_INTER_DYN] = ssc_clear_inter_func;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_INTER_LEARN] = ssc_modify_inter_lean_sta;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_MAC_AGETIME]     = ssc_modify_age_time;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_DS_START]        = ssc_rece_pi_ds_start;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_DS_ING]          = ssc_rece_pi_ds_data;
    g_ssc_mac_opera.ssc_mac_func[PI_MSGID_DS_END]          = ssc_rece_pi_ds_end;
}

/* ssc��pi���ͨ�ų�ʼ�� */
void ssc_comm_init(void)
{
    int ret;
    pthread_t thread_id;

    /* ��ʼ��ͨ�Žӿ�*/
    ret = pthread_create(&thread_id, NULL, pthread_deal_func, NULL);
    if (ret < 0) {
        printf("create thread failed \n");
        return ;
    }
}

/* ssc ά�����ݿ��ʼ��*/
void ssc_db_init(void)
{   
    int i;

    g_ssc_mac_opera.dyn_addr_count    = 0;
    g_ssc_mac_opera.static_addr_count = 0;
    /* ��ʼ����̬�;�̬mac hash ��*/
    for (i = 0; i < HASH_SIZE; i++) {
        INIT_HLIST_HEAD(&g_ssc_mac_opera.static_mac_tbl_head[i]);
        INIT_HLIST_HEAD(&g_ssc_mac_opera.dyn_mac_tbl_head[i]);
    }
    pthread_mutex_init(&g_ssc_mac_opera.mutex, NULL);
}

