#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "common.h"
#include "wan.h"


static size_t strlcpy(char *dest, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}

struct ppp_ctrl * pppc_ctrl_open(char *ctrl_path)
{
	struct ppp_ctrl *ctrl;
	static int counter = 0;
	int ret;
	size_t res;
	int tries = 0;

	ctrl = malloc(sizeof(*ctrl));
	if (ctrl == NULL)
		return NULL;
	memset(ctrl, 0, sizeof(*ctrl));

	ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (ctrl->s < 0) {
		free(ctrl);
		return NULL;
	}

	ctrl->local.sun_family = AF_UNIX;
	counter++;
try_again:
	ret = snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
			  "/tmp/ppp_ctrl_%d-%d", getpid(), counter);
	if (ret < 0 || (size_t) ret >= sizeof(ctrl->local.sun_path)) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}
	tries++;
	if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
			sizeof(ctrl->local)) < 0) {
		if (errno == EADDRINUSE && tries < 2) {
			/*
			 * getpid() returns unique identifier for this instance
			 * of wpa_ctrl, so the existing socket file must have
			 * been left by unclean termination of an earlier run.
			 * Remove the file and try again.
			 */
			unlink(ctrl->local.sun_path);
			goto try_again;
		}
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}

	ctrl->dest.sun_family = AF_UNIX;
	res = strlcpy(ctrl->dest.sun_path, ctrl_path,
			 sizeof(ctrl->dest.sun_path));
	if (res >= sizeof(ctrl->dest.sun_path)) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}
#if 1
	if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
			sizeof(ctrl->dest)) < 0) {
		close(ctrl->s);
		unlink(ctrl->local.sun_path);
		free(ctrl);
		fprintf(stderr, "connect failed\n");
		return NULL;
	}
#endif
	SYSLOG("------pppc ctrl open-------\n");
	return ctrl;
}

void pppc_ctrl_close(struct ppp_ctrl *ctrl)
{
	unlink(ctrl->local.sun_path);
	close(ctrl->s);
	free(ctrl);
	SYSLOG("------pppc ctrl close-------\n");
}

int pppc_ctrl_request(struct ppp_ctrl *ctrl, const char *cmd, size_t cmd_len,
		     char *reply, size_t *reply_len)
{
	struct timeval tv;
	int res;
	fd_set rfds;

	if (send(ctrl->s, cmd, cmd_len, 0) < 0) {
		return -1;
	}

	for (;;) {
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(ctrl->s, &rfds);
		res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
		if (FD_ISSET(ctrl->s, &rfds)) {
			res = recv(ctrl->s, reply, *reply_len, 0);
			if (res < 0)
				return res;
			*reply_len = res;
			break;
		} else {
			return -2;
		}
	}
	return 0;
}

int pppc_cli_enable_wan(struct ppp_ctrl *ctrl)
{
	uint8 cmd[1],reply[128];
	int32 cmd_len,reply_len = 128;
	
	cmd[0] = WAN_UP;
	cmd_len = 1;
	pppc_ctrl_request(ctrl, cmd, cmd_len, reply, &reply_len);
	return 0;
}

int pppc_cli_disable_wan(struct ppp_ctrl *ctrl)
{
	uint8 cmd[1],reply[128];
	int32 cmd_len,reply_len = 128;

	cmd[0] = WAN_DOWN;
	cmd_len = 1;
	pppc_ctrl_request(ctrl, cmd, cmd_len, reply, &reply_len);
	return 0;
}

int pppc_cli_status_wan(struct ppp_ctrl *ctrl, struct modem_state *modem)
{
	uint8 cmd[1],reply[128];
	int32 cmd_len,reply_len = 128;
	int res;

	cmd[0] = WAN_CHK;
	cmd_len = 1;
	res = pppc_ctrl_request(ctrl, cmd, cmd_len, reply, &reply_len);
	if(res != 0)
	{
		DBG("send cmd error : %d\n",res);
		return res;
	}
	if(reply_len != sizeof(struct modem_state))
	{
		DBG("length error : %d\n",reply_len);
		return 0;
	}
	memcpy(modem, reply, sizeof(struct modem_state));
	return sizeof(struct modem_state);
}

int pppc_cli_get_pid(struct ppp_ctrl *ctrl, pid_t *pid)
{
	uint8 cmd[1],reply[128];
	int32 cmd_len,reply_len = 128;
	int res;

	cmd[0] = GET_PID;
	cmd_len = 1;
	res = pppc_ctrl_request(ctrl, cmd, cmd_len, reply, &reply_len);
	if(res != 0)
	{
		DBG("send cmd error : %d\n",res);
		return res;
	}
	if(reply_len != sizeof(pid_t))
	{
		DBG("length error : %d\n",reply_len);
		return 0;
	}
	memcpy(pid, reply, sizeof(pid_t));
	return sizeof(struct modem_state);
}


int pppdd_process_create(void)
{
	FILE *fp;
	int ret = 0;
	char *shell = "/home/root/pppdd";
	fp = popen(shell, "r");
	if(fp != NULL)
		ret = 1;
	SYSLOG("------pppdd recreate-------\n");
	return ret;
}
#define PPPD_SERVER_SOCK_PATH        "/var/run/pppd.ctrl"
#define WAN_USER_SOCK_PATH             "/var/run/pppdc.ctrl"

static const char *nas_string[NASMODE_HYBRID+1] = 
{
	"NO SERVICE",
	"GSM",
	"GPRS",
	"EDGE",
	"WCDMA",
	"HSDPA(WCDMA only)",
	"HSUPA(WCMDA only)",
	"HSPA(WCDMA HSDPA+HSUPA)",
	"LTE",
	"TDS-CDMA",
	"HSDPA(TDSCMDA only)",
	"HSUPA(TDSCMDA only)",
	"HSPA(TDSCMDA only)",
	"CDMA2000 1X",
	"CDMA2000 1X EVDO",
	"CDMA2000 HYBRID",	// »ìºÏÄ£Ê½
};

int wan_ifup(struct mdm_net *mdm_cfg)
{	
	if(pidof("pppdd") <= 0)
	{
		fprintf(stderr, "pppdd recreate\n");
		pppdd_process_create();
		usleep(10000);
		if(mdm_cfg->ppp_ctrl_open == 1)
		{
			pppc_ctrl_close(mdm_cfg->ctrl);
			mdm_cfg->ppp_ctrl_open = 0;
		}
		return 0;
	}
	mdm_cfg->ctrl = pppc_ctrl_open(PPPD_SERVER_SOCK_PATH);
	if (mdm_cfg->ctrl == NULL)
	{
		fprintf(stderr, "pppc_ctrl_open failed\n");
		mdm_cfg->ppp_ctrl_open = 0;
		return 0;
	}
	mdm_cfg->ppp_ctrl_open = 1;
	pppc_cli_enable_wan(mdm_cfg->ctrl);
	return 1;
}
	
int wan_ifdown(struct mdm_net *mdm_cfg)
{
	if (mdm_cfg->ppp_ctrl_open == 0 || pidof("pppdd") <= 0)
		return 0;
	pppc_cli_disable_wan(mdm_cfg->ctrl);
	pppc_ctrl_close(mdm_cfg->ctrl);
	mdm_cfg->ppp_ctrl_open = 0;
	return 0;
}

// e.g, Add APN...
int wan_ifcfg(struct mdm_net *mdm_cfg)
{
	return 0;
}

// Check SIMCARD/NETWORK state
int wan_ifchk(struct mdm_net *mdm_cfg)
{
	if (mdm_cfg->ppp_ctrl_open == 0 || pidof("pppdd") <= 0)
	{
		wan_ifup(mdm_cfg);
		return 0;
	}
	if(pppc_cli_status_wan(mdm_cfg->ctrl, &(mdm_cfg->mdm_state)) < 0)
	{
		wan_ifdown(mdm_cfg);
		return 0;
	}
	DBG("SIM STATE=%d, SIM TYPE %d\n", mdm_cfg->mdm_state.usim_state, 
		mdm_cfg->mdm_state.usim_type);
	if(mdm_cfg->mdm_state.network_mode >= 0 && mdm_cfg->mdm_state.network_mode < 16)
		DBG("NET REG=%d, REGISTER NETWORK %s, QUAL %d\n", mdm_cfg->mdm_state.reg_network, 
			nas_string[mdm_cfg->mdm_state.network_mode], mdm_cfg->mdm_state.quility);
	else
		DBG("NET REG=%d, REGISTER NETWORK %d, QUAL %d\n", mdm_cfg->mdm_state.reg_network, 
			mdm_cfg->mdm_state.network_mode, mdm_cfg->mdm_state.quility);
	return 0;
}

int __sys_wan_get_info(struct mdm_net *mdm_cfg, void *buf, int buflen)
{
	struct wan_link *wl_lk;

	if (buflen < sizeof(struct wan_link))
	{
		return 0;
	}

	wl_lk = (struct wan_link *)buf;
	memset(wl_lk, 0x0, sizeof(struct wan_link));

	wl_lk->usim_state = (uint8)(mdm_cfg->mdm_state.usim_state);
	wl_lk->network_mode = (uint8)(mdm_cfg->mdm_state.usim_type << 4) |(mdm_cfg->mdm_state.network_mode);
	wl_lk->quility = (uint8)(mdm_cfg->mdm_state.quility);

	get_ip("ppp0", wl_lk->ip_addr);

	return sizeof(struct wan_link);
}

