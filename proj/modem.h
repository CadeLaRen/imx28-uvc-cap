#ifndef __QMI_IMSI_H__
#define __QMI_IMSI_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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

int modem_init(void);
void modem_exit(void);
int refresh_modem_status(struct modem_state *modem);

#endif /* __QMI_IMSI_H__ */
