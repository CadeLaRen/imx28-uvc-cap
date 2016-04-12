/* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program;
 */
#include "common.h"
#include "proto.h"
#include "types.h"
#include "input.h"

static inline unsigned int __get_unaligned_be32(const unsigned char *p)
{
        return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline void __put_unaligned_be16(unsigned short val, unsigned char *p)
{
        *p++ = val >> 8;
        *p++ = val;
}

static inline void __put_unaligned_be32(unsigned int val, unsigned char *p)
{
        __put_unaligned_be16(val >> 16, p);
        __put_unaligned_be16(val, p + 2);
}

/******************************************************************************
* @Description.:packet_response 
* @Input Value.:
* 		@main_t
* 		@op
* 		@payload
* 		@len
* @Return Value:
******************************************************************************/
static int packet_response(unsigned char main_t, unsigned char op, unsigned char *payload, unsigned int len)
{
	unsigned short checksum = 0;
	unsigned int i;
	unsigned char *p;

	for (i = 0; i < len; i++)
		checksum += payload[i];

	p = (unsigned char *)(payload - 4);
	__put_unaligned_be32(checksum << 16 | main_t << 8 | (op|CMD_OP_ACK_FLAG) , p);
	p -= 4;
	__put_unaligned_be32(0x8008<<16|(len&0xffff), p);

	return send_packet(p, len + 8);
}


static int get_resulotion(context *pctx, int *width, int *height)
{

}

/******************************************************************************
* @Description.:genl_packet_parse 
* @Input Value.:
* 		@pctx
* 		@op
* 		@data
* 		@len
* @Return Value:
******************************************************************************/
static int genl_packet_parse(context *pctx, unsigned char op, unsigned char *data, int len)
{
	int id;
	int value;
	int ret = -1;
	unsigned char response[64];

	if (len < 4) {
		DBG("packet payload length error\n");
		return -1;
	}
	id = __get_unaligned_be32(data);

	if (id == CMD_GENL_RESOLUTION_CTRLID) {
		int w, h;
		if (op == CMD_SUB_V4L2_GET_VALUE) {
			v4l2_get_resolution(pctx->videoIn, &w, &h);
			__put_unaligned_be32(id, &response[8]);
			__put_unaligned_be32(w, &response[8+4]);
			__put_unaligned_be32(h, &response[8+8]);
			ret = packet_response(CMD_MAIN_GENL_TYPE, op, &response[8], 12);
		} else if (op == CMD_SUB_V4L2_SET_VALUE) {
			if (len >= 12)
				ret = v4l2_set_resolution(pctx->videoIn, __get_unaligned_be32(data+4), __get_unaligned_be32(data+8));
		} else {
			DBG("RESOLUTION not support op=%d\n", op);
		}
	} else if (id == CMD_GENL_FPS_CTRLID) {

	} else {

	}

	return ret;
}

/******************************************************************************
* @Description.:v4l2_class_ctrl 
* @Input Value.:
* 		@pctx
* 		@op
* 		@data
* 		@len
* @Return Value:
******************************************************************************/
static int v4l2_class_ctrl(context *pctx, unsigned char op, unsigned char *data, int len)
{
	int ret = -1;

	return ret;
} 

/******************************************************************************
* @Description.:v4l2_packet_parse 
* @Input Value.:
* 		@pctx
* 		@op
* 		@data
* 		@len
* @Return Value:
******************************************************************************/
static int v4l2_packet_parse(context *pctx, unsigned char op, unsigned char *data, int len)
{
	unsigned int id, value, min, max, val;
	int ret;
	unsigned char response[64];
	
	if (len < 4) {
		DBG("packet payload length error\n");
		return -1;
	}
	id = __get_unaligned_be32(data);
	if ((id & 0xffff0000) != 0x00980000 &&
		(id & 0xffff0000) != 0x009a00) {
		fprintf(stderr, "not support v4l2 ctrl id, id=%d\n", id);
		return -1;
	}

	hex_dump("payload", data, len);
	if (id == CMD_V4L2_CLASS_USER_CTRLID ||
		id == CMD_V4L2_CLASS_CAMERA_CTRLID) {
		return v4l2_class_ctrl(pctx, op, data, len);
	}

	// other
	switch (op) {
		case CMD_SUB_V4L2_GET_VALUE:
			ret = v4l2GetSoftControl(pctx->in, id, &value, &min, &max, &val);	
			if (ret == 0) {
				// response!
				__put_unaligned_be32(id, &response[8]); // id
				__put_unaligned_be32(value, &response[8+4]); // id
				__put_unaligned_be32(min, &response[8+8]); // min
				__put_unaligned_be32(max, &response[8+12]); // max
				__put_unaligned_be32(val, &response[8+16]); // default
				ret = packet_response(CMD_MAIN_V4L2_TYPE, op, &response[8], 20);
			}
			break;
		case CMD_SUB_V4L2_SET_VALUE:
			if (len >= 8) {
				value = __get_unaligned_be32(&data[4]);
				ret = v4l2SetControl(pctx->videoIn, id, value, pctx->in);
			}
			break;
		case CMD_SUB_V4L2_RST_VALUE:
			ret = v4l2ResetControl(pctx, id);
			break;	
		defualt:
			break;
	}

	return ret;
}

static int jpeg_packet_parse(context *pctx, unsigned char op, unsigned char *data, int len)
{
	unsigned int id, value;
	int ret;
	unsigned char response[64];
	
	hex_dump("payload", data, len);
	if (len < 4) {
		DBG("packet payload length error\n");
		return -1;
	}

	if (op != 0x00) {
		fprintf(stderr, "not support jpeg op, op=%d\n", op);
		return -1;
	}

	id = __get_unaligned_be32(data);
	if ((id & 0xffff0000) != 0x00800000) {
		fprintf(stderr, "not support jpeg control, id=%d\n", id);
		return -1;
	}

	switch (id) {
	    case CMD_JPEG_CLASS_CAP_SAVE_FILE:
			if (len >= 8) {
				value = __get_unaligned_be32(&data[4]);
				if (value > 0 && value <= 10)
					save_jpegs(pctx->in, value);
			}
			break;
	    default:
		break;
	}

	return 0;
}

/******************************************************************************
* @Description.:packet_parse 
* @Input Value.:
* 		@pctx
* 		@packet
* 		@len
* @Return Value:
******************************************************************************/
int packet_parse(context *pctx,  unsigned char *packet, int len)
{
	int ret = -1;
	unsigned char *payload, main_t, op;
	int length;
	unsigned short checksum, sum = 0;

	if (len < CMD_HEADER_SIZE)
		return -1;

	if (packet[0] != 0x80 || packet[1] != 0x08)
		return -2;

	length = packet[2] << 8 | packet[3];
	if (length + CMD_HEADER_SIZE > len)
		return -3;

	checksum = packet[4]<<8 | packet[5];	
	/* for (i = 0; i < length; i++) */
		/* sum += packet[i+8]; */

	/* if (sum != checksum) */
		/* return -4; */

	main_t = packet[6];
	op = packet[7];
	payload = (unsigned char *)&packet[8];

	DBG("CMD=%d, OP=%d, len=%d\n", main_t, op, length);
	switch (main_t) {
		case CMD_MAIN_GENL_TYPE:
			ret = genl_packet_parse(pctx, op, payload, length);
			break;
		case CMD_MAIN_V4L2_TYPE:
			ret = v4l2_packet_parse(pctx, op, payload, length);
			break;
		case CMD_MAIN_JPEG_TYPE:
			ret = jpeg_packet_parse(pctx, op, payload, length);
			break;
		default:
			DBG("undefined cmd type:0x%02x\n", main_t);
			break;
	}

	return ret;
}
