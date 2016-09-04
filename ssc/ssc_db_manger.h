#ifndef _SSC_DB_MANGER_H_
#define _SSC_DB_MANGER_H_

extern void ssc_del_dyn_addr(mac_opra_class_t *db_info);
extern int ssc_del_int_addr(mac_opra_class_t *db_info, int phy_id);
extern void ssc_del_vlan_addr(mac_opra_class_t *db_info, int vlan);
extern void ssc_del_age_entry(mac_opra_class_t *db_info, pi_mac_entry_t *entry);
extern void ssc_local_add_addr(mac_opra_class_t *db_info, base_static_mac_t *entry);
extern void ssc_add_dyn_addr(mac_opra_class_t *db_info, pi_mac_entry_t *entry);

#endif  /* _SSC_DB_MANGER_H_ */

