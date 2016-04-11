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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <syslog.h>
#include <linux/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "common.h"
#include "input.h"
static int debug = 0;

/* ioctl with a number of retries in the case of failure
* args:
* fd - device descriptor
* IOCTL_X - ioctl reference
* arg - pointer to ioctl data
* returns - ioctl result
*/
int xioctl(int fd, int IOCTL_X, void *arg)
{
    int ret = 0;
    int tries = IOCTL_RETRY;
    do {
       	ret = ioctl(fd, IOCTL_X, arg);
    } while(ret && tries-- &&
            ((errno == EINTR) || (errno == EAGAIN) || (errno == ETIMEDOUT)));

    if(ret && (tries <= 0)) fprintf(stderr, "ioctl (%i) retried %i times - giving up: %s)\n", IOCTL_X, IOCTL_RETRY, strerror(errno));

    return (ret);
}

static int init_v4l2(struct vdIn *vd);

/******************************************************************************
* @Description.:init_videoin 
* @Input Value.:
* 		@vd
* 		@device
* 		@width
* 		@height
* 		@fps
* 		@format
* 		@grabmethod
* 		@vstd
* 		@in
* @Return Value:
******************************************************************************/
int init_videoin(struct vdIn *vd, char *device, int width,
                 int height, int fps, int format, int grabmethod, v4l2_std_id vstd, input *in)
{
    if(vd == NULL || device == NULL)
        return -1;
    if(width == 0 || height == 0)
        return -1;
    if(grabmethod < 0 || grabmethod > 1)
        grabmethod = 1;     //mmap by default;
    vd->videodevice = NULL;
    vd->status = NULL;
    vd->pictName = NULL;
    vd->videodevice = (char *) calloc(1, 16 * sizeof(char));
    vd->status = (char *) calloc(1, 100 * sizeof(char));
    vd->pictName = (char *) calloc(1, 80 * sizeof(char));
    snprintf(vd->videodevice, (16 - 1), "%s", device);
    vd->toggleAvi = 0;
    vd->getPict = 0;
    vd->signalquit = 1;
    vd->width = width;
    vd->height = height;
    vd->fps = fps;
    vd->formatIn = format;
	vd->vstd = vstd;
    vd->grabmethod = grabmethod;
    vd->soft_framedrop = 0;
    if(init_v4l2(vd) < 0) {
        fprintf(stderr, " Init v4L2 failed !! exit fatal \n");
        goto error;;
    }

    // getting the name of the input source
    struct v4l2_input in_struct;
    memset(&in_struct, 0, sizeof(struct v4l2_input));
    in_struct.index = 0;
    if (xioctl(vd->fd, VIDIOC_ENUMINPUT,  &in_struct) == 0) {
        int nameLength = strlen((char*)&in_struct.name);
        in->name = malloc((1+nameLength)*sizeof(char));
        sprintf(in->name, "%s", in_struct.name);
        DBG("Input name: %s\n", in_struct.name);
    } else {
        DBG("VIDIOC_ENUMINPUT failed\n");
    }

    // enumerating formats

    struct v4l2_format currentFormat;
    memset(&currentFormat, 0, sizeof(struct v4l2_format));
    currentFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(vd->fd, VIDIOC_G_FMT, &currentFormat) == 0) {
        DBG("Current size: %dx%d\n",
             currentFormat.fmt.pix.width,
             currentFormat.fmt.pix.height);
    }

    in->in_formats = NULL;
    for (in->formatCount = 0; 1; in->formatCount++) {
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(struct v4l2_fmtdesc));
        fmtdesc.index = in->formatCount;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(vd->fd, VIDIOC_ENUM_FMT, &fmtdesc) < 0) {
            break;
        }

        if (in->in_formats == NULL) {
            in->in_formats = (input_format*)calloc(1, sizeof(input_format));
        } else {
            in->in_formats = (input_format*)realloc(in->in_formats, (in->formatCount + 1) * sizeof(input_format));
        }

        if (in->in_formats == NULL) {
            LOG("Calloc/realloc failed: %s\n", strerror(errno));
            return -1;
        }

        memcpy(&in->in_formats[in->formatCount], &fmtdesc, sizeof(input_format));

        if(fmtdesc.pixelformat == format)
            in->currentFormat = in->formatCount;

        DBG("Supported format: %s\n", fmtdesc.description);
        struct v4l2_frmsizeenum fsenum;
        memset(&fsenum, 0, sizeof(struct v4l2_frmsizeenum));
        fsenum.pixel_format = fmtdesc.pixelformat;
        int j = 0;
        in->in_formats[in->formatCount].supportedResolutions = NULL;
        in->in_formats[in->formatCount].resolutionCount = 0;
        in->in_formats[in->formatCount].currentResolution = -1;
        while(1) {
            fsenum.index = j;
            j++;
            if(xioctl(vd->fd, VIDIOC_ENUM_FRAMESIZES, &fsenum) == 0) {
                in->in_formats[in->formatCount].resolutionCount++;

                if (in->in_formats[in->formatCount].supportedResolutions == NULL) {
                    in->in_formats[in->formatCount].supportedResolutions = (input_resolution*)
                            calloc(1, sizeof(input_resolution));
                } else {
                    in->in_formats[in->formatCount].supportedResolutions = (input_resolution*)
                            realloc(in->in_formats[in->formatCount].supportedResolutions, j * sizeof(input_resolution));
                }

                if (in->in_formats[in->formatCount].supportedResolutions == NULL) {
                    LOG("Calloc/realloc failed\n");
                    return -1;
                }

                in->in_formats[in->formatCount].supportedResolutions[j-1].width = fsenum.discrete.width;
                in->in_formats[in->formatCount].supportedResolutions[j-1].height = fsenum.discrete.height;
                if(format == fmtdesc.pixelformat) {
                    in->in_formats[in->formatCount].currentResolution = (j - 1);
                    DBG("\tSupported size with the current format: %dx%d\n", fsenum.discrete.width, fsenum.discrete.height);
                } else {
                    DBG("\tSupported size: %dx%d\n", fsenum.discrete.width, fsenum.discrete.height);
                }
            } else {
                break;
            }
        }
    }

    /* alloc a temp buffer to reconstruct the pict */
    vd->framesizeIn = (vd->width * vd->height << 1);

    if (vd->formatIn == V4L2_PIX_FMT_MJPEG) {
    	// in JPG mode the frame size is varies at every frame, so we allocate a bit bigger buffer
        vd->tmpbuffer = (unsigned char *) calloc(1, (size_t) vd->framesizeIn);
        if(!vd->tmpbuffer)
            goto error;
        vd->framebuffer =
            (unsigned char *) calloc(1, (size_t) vd->width * (vd->height + 8) * 2);
    }
    else
	goto error;
  

    if(!vd->framebuffer)
        goto error;
    DBG("Exit\n");
    return 0;
error:
    free(vd->videodevice);
    free(vd->status);
    free(vd->pictName);
    close(vd->fd);
    return -1;
}

/******************************************************************************
* @Description.:init_v4l2 
* @Input Value.:
* 		@vd
* @Return Value:
******************************************************************************/
static int init_v4l2(struct vdIn *vd)
{
    int i;
    int ret = 0;
    if((vd->fd = open(vd->videodevice, O_RDWR)) == -1) {
        perror("ERROR opening V4L interface");
        DBG("errno: %d", errno);
        return -1;
    }

    memset(&vd->cap, 0, sizeof(struct v4l2_capability));
    ret = xioctl(vd->fd, VIDIOC_QUERYCAP, &vd->cap);
    if(ret < 0) {
        fprintf(stderr, "Error opening device %s: unable to query device.\n", vd->videodevice);
        goto fatal;
    }

    if((vd->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        fprintf(stderr, "Error opening device %s: video capture not supported.\n",
                vd->videodevice);
        goto fatal;;
    }

    if(vd->grabmethod) {
        if(!(vd->cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "%s does not support streaming i/o\n", vd->videodevice);
            goto fatal;
        }
    } else {
        if(!(vd->cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf(stderr, "%s does not support read i/o\n", vd->videodevice);
            goto fatal;
        }
    }

    if (vd->vstd != V4L2_STD_UNKNOWN) {
        if (ioctl(vd->fd, VIDIOC_S_STD, &vd->vstd) == -1) {
            fprintf(stderr, "Can't set video standard: %s\n",strerror(errno));
            goto fatal;
        }
    }

    /*
     * set format in
     */
    memset(&vd->fmt, 0, sizeof(struct v4l2_format));
    vd->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->fmt.fmt.pix.width = vd->width;
    vd->fmt.fmt.pix.height = vd->height;
    vd->fmt.fmt.pix.pixelformat = vd->formatIn;
    vd->fmt.fmt.pix.field = V4L2_FIELD_ANY;
    ret = xioctl(vd->fd, VIDIOC_S_FMT, &vd->fmt);
    if(ret < 0) {
        fprintf(stderr, "Unable to set format: %d res: %dx%d\n", vd->formatIn, vd->width, vd->height);
        goto fatal;
    }

    if((vd->fmt.fmt.pix.width != vd->width) ||
            (vd->fmt.fmt.pix.height != vd->height)) {
        fprintf(stderr, "i: The format asked unavailable, so the width %d height %d \n", vd->fmt.fmt.pix.width, vd->fmt.fmt.pix.height);
        vd->width = vd->fmt.fmt.pix.width;
        vd->height = vd->fmt.fmt.pix.height;
        /*
         * look the format is not part of the deal ???
         */
        if(vd->formatIn != vd->fmt.fmt.pix.pixelformat) {
            if(vd->formatIn == V4L2_PIX_FMT_MJPEG) {
                fprintf(stderr, "The input device does not supports MJPEG mode\n"
                                "You may also try the YUV mode (-yuv option), \n"
                                "or the you can set another supported formats using the -fourcc argument.\n"
                                "Note: streaming using uncompressed formats will require much more CPU power on your server\n");
                goto fatal;
            } else if(vd->formatIn == V4L2_PIX_FMT_YUYV) {
                fprintf(stderr, "The input device does not supports YUV format\n");
                goto fatal;
            } else if (vd->formatIn == V4L2_PIX_FMT_RGB565) {
                fprintf(stderr, "The input device does not supports RGB565 format\n");
                goto fatal;
            }
        } else {
            vd->formatIn = vd->fmt.fmt.pix.pixelformat;
        }
    }

    /*
     * set framerate
     */

    if (vd->fps != -1) {
        struct v4l2_streamparm *setfps;
        setfps = (struct v4l2_streamparm *) calloc(1, sizeof(struct v4l2_streamparm));
        memset(setfps, 0, sizeof(struct v4l2_streamparm));
        setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        /*
        * first query streaming parameters to determine that the FPS selection is supported
        */
        ret = xioctl(vd->fd, VIDIOC_G_PARM, setfps);
        if (ret == 0) {
            if (setfps->parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
                memset(setfps, 0, sizeof(struct v4l2_streamparm));
                setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                setfps->parm.capture.timeperframe.numerator = 1;
                setfps->parm.capture.timeperframe.denominator = vd->fps==-1?255:vd->fps; // if no default fps set set it to maximum

                ret = xioctl(vd->fd, VIDIOC_S_PARM, setfps);
                if (ret) {
                    perror("Unable to set the FPS\n");
                } else {
                    if (vd->fps != setfps->parm.capture.timeperframe.denominator) {
                        LOG("FPS coerced ......: from %d to %d\n", vd->fps, setfps->parm.capture.timeperframe.denominator);
                    }

                    // if we selecting lower FPS than the allowed then we will use software framedropping
                    if (vd->fps < setfps->parm.capture.timeperframe.denominator) {
                        vd->soft_framedrop = 1;
                        vd->frame_period_time = 1000/vd->fps; // calcualate frame period time in ms
                        LOG("Frame period time ......: %ld ms\n", vd->frame_period_time);

                        // set FPS to maximum in order to mize the lagging
                        memset(setfps, 0, sizeof(struct v4l2_streamparm));
                        setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        setfps->parm.capture.timeperframe.numerator = 1;
                        setfps->parm.capture.timeperframe.denominator = 255;
                        ret = xioctl(vd->fd, VIDIOC_S_PARM, setfps);
                        if (ret) {
                            perror("Unable to set the FPS\n");
                        }
                    }
                }
            } else {
                perror("Setting FPS on the capture device is not supported, fallback to software framedropping\n");
                vd->soft_framedrop = 1;
                vd->frame_period_time = 1000/vd->fps; // calcualate frame period time in ms
                LOG("Frame period time ......: %ld ms\n", vd->frame_period_time);
            }
        } else {
            perror("Unable to query that the FPS change is supported\n");
        }
    }

    /*
     * request buffers
     */
    memset(&vd->rb, 0, sizeof(struct v4l2_requestbuffers));
    vd->rb.count = NB_BUFFER;
    vd->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->rb.memory = V4L2_MEMORY_MMAP;

    ret = xioctl(vd->fd, VIDIOC_REQBUFS, &vd->rb);
    if(ret < 0) {
        perror("Unable to allocate buffers");
        goto fatal;
    }

    /*
     * map the buffers
     */
    for(i = 0; i < NB_BUFFER; i++) {
        memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
        vd->buf.index = i;
        vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vd->buf.memory = V4L2_MEMORY_MMAP;
        ret = xioctl(vd->fd, VIDIOC_QUERYBUF, &vd->buf);
        if(ret < 0) {
            perror("Unable to query buffer");
            goto fatal;
        }

        if(debug)
            fprintf(stderr, "length: %u offset: %u\n", vd->buf.length, vd->buf.m.offset);

        vd->mem[i] = mmap(0 /* start anywhere */ ,
                          vd->buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, vd->fd,
                          vd->buf.m.offset);
        if(vd->mem[i] == MAP_FAILED) {
            perror("Unable to map buffer");
            goto fatal;
        }
        if(debug)
            fprintf(stderr, "Buffer mapped at address %p.\n", vd->mem[i]);
    }

    /*
     * Queue the buffers.
     */
    for(i = 0; i < NB_BUFFER; ++i) {
        memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
        vd->buf.index = i;
        vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vd->buf.memory = V4L2_MEMORY_MMAP;
        ret = xioctl(vd->fd, VIDIOC_QBUF, &vd->buf);
        if(ret < 0) {
            perror("Unable to queue buffer");
            goto fatal;;
        }
    }
    return 0;
fatal:
    return -1;

}

static int video_enable(struct vdIn *vd)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = xioctl(vd->fd, VIDIOC_STREAMON, &type);
    if(ret < 0) {
        perror("Unable to start capture");
        return ret;
    }
    vd->streamingState = STREAMING_ON;
    return 0;
}

static int video_disable(struct vdIn *vd, streaming_state disabledState)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;
    DBG("STopping capture\n");
    ret = xioctl(vd->fd, VIDIOC_STREAMOFF, &type);
    if(ret != 0) {
        perror("Unable to stop capture");
        return ret;
    }
    DBG("STopping capture done\n");
    vd->streamingState = disabledState;
    return 0;
}

/******************************************************************************
* @Description.:is_huffman 
* @Input Value.:
* 		@buf
* @Return Value:
******************************************************************************/
int is_huffman(unsigned char *buf)
{
    unsigned char *ptbuf;
    int i = 0;
    ptbuf = buf;
    while(((ptbuf[0] << 8) | ptbuf[1]) != 0xffda) {
        if(i++ > 2048)
            return 0;
        if(((ptbuf[0] << 8) | ptbuf[1]) == 0xffc4)
            return 1;
        ptbuf++;
    }
    return 0;
}

/******************************************************************************
* @Description.:memcpy_picture 
* @Input Value.:
* 		@out
* 		@buf
* 		@size
* @Return Value:
******************************************************************************/
int memcpy_picture(unsigned char *out, unsigned char *buf, int size)
{
    unsigned char *ptdeb, *ptlimit, *ptcur = buf;
    int sizein, pos = 0;

    if(!is_huffman(buf)) {
        ptdeb = ptcur = buf;
        ptlimit = buf + size;
        while((((ptcur[0] << 8) | ptcur[1]) != 0xffc0) && (ptcur < ptlimit))
            ptcur++;
        if(ptcur >= ptlimit)
            return pos;
        sizein = ptcur - ptdeb;

        memcpy(out + pos, buf, sizein); pos += sizein;
        //memcpy(out + pos, dht_data, sizeof(dht_data)); pos += sizeof(dht_data);
        memcpy(out + pos, ptcur, size - sizein); pos += size - sizein;
    } else {
        memcpy(out + pos, ptcur, size); pos += size;
    }
    return pos;
}

int uvcGrab(struct vdIn *vd)
{
#define HEADERFRAME1 0xaf
    int ret;

    if(vd->streamingState == STREAMING_OFF) {
        if(video_enable(vd))
            goto err;
    }
    memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
    vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->buf.memory = V4L2_MEMORY_MMAP;

    ret = xioctl(vd->fd, VIDIOC_DQBUF, &vd->buf);
    if(ret < 0) {
        perror("Unable to dequeue buffer");
        goto err;
    }

    if (vd->formatIn == V4L2_PIX_FMT_MJPEG)
    {
        if(vd->buf.bytesused <= HEADERFRAME1) {
            /* Prevent crash
                                                        * on empty image */
            fprintf(stderr, "Ignoring empty buffer ...\n");
            return 0;
        }

        /* memcpy(vd->tmpbuffer, vd->mem[vd->buf.index], vd->buf.bytesused);

        memcpy (vd->tmpbuffer, vd->mem[vd->buf.index], HEADERFRAME1);
        memcpy (vd->tmpbuffer + HEADERFRAME1, dht_data, sizeof(dht_data));
        memcpy (vd->tmpbuffer + HEADERFRAME1 + sizeof(dht_data), vd->mem[vd->buf.index] + HEADERFRAME1, (vd->buf.bytesused - HEADERFRAME1));
        */

        memcpy(vd->tmpbuffer, vd->mem[vd->buf.index], vd->buf.bytesused);
        vd->tmpbytesused = vd->buf.bytesused;
        vd->tmptimestamp = vd->buf.timestamp;

        if(debug)
            fprintf(stderr, "bytes in used %d \n", vd->buf.bytesused);

    }
    else
	goto err;

    ret = xioctl(vd->fd, VIDIOC_QBUF, &vd->buf);
    if(ret < 0) {
        perror("Unable to requeue buffer");
        goto err;
    }

    return 0;

err:
    vd->signalquit = 0;
    return -1;
}

/******************************************************************************
* @Description.:close_v4l2 
* @Input Value.:
* 		@vd
* @Return Value:
******************************************************************************/
int close_v4l2(struct vdIn *vd)
{
    if(vd->streamingState == STREAMING_ON)
        video_disable(vd, STREAMING_OFF);
    if(vd->tmpbuffer)
        free(vd->tmpbuffer);
    vd->tmpbuffer = NULL;
    free(vd->framebuffer);
    vd->framebuffer = NULL;
    free(vd->videodevice);
    free(vd->status);
    free(vd->pictName);
    vd->videodevice = NULL;
    vd->status = NULL;
    vd->pictName = NULL;

    return 0;
}


/******************************************************************************
* @Description.:isv4l2Control 
* @Input Value.:
* 		@vd
* 		@control
* 		@queryctrl
* @Return Value:return >= 0 ok otherwhise -1
******************************************************************************/
static int isv4l2Control(struct vdIn *vd, int control, struct v4l2_queryctrl *queryctrl)
{
    int err = 0;

    queryctrl->id = control;
    if((err = xioctl(vd->fd, VIDIOC_QUERYCTRL, queryctrl)) < 0) {
        fprintf(stderr, "ioctl querycontrol error %d \n",errno);
        return -1;
    }

    if(queryctrl->flags & V4L2_CTRL_FLAG_DISABLED) {
        fprintf(stderr, "control %s disabled \n", (char *) queryctrl->name);
        return -1;
    }

    if(queryctrl->type & V4L2_CTRL_TYPE_BOOLEAN) {
        return 1;
    }

    if(queryctrl->type & V4L2_CTRL_TYPE_INTEGER) {
        return 0;
    }

    fprintf(stderr, "contol %s unsupported  \n", (char *) queryctrl->name);
    return -1;
}

int v4l2GetHalControl(struct vdIn *vd, int control)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control_s;
    int err;

    if((err = isv4l2Control(vd, control, &queryctrl)) < 0) {
        return -1;
    }

    control_s.id = control;
    if((err = xioctl(vd->fd, VIDIOC_G_CTRL, &control_s)) < 0) {
        return -1;
    }

    return control_s.value;
}

/******************************************************************************
* @Description.:v4l2SetControl 
* @Input Value.:
* 		@vd
* 		@control_id
* 		@value
* 		@in
* @Return Value:
******************************************************************************/
int v4l2SetControl(struct vdIn *vd, int control_id, int value, input *in)
{
    struct v4l2_control control_s;
    int min, max;
    int ret = 0;
    int err;
    int i;
    int got = -1;
    DBG("Looking for the 0x%08x V4L2 control\n", control_id);
    for (i = 0; i < in->parametercount; i++) {
        if (in->in_parameters[i].ctrl.id == control_id) {
            got = 0;
            break;
        }
    }

    if (got == 0) { // we have found the control with the specified id
        DBG("V4L2 ctrl 0x%08x found\n", control_id);
        if (in->in_parameters[i].class_id == V4L2_CTRL_CLASS_USER) {
            DBG("Control type: USER\n");
            min = in->in_parameters[i].ctrl.minimum;
            max = in->in_parameters[i].ctrl.maximum;

            if((value >= min) && (value <= max)) {
                control_s.id = control_id;
                control_s.value = value;
                if((err = xioctl(vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
                    DBG("VIDIOC_S_CTRL failed\n");
                    return -1;
                } else {
                    DBG("V4L2 ctrl 0x%08x new value: %d\n", control_id, value);
                    in->in_parameters[i].value = value;	// current value
                }
            } else {
                LOG("Value (%d) out of range (%d .. %d)\n", value, min, max);
            }
            return 0;
        } else { // not user class controls
            DBG("Control type: EXTENDED\n");
            struct v4l2_ext_controls ext_ctrls = {0};
            struct v4l2_ext_control ext_ctrl = {0};
            ext_ctrl.id = in->in_parameters[i].ctrl.id;

            switch(in->in_parameters[i].ctrl.type) {
#ifdef V4L2_CTRL_TYPE_STRING
                case V4L2_CTRL_TYPE_STRING:
                    //string gets set on VIDIOC_G_EXT_CTRLS
                    //add the maximum size to value
                    ext_ctrl.size = value;
                    DBG("STRING extended controls are currently broken\n");
                    //ext_ctrl.string = control->string; // FIXMEE
                    break;
#endif
                case V4L2_CTRL_TYPE_INTEGER64:
                    ext_ctrl.value64 = value;
                    break;
                default:
                    ext_ctrl.value = value;
                    break;
            }

            ext_ctrls.count = 1;
            ext_ctrls.controls = &ext_ctrl;
            ret = xioctl(vd->fd, VIDIOC_S_EXT_CTRLS, &ext_ctrls);
            if(ret) {
                LOG("control id: 0x%08x failed to set value (error %i)\n", ext_ctrl.id, ret);
                return -1;
            } else {
                DBG("control id: 0x%08x new value: %d\n", ext_ctrl.id, ext_ctrl.value);
                in->in_parameters[i].value = value;	// current value
            }
            return 0;
        }
    } else {
        LOG("Invalid V4L2_set_control request for the id: 0x%08x. Control cannot be found in the list\n", control_id);
        return -1;
    }
}

/******************************************************************************
* @Description.:v4l2ResetControl 
* @Input Value.:
* 		@vd
* 		@control
* @Return Value:
******************************************************************************/
int v4l2ResetControl(struct vdIn *vd, int control)
{
    struct v4l2_control control_s;
    struct v4l2_queryctrl queryctrl;
    int val_def;
    int err;

    if(isv4l2Control(vd, control, &queryctrl) < 0)
        return -1;

    val_def = queryctrl.default_value;
    control_s.id = control;
    control_s.value = val_def;

    if((err = xioctl(vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
        return -1;
    }

    return 0;
}

int v4l2GetSoftControl(input *in, int control_id, int *value, int *min, int *max, int *default_val)
{
    int i;
    int got = -1;
    DBG("Looking for the 0x%08x V4L2 control\n", control_id);
    for (i = 0; i < in->parametercount; i++) {
        if (in->in_parameters[i].ctrl.id == control_id) {
            got = 0;
            break;
        }
    }
    if (got == 0) {
    	*value = in->in_parameters[i].value;
        *min = in->in_parameters[i].ctrl.minimum;
        *max = in->in_parameters[i].ctrl.maximum;
        *default_val = in->in_parameters[i].ctrl.default_value;

	return 0;
    }

    return -1;
}

void control_readed(struct vdIn *vd, struct v4l2_queryctrl *ctrl, input *in)
{
    struct v4l2_control c;
    memset(&c, 0, sizeof(struct v4l2_control));
    c.id = ctrl->id;
    static int n = 0;

    if (in->in_parameters == NULL) {
        in->in_parameters = (control*)calloc(1, sizeof(control));
    } else {
        in->in_parameters =
        (control*)realloc(in->in_parameters,(in->parametercount + 1) * sizeof(control));
    }

    if (in->in_parameters == NULL) {
        DBG("Calloc failed\n");
        return;
    }

    memcpy(&in->in_parameters[in->parametercount].ctrl, ctrl, sizeof(struct v4l2_queryctrl));
    in->in_parameters[in->parametercount].group = IN_CMD_V4L2;
    in->in_parameters[in->parametercount].value = c.value;
    DBG("%d:id=0x%08x,name=%s,min=%d, max=%d,default=%d\n", n++, ctrl->id, ctrl->name, ctrl->minimum, ctrl->maximum, ctrl->default_value);
    if(ctrl->type == V4L2_CTRL_TYPE_MENU) {
        in->in_parameters[in->parametercount].menuitems =
            (struct v4l2_querymenu*)malloc((ctrl->maximum + 1) * sizeof(struct v4l2_querymenu));
        int i;
        for(i = ctrl->minimum; i <= ctrl->maximum; i++) {
            struct v4l2_querymenu qm;
            memset(&qm, 0 , sizeof(struct v4l2_querymenu));
            qm.id = ctrl->id;
            qm.index = i;
            if(xioctl(vd->fd, VIDIOC_QUERYMENU, &qm) == 0) {
                memcpy(&in->in_parameters[in->parametercount].menuitems[i], &qm, sizeof(struct v4l2_querymenu));
                DBG("Menu item %d: %s\n", qm.index, qm.name);
            } else {
                DBG("Unable to get menu item for %s, x=%d\n", ctrl->name, qm.index);
            }
        }
    } else {
        in->in_parameters[in->parametercount].menuitems = NULL;
    }

    in->in_parameters[in->parametercount].class_id = (ctrl->id & 0xFFFF0000);

    int ret = -1;
    if (in->in_parameters[in->parametercount].class_id == V4L2_CTRL_CLASS_USER) {
        ret = xioctl(vd->fd, VIDIOC_G_CTRL, &c);
        if(ret == 0) {
            in->in_parameters[in->parametercount].value = c.value;
            DBG("V4L2 parameter found: %s value %d Class: USER \n", ctrl->name, c.value);
        } else {
            DBG("Unable to get the value of %s retcode: %d  %s\n", ctrl->name, ret, strerror(errno));
        }
    } else {
	// camera class control
        struct v4l2_ext_controls ext_ctrls = {0};
        struct v4l2_ext_control ext_ctrl = {0};
        ext_ctrl.id = ctrl->id;
#ifdef V4L2_CTRL_TYPE_STRING
        ext_ctrl.size = 0;
        if(ctrl.type == V4L2_CTRL_TYPE_STRING) {
            ext_ctrl.size = ctrl->maximum + 1;
            // FIXMEEEEext_ctrl.string = control->string;
        }
#endif
        ext_ctrls.count = 1;
        ext_ctrls.controls = &ext_ctrl;
        ret = xioctl(vd->fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls);
        if(ret) {
            switch (ext_ctrl.id) {
                case V4L2_CID_PAN_RESET:
                    in->in_parameters[in->parametercount].value = 1;
                    DBG("Setting PAN reset value to 1\n");
                    break;
                case V4L2_CID_TILT_RESET:
                    in->in_parameters[in->parametercount].value = 1;
                    DBG("Setting the Tilt reset value to 2\n");
                    break;

#define V4L2_CID_BASE_LOGITECH			V4L2_CID_BASE_EXTCTR
#define V4L2_CID_PANTILT_RESET_LOGITECH         V4L2_CID_BASE_LOGITECH+2
                /* case V4L2_CID_PANTILT_RESET_LOGITECH: */
                    /* in->in_parameters[in->parametercount].value = 3; */
                    /* DBG("Setting the PAN/TILT reset value to 3\n"); */
                    /* break; */
                default:
                    DBG("control id: 0x%08x failed to get value (error %i)\n", ext_ctrl.id, ret);
            }
        } else {
            switch(ctrl->type)
            {
#ifdef V4L2_CTRL_TYPE_STRING
                case V4L2_CTRL_TYPE_STRING:
                    //string gets set on VIDIOC_G_EXT_CTRLS
                    //add the maximum size to value
                    in->in_parameters[in->parametercount].value = ext_ctrl.size;
		    DBG("Setting string 0x%08x: %d\n", ext_ctrl.id, ext_ctrl.size);
                    break;
#endif
                case V4L2_CTRL_TYPE_INTEGER64:
                    in->in_parameters[in->parametercount].value = ext_ctrl.value64;
                    break;
                default:
                    in->in_parameters[in->parametercount].value = ext_ctrl.value;
		    DBG("Setting vendor 0x%08x: %d\n", ext_ctrl.id, ext_ctrl.size);
                    break;
            }
        }
        DBG("V4L2 parameter found: %s value %d Class: EXTENDED \n", ctrl->name, in->in_parameters[in->parametercount].value);
    }

    in->parametercount++;
}


int v4l2_get_resolution(struct vdIn *vd, int *width, int *height)
{
        *width = vd->width;
        *height = vd->height;
}

/*  It should set the capture resolution
    Cheated from the openCV cap_libv4l.cpp the method is the following:
    Turn off the stream (video_disable)
    Unmap buffers
    Close the filedescriptor
    Initialize the camera again with the new resolution
*/
int v4l2_set_resolution(struct vdIn *vd, int width, int height)
{
    int ret;
    DBG("v4l2_set_resolution(%d, %d)\n", width, height);

    vd->streamingState = STREAMING_PAUSED;
    if(video_disable(vd, STREAMING_PAUSED) == 0) {  // do streamoff
        DBG("Unmap buffers\n");
        int i;
        for(i = 0; i < NB_BUFFER; i++)
            munmap(vd->mem[i], vd->buf.length);

        if(close(vd->fd) == 0) {
            DBG("Device closed successfully\n");
        }

        vd->width = width;
        vd->height = height;
        if(init_v4l2(vd) < 0) {
            fprintf(stderr, " Init v4L2 failed !! exit fatal \n");
            return -1;
        } else {
            DBG("re done\n");
            video_enable(vd);
            return 0;
        }
    } else {
        DBG("Unable to disable streaming\n");
        return -1;
    }
    return ret;
}

/******************************************************************************
* @Description.:enumerateControls 
*		 Enumarates all V4L2 controls using various methods.
* @Input Value.:
* 		@vd
* 		@in
******************************************************************************/
void enumerateControls(struct vdIn *vd, input *in)
{
    // enumerating v4l2 controls
    struct v4l2_queryctrl ctrl;
    memset(&ctrl, 0, sizeof(struct v4l2_queryctrl));
    in->parametercount = 0;
    in->in_parameters = malloc(0 * sizeof(control));

    /* Enumerate the v4l2 controls
     Try the extended control API first */
    ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    if(0 == ioctl(vd->fd, VIDIOC_QUERYCTRL, &ctrl)) {
        do {
            control_readed(vd, &ctrl, in);
            ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
        } while(0 == ioctl(vd->fd, VIDIOC_QUERYCTRL, &ctrl));
        // note: use simple ioctl or v4l2_ioctl instead of the xioctl
    }

    memset(&in->jpegcomp, 0, sizeof(struct v4l2_jpegcompression));
    if(xioctl(vd->fd, VIDIOC_G_JPEGCOMP, &in->jpegcomp) != EINVAL) {
        DBG("JPEG compression details:\n");
        DBG("Quality: %d\n", in->jpegcomp.quality);
        DBG("APPn: %d\n", in->jpegcomp.APPn);
        DBG("APP length: %d\n", in->jpegcomp.APP_len);
        DBG("APP data: %s\n", in->jpegcomp.APP_data);
        DBG("COM length: %d\n", in->jpegcomp.COM_len);
        DBG("COM data: %s\n", in->jpegcomp.COM_data);
        struct v4l2_queryctrl ctrl_jpeg;
        ctrl_jpeg.id = 1;
        sprintf((char*)&ctrl_jpeg.name, "JPEG quality");
        ctrl_jpeg.minimum = 0;
        ctrl_jpeg.maximum = 100;
        ctrl_jpeg.step = 1;
        ctrl_jpeg.default_value = 50;
        ctrl_jpeg.flags = 0;
        ctrl_jpeg.type = V4L2_CTRL_TYPE_INTEGER;
        if (in->in_parameters == NULL) {
            in->in_parameters = (control*)calloc(1, sizeof(control));
        } else {
            in->in_parameters = (control*)realloc(in->in_parameters,(in->parametercount + 1) * sizeof(control));
        }

        if (in->in_parameters == NULL) {
            DBG("Calloc/realloc failed\n");
            return;
        }

        memcpy(&in->in_parameters[in->parametercount].ctrl, &ctrl_jpeg, sizeof(struct v4l2_queryctrl));
        /* in->in_parameters[in->parametercount].group = IN_CMD_JPEG_QUALITY; */
        in->in_parameters[in->parametercount].value = in->jpegcomp.quality;
        in->parametercount++;
    } else {
        DBG("Modifying the setting of the JPEG compression is not supported\n");
        in->jpegcomp.quality = -1;
    }
}

int input_init(context *pctx, char *dev)
{
    int width = 1280 , height = 720, fps = -1, format = V4L2_PIX_FMT_MJPEG, i;
    v4l2_std_id tvnorm = V4L2_STD_UNKNOWN;
	
    DBG_ENTER();
    /* initialize the mutes variable */
    if(pthread_mutex_init(&pctx->controls_mutex, NULL) != 0) {
        LOG("could not initialize mutex variable\n");
	return -1;
    }

    /* allocate webcam datastructure */
    pctx->videoIn = calloc(1, sizeof(struct vdIn));
    if(pctx->videoIn == NULL) {
        LOG("not enough memory for videoIn\n");
	return -ENOMEM;
    }

    /* allocate webcam datastructure */
    pctx->in = calloc(1, sizeof(struct _input));
    if(pctx->videoIn == NULL) {
        LOG("not enough memory for input\n");
	free(pctx->videoIn);
	return -ENOMEM;
    }
    pctx->in->context = pctx;

    /* display the parsed values */
    LOG("----------Default Video params----------\n");
    LOG("Using V4L2 device.: %s\n", dev);
    LOG("Desired Resolution: %i x %i\n", width, height);
    LOG("Frames Per Second.: %i\n", fps);
    LOG("Format............: JPEG\n");

    /* open video device and prepare data structure */
    if(init_videoin(pctx->videoIn, dev, width, height, fps, format, 1, tvnorm, pctx->in) < 0) {
        LOG("init_VideoIn failed\n");
	free(pctx->videoIn);
	free(pctx->in);
	return -2;
    }
    
    enumerateControls(pctx->videoIn, pctx->in); // enumerate V4L2 controls after UVC extended mapping
    DBG("Exit\n");
    return 0;
}

/******************************************************************************
Description.: process commands, allows to set v4l2 controls
Input Value.: * control specifies the selected v4l2 control's id
                see struct v4l2_queryctr in the videodev2.h
              * value is used for control that make use of a parameter.
Return Value: depends in the command, for most cases 0 means no errors and
              -1 signals an error. This is just rule of thumb, not more!
******************************************************************************/
int input_cmd(context *pctx, unsigned int control_id, unsigned int group, int value, char *value_string)
{
    input * in = pctx->in;
    int ret = -1;
    int i = 0;
    DBG("Requested cmd (id: %d) for the plugin. Group: %d value: %d\n", control_id, group, value);
    switch(group) {
    case IN_CMD_GENERIC: {
            int i;
            for (i = 0; i<in->parametercount; i++) {
                if ((in->in_parameters[i].ctrl.id == control_id) &&
                    (in->in_parameters[i].group == IN_CMD_GENERIC)){
                    DBG("Generic control found (id: %d): %s\n", control_id, in->in_parameters[i].ctrl.name);
                    DBG("New %s value: %d\n", in->in_parameters[i].ctrl.name, value);
                    return 0;
                }
            }
            DBG("Requested generic control (%d) did not found\n", control_id);
            return -1;
        } break;
    case IN_CMD_V4L2: {
           ret = v4l2SetControl(pctx->videoIn, control_id, value, in);
            if(ret == 0) {
                in->in_parameters[i].value = value;
            } else {
                DBG("v4l2SetControl failed: %d\n", ret);
            }
            return ret;
        } break;
    case IN_CMD_RESOLUTION: {
        // the value points to the current formats nth resolution
        if(value > (in->in_formats[in->currentFormat].resolutionCount - 1)) {
            DBG("The value is out of range");
            return -1;
        }
        int height = in->in_formats[in->currentFormat].supportedResolutions[value].height;
        int width = in->in_formats[in->currentFormat].supportedResolutions[value].width;
        ret = v4l2_set_resolution(pctx->videoIn, width, height);
        if(ret == 0) {
            in->in_formats[in->currentFormat].currentResolution = value;
        }
        return ret;
    } break;
    case IN_CMD_JPEG_QUALITY:
        if((value >= 0) && (value < 101)) {
            in->jpegcomp.quality = value;
            if(ioctl(pctx->videoIn->fd, VIDIOC_S_JPEGCOMP, &in->jpegcomp) != EINVAL) {
                DBG("JPEG quality is set to %d\n", value);
                ret = 0;
            } else {
                DBG("Setting the JPEG quality is not supported\n");
            }
        } else {
            DBG("Quality is out of range\n");
        }
        break;
    }
    return ret;
}

void video_set(context *pctx, context_settings *settings)
{
    unsigned int every_count = 0;
    int quality = settings->quality;

    #define V4L_OPT_SET(vid, var, desc) \
      if (input_cmd(pctx, vid, IN_CMD_V4L2, settings->var, NULL) != 0) {\
          fprintf(stderr, "Failed to set " desc "\n"); \
      } else { \
          printf(" i: %-18s: %d\n", desc, settings->var); \
      }
    
    #define V4L_INT_OPT(vid, var, desc) \
      if (settings->var##_set) { \
          V4L_OPT_SET(vid, var, desc) \
      }
    
    /* V4L options */
    V4L_INT_OPT(V4L2_CID_SHARPNESS, sh, "sharpness")
    V4L_INT_OPT(V4L2_CID_CONTRAST, co, "contrast")
    V4L_INT_OPT(V4L2_CID_SATURATION, sa, "saturation")
    V4L_INT_OPT(V4L2_CID_BACKLIGHT_COMPENSATION, bk, "backlight compensation")
    V4L_INT_OPT(V4L2_CID_ROTATE, rot, "rotation")
    V4L_INT_OPT(V4L2_CID_HFLIP, hf, "hflip")
    V4L_INT_OPT(V4L2_CID_VFLIP, vf, "vflip")
    V4L_INT_OPT(V4L2_CID_VFLIP, pl, "power line filter")
    
    if (settings->br_set) {
        V4L_OPT_SET(V4L2_CID_AUTOBRIGHTNESS, br_auto, "auto brightness mode")
        
        if (settings->br_auto == 0) {
            V4L_OPT_SET(V4L2_CID_BRIGHTNESS, br, "brightness")
        }
    }
    
    if (settings->wb_set) {
        V4L_OPT_SET(V4L2_CID_AUTO_WHITE_BALANCE, wb_auto, "auto white balance mode")
        
        if (settings->wb_auto == 0) {
            V4L_OPT_SET(V4L2_CID_WHITE_BALANCE_TEMPERATURE, wb, "white balance temperature")
        }
    }
    
    if (settings->ex_set) {
        V4L_OPT_SET(V4L2_CID_EXPOSURE_AUTO, ex_auto, "exposure mode")
        if (settings->ex_auto == V4L2_EXPOSURE_MANUAL) {
            V4L_OPT_SET(V4L2_CID_EXPOSURE_ABSOLUTE, ex, "absolute exposure")
        }
    }
    
    if (settings->gain_set) {
        V4L_OPT_SET(V4L2_CID_AUTOGAIN, gain_auto, "auto gain mode")
        
        if (settings->gain_auto == 0) {
            V4L_OPT_SET(V4L2_CID_GAIN, gain, "gain")
        }
    }
    
    if (settings->cagc_set) {
        V4L_OPT_SET(V4L2_CID_AUTO_WHITE_BALANCE, cagc_auto, "chroma gain mode")
        
        if (settings->cagc_auto == 0) {
            V4L_OPT_SET(V4L2_CID_WHITE_BALANCE_TEMPERATURE, cagc, "chroma gain")
        }
    }
    
    if (settings->cb_set) {
        V4L_OPT_SET(V4L2_CID_HUE_AUTO, cb_auto, "color balance mode")
        
        if (settings->cb_auto == 0) {
            V4L_OPT_SET(V4L2_CID_HUE, cagc, "color balance")
        }
    }
}

int *save_jpeg(void *buf, int len)
{
	char buffer1[256] = {0}, buffer2[512] = {0};
	static unsigned long long counter = 0;
	time_t t;
	struct tm *now;
	int fd;


	/* prepare filename */
	memset(buffer1, 0, sizeof(buffer1));
	memset(buffer2, 0, sizeof(buffer2));

	/* get current time */
	t = time(NULL);
	now = localtime(&t);
	if(now == NULL) {
		perror("localtime");
		return NULL;
	}
	/* prepare string, add time and date values */
	if(strftime(buffer1, sizeof(buffer1), "%%s/%Y_%m_%d_%H_%M_%S_picture_%%09llu.jpg", now) == 0) {
		DBG("strftime returned 0\n");
		return NULL;
	}

	/* finish filename by adding the foldername and a counter value */
	snprintf(buffer2, sizeof(buffer2), buffer1, ".", counter);

	counter++;

	DBG("writing file: %s\n", buffer2);

	/* open file for write */
	if((fd = open(buffer2, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
		DBG("could not open the file %s\n", buffer2);
		return NULL;
	}

	/* save picture to file */
	if(write(fd, buf, len) < 0) {
		DBG("could not write to file %s\n", buffer2);
		perror("write()");
	}

	close(fd);

	return NULL;
}

int save_jpegs(input *in, int cnt)
{

	while (cnt--) {

		/* copy JPG picture to global buffer */
		pthread_mutex_lock(&in->db);
		pthread_cond_wait(&in->db_update, &in->db);
		save_jpeg(in->buf, in->size);

		pthread_mutex_unlock(&in->db);

	}
	return 0;
}

