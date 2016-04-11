#include "common.h"
#include "sys_if_dev.h"
#include "modem.h"

static int cp_fd = -1;

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
	int MCC = 0;
	int MNC	= 0;
	usim_type usim = USIM_UNDEFINED;
	
	if (imsi_len < 15)
		return USIM_NO_DETECT;
	
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

/**<   UIM state. Values: \n
  - 0x00 -- UIM initialization completed \n
  - 0x01 -- UIM is locked or the UIM failed \n
  - 0x02 -- UIM is not present \n
  - 0x03 -- Reserved \n
  - 0xFF -- UIM state is currently unavailable 
  */
static int get_usim_state(void)
{
	int qmi = -1;

	qmi_idl_service_object_type srv_obj;
	qmi_client_type qmi_clnt_hdl = NULL;
	qmi_client_error_type rc;
	dms_uim_get_state_resp_msg_v01 uim_msg;

	// Init qmi interface
	qmi = qmi_init(NULL, NULL);
	if (qmi < 0)
	{
		return -1;
	}

	// Get dms operation
	srv_obj = dms_get_service_object_v01();
	if(srv_obj == NULL)
	{
		return -1;
	}

	// connect qmi channel
	rc = qmi_client_init("rmnet0",
			srv_obj,
			NULL,
			srv_obj,
			&qmi_clnt_hdl);
	if(rc != QMI_NO_ERR)
	{
		qmi_client_release(qmi_clnt_hdl);
		qmi_release(qmi);
		return -1;
	}
		
	// Queries the state of the UIM
	rc = qmi_client_send_msg_sync(qmi_clnt_hdl,
			QMI_DMS_UIM_GET_STATE_REQ_V01,
			NULL,
			0,
			&uim_msg,
			sizeof(uim_msg),
			15000
			);
	if (rc != QMI_NO_ERR)
	{
		return -1;
	}
	
	if (uim_msg.resp.result != 0)
	{
		return -1;
	}

	qmi_client_release(qmi_clnt_hdl);
	qmi_release(qmi);

	return (int)uim_msg.uim_state;
}

static usim_type get_usim_imsi(void)
{
	int qmi = -1;
	qmi_idl_service_object_type srv_obj;
	qmi_client_type qmi_clnt_hdl = NULL;
	qmi_client_error_type rc;
	dms_uim_get_imsi_resp_msg_v01 resp_msg;

	// Init qmi interface
	qmi = qmi_init(NULL, NULL);
	if (qmi < 0)
	{
		return USIM_DETECT_FAILED;
	}

	// Get dms operation
	srv_obj = dms_get_service_object_v01();
	if(srv_obj == NULL)
	{
		return USIM_DETECT_FAILED;
	}

	// connect qmi channel
	rc = qmi_client_init("rmnet0",
			srv_obj,
			NULL,
			srv_obj,
			&qmi_clnt_hdl);
	if(rc != QMI_NO_ERR)
	{
		qmi_client_release(qmi_clnt_hdl);
		qmi_release(qmi);
		return USIM_DETECT_FAILED;
	}

__RESEND_QMI_MSG__:
	rc = qmi_client_send_msg_sync(qmi_clnt_hdl,
			QMI_DMS_UIM_GET_IMSI_REQ_V01,
			NULL,
			0,
			&resp_msg,
			sizeof(resp_msg),
			15000
			);

	if(rc != QMI_NO_ERR)
	{
		goto __RESEND_QMI_MSG__;
	}

	if(resp_msg.resp.result != 0)
	{
		qmi_client_release(qmi_clnt_hdl);
		qmi_release(qmi);
		return USIM_NO_DETECT;
	}

	qmi_client_release(qmi_clnt_hdl);
	qmi_release(qmi);
	
	return usim_imsi_parse(resp_msg.imsi, strlen(resp_msg.imsi));
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

	return rc;
}

static network_mode get_nas_mode(int fd)
{
	char *cmd = "at+cnsmod?\r\n";	//at+cpsi?/at*cnti?
	char resp[64];
	char *end;
	network_mode mode;

	memset(resp, 0x0, sizeof(resp));
	if (get_info_from_at_port(fd, cmd, resp, sizeof(resp)) <= 0)
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

static int get_nas_csq(int fd)
{
	char *cmd = "at+csq\r\n";
	char resp[64];
	char *end;

	memset(resp, 0x0, sizeof(resp));
	if (get_info_from_at_port(fd, cmd, resp, sizeof(resp)) <= 0)
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

void modem_exit(void)
{
	close(cp_fd);
	cp_fd = 0;
}

int modem_init(void)
{
	int cpfd;
	char resp[64];
	char *cmd = "ate0\r\n";

	struct serl_attr attr = 
	{
		8,1,115200,0,
	};

	DBG_ENTER();

	cpfd = open_serial("/dev/ttySM1", &attr);
	if (cpfd < 0)
		return cpfd;

	get_info_from_at_port(cpfd, cmd, resp, sizeof(resp));

	DBG_EXIT();
	
	cp_fd = cpfd;
	return cpfd;
}

int refresh_modem_status(struct modem_state *modem)
{
	if (cp_fd <= 0)
	{
		cp_fd = modem_init();
		if (cp_fd < 0)
		{
			DBG("modem at via open failed\n", cp_fd);
			return -1;
		}
	}

	modem->usim_state = get_usim_state();//QMI
	//针对4G网络读取CREG信息不匹配,不建议采用字段判断注册情况
	modem->reg_network = get_reg_network(cp_fd);
	// 实际测试 4GCE模块读取IMSI异常,建议读取当前注册网络
	modem->usim_type= get_usim_imsi();//QMI
	modem->network_mode = get_nas_mode(cp_fd);
	modem->quility = get_nas_csq(cp_fd);
	DBG("SIM STATE=%d, SIM TYPE %d\n", modem->usim_state, modem->usim_type);
	DBG("NET REG=%d, REGISTER NETWORK %s, QUAL %d\n", modem->reg_network, nas_string[modem->network_mode], modem->quility);

	return 0;
}

