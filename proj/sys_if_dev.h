#ifndef __SYS_IF_DEV_H__
#define  __SYS_IF_DEV_H__

#define IMX_SERL_DEV	"/dev/ttySP1"

struct serl_attr {
	int databits; /* 5 6 7 8 */
	int stopbits; /* 1 or 2 */
	int baudrate; /* 38400, or 9600 , or others. */
	int parity;  /* 0-> no, 1->odd, 2->even */
};

extern int serial_init(struct serl_attr *pattr);
extern void serial_exit(void);
extern int recv_packet(void *data, int data_len);
extern int send_packet(void *data, int data_len);
#endif /*  __SYS_IF_DEV_H__ */

