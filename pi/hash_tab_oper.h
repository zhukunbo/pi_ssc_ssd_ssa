#ifndef _HASH_TAB_OPER_H_
#define _HASH_TAB_OPER_H_

#include <rg_sys/list.h>

extern int insert_hash_table(pi_mac_entry_t *entry, struct hlist_head *head);
extern void del_dealed_msg(struct list_head *head);
extern msg_info_t * get_msg_data(struct list_head *head);
extern void insert_msg_to_queue(struct msg_queue *entry, struct list_head *head);
extern int del_hash_entry(pi_mac_entry_t *entry, struct hlist_head *head);


#endif  /* _HASH_TAB_OPER_H_ */
