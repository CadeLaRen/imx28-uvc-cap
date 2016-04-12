/*****************************************************************
* @file main.c
* @Function	
* @author xz.wang
* @version 
* @date 2016-04-07
*****************************************************************/
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
#include "types.h"
#include "input.h"

#define UVCDEV	"/dev/video0"

static unsigned int minimum_size = 0;
static unsigned int every = 1;
static int stop = 0;

/******************************************************************************
Description.: spins of a worker thread
Input Value.: -
Return Value: always 0
******************************************************************************/
void *cam_thread(void *arg);
int input_run(context *pctx)
{
    input *in = pctx->in;
    
    DBG("Enter\n");
    in->buf = malloc(pctx->videoIn->framesizeIn);
    if(in->buf == NULL) {
        fprintf(stderr, "could not allocate memory\n");
	return -ENOMEM;
    }

    /* create thread and pass context to thread function */
    pthread_create(&(pctx->threadID), NULL, cam_thread, in);
    pthread_detach(pctx->threadID);
    DBG("Exit\n");
    return 0;
}


void cam_cleanup(void *arg);
/******************************************************************************
Description.: this thread worker grabs a frame and copies it to the global buffer
Input Value.: unused
Return Value: unused, always NULL
******************************************************************************/
void *cam_thread(void *arg)
{
    input *in = (input*)arg;
    context *pcontext = in->context;
    
    unsigned int every_count = 0;

    DBG("Enter\n");
    /* set cleanup handler to cleanup allocated resources */
    pthread_cleanup_push(cam_cleanup, in);

    while(!stop) {
        while(pcontext->videoIn->streamingState == STREAMING_PAUSED) {
            usleep(1); // maybe not the best way so FIXME
        }

        /* grab a frame */
        if(uvcGrab(pcontext->videoIn) < 0) {
            LOG("Error grabbing frames\n");
            exit(EXIT_FAILURE);
        }

        if ( every_count < every - 1 ) {
            DBG("dropping %d frame for every=%d\n", every_count + 1, every);
            ++every_count;
            continue;
        } else {
            every_count = 0;
        }

        /*
         * Workaround for broken, corrupted frames:
         * Under low light conditions corrupted frames may get captured.
         * The good thing is such frames are quite small compared to the regular pictures.
         * For example a VGA (640x480) webcam picture is normally >= 8kByte large,
         * corrupted frames are smaller.
         */
        if(pcontext->videoIn->tmpbytesused < minimum_size) {
            DBG("dropping too small frame, assuming it as broken\n");
            continue;
        }

        // use software frame dropping on low fps
        if (pcontext->videoIn->soft_framedrop == 1) {
            unsigned long last = in->timestamp.tv_sec * 1000 +
                                (in->timestamp.tv_usec/1000); // convert to ms
            unsigned long current = pcontext->videoIn->buf.timestamp.tv_sec * 1000 +
                                    pcontext->videoIn->buf.timestamp.tv_usec/1000; // convert to ms

            // if the requested time did not esplashed skip the frame
            if ((current - last) < pcontext->videoIn->frame_period_time) {
                //DBG("Last frame taken %d ms ago so drop it\n", (current - last));
                continue;
            }
            DBG("Lagg: %ld\n", (current - last) - pcontext->videoIn->frame_period_time);
        }

        /* copy JPG picture to global buffer */
	pthread_mutex_lock(&in->db);

	/*
	 * If capturing in YUV mode convert to JPEG now.
	 * This compression requires many CPU cycles, so try to avoid YUV format.
	 * Getting JPEGs straight from the webcam, is one of the major advantages of
	 * Linux-UVC compatible devices.
	 */
	DBG("copying frame from input:\n");
	in->size = memcpy_picture(in->buf, pcontext->videoIn->tmpbuffer, pcontext->videoIn->tmpbytesused);
	/* copy this frame's timestamp to user space */
	in->timestamp = pcontext->videoIn->tmptimestamp;


        /* signal fresh_frame */
        pthread_cond_broadcast(&in->db_update);
        pthread_mutex_unlock(&in->db);
    }

    DBG("leaving input thread, calling cleanup function now\n");
    pthread_cleanup_pop(1);

    return NULL;
}

/******************************************************************************
Description.:
Input Value.:
Return Value:
******************************************************************************/
void cam_cleanup(void *arg)
{
    input * in = (input*)arg;
    context *pctx = in->context;
    
    LOG("cleaning up resources allocated by input thread\n");

    if (pctx->videoIn != NULL) {
        close_v4l2(pctx->videoIn);
        free(pctx->videoIn);
        pctx->videoIn = NULL;
    }
    
    free(in->buf);
    in->buf = NULL;
    in->size = 0;
}

void fatal_signal(int signal)
{
	stop = 1;
}

static int fatal_signals[] = {
	SIGQUIT,
	SIGILL,
	SIGINT,
};

int signal_init(void)
{
	int i;
	int ret;

	for (i = 0; sizeof(fatal_signals)/sizeof(fatal_signals[0]); i++) {
		signal(fatal_signals[i], fatal_signal);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ret, len;
	context *pctx;
	input *in;
	char buf[512];
	int send = 0, backgroud = 0, i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "tcp")) {
			send = 1;
		}
		if (!strcmp(argv[i], "deamon")) {
			backgroud = 1;
		}
	}
	if (backgroud)
		daemon_mode();

	pctx = calloc(1, sizeof(context));
	if (pctx == NULL) {
		DBG("error allocating context\n");
		return -ENOMEM;
	}

	if(signal(SIGINT, fatal_signal) == SIG_ERR) {
		DBG("could not register signal handler\n");
		return -EINVAL;
	}

	// create syslog
	openlog("iMX28-UVC", LOG_PID | LOG_CONS, LOG_USER);


	// get params from flash
	ret = params_load();
	if (ret < 0) {
		fprintf(stderr, "load nvram params failed...\n");
	}

	// open manage serial, 1152008N1
	ret = serial_init(NULL);
	if (ret < 0) {
		fprintf(stderr, "serial_init failed\n");
		free(pctx);
		closelog();
		return -1;
	}

	// register input video stream
	ret = input_init(pctx, UVCDEV);
	if (ret < 0) {
		fprintf(stderr, "input_init failed, exit\n");
		goto exit;
        }

	if (send) {
	    // register output video stream
            ret = output_init("192.168.0.99", 5500);	// tcp
	    if (ret < 0 ) {
		goto exit;
	    }
	}
	ret = input_run(pctx);
	if (ret < 0) {
		goto exit;	
	}

	if (send)
		ret = output_run(pctx->in);

	in = pctx->in;
	while (!stop && in) {
		/* save_jpeg(in->buf, in->size); */
		/* allow others to access the global buffer again */
		/* sys_if_test();	 */
		len = recv_packet(buf, sizeof(buf));
		if (len > 0) {
			hex_dump("SP0 RECV:", buf, len);
			packet_parse(pctx, buf, len);
		} 
	}

	/* wait for signals */
	pause();

	if (send)
		output_stop();
exit:
	if (pctx->in->buf)
		free(pctx->in->buf);
	free(pctx);
	closelog();
	serial_exit();
	DBG("%s:Exit\n", __func__);

	return 0;
}

