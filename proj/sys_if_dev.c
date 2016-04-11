#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include "common.h"
#include "sys_if_dev.h"

#define _SCHEDULE() do {			\
	usleep(1);			\
} while(0)

static void __set_baudrate(struct termios *io, struct serl_attr *attr)
{	
	speed_t speed = B9600;
	switch (attr->baudrate) {
		case 9600:
			speed = B9600;
			break;
		case 19200:
			speed = B19200;
			break;
		case 38400:
			speed = B38400;
			break;
		case 115200:
			speed = B115200;
			break;
		case 4800:
			speed = B4800;
			break;
		case 2400:
			speed = B2400;
			break;
		case 1200:
			speed = B1200;
			break;
		case 57600:
			speed = B57600;
			break;
	}
	cfsetispeed(io, speed);
	cfsetospeed(io, speed);
}

static void __set_databits(struct termios *io, struct serl_attr *attr)
{
	switch (attr->databits) {
		case 5:
			io->c_cflag |= CS5;
			break;
		case 6:
			io->c_cflag |= CS6;
			break;
		case 7:
			io->c_cflag |= CS7;
			break;
		case 8:
			io->c_cflag |= CS8;
			break;
	}
}

static void __set_parity(struct termios *io, struct serl_attr *attr)
{
	switch (attr->parity) {
		case 0:
			io->c_cflag &= ~PARENB;  /* Clear parity enable */
			io->c_iflag &= ~INPCK;   /* disable parity checking */
			break;
		case 1:
			io->c_cflag |= (PARODD | PARENB); 
			io->c_iflag |= INPCK;    /* odd parity checking */
			break;
		case 2:
			io->c_cflag |= PARENB;   /* Enable parity */
			io->c_cflag &= ~PARODD;
			io->c_iflag |= INPCK;    /*enable parity checking */
			break;
	}
}

static void __set_stopbits(struct termios *io, struct serl_attr *attr)
{
	switch (attr->stopbits) {
		case 1:
			io->c_cflag &= ~CSTOPB;
			break;
		case 2:
			io->c_cflag |= CSTOPB;
			break;
	}
}

void flush_serial(int fd)
{
	int flag = 0, isblock = 0;
	unsigned char nr=0;
	char buf[64];

	do {
		if (fd <= 0)    break;
		/* 1. set it to nonblock */
		if (fcntl(fd, F_GETFL, &flag) < 0)
			break;

		if (flag & O_NONBLOCK)
			isblock = 0;
		else
			isblock = 1;

		if (isblock) {
			flag |= O_NONBLOCK;
			if (fcntl(fd, F_SETFL, flag) < 0)
				break;
		}

		/* 2. read out */
		while (read(fd, buf, sizeof(buf)) > 0) {
			if (++nr == 10) {
				nr = 0;
				_SCHEDULE();
			}
		}

		/* 3. restore it to block */
		if (isblock) {
			flag &= ~O_NONBLOCK;
			fcntl(fd, F_SETFL, flag);
		}

	} while(0);

}

int open_serial(char *dev_name, struct serl_attr *attr)
{
	int fd;
	struct termios tio;

	fd = open(dev_name, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		return -1;
	}

	tcgetattr(fd, &tio);
	cfmakeraw(&tio);
	__set_baudrate(&tio, attr);
	__set_databits(&tio, attr);
	__set_parity(&tio, attr);
	__set_stopbits(&tio, attr);
	tcsetattr(fd, TCSANOW, &tio);

	flush_serial(fd);
	return fd;
}

void close_serial(int fd)
{
	if (fd > 0)
	{
		flush_serial(fd);
		close(fd);
	}
}

void set_serial_attr(int fd, struct serl_attr *attr)
{
	struct termios tio;

	tcgetattr(fd, &tio);
	cfmakeraw(&tio);
	__set_baudrate(&tio, attr);
	__set_databits(&tio, attr);
	__set_parity(&tio, attr);
	__set_stopbits(&tio, attr);
	tcsetattr(fd, TCSANOW, &tio);
}

int read_serial(int fd, long time_us, char *buf, int buf_size)
{
	int ret = -1;
	fd_set rfds;
	struct timeval to;

	if (fd <= 0)
	    return -1;

	if (!buf || buf_size <= 0)
	    return -1;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	/* BLOCK READ */
	if (time_us < 0) {
		ret = select(fd+1, &rfds, NULL, NULL, NULL);
	}
	else {
		to.tv_sec  = 0;
		to.tv_usec = time_us;
		ret = select(fd+1, &rfds, NULL, NULL, &to);
	}

	if (ret > 0 && FD_ISSET(fd, &rfds)) {
		/* Readable, give it a schedule */
		_SCHEDULE();
		ret = read(fd, buf, buf_size);
	}

	return ret;
}

void send_break(int fd)
{
	tcsendbreak(fd, 0);
}

static int fd = -1;

void serial_exit(void)
{
	close_serial(fd);	
	fd = -1;
}

int serial_init(struct serl_attr *pattr)
{
	unsigned char *buf = NULL;
	int ret;
	struct serl_attr attr = 
	{
		8,1,115200,0,
	};

	
	if (!pattr)
		pattr = &attr;

	fd = open_serial(IMX_SERL_DEV, pattr);
	if (fd < 0) {
		DBG("%s: open %s failed\n", __func__, IMX_SERL_DEV);
		return -1;
	}
	return 0;
}

static inline int get_char(char *ch)
{
	return read_serial(fd, 5000, ch, 1);
}

static inline int put_char(char ch)
{
	return write(fd, &ch, 1);
}

int recv_packet(void *data, int data_len)
{
	static int oft = 0;
	static char buf[1024];
	static int len;
	int rc;
	static int timeout = 0;

	if (fd <= 0 || data_len > sizeof(buf))
		return -1;

	while ((rc = get_char(&buf[oft]) == 1)) {
		
		switch (oft) {
			case 0:
				if (buf[0] == 0x80)
					oft++;
				break;
			case 1:
				if (buf[1] == 0x08)
					oft++;
				break;
			case 2:
				len = buf[2] << 8;
				oft++;
				break;
			case 3:
				len = len | buf[3];
				oft++;
				break;
			default:
				oft++;
				if (oft == len+8) {
					memcpy(data, buf, oft);
					rc = oft;
					oft = 0;
					len = 0;
				} else
					timeout = get_tick_1khz();
				break;
		}
		if (rc > 1)	// transform complete
			break;
	}
	if (len && rc <= 0) {
		//timeout
		if (get_tick_1khz() - timeout > 10) {
			// clean
			oft = 0;
			len = 0;
			timeout = get_tick_1khz();
		}
	}

	return rc;
}

int send_packet(void *data, int data_len)
{
	int ret;

	if (fd <= 0)
		return -1;

	return write(fd, data, data_len);
}

int sys_if_test(void)
{
	char buf[] = "hello imx /dev/ttySPX\n";

	send_packet(buf, strlen(buf));
}

