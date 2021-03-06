#include<unistd.h>
#include <rg_ss/public/ss_errno.h>
#include <rg_ss/lib/libpub/ss_comm.h>
#include <rg_ss/lib/libpub/ssc_comm_phyid.h>
#include <rg_ss/lib/libpub/ss_msg_notify.h>
#include <rg_ss/lib/libpub/ss_bitmap.h>
#include <rg_ss/lib/libpub/ss_msg_com.h>

#include "../include/ss_public.h"
#include "../include/ssc_comm_pi.h"

/* ssc mac相关的初始化 */
void ssc_mac_rookie_init(void)
{
    ssc_comm_init();
    ssc_mac_opera_init();
    ssc_db_init();
    /* SSD事件处理函数注册 */
    (void)ss_msg_register_handler(SSD_MSGID_MAC_UPDATE, ssc_mac_update_recv);
}

