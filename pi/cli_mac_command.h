#ifndef _CLI_MAC_COMMAND_H_
#define _CLI_MAC_COMMAND_H_

#define FIRST_ARG        1
#define SECOND_ARG       2

#undef  ALTERNATE
#define ALTERNATE        no_alt

/* 显示mac表项的内容 */
extern void exec_show_rookie_mac_table_cmd(struct_command_data_block *pcdb);
/* 显示设置的老化时间 */
extern void exec_show_rookie_mac_agetime_cmd(struct_command_data_block *pcdb);
/* 显示mac条目数目 */
extern void exec_show_rookie_mac_count_cmd(struct_command_data_block *pcdb);
/* 显示静态路由 */
extern void exec_show_rookie_mac_static_cmd(struct_command_data_block *pcdb);
/* 显示指定vlan下的mac表信息 */
extern void exec_show_rookie_mac_vlan_num_cmd(struct_command_data_block *pcdb);
/* 清楚动态mac表项 */
extern void exec_clear_rookie_mac_dynamic_cmd(struct_command_data_block *pcdb);
/* 清除指定vlan下的表项 */
extern void exec_clear_rookie_mac_vlan_num_cmd(struct_command_data_block *pcdb);
/* 清除指定接口下的表项 */
extern void exec_clear_rookie_mac_interface_num_cmd(struct_command_data_block *pcdb);
/* 配置是否启动mac地址学习 */
extern void cfg_rookie_mac_learn_cmd(struct_command_data_block *pcdb);
/* 配置老化时间 */
extern void cfg_rookie_mac_table_agetime_num_cmd(struct_command_data_block *pcdb);
/* 配置静态mac */
extern void cfg_rookie_mac_table_static__cmd(struct_command_data_block *pcdb);

/* 声明count结束符 */
EOLWOS(exec_show_rookie_mac_count_eol, exec_show_rookie_mac_count_cmd)
/* 声明显示mac表项结束符 */
EOLWOS(exec_show_rookie_mac_table_eol, exec_show_rookie_mac_table_cmd)
/* 声明agetime结束符 */
EOLWOS(exec_show_rookie_mac_agetime_eol, exec_show_rookie_mac_agetime_cmd)
/* 声明static结束符 */
EOLWOS(exec_show_rookie_mac_static_eol, exec_show_rookie_mac_static_cmd)
/* 声明vlan num结束符 */
EOLWOS(exec_show_rookie_mac_vlan_num_eol, exec_show_rookie_mac_vlan_num_cmd)

NUMBER(exec_show_rookie_mac_vlan_num, exec_show_rookie_mac_vlan_num_eol, exec_show_rookie_mac_vlan_num_eol,
    "vlan num", CDBVAR(int, FIRST_ARG), FIRST_ARG, VLAN_MAX_SIZE);
/* 匹配关键字"vlan" */
KEYWORD(exec_show_rookie_mac_vlan, exec_show_rookie_mac_vlan_num, exec_show_rookie_mac_table_eol,
    "vlan", " match vlan ", PRIVILEGE_USER)
/* 匹配关键字"static" */
KEYWORD(exec_show_rookie_mac_static, exec_show_rookie_mac_static_eol, exec_show_rookie_mac_vlan,
    "static", " match static ", PRIVILEGE_USER)
/* 匹配关键字"count" */
KEYWORD(exec_show_rookie_mac_count, exec_show_rookie_mac_count_eol, exec_show_rookie_mac_static,
    "count", " match count ", PRIVILEGE_USER)
/* 匹配关键字"age-time" */
KEYWORD(exec_show_rookie_mac_agetime, exec_show_rookie_mac_agetime_eol, exec_show_rookie_mac_count,
    "age-time", " match age-time ", PRIVILEGE_USER)
/* 匹配关键字"mac-address-table" */
KEYWORD(exec_show_rookie_mac_table, exec_show_rookie_mac_agetime, no_alt,
    "mac-address-table", " match mac-address-table ", PRIVILEGE_USER)
/* 匹配关键字"rookie" */
KEYWORD(exec_show_rookie, exec_show_rookie_mac_table, ALTERNATE,
    "rookie", " match rookie ", PRIVILEGE_USER)

#undef  ALTERNATE
#define ALTERNATE   exec_show_rookie

#undef  ALTERNATE
#define ALTERNATE   no_alt

/* 声明结束符 */
EOLWOS(exec_clear_rookie_mac_dynamic_eol, exec_clear_rookie_mac_dynamic_cmd)
EOLWOS(exec_clear_rookie_mac_vlan_num_eol, exec_clear_rookie_mac_vlan_num_cmd)
EOLWOS(exec_clear_rookie_mac_interface_num_eol, exec_clear_rookie_mac_interface_num_cmd)

NUMBER(exec_clear_rookie_mac_giga_num, exec_clear_rookie_mac_interface_num_eol, no_alt,
    "interface num", CDBVAR(int, FIRST_ARG), FIRST_ARG, PORT_NUM);
/* 匹配关键字"inter" */
KEYWORD(exec_clear_rookie_mac_interface_num,exec_clear_rookie_mac_giga_num, no_alt,
    "gigabitEthernet"," match gigabitEthernet ", PRIVILEGE_USER) 
NUMBER(exec_clear_rookie_mac_vlan_num, exec_clear_rookie_mac_vlan_num_eol, no_alt,
    "vlan num", CDBVAR(int, FIRST_ARG), FIRST_ARG, VLAN_MAX_SIZE);
/* 匹配关键字"interface" */
KEYWORD(exec_clear_rookie_mac_interface, exec_clear_rookie_mac_interface_num, no_alt,
    "interface", " match interface ", PRIVILEGE_USER)
/* 匹配关键字"vlan" */
KEYWORD(exec_clear_rookie_mac_vlan, exec_clear_rookie_mac_vlan_num, exec_clear_rookie_mac_interface,
    "vlan", " match vlan ", PRIVILEGE_USER)
/* 匹配关键字"dynamic" */
KEYWORD(exec_clear_rookie_mac_dynamic, exec_clear_rookie_mac_dynamic_eol, exec_clear_rookie_mac_vlan,
    "dynamic", " match dynamic ", PRIVILEGE_USER)
/* 匹配关键字"mac-address-table" */
KEYWORD(exec_clear_rookie_mac, exec_clear_rookie_mac_dynamic, ALTERNATE,
    "mac-address-table", " match mac-address-table ", PRIVILEGE_USER)
/* 匹配关键字"rookie" */
KEYWORD(exec_clear_rookie,exec_clear_rookie_mac, ALTERNATE,
    "rookie", " match rookie ", PRIVILEGE_USER)

#undef  ALTERNATE
#define ALTERNATE   exec_clear_rookie

#undef  ALTERNATE
#define ALTERNATE   no_alt

/* 声明结束符 */
EOLWOS(cfg_rookie_mac_learn_eol, cfg_rookie_mac_learn_cmd)
EOLWOS(cfg_rookie_mac_table_agetime_num_eol, cfg_rookie_mac_table_agetime_num_cmd)
EOLWOS(cfg_rookie_mac_table_static_eol, cfg_rookie_mac_table_static__cmd)

NUMBER(cfg_rookie_mac_table_agetime_num, cfg_rookie_mac_table_agetime_num_eol, no_alt,
    "age-time", CDBVAR(int, FIRST_ARG), 0, 600);

/* 输入giga num*/
NUMBER(cfg_rookie_mac_table_static_inter_giga_num, cfg_rookie_mac_table_static_eol, no_alt,
"giga num", CDBVAR(int, 3), FIRST_ARG, PORT_NUM);
/* 匹配关键字"inter" */
KEYWORD(cfg_rookie_mac_table_static_inter_giga, cfg_rookie_mac_table_static_inter_giga_num, no_alt,
    "gigabitEthernet", " match gigabitEthernet ", PRIVILEGE_USER) 
/* 匹配关键字"inter" */
KEYWORD(exec_clear_rookie_mac_inter, cfg_rookie_mac_table_static_inter_giga, no_alt,
    "interface", " match interface ", PRIVILEGE_USER)
/* 输入vlan num*/
NUMBER(cfg_rookie_mac_table_static_vlan_num, exec_clear_rookie_mac_inter, no_alt,
"vlan num", CDBVAR(int, SECOND_ARG), FIRST_ARG, VLAN_MAX_SIZE);
/* 匹配关键字"vlan" */
KEYWORD(cfg_rookie_mac_table_static_vlan, cfg_rookie_mac_table_static_vlan_num, no_alt,
    "vlan", " match vlan ", PRIVILEGE_USER) 
/* 输入MAC地址*/    
MACADDR(cfg_rookie_mac_table_static_mac, cfg_rookie_mac_table_static_vlan, no_alt,
    "input the mac address ", CDBVAR(paddr,FIRST_ARG))
/* 匹配关键字"static" */
KEYWORD(cfg_rookie_mac_table_static, cfg_rookie_mac_table_static_mac, no_alt,
    "static", " match static ", PRIVILEGE_USER) 
/* 匹配关键字"age-time" */
KEYWORD(cfg_rookie_mac_table_agetime, cfg_rookie_mac_table_agetime_num, cfg_rookie_mac_table_static,
    "age-time", " match age-time ", PRIVILEGE_USER)
/* 匹配关键字"mac-address-learing" */
KEYWORD(cfg_rookie_mac_table, cfg_rookie_mac_table_agetime, no_alt,
    "mac-address-table", " match mac-address-table ", PRIVILEGE_USER)
/*输入字符串*/
TXT_LINE(cfg_rookie_mac_learn, cfg_rookie_mac_learn_eol, ALTERNATE, \
    "get enable or disable", CDBVAR(string,FIRST_ARG))
/* 匹配关键字"mac-address-learing" */
KEYWORD(cfg_rookie_mac, cfg_rookie_mac_learn, cfg_rookie_mac_table,
    "mac-address-learing", " match mac-address-learing ", PRIVILEGE_USER)
/* 匹配关键字"rookie" */
KEYWORD(cfg_rookie, cfg_rookie_mac, ALTERNATE, "rookie", " match rookie ", PRIVILEGE_USER)

#undef  ALTERNATE
#define ALTERNATE   cfg_rookie

#endif  /* _CLI_MAC_COMMAND_H_ */

