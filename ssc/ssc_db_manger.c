/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/*
 * File Name: main.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2013-5-14
 *
 * function: 主程序入口
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <rg_ss/lib/libpub/ss_msg_com.h>
#include <rg_ss/lib/libpub/ss_comm.h>

#include "../include/ss_public.h"
#include "../include/ssc_comm_pi.h"

#define NO_FOUND        -1
#define FOUND           0
#define STATIC_TYPE     (1<<6)

static int hash_function(u8 *mac, u32 vid)
{
    u32 hash;
    u32 lmac;
    u32 hmac;

    memcpy(&lmac, &mac[2], 4);
    memcpy(&hmac, &mac[0], 2);
    hash = vid ^ hmac ^ lmac;

    return (hash % HASH_SIZE);
}

/* 查找hash表中指定的元素*/
static int find_hash_key(int index, base_static_mac_t *entry, struct hlist_head *head)
{
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;

    if (head[index].first == NULL) {
        return ERR;
    } else {
        hlist_for_each_safe(tmp, n, &head[index]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            if ((memcmp((char *)assist->mac, (char *)entry->mac, MAC_LEN) == 0) &&
                (assist->vlan_id == entry->vlan_id)) {
                assist->port_id = entry->port_id;
                return SUCC;
            }
        }
    }

    return ERR;
}

/*
 * 删除所有的动态地址
 * @db_info : 指向数据库的指针
 */
void ssc_del_dyn_addr(mac_opra_class_t *db_info)
{
    int i;
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;

    pthread_mutex_lock(&db_info->mutex);
    for (i = 0; i < HASH_SIZE; i++) {
        if (db_info->dyn_mac_tbl_head[i].first == NULL) {
            continue;
        }
        hlist_for_each_safe(tmp, n, &db_info->dyn_mac_tbl_head[i]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            hlist_del(&assist->tb_hlist);
            free(assist);
            assist = NULL;
        }
    }
    pthread_mutex_unlock(&db_info->mutex);
}

static int find_hash_key_dyn(int index, pi_mac_entry_t *entry, struct hlist_head *head)
{
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;

    if (head[index].first == NULL) {
        return ERR;
    } else {
        hlist_for_each_safe(tmp, n, &head[index]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            if ((strncmp((char *)assist->mac, (char *)entry->mac, MAC_LEN) == 0) &&
                (assist->vlan_id == entry->vlan_id)){
                assist->port_id = entry->port_id;
                return SUCC;
            }
        }
    }

    return ERR;
}

/*
 * 添加动态地址库
 * @db_info : 指向数据库的指针
 * @entry : 待插入信息条目
 */
void ssc_add_dyn_addr(mac_opra_class_t *db_info, pi_mac_entry_t *entry)
{
    int ret;
    u32 index;
    pi_mac_entry_t *tmp;

    index = hash_function(entry->mac, entry->vlan_id);
    pthread_mutex_lock(&db_info->mutex);
    ret = find_hash_key_dyn(index, entry, db_info->dyn_mac_tbl_head);
    if (ret == SUCC) {
        /* 条目在表中已经存在 */
        pthread_mutex_unlock(&db_info->mutex);
        return;
    }
    
    tmp = (pi_mac_entry_t *)malloc(sizeof(pi_mac_entry_t));
    if (tmp == NULL) {
        printf("no mem \n");
        pthread_mutex_unlock(&db_info->mutex);
        return;
    }
    
    memcpy(tmp, entry, sizeof(pi_mac_entry_t));
    if (db_info->dyn_mac_tbl_head[index].first == NULL) {
        hlist_add_head(&tmp->tb_hlist, &db_info->dyn_mac_tbl_head[index]);
    } else {
        hlist_add_before(&tmp->tb_hlist, db_info->dyn_mac_tbl_head[index].first);
    }
    pthread_mutex_unlock(&db_info->mutex);
}

/*
 * 添加静态地址到SSC 数据库
 * @db_info : 指向数据库的指针
 * @entry : 待插入信息条目
 */
void ssc_local_add_addr(mac_opra_class_t *db_info, base_static_mac_t *entry)
{
    int ret;
    u32 index;
    pi_mac_entry_t *tmp;

    index = hash_function(entry->mac, entry->vlan_id);
    pthread_mutex_lock(&db_info->mutex);
    ret = find_hash_key(index, entry, db_info->static_mac_tbl_head);
    if (ret == SUCC) {
        /* 条目在表中已经存在*/
        pthread_mutex_unlock(&db_info->mutex);
        return;
    }
    tmp = (pi_mac_entry_t *)malloc(sizeof(pi_mac_entry_t));
    if (tmp == NULL) {
        printf("no mem \n");
        return;
    }

    memset(tmp, 0, sizeof(pi_mac_entry_t));
    memcpy(tmp->mac, entry->mac, MAC_LEN);
    tmp->addr_type = STATIC_TYPE;
    tmp->port_id   = entry->port_id;
    tmp->vlan_id   = entry->vlan_id;
    
    if (db_info->static_mac_tbl_head[index].first == NULL) {
        hlist_add_head(&tmp->tb_hlist, &db_info->static_mac_tbl_head[index]);
    } else {
        hlist_add_before(&tmp->tb_hlist, db_info->static_mac_tbl_head[index].first);
    }
    pthread_mutex_unlock(&db_info->mutex);
}

/*
 * 删除老化地址
 * @db_info : 指向数据库的指针
 * @vlan : vlan id
 */
void ssc_del_age_entry(mac_opra_class_t *db_info, pi_mac_entry_t *entry)
{
    int index;
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;

    index = hash_function(entry->mac, entry->vlan_id);
    pthread_mutex_lock(&db_info->mutex);
    if (db_info->dyn_mac_tbl_head[index].first == NULL) {
        pthread_mutex_unlock(&db_info->mutex);
        return;
    } else {
        hlist_for_each_safe(tmp, n, &db_info->dyn_mac_tbl_head[index]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            if ((memcmp((char *)assist->mac, (char *)entry->mac, MAC_LEN) == 0) &&
                (assist->vlan_id == entry->vlan_id)){
                hlist_del(&assist->tb_hlist);
                free(assist);
                assist = NULL;
            }
        }
    }
    
    pthread_mutex_unlock(&db_info->mutex);
}

/*
 * 删除某VLAN 下的动态地址
 * @db_info : 指向数据库的指针
 * @vlan : vlan id
 */
void ssc_del_vlan_addr(mac_opra_class_t *db_info, int vlan)
{
    int i;
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;

    pthread_mutex_lock(&db_info->mutex);
    for (i = 0; i < HASH_SIZE; i++) {
        if (db_info->dyn_mac_tbl_head[i].first == NULL) {
            continue;
        }
        hlist_for_each_safe(tmp, n, &db_info->dyn_mac_tbl_head[i]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            if (assist->vlan_id == vlan){
                hlist_del(&assist->tb_hlist);
                free(assist);
                assist = NULL;
            }
        }
    }
    pthread_mutex_unlock(&db_info->mutex);
}

/*
 * 删除某端口下的动态地址
 * @db_info : 指向数据库的指针
 * @phy_id : ssc数据库中保存的全局PHY_ID
 */
int ssc_del_int_addr(mac_opra_class_t *db_info, int phy_id)
{
    int i;
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;

    pthread_mutex_lock(&db_info->mutex);
    for (i = 0; i < HASH_SIZE; i++) {
        if (db_info->dyn_mac_tbl_head[i].first == NULL) {
            continue;
        }
        hlist_for_each_safe(tmp, n, &db_info->dyn_mac_tbl_head[i]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            if (assist->port_id == phy_id){
                hlist_del(&assist->tb_hlist);
                free(assist);
                assist = NULL;
            }   
        }
    }
    pthread_mutex_unlock(&db_info->mutex);

    return 0;
}

