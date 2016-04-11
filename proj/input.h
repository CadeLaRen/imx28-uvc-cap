#ifndef __IMX_V4L2_H__
#define __IMX_V4L2_H__

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

#define NB_BUFFER 4
#define IOCTL_RETRY 4

/* commands which can be send to the input plugin */
//typedef enum _cmd_group cmd_group;
enum _cmd_group {
    IN_CMD_GENERIC =        0, // if you use non V4L2 input plugin you not need to deal the groups.
    IN_CMD_V4L2 =           1,
    IN_CMD_RESOLUTION =     2,
    IN_CMD_JPEG_QUALITY =   3,
    IN_CMD_PWC =            4,
};
typedef struct _control control;
struct _control {
    struct v4l2_queryctrl ctrl;
    int value; //ctrl->cur_set_value
    struct v4l2_querymenu *menuitems;
    /*  In the case the control a V4L2 ctrl this variable will specify
        that the control is a V4L2_CTRL_CLASS_USER control or not.
        For non V4L2 control it is not acceptable, leave it 0.
    */
    int class_id; // 0xffff0000 USER/EXTEND
    int group;	// main_t
};


typedef struct _input_resolution input_resolution;
struct _input_resolution {
    unsigned int width;
    unsigned int height;
};

typedef struct _input_format input_format;
struct _input_format {
    struct v4l2_fmtdesc format;
    input_resolution *supportedResolutions;
    int resolutionCount;
    char currentResolution;
};

/* structure to store variables/functions for input plugin */
typedef struct _input input;
struct _input {
    char *name;

    // input plugin parameters
    struct _control *in_parameters;
    int parametercount;

    struct v4l2_jpegcompression jpegcomp;

    /* signal fresh frames */
    pthread_mutex_t db;
    pthread_cond_t  db_update;

    /* global JPG frame, this is more or less the "database" */
    unsigned char *buf;
    int size;

    /* v4l2_buffer timestamp */
    struct timeval timestamp;

    input_format *in_formats;	// MJPEG, YUV, RGB...
    int formatCount;
    int currentFormat; // holds the current format number, point to index
    
    void *context; // private data for the plugin
};

typedef enum _streaming_state streaming_state;
enum _streaming_state {
    STREAMING_OFF = 0,
    STREAMING_ON = 1,
    STREAMING_PAUSED = 2,
};
struct vdIn {
    int fd;
    char *videodevice;
    char *status;
    char *pictName;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    void *mem[NB_BUFFER];
    unsigned char *tmpbuffer;
    unsigned char *framebuffer;
    streaming_state streamingState;
    int grabmethod;
    int width;
    int height;
    int fps;
    int formatIn;
    int formatOut;
    int framesizeIn;
    int signalquit;
    int toggleAvi;
    int getPict;
    int rawFrameCapture;
    /* raw frame capture */
    unsigned int fileCounter;
    /* raw frame stream capture */
    unsigned int rfsFramesWritten;
    unsigned int rfsBytesWritten;
    /* raw stream capture */
    FILE *captureFile;
    unsigned int framesWritten;
    unsigned int bytesWritten;
    int framecount;
    int recordstart;
    int recordtime;
    uint32_t tmpbytesused;
    struct timeval tmptimestamp;
    v4l2_std_id vstd;
    unsigned long frame_period_time; // in ms
    unsigned char soft_framedrop;
};
/* optional initial settings */
typedef struct {
    int quality_set, quality,
        sh_set, sh,
        co_set, co,
        br_set, br_auto, br,
        sa_set, sa,
        wb_set, wb_auto, wb,
        ex_set, ex_auto, ex,
        bk_set, bk,
        rot_set, rot,
        hf_set, hf,
        vf_set, vf,
        pl_set, pl,
        gain_set, gain_auto, gain,
        cagc_set, cagc_auto, cagc,
        cb_set, cb_auto, cb;
} context_settings;

/* context of each camera thread */
typedef struct {
    pthread_t threadID;
    pthread_mutex_t controls_mutex;
    struct vdIn *videoIn;
    input *in;
} context;
#endif /* __IMX_V4L2_H__ */
 
