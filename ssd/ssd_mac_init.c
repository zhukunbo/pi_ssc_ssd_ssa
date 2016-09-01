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

#include "../include/ssd_mac_init.h"

static mac_opra_class_t g_ssd_mac_opera;

/* ��������SSC ������ͬ����ʼ����*/
static u8 ssd_mac_recv_ds_begin(ss_rcv_msg_t *rcv_msg, int *ret)
{
	return true;	
}

/* ��������SSC ������ͬ���������� */
static u8 ssd_mac_recv_ds_end(ss_rcv_msg_t *rcv_msg, int *ret)
{
	return true;	
}


/* ���¶�̬��ַ*/
void ssd_add_dyn_func(void *msg)
{
	PRINT_DUG("enter ssd_add_dyn_func \n");
}

/* ɾ���ϻ�����*/
void ssd_del_age_time_mac(void *msg)
{
	PRINT_DUG("enter ssd_del_age_time_mac \n");
}

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
    ss_info->hdr.sender   	= MSG_ROLE_SSD;
    ss_info->hdr.reciver  	= MSG_ROLE_SSC;
    ss_info->hdr.dst_type 	= SS_MSG_DST_SELF_NODE;
    
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
static u8 ssd_mac_recv_ssa_msg(ss_rcv_msg_t *rcv_msg, int *ret)
{
	ss_info_t *msg;
	msg_info_t *payload;
	pi_mac_entry_t *data;

	/* ��ȡ������������ */
	msg = (ss_info_t *)rcv_msg->data;
	payload = (msg_info_t *)msg->payload;
    data = (pi_mac_entry_t *)payload->msg;

	/* �����Ӷ�̬��ַ���ϴ���ssc */
	ssd_send_msg_ssc(payload);

	if (data->flags == 1) {
		/* ���±������ݿ�*/
		ssd_add_dyn_func((void *)payload);
	} else {
		/* ��ַ�ϻ�*/
		ssd_del_age_time_mac((void *)payload);
	}
	
	return true;
}

/* ����SSA ��ʼ����ͬ����Ϣ */
static u8 ssd_mac_recv_up_dyn_begin(ss_rcv_msg_t *rcv_msg, int *ret)
{
	return true;
}

/* ����SSA ��������ͬ����Ϣ */
static u8 ssd_mac_recv_up_dyn_end(ss_rcv_msg_t *rcv_msg, int *ret)
{
	return true;
}

/* ssd �·���Ϣ��ssa */
void ssd_send_ssa_conf(int unit, int msgid, int msg_len, void *msg)
{
	ssdmw_mac_conf_msg_t *payload;
    ss_info_t *ss_info;
    int rv;

	PRINT_DUG("enter ssd_send_ssa_conf \n");

    ss_info = ss_info_alloc_init(sizeof(ssdmw_mac_conf_msg_t) + msg_len);
    if (ss_info == NULL) {
        PRINT_DUG("ss_info: %p", ss_info);
        return;
    }
    /* �����Ϣ�ṹ�Ͳ��� */
    ss_info->hdr.msgid = SSD_MSGID_MAC_COMM; 
    ss_info->hdr.msg_ver = 1;
    ss_info->hdr.concurrent = 1;
    ss_info->hdr.sender = MSG_ROLE_SSD;
    ss_info->hdr.reciver = MSG_ROLE_SSA;
    ss_info->hdr.dst_type = SS_MSG_DST_SELF_NODE;
    payload = (ssdmw_mac_conf_msg_t *)ss_info->payload;
    payload->unit = unit;
    payload->msgid = msgid;
    payload->msg_len = msg_len;
    if (msg_len > 0) {
        memcpy(payload->msg, (char *)msg, msg_len);
    }
    /* ������Ϣ */
    rv = ss_msg_send(ss_info);
    if (rv != 0) {
        PRINT_DUG("rv: %d", rv);
    }
    /* �ͷ���Ϣ���� */
    ss_info_free(ss_info);
    
    return;
}

/* ��Ӿ�̬��ַ*/
void ssd_add_static_func(void *msg)
{
	msg_info_t *reve_info;	

	PRINT_DUG("enter ssd_add_static_func \n");
	reve_info = (msg_info_t *)msg;
	ssd_send_ssa_conf(0, SSD_MSGID_CLEAR_VLAN_MAC, reve_info->msg_len, msg);
}

/* ��ն�̬��ַ����*/
void ssd_clear_dyn_func(void *msg)
{	
	PRINT_DUG("enter ssd_clear_dyn_func \n");
	/* ���´�����Ϣ��SSA */
	ssd_send_ssa_conf(0, SSD_MSGID_MAC_CLEAR_DYN, 0, NULL);

	/* ���±������ݿ�*/		
}

/* ɾ��ĳһvlan�µĵ�ַ*/
void ssd_clear_vlan_func(void *msg)
{
	msg_info_t *reve_info;	

	PRINT_DUG("enter ssd_clear_vlan_func \n");
	reve_info = (msg_info_t *)msg;
	ssd_send_ssa_conf(0, SSD_MSGID_CLEAR_VLAN_MAC, reve_info->msg_len, msg);

	/* �������ݿ�*/
	
}

/* ɾ��ĳһ�˿��µĵ�ַ*/
void ssd_clear_inter_func(void *msg)
{
	msg_info_t *reve_info;	

	PRINT_DUG("enter ssd_clear_inter_func \n");
	reve_info = (msg_info_t *)msg;
	ssd_send_ssa_conf(0, SSD_MSGID_CLEAR_INTER_DYN, reve_info->msg_len, msg);

	/* �������ݿ�*/
}

/* �޸��ϻ�ʱ��*/
void ssd_modify_age_time(void *msg)
{
	msg_info_t *reve_info;	

	PRINT_DUG("enter ssd_modify_age_time \n");
	reve_info = (msg_info_t *)msg;
	ssd_send_ssa_conf(0, SSD_MSGID_MAC_AGETIME, reve_info->msg_len, msg);
	/* �������ݿ�*/
}

/* �޸Ķ˿�ѧϰ״̬*/
void ssd_modify_inter_lean_sta(void *msg)
{
	msg_info_t *reve_info;	

	PRINT_DUG("enter ssd_modify_age_time \n");
	reve_info = (msg_info_t *)msg;
	ssd_send_ssa_conf(0, SSD_MSGID_MAC_INTER_LEARN, reve_info->msg_len, msg);
	/* �������ݿ�*/
}

/* ��������SSC ����Ϣ*/
static u8 ssd_mac_recv_ssc_command(ss_rcv_msg_t *rcv_msg, int *ret)
{
	int msgid;
	ss_info_t *msg;
    sscmw_mac_msg_t *payload;

	/* ��ȡ������������*/
	msg = ss_rcv_msg_get_ss_info(rcv_msg);
	payload = (sscmw_mac_msg_t *)msg->payload;
    msgid = payload->msgid;

	g_ssd_mac_opera.ssd_mac_func[msgid](payload->msg);

	return true;
}

/* ssd mac��������ʼ��*/
void ssd_mac_func_init(void)
{
	g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_CLEAR_DYN] = ssd_clear_dyn_func;
	g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_CLEAR_VLAN_MAC] = ssd_clear_vlan_func;
	g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_CLEAR_INTER_DYN] = ssd_clear_inter_func;
	g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_INTER_LEARN] = ssd_modify_inter_lean_sta;
	g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_AGETIME] = ssd_modify_age_time;
	g_ssd_mac_opera.ssd_mac_func[SSC_MSGID_MAC_ADD_ADDR] = ssd_add_static_func;

}

/* ssd ��س�ʼ����������*/
void ssd_mac_opera_init(void)
{
	ssd_mac_func_init();
	/* SSC�¼�������ע�� */
	(void)ss_msg_register_handler(SSC_MSGID_MAC_COMM, ssd_mac_recv_ssc_command);
	(void)ss_msg_register_handler(SSC_MSGID_MAC_DATA_SYN_BEGIN, ssd_mac_recv_ds_begin);
	(void)ss_msg_register_handler(SSC_MSGID_MAC_DATA_SYNC_END, ssd_mac_recv_ds_end);
	
	/* SSA�¼�������ע�� */
	(void)ss_msg_register_handler(SSA_MSGID_MAC_UPDATE, ssd_mac_recv_ssa_msg);
	(void)ss_msg_register_handler(SSA_MSGID_MAC_DATA_SYNC_BEGIN, ssd_mac_recv_up_dyn_begin);
	(void)ss_msg_register_handler(SSA_MSGID_MAC_DATA_SYNC_END, ssd_mac_recv_up_dyn_end);	
}

