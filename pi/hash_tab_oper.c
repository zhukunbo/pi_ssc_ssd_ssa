/*
 * Copyright(C) 2016 Ruijie Network. All rights reserved.
 */
/*
 * File Name: hash_tab_opera.c
 * Original Author:  zhukunbo@ruijie.com.cn, 2016-8-24
 *
 * function: HASH表操作相关
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ss_public.h"

struct msg_queue{
    struct list_head list;
    char *data;
};

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

/* 查找hash表中指定的元素 */
static int find_hash_key(int index, pi_mac_entry_t *entry, struct hlist_head *head)
{
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;
    
    if (head[index].first == NULL) {
        return 0;
    } else {
        hlist_for_each_safe(tmp, n, &head[index]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            if ((memcmp((char *)assist->mac, (char *)entry->mac, MAC_LEN) == 0) &&
                (assist->vlan_id == entry->vlan_id)){
                assist->port_id = entry->port_id;
                return 1;
            }
        }
    }
    
    return 0;
}

static int find_hash_key_del(int index, pi_mac_entry_t * entry, struct hlist_head * head)
{
    pi_mac_entry_t *assist;
    struct hlist_node *tmp, *n;
    
    if (head[index].first == NULL) {
        return ERR;
    } else {
        hlist_for_each_safe(tmp, n, &head[index]) {
            assist = hlist_entry(tmp, pi_mac_entry_t, tb_hlist);
            if ((strncmp((char *)assist->mac, (char *)entry->mac, MAC_LEN) == 0) &&
                    (assist->vlan_id == entry->vlan_id) && (assist->port_id == entry->port_id)){
                hlist_del(&assist->tb_hlist);
                free(assist);
                assist = NULL;
                return SUCC;
            }
        }
    }
    
    return ERR;
}

/* 
 * hash 插入函数
 * @entry: 待插入的表项
 * @head: hash链表头
 */
int insert_hash_table(pi_mac_entry_t *entry, struct hlist_head *head)
{
    int ret;
    u32 index;

    index = hash_function(entry->mac, entry->vlan_id);
    ret = find_hash_key(index, entry, head);
    if (ret == 1) {
        /* 条目在表中已经存在*/
        free(entry);
        return ret;
    }
    
    if (head[index].first == NULL) {
        hlist_add_head(&entry->tb_hlist, &head[index]);
    } else {
        hlist_add_before(&entry->tb_hlist, head[index].first);
    }
    
    return 0;
}

/* 
 * hash 删除函数
 * @entry: 待删除的表项
 * @head: hash链表头
 */
int del_hash_entry(pi_mac_entry_t *entry, struct hlist_head *head)
{
    u32 index;
    
    index = hash_function(entry->mac, entry->vlan_id);

    return find_hash_key_del(index, entry, head);
}

/* 
 * 接收到的消息插入到消息队列
 * @entry: 待删除的表项
 * @head: 队列头
 */
void insert_msg_to_queue(struct msg_queue *entry, struct list_head *head)
{
    list_add(&entry->list, head);
}

/* 
 * 获取消息队列中的数据
 * @head: 队列头
 */
msg_info_t * get_msg_data(struct list_head *head)
{
    struct msg_queue *entry;

    entry = list_entry(head->next, struct msg_queue, list);

    return (msg_info_t *)entry->data;
}

/* 
 * 删除已经处理的消息
 * @head: 队列头
 */
void del_dealed_msg(struct list_head *head)
{
    struct msg_queue *entry;

    if (head->next) {
        entry = list_entry(head->next, struct msg_queue, list);
        list_del(head->next);
        if (entry->data) {
            free(entry->data);
        }
        free(entry);
    }
}

