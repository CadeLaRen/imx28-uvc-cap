#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>

#include "input.h"

static pthread_t worker;
static int delay, max_frame_size;
static unsigned char *frame = NULL;

// UDP port
static int port = 0;

/******************************************************************************
Description.: clean up allocated resources
Input Value.: unused argument
Return Value: -
******************************************************************************/
void worker_cleanup(void *arg)
{
    static unsigned char first_run = 1;

    if(!first_run) {
        DBG("already cleaned up resources\n");
        return;
    }

    first_run = 0;
    LOG("cleaning up resources allocated by worker thread\n");

    if(frame != NULL) {
        free(frame);
    }
}

/******************************************************************************
Description.: this is the main worker thread
              it loops forever, grabs a fresh frame and stores it to file
Input Value.:
Return Value:
******************************************************************************/
void *worker_thread(void *arg)
{
    int ok = 1, frame_size = 0, rc = 0;
    unsigned char *tmp_framebuffer = NULL;
    struct sockaddr_in addr;
    int sd;
    int bytes;
    unsigned int addr_len = sizeof(addr);
    char udpbuffer[1024] = {0};
    input *in = arg;

    /* set cleanup handler to cleanup allocated resources */
    pthread_cleanup_push(worker_cleanup, NULL);

    // set UDP server data structures ---------------------------
    if(port <= 0) {
        LOG("a valid UDP port must be provided\n");
        return NULL;
    }
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    /* addr.sin_addr.s_addr = INADDR_ANY; */
    addr.sin_addr.s_addr = inet_addr("192.168.1.100");
    addr.sin_port = htons(port);
    if(bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
        perror("bind");
    // -----------------------------------------------------------

    DBG("Bind udp %s:%d\n", (char *)inet_ntoa(addr.sin_addr), port);

    struct sockaddr_in caddr;
    while(ok >= 0) {
        DBG("waiting for a UDP message\n");

        // UDP receive ---------------------------------------------
	memset(udpbuffer, 0, sizeof(udpbuffer));
	bytes = recvfrom(sd, udpbuffer, sizeof(udpbuffer), 0, (struct sockaddr*)&caddr, &addr_len);
        // ---------------------------------------------------------
	DBG("Got msg from %s:%u\n", inet_ntoa(caddr.sin_addr), ntohs(addr.sin_port));

        pthread_mutex_lock(&in->db);
        pthread_cond_wait(&in->db_update, &in->db);

        /* read buffer */
        frame_size = in->size;

        /* check if buffer for frame is large enough, increase it if necessary */
        if(frame_size > max_frame_size) {
            DBG("increasing buffer size to %d\n", frame_size);

            max_frame_size = frame_size + (1 << 16);
            if((tmp_framebuffer = realloc(frame, max_frame_size)) == NULL) {
                pthread_mutex_unlock(&in->db);
                LOG("not enough memory\n");
                return NULL;
            }

            frame = tmp_framebuffer;
        }

        /* copy frame to our local buffer now */
        memcpy(frame, in->buf, frame_size);

        /* allow others to access the global buffer again */
        pthread_mutex_unlock(&in->db);

        // send back client's message that came in udpbuffer
        /* sendto(sd, frame, frame_size, 0, (struct sockaddr*)&caddr, sizeof(caddr)); */
	while (frame_size) {
	    if (frame_size > 6000 ) {
	    	if(sendto(sd, frame, 6000, 0, (struct sockaddr *)&caddr, sizeof(struct sockaddr_in)) == -1) {
	        	perror("sendto");
			break;
		}
	    	frame_size -= 6000;
	    } else {
	    	if(sendto(sd, frame, frame_size, 0, (struct sockaddr *)&caddr, sizeof(struct sockaddr_in)) == -1) {
	        	perror("sendto");
			break;
		}
	    	frame_size -= frame_size;
	    }
	}

        /* if specified, wait now */
        if(delay > 0) {
            usleep(1000 * delay);
        }
    }

    // close UDP port
    if(port > 0)
        close(sd);

    /* cleanup now */
    pthread_cleanup_pop(1);

    return NULL;
}

/******************************************************************************
Description.: this function is called first, in order to initialise
              this plugin and pass a parameter string
Input Value.: parameters
Return Value: 0 if everything is ok, non-zero otherwise
******************************************************************************/
int output_init(int dst_port)
{
    port = dst_port;
    return 0;
}

/******************************************************************************
Description.: calling this function stops the worker thread
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_stop(int id)
{
    DBG("will cancel worker thread\n");
    pthread_cancel(worker);
    return 0;
}

/******************************************************************************
Description.: calling this function creates and starts the worker thread
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_run(input *in)
{
    DBG("launching worker thread\n");
    pthread_create(&worker, 0, worker_thread, in);
    pthread_detach(worker);
    return 0;
}

