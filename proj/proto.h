#ifndef __PROTO_H__
#define __PROTO_H__

/*----------------------------------------------------------------------------
 *
 *
 *  +------------------------------+--------------+--
 *  |        frame header(8)       |    ctrl data |
 *  +------------------------------+--------------+--
 *  | magic | plen | check| type  |   	Payload   |
 *  +-------+------+------+------- |--------------|--
 *   <--2--><--2--><--2--><---2--->^-----plen-----^
 *
 *  1.magic format: 0x8008(big endian)
 *
 *  2.plen format:(big endian), payload length
 *
 *  3.check sum  : payload checksum
 *
 *  4.type format: (big endian)
 *  +---------------------+---------------------+--
 *  |    bit15-bit8       |     bit8-bit0       | 
 *  +---------------------+---------------------+--
 *  bit15 - bit8: main type
 *  bit7  - bit0:  sub type, operations mode(set, get, reset)
 * eg: 0x80 0x08 0x00 0x04 0x00 0x01 0x00 0x00 0x00000001 (get current resolution setting value)
 * eg: 0x80 0x08 0x00 0x04 0xXX 0xXX 0x01 0x01 0x00980900  0x000000XX(set brightness value is xx)
 *--------------------------------------------------------------------------*/
#define CMD_HEADER_SIZE		(8)

/*-----------------------Main Type----------------------*/
#define CMD_MAIN_GENL_TYPE (0x00)
#define CMD_MAIN_V4L2_TYPE (0x01)
#define CMD_MAIN_JPEG_TYPE 	(0x02)

/*-----------------------Sub Type, operation mode-------------------*/
// 1.get control id value
#define CMD_SUB_V4L2_GET_VALUE	(0x00)	// return:id + current + minimum + maximum + default
#define CMD_SUB_V4L2_SET_VALUE	(0x01)  // id + current
#define CMD_SUB_V4L2_RST_VALUE	(0x02)  // id
#define CMD_OP_ACK_FLAG	(0x80)

// generic control
// 1.resolution 
// get: 0x00000001 --> return 0x00000001 +  4Byte(width) + 4Byte(height)
// set: 0x00000001 +  4Byte(width) + 4Byte(height)
// reset: 0x00000001
#define CMD_GENL_RESOLUTION_CTRLID	(0x00000001)
// 2.fps
#define CMD_GENL_FPS_CTRLID	(0x00000002)

// V4L2 control
#define CMD_V4L2_CLASS_USER_CTRLID	(0x0098ffff)	//0x
// 1.brightness value
#define CMD_V4L2_BRIGHTNESS_CTRLID 	(0x00980900)
// 2.contrast value
#define CMD_V4L2_CONTRASE_CTRLID 	(0x00980901)
// 3.Saturation value
#define CMD_V4L2_SATURATION_CTRLID 	(0x00980902)
// 4.Hue value
#define CMD_V4L2_HUE_CTRLID 		(0x00980903)
// 5.White Balance
#define CMD_V4L2_WHITEBALANCE_CTRLID 	(0x0098090C)
// 6.Gamma control
#define CMD_V4L2_GAMMA_CTRLID 		(0x00980910)
// 7.GAIN control
#define CMD_V4L2_GAIN_CTRLID 		(0x00980913)
// 8.Power Line frequency
#define CMD_V4L2_PWRLF_CTRLID 		(0x00980918)
// 9.White balance temperature
#define CMD_V4L2_WHITEBT_CTRLID 	(0x0098091A)
// 10.Sharpness
#define CMD_V4L2_SHARPNESS_CTRLID 	(0x0098091B)
// 11.Backlight compensation
#define CMD_V4L2_BACKLIGHT_CTRLID 	(0x0098091C)

/*-----------------camera cmd------------------*/
#define CMD_V4L2_CLASS_CAMERA_CTRLID	(0x009affff)
// 12.Exposure
#define CMD_V4L2_EXPOSURE_CTRLID 	(0x009a0901)
// 13.Exposure absolute
#define CMD_V4L2_EXPOSURE_ABS_CTRLID 	(0x009a0902)
// 14.Focus absolute
#define CMD_V4L2_FOCUS_ABS_CTRLID 	(0x009a090a)
// 15.Focus, auto.
#define CMD_V4L2_FOCUS_CTRLID 		(0x009a090c)


/*----------------------------JPEG----------------------------*/
#define CMD_JPEG_CLASS_CAP_SAVE_FILE	(0x00800001)

#endif


