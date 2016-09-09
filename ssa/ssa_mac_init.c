#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <rg_ss/public/ss_errno.h>
#include <rg_ss/lib/libpub/ss_comm.h>
#include <rg_ss/lib/libpub/ssc_comm_phyid.h>
#include <rg_ss/public/msgdef/switch/ss_msg_switch_mac.h>
#include <rg_ss/lib/libpub/ss_msg_notify.h>
#include <rg_ss/lib/libpub/ss_bitmap.h>
#include <rg_ss/lib/libpub/ss_msg_com.h>
#include <rg_sys/rg_types.h>
#include <rg_ss/public/ss_errno.h>
#include <rg_dev/dm_lib/rg_dm_spf.h>
#include <rg_dev/dm_lib/rg_dm_addr.h>
#include <rg_dev/dm_lib/phyid.h>
#include <soc/drv.h>
#include <bcm/port.h>
#include <bcm/vlan.h>
#include <bcm/trunk.h>
#include <bcm/l2.h>
#include <bcm/error.h>
#include <bcm/switch.h>
#include <bcm/types.h>
#include <bcm_int/common/lock.h>
#include <bcm_int/esw/vpn.h>
#include <customer/cust_mac.h>
#include <customer/cust_public.h>
#include <bcm_int/esw/trunk.h>

#include "../include/ss_public.h"
#include "../include/ssa_mac.h"
#include "../include/ssa_mac_adapt.h"
#include "../include/ssa_mac_defs.h"
#include "../include/ssa_mac_debug.h"
#include "../include/ssa_mac_test.h"
#include "../include/ssa_mac_drv.h"
#include "../include/ssa_mac_esw.h"

/* 配置消息处理函数 */
typedef void (*ssa_mac_drv_t)(void *data);
static ssa_mac_drv_t ssa_mac_drv[SSD_MSGID_MAC_ID_NUM];
static pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

static void esw_mac_flag_to_hw(int32_t unit, uint32_t usr_flag, uint32_t *hw_flag)
{
    uint32_t tmp_flag = 0;
    
    if (usr_flag & MAC_FLAG_STATIC) {
        tmp_flag |= BCM_L2_STATIC | BCM_L2_REPLACE_DYNAMIC;
    }
    tmp_flag |= BCM_L2_SRC_HIT;
    *hw_flag = tmp_flag;
}

/* 添加静态地址 */
static void ssa_add_static_func(void *msg)
{
    u32 vlan;
    msg_info_t *reve_info;  
    base_static_mac_t *data;
    bcm_l2_addr_t l2_addr;
    ssa_mac_port_t port_info;

    memset(&l2_addr, 0, sizeof(bcm_l2_addr_t));
    memset(&port_info, 0, sizeof(port_info));
    reve_info = (msg_info_t *)msg;
    data = (base_static_mac_t *)reve_info->msg;
    port_info.mac_flag  = MAC_FLAG_STATIC;
    port_info.port_id   = data->port_id;
    port_info.port_type = 0; /* SS_MAC_PPORT_PHYID */
    /* 将用户PORT 转换为芯片PORT */
    esw_mac_port_to_hw(0, &port_info, &l2_addr.port);
    memcpy(l2_addr.mac, data->mac, MAC_LEN);
    vlan = data->vlan_id;
    l2_addr.vid = (unsigned short)vlan;
    esw_mac_flag_to_hw(0, MAC_FLAG_STATIC, &l2_addr.flags);
    bcm_l2_addr_add(0, &l2_addr);

#if 0
    sleep(1);
    bcm_l2_addr_t l2_addr_1;
    ret = bcm_l2_addr_get(0, data->mac,(unsigned short)vlan,&l2_addr_1);
    PRINT_DUG("l2_addr.port = %d \n", ret);
    PRINT_DUG("l2_addr_1.port = 0x%x \n", l2_addr_1.port);
    PRINT_DUG("l2_addr_1.encap_id = 0x%x \n", l2_addr_1.encap_id);
    PRINT_DUG("l2_addr_1.modid = 0x%x \n", l2_addr_1.modid);
#endif
}

/* 清空动态地址表项 */
static void ssa_clear_dyn_func(void *msg)
{
    unsigned int rp_flags;
   
    rp_flags = BCM_L2_REPLACE_DELETE;
    bcm_l2_replace(0, rp_flags, NULL, 0, 0, 0);
}

/* 删除某一vlan下的地址 */
static void ssa_clear_vlan_func(void *msg)
{
    int vlan;
    unsigned short vid;
    msg_info_t *reve_info;

    reve_info = (msg_info_t *)msg;
    vlan = *(int *)reve_info->msg;  
    vid = (unsigned short)vlan;
    bcm_l2_addr_delete_by_vlan(0, vid, MAC_FLUSH_VID);
}

/* 删除某一端口下的地址 */
static void ssa_clear_inter_func(void *msg)
{
    int port;
    u32 port_id;
    ssa_mac_port_t port_info;
    msg_info_t *reve_info;

    reve_info = (msg_info_t *)msg;
    port_id   = *(int *)reve_info->msg;
    memset(&port_info, 0, sizeof(port_info));
    port_info.mac_flag  = MAC_FLAG_DYN;
    port_info.port_id   = port_id;
    port_info.port_type = 0;    /* SS_MAC_PPORT_PHYID */
    /* 将用户PORT 转换为芯片PORT*/
    esw_mac_port_to_hw(0, &port_info, &port);
    bcm_l2_addr_delete_by_port(0, 0, port, MAC_FLUSH_PHYID);
}

/* 修改老化时间*/
static void ssa_modify_age_time(void *msg)
{
    int age_timer;
    msg_info_t *reve_info;

    reve_info = (msg_info_t *)msg;
    age_timer =  *((int *)reve_info->msg);
    bcm_l2_age_timer_set(0, age_timer);
}

/* 修改端口学习状态*/
static void ssa_modify_inter_lean_sta(void *msg)
{
    int i, unit, port;
    u32 flags;
    inter_learn_sta_t *tmp;
    msg_info_t *reve_info;
    
    reve_info = (msg_info_t *)msg;
    tmp = (inter_learn_sta_t *)reve_info->msg;
    for (i = 1; i < PORT_NUM; i++) {
        ssa_mac_get_unit_port_from_phyid(tmp->port_id[i], &unit, &port);
        bcm_port_learn_get(unit, port, &flags);
        flags &= ~(BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD | BCM_PORT_LEARN_CPU);
        flags |= tmp->status;    
        bcm_port_learn_set(unit, port, flags);
    }
}

/* 接收来自ssd 的信息 */
static bool ssa_mac_recv_ssd_comm(ss_rcv_msg_t *rcv_msg, int *ret)
{
    int msgid;
    ss_info_t *msg;
    msg_info_t *tmp;
    
    msg = (ss_info_t *)rcv_msg->data;
    tmp = (msg_info_t *)msg->payload;
    msgid = tmp->msg_type;
    ssa_mac_drv[msgid](tmp); 
    
    return true;
}

/* 将芯片ID 转换为user id */
static void convert_port_to_usr(int unit, bcm_l2_addr_t *l2addr, ss_mac_notify_msg_t *noti_mac)
{
    int32_t mod, port;
    uint32_t *phyid;
    
    noti_mac->port_type = SS_MAC_PPORT_PHYID;
    noti_mac->port_id = 0;
    phyid = &noti_mac->port_id;
    
    mod = port = -1;
    if (BCM_GPORT_IS_SET(l2addr->port)) {
        if (BCM_GPORT_IS_MODPORT(l2addr->port)) {
            mod = BCM_GPORT_MODPORT_MODID_GET(l2addr->port);
            port = BCM_GPORT_MODPORT_PORT_GET(l2addr->port);              
        } 
    } else {  
        mod = l2addr->modid;
        port = l2addr->port;
    }
    if (mod != -1 && port != -1) {
        (void)ssa_mac_get_phyid_from_chip_port(mod, port, (int *)phyid);
    }
}

static void ssa_send_to_ssd(pi_mac_entry_t *data)
{
    msg_info_t *nt_msg;
    ss_info_t *ss_info;
    uint msg_len;
    int rv;

    msg_len = sizeof(msg_info_t) + sizeof(pi_mac_entry_t);
    ss_info = ss_info_alloc_init(msg_len);
    if (ss_info == NULL) {
        printf("no memory!");
        return;
    }
    /* 填充消息结构和参数 */
    ss_info->hdr.msgid          = SSA_MSGID_MAC_UPDATE;
    ss_info->hdr.msg_ver        = 1;
    ss_info->hdr.concurrent     = 1;
    ss_info->hdr.sender         = MSG_ROLE_SSA;
    ss_info->hdr.reciver        = MSG_ROLE_SSD;
    ss_info->hdr.dst_type       = SS_MSG_DST_SELF_NODE;
    nt_msg = (msg_info_t *)ss_info->payload;
    nt_msg->msg_len = msg_len;
    memcpy(nt_msg->msg, data, sizeof(pi_mac_entry_t));
    /* 发送消息 */
    rv = ss_msg_send(ss_info);
    if (rv != 0) {
        printf("ss_msg_send is failed \n");
    }
    /* 释放消息内容 */
    ss_info_free(ss_info);
}

/** 
 * mac_l2_addr_clbk - 接收SDK地址通告的处理函数 
 * @unit:    芯片ID
 * @l2addr:  SDK地址表项,bcm_l2_addr_t,学习到或要删除的地址
 * @insert:  通告类型，添加(1)或者删除(0)。
 * @uerdata: 额外信息
 *
 */
static void mac_clbk(int unit, bcm_l2_addr_t *l2addr, int insert, void *userdata)
{
    ss_mac_notify_msg_t noti_mac;
    pi_mac_entry_t tmp;

    pthread_mutex_lock(&send_mutex);
    /* 静态地址，不发出通告*/
    if (l2addr->flags & BCM_L2_STATIC) {
        pthread_mutex_unlock(&send_mutex);
        return;
    }
    memset(&tmp, 0, sizeof(pi_mac_entry_t));
    tmp.flags = insert;     /* 添加或者老化标志*/
    memcpy(tmp.mac, l2addr->mac, MAC_LEN);
    tmp.vlan_id   = l2addr->vid;
    tmp.addr_type = MAC_FLAG_DYN;
    convert_port_to_usr(0, l2addr, &noti_mac);
    tmp.port_id   = noti_mac.port_id;
    ssa_send_to_ssd(&tmp);
    pthread_mutex_unlock(&send_mutex);
    usleep(20);
}

static void ssa_mac_drv_func_init(void)
{
    ssa_mac_drv[SSD_MSGID_MAC_ADD_ADDR]    = ssa_add_static_func;
    ssa_mac_drv[SSD_MSGID_MAC_CLEAR_DYN]   = ssa_clear_dyn_func;
    ssa_mac_drv[SSD_MSGID_CLEAR_VLAN_MAC]  = ssa_clear_vlan_func;
    ssa_mac_drv[SSD_MSGID_CLEAR_INTER_DYN] = ssa_clear_inter_func;
    ssa_mac_drv[SSD_MSGID_MAC_INTER_LEARN] = ssa_modify_inter_lean_sta;
    ssa_mac_drv[SSD_MSGID_MAC_AGETIME]     = ssa_modify_age_time;
}

/* ssa相关操作函数初始化*/
void ssa_opera_mac_init(void)
{
    ssa_mac_drv_func_init();
    /* 注册接受来自SSD的信息*/
    (void)ss_msg_register_handler(SSD_MSGID_MAC_COMM, ssa_mac_recv_ssd_comm);
    /* 注册回调函数,加锁要*/
    bcm_l2_addr_register(0, mac_clbk, NULL);

}

