/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/* 
 * ssd_mac_init.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2016-8-22
 *
 * �ֲ�ʽ�ڵ�MACģ���߼������ļ�
 *
 */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <rg_ss/public/ss_errno.h>
#include <rg_ss/public/shell/ss_switch_mtrace.h>
#include <rg_ss/lib/libpub/spub_hash.h>
#include <rg_ss/lib/libpub/ss_msg_com.h>
#include <rg_ss/public/ss_errno.h>
#include <rg_ss/public/msgdef/switch/ss_msg_switch_mac.h>
#include <rg_ss/lib/libpub/ss_public.h>

#include "../include/ss_public.h"
#include "../include/ssd_mac_init.h"

static mac_opra_class_t g_ssd_mac_opera;

static void ssd_send_msg_ssc(msg_info_t *msg_info)
{
    ss_info_t *ss_info;
    int rv;
    
    ss_info = ss_info_alloc_init(msg_info->msg_len);
    if (ss_info == NULL) {
        printf("ss_info: %p \n", ss_info);
        return;
    }
    /* �����Ϣ�ṹ�Ͳ��� */
    ss_info->hdr.msgid      = SSD_MSGID_MAC_UPDATE; 
    ss_info->hdr.msg_ver    = 1;
    ss_info->hdr.concurrent = 1;
    ss_info->hdr.sender     = MSG_ROLE_SSD;
    ss_info->hdr.reciver    = MSG_ROLE_SSC;
    ss_info->hdr.dst_type   = SS_MSG_DST_SELF_NODE;
    memcpy(ss_info->payload, (char *)msg_info, msg_info->msg_len);
    /* ������Ϣ */
    rv = ss_msg_send(ss_info);
    if (rv != 0) {
        printf("rv: %d", rv);
    }
    /* �ͷ���Ϣ���� */
    ss_info_free(ss_info);
    
    return;
}

/* ����SSA ��Ϣ */
static bool ssd_mac_recv_ssa_msg(ss_rcv_msg_t *rcv_msg, int *ret)
{
    ss_info_t *msg;
    msg_info_t *payload;
    pi_mac_entry_t *data;

    /* ��ȡ������������ */
    msg = (ss_info_t *)rcv_msg->data;
    payload = (msg_info_t *)msg->payload;
    data = (pi_mac_entry_t *)payload->msg;
    data->port_id = ss_phyid_fe2ce(data->port_id);
    /* �����Ӷ�̬��ַ���ϴ���ssc */
    ssd_send_msg_ssc(payload);

    return true;
}

/* ssd�·���Ϣ��ssa */
void ssd_send_ssa_conf(int unit, int msgid, int msg_len, void *msg)
{
    msg_info_t *reve_info;  
    ss_info_t *ss_info;
    int rv;

    ss_info = ss_info_alloc_init(msg_len);
    if (ss_info == NULL) {
        PRINT_DUG("ss_info: %p", ss_info);
        return;
    }
    /* �����Ϣ�ṹ�Ͳ��� */
    ss_info->hdr.msgid        = SSD_MSGID_MAC_COMM;
    ss_info->hdr.msg_ver      = 1;
    ss_info->hdr.concurrent   = 1;
    ss_info->hdr.sender       = MSG_ROLE_SSD;
    ss_info->hdr.reciver      = MSG_ROLE_SSA;
    ss_info->hdr.dst_type     = SS_MSG_DST_SELF_NODE;
    reve_info = (msg_info_t *)ss_info->payload;
    reve_info->msg_type= msgid;
    reve_info->msg_len = msg_len;
    
    if (msg_len > 0) {
        memcpy(reve_info, msg, msg_len);
    }
    /* ������Ϣ */
    rv = ss_msg_send(ss_info);
    if (rv != 0) {
        PRINT_DUG("rv: %d", rv);
    }
    /* �ͷ���Ϣ���� */
    ss_info_free(ss_info);
}

/* ��Ӿ�̬��ַ */
void ssd_add_static_func(void *msg)
{
    msg_info_t *reve_info;
    base_static_mac_t *static_mac;

    reve_info = (msg_info_t *)msg;
    static_mac = (base_static_mac_t *)reve_info->msg;
    static_mac->port_id = ss_phyid_ce2fe(static_mac->port_id);
    ssd_send_ssa_conf(0, SSD_MSGID_CLEAR_VLAN_MAC, reve_info->msg_len, msg);
}

/* ��ն�̬��ַ���� */
void ssd_clear_dyn_func(void *msg)
{   
    msg_info_t *reve_info;
    reve_info = (msg_info_t *)msg;
    /* ���´�����Ϣ��SSA */
    ssd_send_ssa_conf(0, SSD_MSGID_MAC_CLEAR_DYN, reve_info->msg_len, msg);
}

/* ɾ��ĳһvlan�µĵ�ַ */
void ssd_clear_vlan_func(void *msg)
{
    msg_info_t *reve_info;

    reve_info = (msg_info_t *)msg;
    ssd_send_ssa_conf(0, SSD_MSGID_CLEAR_VLAN_MAC, reve_info->msg_len, msg);
}

/* ɾ��ĳһ�˿��µĵ�ַ */
void ssd_clear_inter_func(void *msg)
{
    u32 port;
    msg_info_t *reve_info;  

    reve_info = (msg_info_t *)msg;
    port = *((int *)reve_info->msg);
    port = ss_phyid_ce2fe(port);
    *((int *)reve_info->msg) = port;
    ssd_send_ssa_conf(0, SSD_MSGID_CLEAR_INTER_DYN, reve_info->msg_len, msg);
}

/* �޸��ϻ�ʱ�� */
void ssd_modify_age_time(void *msg)
{
    msg_info_t *reve_info;

    reve_info = (msg_info_t *)msg;
    ssd_send_ssa_conf(0, SSD_MSGID_MAC_AGETIME, reve_info->msg_len, msg);
}

/* �޸Ķ˿�ѧϰ״̬ */
void ssd_modify_inter_lean_sta(void *msg)
{
    int i;
    inter_learn_sta_t *tmp;
    msg_info_t *reve_info;

    reve_info = (msg_info_t *)msg;
    tmp = (inter_learn_sta_t *)reve_info->msg;
    for (i = 1; i < PORT_NUM + 1; i++) {
        tmp->port_id[i] = ss_phyid_ce2fe(tmp->port_id[i]);
    }   
    ssd_send_ssa_conf(0, SSD_MSGID_MAC_INTER_LEARN, reve_info->msg_len, msg);
}

/* ��������SSC ����Ϣ*/
static bool ssd_mac_recv_ssc_command(ss_rcv_msg_t *rcv_msg, int *ret)
{
    int msgid;
    ss_info_t *msg;
    msg_info_t *tmp;

    /* ��ȡ������������*/
    msg = (ss_info_t *)rcv_msg->data;
    tmp = (msg_info_t *)msg->payload;
    msgid = tmp->msg_type;
    g_ssd_mac_opera.ssd_mac_func[msgid](tmp);

    return true;
}

/* ssd mac��������ʼ��*/
void ssd_mac_func_init(void)
{
    g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_ADD_ADDR]    = ssd_add_static_func;
    g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_CLEAR_DYN]   = ssd_clear_dyn_func;
    g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_CLEAR_VLAN_MAC]  = ssd_clear_vlan_func;
    g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_CLEAR_INTER_DYN] = ssd_clear_inter_func;
    g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_INTER_LEARN] = ssd_modify_inter_lean_sta;
    g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_AGETIME]     = ssd_modify_age_time;
}

/* ssd ��س�ʼ����������*/
void ssd_mac_opera_init(void)
{
    ssd_mac_func_init();
    /* SSC�¼�������ע�� */
    (void)ss_msg_register_handler(SSC_MSGID_MAC_COMM, ssd_mac_recv_ssc_command);
    /* SSA�¼�������ע�� */
    (void)ss_msg_register_handler(SSA_MSGID_MAC_UPDATE, ssd_mac_recv_ssa_msg);
}

