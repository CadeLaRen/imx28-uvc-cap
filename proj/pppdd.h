#ifndef __PPPDD_H__
#define __PPPDD_H__

#include <sys/un.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <string.h>
#include <net/route.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#if 1
#define DBG(fmt, arg...) \
	do {fprintf(stderr, "DBG>> " fmt, ##arg);} while(0)

#define DBG_ENTER()		do{fprintf(stderr, "DBG>> %s: Enter\n", __func__);}while(0)
#define DBG_EXIT()		do{fprintf(stderr, "DBG>> %s: Exit\n", __func__);}while(0)
#else
#define DBG(fmt, arg...)   do{;}while(0)
#define DBG_ENTER()   do{;}while(0)
#define DBG_EXIT()   do{;}while(0)
#endif

#define WAN_UP             0
#define WAN_DOWN        1
#define WAN_CHK           2

// USIM 制式,根据IMSI中MNC字段区别
typedef enum
{
	USIM_DETECT_FAILED = -2,
	USIM_UNDEFINED = -1,
	USIM_NO_DETECT = 0,
	USIM_UNION,		// 中国联通,MNC = 01,06
	USIM_MOBILE,	// 中国移动,MNC = 00,02,07
	USIM_TELE,		// 中国电信,MNC = 03,05,11
	USIM_TIETONG,	// 中国铁通,MNC = 20
}usim_type;

// 网络制式
typedef enum
{
	NASMODE_NO_SERVICE = 0,
	NASMODE_GSM,
	NASMODE_GPRS,
	NASMODE_EDGE,
	NASMODE_WCDMA,
	NASMODE_WCDMA_HSDPA,
	NASMODE_WCDMA_HSUPA,
	NASMODE_WCDMA_HSPA,
	NASMODE_LTE,
	NASMODE_TDS_CDMA,
	NASMODE_TDS_HSDPA,
	NASMODE_TDS_HSUPA,
	NASMODE_TDS_HSPA,
	NASMODE_CDMA,	// 13
	NASMODE_EVDO,
	NASMODE_HYBRID,	// 15, cdma and evdo
	
	NASMODE_NO_VALID = 2147483647,	//
}network_mode;

// 获取信息失败
#define TE_STATUS_ERROR	-1
// 未注册,且并未寻找新的运营商网络
#define TE_NOT_REGISTER_AND_NOT_REGTO	0
// 已注册本地网络
#define TE_ALREADY_REGISTER	1
// 未注册,且尝试寻找新的运营商网络
#define TE_NOT_REGISTER_AND_REGTO	2
// 运营商网络拒绝
#define TE_REGISTER_DENIED	3
// 未知状态
#define TE_UNKNOWN	4
// 已注册, 漫游状态
#define TE_REGISTER_AND_ROAMING	5

// Modem状态信息
struct modem_state {
	/**<   UIM state. Values: \n
  - 0x00 -- UIM initialization completed \n
  - 0x01 -- UIM is locked or the UIM failed \n
  - 0x02 -- UIM is not present \n
  - 0x03 -- Reserved \n
  - 0xFF -- UIM state is currently unavailable 
  */
	int usim_state;
	int reg_network;
	// USIM卡类型
	usim_type usim_type;
	// 网络访问运行商(区分2G/3G/4G)
	network_mode network_mode;
	// 连接质量
	int quility;
};

// PPP连接配置
struct wan_cfg
{
	char name[16];
	char apn[16];
	char number[8];
	char user[32];
	char passwd[32];
	char timeout;
};

// ppp连接信息
struct wan_link
{
	  /**<	 UIM state. Values: \n
	- 0x00 -- UIM initialization completed \n
	- 0x01 -- UIM is locked or the UIM failed \n
	- 0x02 -- UIM is not present \n
	- 0x03 -- Reserved \n
	- 0xFF -- UIM state is currently unavailable 
	*/
	char usim_state;	
	char network_mode;	// bit7~bit4 运营商,bit3~bit1网络制式 
	char quility;		// 0~31,其他值无效
	char link_state;
	char ip_addr[4];
	char peer_addr[4];
}__attribute__((__packed__));

enum PPPD_REQ
{
	MODEM_STATUS = 1,	// 获取MODEM状态 SIM卡,网络,信号强度		
	PPPD_STATUS,		// PPPD拨号状态
	PPPD_REQ_CFG,		// 获取当前拨号参数
	PPPD_SET_CFG,		// 设置新的拨号参数
	PPPD_ENABLE,		// 使能PPP拨号
	PPPD_DISABLE,		// 禁止PPP拨号
	NETWORK_INFO,		// 当前网络状态(ip, peer_ip...)
};

struct pppd_request
{
	int req;
	int req_len;
	char req_data[0];
}__attribute__((__packed__));

struct pppd_respone
{
	int res;
	int res_len;
	char res_data[0];
}__attribute__((__packed__));

#endif /* __WAN_H__ */

