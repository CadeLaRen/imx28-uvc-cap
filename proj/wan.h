#ifndef __WAN_H__
#define __WAN_H__
#include "common.h"
#include "types.h"
#include "modem.h"

struct wan_cfg
{
	char name[16];
	char apn[16];
	char number[8];
	char user[32];
	char passwd[32];
	char timeout;
};

struct wan_link
{
	uint8 usim_state;	
	uint8 network_mode;	// bit7~bit4 运营商,bit3~bit1网络制式 
	uint8 quility;		// 0~31,其他值无效
	uint8 link_state;
	uint8 ip_addr[4];
	uint8 peer_addr[4];
}__attribute__((__packed__));

struct mdm_net {
	int p_pid;
	int pppdd_pid;
	int ppp_ctrl_open;
	struct ppp_ctrl *ctrl;
	struct wan_cfg cfg;
	struct modem_state mdm_state;
};

int wan_ifdown(struct mdm_net *mdm_cfg);
int wan_ifup(struct mdm_net *mdm_cfg);
int wan_ifcfg(struct mdm_net *mdm_cfg);
int wan_ifchk(struct mdm_net *mdm_cfg);
int __sys_wan_get_info(struct mdm_net *mdm_cfg, void *buf, int buflen);

#define WAN_UP             0
#define WAN_DOWN        1
#define WAN_CHK           2
#define GET_PID             3

struct ppp_ctrl {
	int s;
	struct sockaddr_un local;
	struct sockaddr_un dest;
};

struct ppp_ctrl * pppc_ctrl_open(char *ctrl_path);
void pppc_ctrl_close(struct ppp_ctrl *ctrl);
int pppc_ctrl_request(struct ppp_ctrl *ctrl, const char *cmd, size_t cmd_len,
		     char *reply, size_t *reply_len);

int pppc_cli_enable_wan(struct ppp_ctrl *ctrl);
int pppc_cli_disable_wan(struct ppp_ctrl *ctrl);
int pppc_cli_status_wan(struct ppp_ctrl *ctrl, struct modem_state *modem);
int pppc_cli_get_pid(struct ppp_ctrl *ctrl, pid_t *pid);
int pppdd_process_create(void);

#endif /* __WAN_H__ */

