#include "nvram.h"
#include "params.h"
#include "common.h"

static struct nvram_tuple defaults[] = {
    {"wan_enable", "enable", 0},
    {"wan_ipaddr", "0.0.0.0", 0},   		/* WAN IP address */
    {"wan_netmask", "0.0.0.0", 0},  		/* WAN netmask */
    {"wan_gateway", "0.0.0.0", 0},  		/* WAN gateway */
    {"wan_dns", "", 0},     			/* x.x.x.x x.x.x.x ... */
    {"wan_proto", "3g", 0},      		/* [static|dhcp|pppoe|disabled][3g]*/
    {"wan_modem", "/dev/ttyUSB2", 0},
    {"wan_atcmd", "/dev/ttyUSB3", 0},
    {"wan_apn","cmnet",0},
    {"wan_dial","0",0},
    {"mtu_enable","0",0},
    {"wan_mtu","0",0},
    {"wan_mru","0",0},
    {"ppp_debug","1",0},
    //{"ppp_username", "ctnet@mycdma.cn", 0},	/* PPP username */
    //{"ppp_passwd", "vnet.mobi", 0},			/* PPP password */
    {"ppp_username", "", 0},	/* PPP username */
    {"ppp_passwd", "", 0},			/* PPP password */

    {"ppp_idletime", "5", 0},			/* Dial on demand max idle time (mins) */
    {"ppp_keepalive", "0", 0},			/* Restore link automatically */
    {"ppp_demand", "0", 0},			/* Dial on demand */
    {"ppp_redialperiod", "30", 0},		/* Redial Period (seconds) */

};

static void params_commit(struct nvram_tuple cfg[], int n)
{
	int i;

	for (i = 0; i < n; i++) {
		if (cfg[i].name != NULL)
			nvram_set(cfg[i].name, cfg[i].value);
		fprintf(stderr, "cfg>>name:%s,value:%s\n", cfg[i].name, cfg[i].value);
	}

	nvram_commit();
}

int params_load(void)
{
	int fd;
	int i;

	DBG_ENTER();
	// if nvram.tmp no found,create it
	if (nvram_find_staging() == NULL)
	{
		if (nvram_find_nvram() == NULL)
		{
			fd = open(NVRAM_MTD, O_RDWR|O_CREAT, 0666);
			if (fd < 0) {
				fprintf(stderr, "%s: open %s devices failed, error=%d\n", __func__, NVRAM_MTD, errno);
				return -1;
			}

			lseek(fd, 0x10000, SEEK_SET);
			write(fd, "0", 1);
			close(fd);
			fprintf(stderr, "%s: create params...\n", __func__);

			for (i = 0; i <sizeof(defaults)/sizeof(defaults[0]); i++)
			{
				if (defaults[i].name != NULL)
					nvram_set(defaults[i].name, defaults[i].value);
			}
			nvram_commit();
		}
		else
		{
			nvram_to_staging();
		}
	}

	nvram_open();

	return 0;
}
