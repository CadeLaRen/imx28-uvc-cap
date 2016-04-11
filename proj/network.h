#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "common.h"
#include "wlan.h"
#include "lan.h"
#include "wan.h"

#define NET_IF_NONE	0x0
#define NET_IF_WLAN	0x1
#define NET_IF_LAN	0x2
#define NET_IF_WAN	0x4

#define NET_IF_ALL	\
	(NET_IF_WLAN|NET_IF_WAN|NET_IF_LAN)

#define WLAN_IF_POLL_CYCLE	1	 // 1s
#define WAN_IF_POLL_CYCLE	3	 // 3s
#define LAN_IF_POLL_CTCLE	1	 // 1s

struct net_cfg {
	// ��ǰ�ӿ�:�ӿ����ȼ� eth > wlan > modem
	int cur_netif;
	int netif;
	int event;

	// ��̫����Ϣ
	struct eth_net eth;
	
	// modem��Ϣ
	struct mdm_net mdm;

	// wlan��Ϣ
	struct wlan_net wlan;
};

extern struct net_cfg net_cfg;

int net_ifcfg(int net_if);

void start_services(void);
void check_services(void);
void stop_services(void);


#endif
