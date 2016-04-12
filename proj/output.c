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

struct output_tcp {
	struct sockaddr_in addr;
	int sockfd;
	int init_conn;
	char peer[16];
	int port;
};

static struct output_tcp tcp_t;

void worker_cleanup(void *arg)
{
    static unsigned char first_run = 1;

    if(!first_run) {
        DBG("already cleaned up resources\n");
        return;
    }

    first_run = 0;
    LOG("cleaning up resources allocated by worker thread\n");

}

/**
* @brief worker_thread this is the main worker thread
*
* @param arg
*
* @retval 
*/
void *worker_thread(void *arg)
{
    int ok = 1, frame_size = 0, n, cnt;
    unsigned char *tmp_framebuffer = NULL, *frame = NULL;
    input *in = arg;
    char *p;

    /* set cleanup handler to cleanup allocated resources */
    pthread_cleanup_push(worker_cleanup, NULL);

    while(ok >= 0 && tcp_t.init_conn) {

	DBG("wait frame complete\n");
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

	p = in->buf;
	while (frame_size) {
		cnt = frame_size > 10240 ? 10240 : frame_size;
		/* cnt = frame_size > 1480 ? 1480 : frame_size; */
		n = write(tcp_t.sockfd, p, cnt);
		if (n >= 0) {
			p += n;
			frame_size -= n;
		} else 
			break;
	}
	DBG("send to udp server complete\n");

        /* if specified, wait now */
        if(delay > 0) {
            usleep(1000 * delay);
        }
    }

    // close TCP port
    if(tcp_t.port > 0) {
        tcp_t.init_conn = 0;	
        close(tcp_t.sockfd);
    }

    /* cleanup now */
    pthread_cleanup_pop(1);

    return NULL;
}

int output_init(char *peer, int dst_port)
{
    int ret;
    struct sockaddr_in *addr = &tcp_t.addr;
    
    bzero(&tcp_t, sizeof(struct output_tcp));

    tcp_t.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_t.sockfd < 0) {
	perror("socket");
	return -1;
    }

    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(peer);
    addr->sin_port = htons(dst_port);

    ret = connect(tcp_t.sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
    if (ret == 0) {
    	memset(tcp_t.peer, 0x0, sizeof(tcp_t.peer));
    	snprintf(tcp_t.peer, sizeof(tcp_t.peer), "%s", peer);
	tcp_t.port = dst_port;
	tcp_t.init_conn = 1;

    } else {
	perror("connect");
   }
 
    return ret;
}

int output_stop(int id)
{
    DBG("will cancel worker thread\n");
    pthread_cancel(worker);
    return 0;
}

int output_run(input *in)
{
    if (tcp_t.init_conn) {
     	    DBG("launching worker thread\n");
	    pthread_create(&worker, 0, worker_thread, in);
	    pthread_detach(worker);
	    return 0;
    }

    return -1;
}

