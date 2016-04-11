#include "pppdd.h"

#define PPPD_DAEMON_SERVER_SOCK_PATH	"/var/run/pppd.ctrl"

static struct modem_state modem;
static int server_start = 1;

struct wwan_ctrl
{
	int fd;
	int sock;
};

static void hex_dump(const char *prompt, unsigned char *buf, int len)
{
	int i = 0;
	
	for (i = 0; i < len; i++) {
		if ((i & 0x0f) == 0) {
			printf("\n%s | 0x%04x", prompt, i);
		}
		printf(" %02x", *buf);
		buf++;
	}
	printf("\n");
}

int system_call(const char *cmd, unsigned int cmdlen)
{
	int result;
	FILE *stream;
	unsigned int vallen = strlen(cmd);

	if (vallen != cmdlen)
	{
		return -1;
	}

	stream = popen(cmd, "w");
	if (stream == NULL)
	{
		DBG("open process pppd failed : %s", cmd);
		return -1;
	}

	pclose(stream);

	return 0;
}

int f_read(const char *path, void *buffer, int max)
{
        int f;
        int n;

        if ((f = open(path, O_RDONLY)) < 0)
                return -1;
        n = read(f, buffer, max);
        close(f);
        return n;
}

int f_read_string(const char *path, char *buffer, int max)
{
        if (max <= 0)
                return -1;
        int n = f_read(path, buffer, max - 1);

        buffer[(n > 0) ? n : 0] = 0;
        return n;
}


char *psname(int pid, char *buffer, int maxlen)
{
        char buf[512];
        char path[64];
        char *p;

        if (maxlen <= 0)
                return NULL;
        *buffer = 0;
        sprintf(path, "/proc/%d/stat", pid);
        if ((f_read_string(path, buf, sizeof(buf)) > 4)
            && ((p = strrchr(buf, ')')) != NULL)) {
                *p = 0;
                if (((p = strchr(buf, '(')) != NULL) && (atoi(buf) == pid)) {
                        strncpy(buffer, p + 1, maxlen);
                }
        }
        return buffer;
}


static int _pidof(const char *name, pid_t ** pids)
{
        const char *p;
        char *e;
        DIR *dir;
        struct dirent *de;
        pid_t i;
        int count;
        char buf[256];

        count = 0;
        *pids = NULL;
        if ((p = strchr(name, '/')) != NULL)
                name = p + 1;
        if ((dir = opendir("/proc")) != NULL) {
                while ((de = readdir(dir)) != NULL) {
                        i = strtol(de->d_name, &e, 10);
                        if (*e != 0)
                                continue;
                        if (strcmp(name, psname(i, buf, sizeof(buf))) == 0) {
                                if ((*pids =
                                     realloc(*pids,
                                             sizeof(pid_t) * (count + 1))) ==
                                    NULL) {
                                        return -1;
                                }
                                (*pids)[count++] = i;
                        }
                }
        }
        closedir(dir);
        return count;
}


int pidof(const char *name)
{
        pid_t *pids;
        pid_t p;

        if (_pidof(name, &pids) > 0) {
                p = *pids;
                free(pids);
                return p;
        }
        return -1;
}


int killall(const char *name, int sig)
{
        pid_t *pids;
        int i;
        int r;

        if ((i = _pidof(name, &pids)) > 0) {
                r = 0;
                do {
                        r |= kill(pids[--i], sig);
                }
                while (i > 0);
                free(pids);
                return r;
        }
        return -2;
}

int kill_pid(int pid)
{
	int deadcnt = 20;
	struct stat s;
	char pfile[32];
	
	if (pid > 0)
	{
		kill(pid, SIGTERM);

		sprintf(pfile, "/proc/%d/stat", pid);
		while (deadcnt--)
		{
			usleep(100*1000);
			if ((stat(pfile, &s) > -1) && (s.st_mode & S_IFREG))
			{
				kill(pid, SIGKILL);
			}
			else
				break;
		}
		return 1;
	}
	
	return 0;
}

int stop_process(char *name)
{
	int deadcounter = 20;

        if (pidof(name) > 0) {
                killall(name, SIGTERM);
                while (pidof(name) > 0 && deadcounter--) {
                        usleep(100*1000);
                }
                if (pidof(name) > 0) {
                        killall(name, SIGKILL);
                }
                return 1;
        }
        return 0;
}

static pid_t ppp_dial(usim_type usim, network_mode net_regs)
{
	FILE *fp;
	pid_t pid;
	char *options = "/etc/ppp/peers/options.ppp";
	char *cmd = "pppd call options.ppp &";

	unlink(options);

	if (net_regs == NASMODE_LTE)
	{
		// LTE
		symlink("/etc/ppp/peers/lte", options);
	}
	else
	{
		if (usim == USIM_MOBILE)
		{
			// TDS-CDMA GSM/GPRS
			symlink("/etc/ppp/peers/wcdma-tds", options);
		}
		else if (usim == USIM_UNION)
		{
			// WCDMA GSM
			symlink("/etc/ppp/peers/wcdma-tds", options);
		}
		else if (usim == USIM_TELE)
		{
			//CDMA2000 EVDO/1X
			symlink("/etc/ppp/peers/cdma", options);
		}
		else
		{
			// default eg: wandafo 20404
			symlink("/etc/ppp/peers/lte", options);
		}

	}

	// nonblcok process
	pid = system_call(cmd, strlen(cmd));
	fprintf(stderr, "pppd start.pid[%d]: %d\n", pid,pidof("pppd"));

	return pidof("pppd");
}

static void wan_start_server(struct modem_state *modem)
{
	DBG("%s:Enter\n", __func__);
	if(server_start == 0)
		return;
	// 1.check modem status
	if (modem->usim_state > 0
		|| modem->network_mode == NASMODE_NO_SERVICE)
	{
		DBG("STATE: %d  MODE: %d\n",modem->usim_state,modem->network_mode);
		return;
	}

	// 2.start pppd process daemon
	ppp_dial(modem->usim_type, modem->network_mode);
}

static void wan_stop_server(struct modem_state *modem)
{
	// 1.stop pppd process if present
	if (pidof("pppd") > 0)
	{
		stop_process("pppd");
	}

	modem->reg_network = 0;
	modem->quility = 0;
	modem->network_mode = 0;
	// USIM status
}

static void wan_restart_server(struct modem_state *modem, struct modem_state *state)
{

	// check sim type, check network type
	// 1.check modem status
	if (state->usim_state != 0
		|| state->network_mode == NASMODE_NO_SERVICE
		|| state->quility < 10)
	{
		return;
	}
	
	// 2.stop ppp-server
	if (state->network_mode == NASMODE_LTE ||
		modem->network_mode == NASMODE_LTE)
	{
		wan_stop_server(modem);
	
		// 3.start pppd process daemon
		wan_start_server(state);
	}
	else if (pidof("pppd") <= 0)
	{
		wan_start_server(state);
	}
}

static int pppd_server_socket_init(void)
{
	int s;
	struct sockaddr_un addr;
	DBG_ENTER();
	s = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (s < 0)
	{
		perror("pppd server socket failed");
		return -1;
	}

	bzero(&addr, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, PPPD_DAEMON_SERVER_SOCK_PATH, sizeof(addr.sun_path) - 1);
	unlink(PPPD_DAEMON_SERVER_SOCK_PATH);

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("pppd bind failed");
		close(s);
		return -2;
	}

	chmod(PPPD_DAEMON_SERVER_SOCK_PATH, S_IRWXU | S_IRWXG);
	/* server_start = 0; */
	DBG_EXIT();
	return s;
}

static void pppd_server_socket_close(int sock)
{

	close(sock);
	unlink(PPPD_DAEMON_SERVER_SOCK_PATH);
}

static int pppd_message_respone(char *req, int req_len, char *respone, int *res_len)
{
	int ret;
}

static void *pppd_message_thread(void *sock)
{
	int sockfd = *((int *)sock);
	char *msg = NULL;
	int msg_size = 2048;
	struct sockaddr_un addr;
	socklen_t socklen;
	int rbytes;
	DBG_ENTER();
	msg = (char *)malloc(msg_size);
	if (msg == NULL)
	{
		perror("msg malloc");
		exit(EXIT_FAILURE);
	}
	socklen = sizeof(struct sockaddr_un);
	while((rbytes = recvfrom(sockfd, msg, msg_size, 0,
						(struct sockaddr *)&addr, &socklen)) != 0)
	{
		pthread_testcancel();
		fprintf(stderr, "recv from %s\n", addr.sun_path);

		if (rbytes > 0)
		{
			hex_dump("recv msg", msg, rbytes);
			switch(msg[0])
			{
				case WAN_UP:
					server_start = 1;
					if(sendto(sockfd, msg, rbytes, 0, 
							(struct sockaddr *)&addr, 
							sizeof(struct sockaddr_un)) == -1) {
						perror("sendto");
					}
					break;
				case WAN_DOWN:
					server_start = 0;
					msg[2] = 8;
					if(sendto(sockfd, msg, rbytes, 0, 
							(struct sockaddr *)&addr, 
							sizeof(struct sockaddr_un)) == -1) {
						perror("sendto");
					}
					break;
				case WAN_CHK:
					fprintf(stderr, "send to %s\n", addr.sun_path);
					if(sendto(sockfd, &modem, sizeof(modem), 0, 
							(struct sockaddr *)&addr, 
							sizeof(struct sockaddr_un)) == -1) {
						perror("sendto");
					}
					break;
				default:
					break;
			}
		}
		else
		{
			perror("recvfrom");
		}
	}
	free(msg);
	pthread_exit((void *)0);
}

static int pppd_pthread_start(void *sock)
{
	pthread_attr_t pppd_attr;
	pthread_t pppd_t;
	int ret;
	DBG_ENTER();
	ret = pthread_attr_init(&pppd_attr);
	if (ret < 0)
	{
		perror("pppd pthread init failed");
		return ret;
	}

	ret = pthread_create(&pppd_t, &pppd_attr, &pppd_message_thread, sock);
	if (ret < 0) {
		perror("pppd pthread_create failed");
		pthread_attr_destroy(&pppd_attr);
	}
	//pthread_join(pppd_t, NULL);
	DBG_EXIT();
	return ret;
}


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
	"CDMA2000 HYBRID",	// 混合模式
};

static usim_type usim_imsi_parse(char *imsi, int imsi_len)
{
	unsigned int MCC = 0;
	unsigned int MNC	= 0;
	usim_type usim = USIM_UNDEFINED;
	
	if (imsi_len < 15)
		return USIM_NO_DETECT;
	
	fprintf(stderr, "%s: %s\n", __func__, imsi);
	// check MCC, china, 460
	MCC = (imsi[0] - 0x30) * 100;;
	MCC += (imsi[1] - 0x30) * 10;
	MCC += (imsi[2] - 0x30);

	// MNC
	MNC = (imsi[3] - 0x30) * 10;
	MNC += imsi[4] - 0x30;

	if (MCC == 460)
	{
		switch (MNC)
		{
			case 0:
			case 2:
			case 7:
				usim = USIM_MOBILE;
				break;
			case 1:
			case 6:
				usim = USIM_UNION;
				break;
			case 3:
			case 5:
			case 11:
				usim = USIM_TELE;
				break;
			case 20:
				usim = USIM_TIETONG;
				break;
			default:break;
		}
	}
	else if (MCC == 204)
	{
		// telecom used wandafo WCDMA
		usim = USIM_TELE;
	}
	else
	{
		usim = USIM_UNDEFINED;
	}

	fprintf(stderr, "MCC = %d, MNC = %d\n", MCC, MNC);
	
	return usim;
}

static int get_info_from_at_port(int fd, char *cmd, char *resp, int resp_len)
{
	int ret, rc = 0;
	fd_set rfds;
	struct timeval to;

	ret = write(fd, cmd, strlen(cmd));
	if (ret < strlen(cmd))
	{
		return 0;
	}
	
	do {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		to.tv_sec  = 0;
		to.tv_usec = 5000;	//5ms
		ret = select(fd+1, &rfds, NULL, NULL, &to);
		if (ret > 0 && FD_ISSET(fd, &rfds))
		{
			rc += read(fd, resp+rc, resp_len-rc);
			//DBG("%s:read success %d\n", __func__,rc);
		}
		else if (ret == 0)
		{
			//DBG("%s:read timeout...\n", __func__);
			break;
		}
		else
		{			
			//DBG("%s:read error...\n", __func__);
			return rc;
		}
	} while(rc < resp_len);

	fprintf(stderr, "AT RESP: %s\n", resp);
	return rc;
}

static network_mode get_nas_mode(int fd)
{
	char *cmd = "at+cnsmod?\r\n";	//at+cpsi?/at*cnti?
	char resp[64];
	char *end;
	network_mode mode;

	memset(resp, 0x0, sizeof(resp));
	if (get_info_from_at_port(fd, cmd, resp, sizeof(resp)- 1) <= 0)
		return NASMODE_NO_VALID;

	end = strstr(resp, "+CNSMOD");
	if (end == 0)
	{
		//DBG("NOT FOUND CNSMOD\n");
		return NASMODE_NO_VALID;
	}
	
	end = strstr(resp, ",");
	if (end == NULL)
	{
		//DBG("NOT FOUND ,\n");
		return NASMODE_NO_VALID;
	}

	end++;
	mode = atoi(end);

	return mode;
}

static int get_reg_network(int fd)
{
	char *cmd = "at+creg?\r\n";
	char resp[32];
	char *end;
	
	memset(resp, 0x0, sizeof(resp));
	if (get_info_from_at_port(fd, cmd, resp, sizeof(resp)) <= 0)
	{
		return TE_STATUS_ERROR;
	}
	if (strstr(resp, "+CREG:") == NULL)
		return TE_UNKNOWN;

	end = strstr(resp, ",");
	if (end == NULL)
		return TE_UNKNOWN;
	end++;
	
	return atoi(end);
}

static usim_type get_usim_imsi(int fd, struct modem_state *modem)
{
	char *cmd = "at+cimi\r\n";
	char resp[32];
	char *end;
	usim_type type;
	
	memset(resp, 0x0, sizeof(resp));
	if (get_info_from_at_port(fd, cmd, resp, sizeof(resp) - 1) <= 0)
	{
		modem->usim_state = 1;
		return -1;
	}

	end = strstr(resp, "460");
	if (end == NULL) {
		modem->usim_state = 1;
		return -1;
	} 

	type = usim_imsi_parse(end, strlen(end));
	switch (type)
	{

		case USIM_DETECT_FAILED:
		case USIM_UNDEFINED:
			modem->usim_state = 0x01;// UIM is locked or the UIM failed.
			break;
		case USIM_NO_DETECT:
			modem->usim_state = 0x02;// UIM is not present.
			break;
		case USIM_UNION:
		case USIM_MOBILE:	// 中国移动,MNC = 00,02,07
		case USIM_TELE:		// 中国电信,MNC = 03,05,11
		case USIM_TIETONG: 	// 中国铁通,MNC = 20
		default:
			modem->usim_state = 0;
			break;
	}

	return type;
}

static int get_nas_csq(int fd)
{
	char *cmd = "at+csq\r\n";
	char resp[64];
	char *end;

	memset(resp, 0x0, sizeof(resp));
	if (get_info_from_at_port(fd, cmd, resp, sizeof(resp) - 1) <= 0)
		return -1;

	end = strstr(resp, ",");
	if (end == 0)
		return 0;
	*end = '\0';

	end = strstr(resp, ":");
	if (end == 0)
		return 0;

	end++;
	return atoi(end);
}

int connect_to_modem(void)
{
	int fd;
	struct termios tio;
	char *at = "ate0\r\n";
	char res[64];
	DBG_ENTER();
	fd = open("/dev/ttyUSB3", O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		DBG("open ttySM3 error\n");
		return -1;
	}

	tcgetattr(fd, &tio);
	cfmakeraw(&tio);
	cfsetispeed(&tio, B115200);
	cfsetospeed(&tio, B115200);

	tio.c_cflag |= CS8;
	tio.c_cflag &= ~PARENB;  /* Clear parity enable */
	tio.c_iflag &= ~INPCK;	 /* disable parity checking */

	tio.c_cflag &= ~CSTOPB;

	tcsetattr(fd, TCSANOW, &tio);

	get_info_from_at_port(fd, at, res, sizeof(res) - 1);
	DBG_EXIT();
	return fd;
}

void disconnect_form_modem(int fd)
{
	close(fd);
}

int refresh_modem_status(int fd, struct modem_state *modem)
{
	if (fd <= 0)
	{
		fd = connect_to_modem();
		if (fd < 0)
		{
			DBG("modem at via open failed\n");
			return -1;
		}
	}

	//针对4G网络读取CREG信息不匹配,不建议采用字段判断注册情况
	modem->reg_network = get_reg_network(fd);
	// 实际测试 4GCE模块读取IMSI异常,建议读取当前注册网络
	modem->usim_type= get_usim_imsi(fd, modem);
	modem->network_mode = get_nas_mode(fd);
	modem->quility = get_nas_csq(fd);
	DBG("SIM STATE=%d, SIM TYPE %d\n", modem->usim_state, modem->usim_type);
	if (modem->network_mode >= 0 && modem->network_mode <= NASMODE_HYBRID)
		DBG("NET REG=%d, REGISTER NETWORK %s, QUAL %d\n", modem->reg_network, nas_string[modem->network_mode], modem->quility);

	return 0;
}

static struct timeval stick = {0,0};

static void tick_init(void)
{
	gettimeofday(&stick, NULL);
}

static int tick_1hz(void)
{
	int tick;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	tick = (tv.tv_sec - stick.tv_sec)*1000 + (tv.tv_usec - stick.tv_usec)/1000;

	return tick/1000;
}

int main(int argc, char *argv[])
{
	int sk;
	int fd;
	
	struct modem_state refresh_modem;
	int refresh_tick = 0;

	tick_init();
	fd = connect_to_modem();
	sk = pppd_server_socket_init();
	if (sk < 0)
	{
		return 0;
	}
	pppd_pthread_start(&sk);

	wan_stop_server(&modem);
	
	if(refresh_modem_status(fd, &modem) == 0)
		wan_start_server(&modem);
	else
		return 0;
	while (1)
	{
		// 1.refresh modem status(interval 5s)
		if(tick_1hz() - refresh_tick > 5 && server_start == 1)
		{
			DBG("refresh\n");
			if(refresh_modem_status(fd, &refresh_modem) == 0 && pidof("pppd") == 0)
				wan_start_server(&modem);
			
			if (refresh_modem.usim_state != 0)
			{
				// simcard pullout or sim type change??? stop ppp-server
				DBG("USIM CARD STATE ERROR\n");
				wan_stop_server(&modem);
			}
			else if (refresh_modem.network_mode != modem.network_mode
				|| refresh_modem.usim_type != modem.usim_type)
			{
				// 网络已自动切换
				DBG("Modem Register Network Mode Change:%d-->%d\n",
					modem.network_mode, refresh_modem.network_mode);
				wan_restart_server(&modem, &refresh_modem);	// CDMA2000<-->LTE???
			}
			memcpy(&modem, &refresh_modem, sizeof(struct modem_state));
			refresh_tick = tick_1hz();
		}
		else if(server_start == 0)
			wan_stop_server(&modem);
		// 2.exec 
	}

	pppd_server_socket_close(sk);
	
	disconnect_from_modem(fd);
	return 0;
}


