#ifndef _CLI_MAC_COMMAND_H_
#define _CLI_MAC_COMMAND_H_

#define FIRST_ARG        1
#define SECOND_ARG       2

#undef  ALTERNATE
#define ALTERNATE        no_alt

/* ��ʾmac��������� */
extern void exec_show_rookie_mac_table_cmd(struct_command_data_block *pcdb);
/* ��ʾ���õ��ϻ�ʱ�� */
extern void exec_show_rookie_mac_agetime_cmd(struct_command_data_block *pcdb);
/* ��ʾmac��Ŀ��Ŀ */
extern void exec_show_rookie_mac_count_cmd(struct_command_data_block *pcdb);
/* ��ʾ��̬·�� */
extern void exec_show_rookie_mac_static_cmd(struct_command_data_block *pcdb);
/* ��ʾָ��vlan�µ�mac����Ϣ */
extern void exec_show_rookie_mac_vlan_num_cmd(struct_command_data_block *pcdb);
/* �����̬mac���� */
extern void exec_clear_rookie_mac_dynamic_cmd(struct_command_data_block *pcdb);
/* ���ָ��vlan�µı��� */
extern void exec_clear_rookie_mac_vlan_num_cmd(struct_command_data_block *pcdb);
/* ���ָ���ӿ��µı��� */
extern void exec_clear_rookie_mac_interface_num_cmd(struct_command_data_block *pcdb);
/* �����Ƿ�����mac��ַѧϰ */
extern void cfg_rookie_mac_learn_cmd(struct_command_data_block *pcdb);
/* �����ϻ�ʱ�� */
extern void cfg_rookie_mac_table_agetime_num_cmd(struct_command_data_block *pcdb);
/* ���þ�̬mac */
extern void cfg_rookie_mac_table_static__cmd(struct_command_data_block *pcdb);

/* ����count������ */
EOLWOS(exec_show_rookie_mac_count_eol, exec_show_rookie_mac_count_cmd)
/* ������ʾmac��������� */
EOLWOS(exec_show_rookie_mac_table_eol, exec_show_rookie_mac_table_cmd)
/* ����agetime������ */
EOLWOS(exec_show_rookie_mac_agetime_eol, exec_show_rookie_mac_agetime_cmd)
/* ����static������ */
EOLWOS(exec_show_rookie_mac_static_eol, exec_show_rookie_mac_static_cmd)
/* ����vlan num������ */
EOLWOS(exec_show_rookie_mac_vlan_num_eol, exec_show_rookie_mac_vlan_num_cmd)

NUMBER(exec_show_rookie_mac_vlan_num, exec_show_rookie_mac_vlan_num_eol, exec_show_rookie_mac_vlan_num_eol,
    "vlan num", CDBVAR(int, FIRST_ARG), FIRST_ARG, VLAN_MAX_SIZE);
/* ƥ��ؼ���"vlan" */
KEYWORD(exec_show_rookie_mac_vlan, exec_show_rookie_mac_vlan_num, exec_show_rookie_mac_table_eol,
    "vlan", " match vlan ", PRIVILEGE_USER)
/* ƥ��ؼ���"static" */
KEYWORD(exec_show_rookie_mac_static, exec_show_rookie_mac_static_eol, exec_show_rookie_mac_vlan,
    "static", " match static ", PRIVILEGE_USER)
/* ƥ��ؼ���"count" */
KEYWORD(exec_show_rookie_mac_count, exec_show_rookie_mac_count_eol, exec_show_rookie_mac_static,
    "count", " match count ", PRIVILEGE_USER)
/* ƥ��ؼ���"age-time" */
KEYWORD(exec_show_rookie_mac_agetime, exec_show_rookie_mac_agetime_eol, exec_show_rookie_mac_count,
    "age-time", " match age-time ", PRIVILEGE_USER)
/* ƥ��ؼ���"mac-address-table" */
KEYWORD(exec_show_rookie_mac_table, exec_show_rookie_mac_agetime, no_alt,
    "mac-address-table", " match mac-address-table ", PRIVILEGE_USER)
/* ƥ��ؼ���"rookie" */
KEYWORD(exec_show_rookie, exec_show_rookie_mac_table, ALTERNATE,
    "rookie", " match rookie ", PRIVILEGE_USER)

#undef  ALTERNATE
#define ALTERNATE   exec_show_rookie

#undef  ALTERNATE
#define ALTERNATE   no_alt

/* ���������� */
EOLWOS(exec_clear_rookie_mac_dynamic_eol, exec_clear_rookie_mac_dynamic_cmd)
EOLWOS(exec_clear_rookie_mac_vlan_num_eol, exec_clear_rookie_mac_vlan_num_cmd)
EOLWOS(exec_clear_rookie_mac_interface_num_eol, exec_clear_rookie_mac_interface_num_cmd)

NUMBER(exec_clear_rookie_mac_giga_num, exec_clear_rookie_mac_interface_num_eol, no_alt,
    "interface num", CDBVAR(int, FIRST_ARG), FIRST_ARG, PORT_NUM);
/* ƥ��ؼ���"inter" */
KEYWORD(exec_clear_rookie_mac_interface_num,exec_clear_rookie_mac_giga_num, no_alt,
    "gigabitEthernet"," match gigabitEthernet ", PRIVILEGE_USER) 
NUMBER(exec_clear_rookie_mac_vlan_num, exec_clear_rookie_mac_vlan_num_eol, no_alt,
    "vlan num", CDBVAR(int, FIRST_ARG), FIRST_ARG, VLAN_MAX_SIZE);
/* ƥ��ؼ���"interface" */
KEYWORD(exec_clear_rookie_mac_interface, exec_clear_rookie_mac_interface_num, no_alt,
    "interface", " match interface ", PRIVILEGE_USER)
/* ƥ��ؼ���"vlan" */
KEYWORD(exec_clear_rookie_mac_vlan, exec_clear_rookie_mac_vlan_num, exec_clear_rookie_mac_interface,
    "vlan", " match vlan ", PRIVILEGE_USER)
/* ƥ��ؼ���"dynamic" */
KEYWORD(exec_clear_rookie_mac_dynamic, exec_clear_rookie_mac_dynamic_eol, exec_clear_rookie_mac_vlan,
    "dynamic", " match dynamic ", PRIVILEGE_USER)
/* ƥ��ؼ���"mac-address-table" */
KEYWORD(exec_clear_rookie_mac, exec_clear_rookie_mac_dynamic, ALTERNATE,
    "mac-address-table", " match mac-address-table ", PRIVILEGE_USER)
/* ƥ��ؼ���"rookie" */
KEYWORD(exec_clear_rookie,exec_clear_rookie_mac, ALTERNATE,
    "rookie", " match rookie ", PRIVILEGE_USER)

#undef  ALTERNATE
#define ALTERNATE   exec_clear_rookie

#undef  ALTERNATE
#define ALTERNATE   no_alt

/* ���������� */
EOLWOS(cfg_rookie_mac_learn_eol, cfg_rookie_mac_learn_cmd)
EOLWOS(cfg_rookie_mac_table_agetime_num_eol, cfg_rookie_mac_table_agetime_num_cmd)
EOLWOS(cfg_rookie_mac_table_static_eol, cfg_rookie_mac_table_static__cmd)

NUMBER(cfg_rookie_mac_table_agetime_num, cfg_rookie_mac_table_agetime_num_eol, no_alt,
    "age-time", CDBVAR(int, FIRST_ARG), 0, 600);

/* ����giga num*/
NUMBER(cfg_rookie_mac_table_static_inter_giga_num, cfg_rookie_mac_table_static_eol, no_alt,
"giga num", CDBVAR(int, 3), FIRST_ARG, PORT_NUM);
/* ƥ��ؼ���"inter" */
KEYWORD(cfg_rookie_mac_table_static_inter_giga, cfg_rookie_mac_table_static_inter_giga_num, no_alt,
    "gigabitEthernet", " match gigabitEthernet ", PRIVILEGE_USER) 
/* ƥ��ؼ���"inter" */
KEYWORD(exec_clear_rookie_mac_inter, cfg_rookie_mac_table_static_inter_giga, no_alt,
    "interface", " match interface ", PRIVILEGE_USER)
/* ����vlan num*/
NUMBER(cfg_rookie_mac_table_static_vlan_num, exec_clear_rookie_mac_inter, no_alt,
"vlan num", CDBVAR(int, SECOND_ARG), FIRST_ARG, VLAN_MAX_SIZE);
/* ƥ��ؼ���"vlan" */
KEYWORD(cfg_rookie_mac_table_static_vlan, cfg_rookie_mac_table_static_vlan_num, no_alt,
    "vlan", " match vlan ", PRIVILEGE_USER) 
/* ����MAC��ַ*/    
MACADDR(cfg_rookie_mac_table_static_mac, cfg_rookie_mac_table_static_vlan, no_alt,
    "input the mac address ", CDBVAR(paddr,FIRST_ARG))
/* ƥ��ؼ���"static" */
KEYWORD(cfg_rookie_mac_table_static, cfg_rookie_mac_table_static_mac, no_alt,
    "static", " match static ", PRIVILEGE_USER) 
/* ƥ��ؼ���"age-time" */
KEYWORD(cfg_rookie_mac_table_agetime, cfg_rookie_mac_table_agetime_num, cfg_rookie_mac_table_static,
    "age-time", " match age-time ", PRIVILEGE_USER)
/* ƥ��ؼ���"mac-address-learing" */
KEYWORD(cfg_rookie_mac_table, cfg_rookie_mac_table_agetime, no_alt,
    "mac-address-table", " match mac-address-table ", PRIVILEGE_USER)
/*�����ַ���*/
TXT_LINE(cfg_rookie_mac_learn, cfg_rookie_mac_learn_eol, ALTERNATE, \
    "get enable or disable", CDBVAR(string,FIRST_ARG))
/* ƥ��ؼ���"mac-address-learing" */
KEYWORD(cfg_rookie_mac, cfg_rookie_mac_learn, cfg_rookie_mac_table,
    "mac-address-learing", " match mac-address-learing ", PRIVILEGE_USER)
/* ƥ��ؼ���"rookie" */
KEYWORD(cfg_rookie, cfg_rookie_mac, ALTERNATE, "rookie", " match rookie ", PRIVILEGE_USER)

#undef  ALTERNATE
#define ALTERNATE   cfg_rookie

#endif  /* _CLI_MAC_COMMAND_H_ */

