#include "common.h"
#include "nvram.h"
#include "params.h"
#include "network.h"

struct net_cfg net_cfg;

void net_ip_forward(int val)
{
	int v = val ? 1 : 0;

	sysprintf("echo %d > /proc/sys/net/ipv4/ip_forward", v);
	// iptable -t nat -A POSTROUTING -o ppp0 -s IP -j SNAT --to PPP_IP
}

static void network_params_init(void)
{
	char *net;
	int net_if;

	memset(&net_cfg, 0, sizeof(struct net_cfg));

	// 预加载所有网络信息
	net = NVRAM_SAFE_GET("net_if");
	if (net != NULL)
		net_if = atoi(net);
	else
		net_if = NET_IF_NONE;

	if (net_if < 0 || net_if > 7)
		net_if = NET_IF_NONE;
	DBG("net_if = %d\n", net_if);
	net_cfg.netif = net_if;
	net_cfg.cur_netif = net_if;
}

static int get_default_route(void)
{
	char msg[64];
	char dev[16];
	char *route;
	FILE *fpin;
	int res;
	
	memset(dev , 0 ,16);
	if((fpin = popen("ip route show|grep default","r")) == NULL)
		return -1;
	while(fgets(msg, sizeof(msg), fpin) != NULL)
	{
		if((route = strstr(msg,"dev")) != NULL)
		{
			sscanf(route+4,"%[^ ]",dev);
			break;
		}
	}
	pclose(fpin);
	
	if(!strcmp(dev, "ppp0"))
		res = NET_IF_WAN;
	else if(!strcmp(dev, "wlan0"))
		res = NET_IF_WLAN;
	else if(!strcmp(dev, "eth0"))
		res = NET_IF_LAN;
	else
		res = NET_IF_NONE;
	return res;
}

static void net_ifup(void)
{
	int net_if;
	char *net;
	int time;
	DBG_ENTER();

	net_if = net_cfg.netif;
	if (net_if & NET_IF_WAN)
	{
		time = get_tick_1khz();
		wan_ifup(&net_cfg.mdm);
		DBG("wan_ifup time[%d]\n", get_tick_1khz()-time);
	}

	if (net_if & NET_IF_WLAN)
	{
		time = get_tick_1khz();
		wlan_ifup(&net_cfg.wlan);
		DBG("wlan_ifup time[%d]\n", get_tick_1khz()-time);
	}

	if (net_if & NET_IF_LAN)
	{
		time = get_tick_1khz();
		lan_ifup(&net_cfg.eth);
		DBG("lan_ifup time[%d]\n", get_tick_1khz()-time);
	}

	DBG_EXIT();
}

static void net_ifdown(void)
{
	int net_if = net_cfg.netif;
	DBG_ENTER();
	
	// NET_PPP
	if (net_if & NET_IF_WAN)
	{
		wan_ifdown(&net_cfg.mdm);
	}
	// NET_WIFI
	if (net_if & NET_IF_WLAN)
	{
		wlan_ifdown(&net_cfg.wlan);
	}
	// NET_ETHERNET
	if (net_if & NET_IF_LAN)
	{
		lan_ifdown(&net_cfg.eth);
	}
	
	DBG_EXIT();
}

int net_ifcfg(int net_if)
{
	int netif = net_cfg.netif;
	char gw[16];
	char net[2] = {0 , 0};
	DBG_ENTER();

	netif ^= net_if; // 判断哪种网络改变

	if (netif & NET_IF_WAN)
	{
		if (net_if & NET_IF_WAN)
			wan_ifup(&net_cfg.mdm);
		else	
		{
			wan_ifdown(&net_cfg.mdm);
			net_cfg.cur_netif = NET_IF_NONE;
		}
	}

	if (netif & NET_IF_WLAN)
	{
		if (net_if & NET_IF_WLAN)
			wlan_ifup(&net_cfg.wlan);
		else
		{
			if(net_if & NET_IF_WAN)
				eval("route", "add", "default","dev", "ppp0");
			wlan_ifdown(&net_cfg.wlan);
		}
	}

	if (netif & NET_IF_LAN)
	{
		if (net_if & NET_IF_LAN)
			lan_ifup(&net_cfg.eth);			
		else
		{
			lan_ifdown(&net_cfg.eth);
		}
	}

	net_cfg.netif = net_if;
	
	net[0] = 0x30 + (char)net_if;
	NVRAM_SET("net_if", &net[0]);
	NVRAM_COMMIT();
	DBG_EXIT();
	return 0;
}

static int set_route(int net_if)
{
	char gw[16];
	
	//若以太网连接，路由至以太网(最高优先)	
	if(net_if & NET_IF_LAN)
	{
		if(net_cfg.cur_netif != NET_IF_LAN)
		{
			DBG("-----route to lan!\n");
			net_cfg.cur_netif = NET_IF_LAN;
			goto check_route;
		}
		else 
			goto check_route;
	}
	//若以太网未连接且wifi 启动，尝试路由至wifi 网关(第二优先)
	if(net_if & NET_IF_WLAN && net_cfg.cur_netif != NET_IF_LAN)
	{
		//wifi 连接断开，修改路由状态标记
		if(net_cfg.wlan.dhcp_pid <= 0 && net_cfg.cur_netif == NET_IF_WLAN)
		{
			net_cfg.cur_netif = NET_IF_NONE;
		}
		//尝试路由至wifi 网关
		if(net_cfg.cur_netif != NET_IF_WLAN)
		{
			if(net_cfg.wlan.dhcp_pid > 0)//确定DHCP已启动
			{
				DBG("-----route to wlan!\n");
				eval("route", "del", "default" , "dev", "ppp0");
				net_cfg.cur_netif = NET_IF_WLAN;
				goto check_route;
			}
		}
		else
			goto check_route;
		
	}
	//若以太网和WIFI均未开启、未连接或断开，选择4G(最低优先)
	if(net_if & NET_IF_WAN)
	{
		if(net_cfg.cur_netif != NET_IF_WAN )
		{
			DBG("-----route to wan!\n");
			eval("route", "del", "default" , "dev", "wlan0");
			net_cfg.cur_netif = NET_IF_WAN;
		}
		
	}
	if(net_if == NET_IF_NONE)
	{
		net_cfg.cur_netif = NET_IF_NONE;
	}

check_route:
	if(net_cfg.cur_netif != get_default_route())
	{
		if(net_cfg.cur_netif == NET_IF_WAN)
			eval("route", "add", "default","dev", "ppp0");
		else if(net_cfg.cur_netif == NET_IF_WLAN)
		{
			sprintf(gw, "%d.%d.%d.%d",
				net_cfg.wlan.info.gw[0], net_cfg.wlan.info.gw[1],
				net_cfg.wlan.info.gw[2], net_cfg.wlan.info.gw[3]);
			eval("route", "add", "default","gw", gw,"dev", net_cfg.wlan.intf);
		}
		else if(net_cfg.cur_netif == NET_IF_LAN)
		{
			sprintf(gw, "%d.%d.%d.%d",
				net_cfg.eth.info.gw[0], net_cfg.eth.info.gw[1],
				net_cfg.eth.info.gw[2], net_cfg.eth.info.gw[3]);
			eval("route", "add", "default","gw", gw,"dev", net_cfg.eth.intf);
		}
	}
	return 0;
}

static int check_network(void)
{
	int ret = -1;
	int net_if = net_cfg.netif;
	int tick;
	static int wlan_tick = 0;
	static int lan_tick = 0;
	static int wan_tick = 0;

	tick = get_tick_1hz();
	if (net_if & NET_IF_WAN
		&& tick - wan_tick >= WAN_IF_POLL_CYCLE)
	{
		// if choose modem,	check ppp
		wan_ifchk(&net_cfg.mdm);
		wan_tick = tick;
	}
	if (net_if & NET_IF_WLAN
		&& tick - wlan_tick >= WLAN_IF_POLL_CYCLE)
	{
		// if choose wifi, check wifi
		wlan_ifchk(&net_cfg.wlan);
		wlan_tick = tick;
	}
	if (net_if & NET_IF_LAN
		&& tick - lan_tick >= LAN_IF_POLL_CTCLE)
	{
		// if choose ethernet, check lan, rate, link...
		lan_ifchk(&net_cfg.eth);
		lan_tick = tick;
	}

	set_route(net_if);

	return ret;
}

void start_services(void)
{
	// 初始化设备连接接口及信号
	network_params_init();
	net_ifup();
	
	// 初始化定时器等
}
void check_services(void)
{
	int ret;
	// 检查网络连接状态
	ret = check_network();
	
	// 检查设备状态,JWSOC/FPGA
}

void stop_services(void)
{
	int ret = -1;

	// 向设备发送停止服务处理

	// 断开网络连接
	net_ifdown();

	// 保存当前参数
	NVRAM_COMMIT();

}

