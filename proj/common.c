/******************************************************************************
* @file common.c
* @Description.:
* @author xz.wang 15778225@qq.com
* @version 
* @date 2016-04-10
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <dirent.h>


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <string.h>
#include <net/route.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "common.h"

/******************************************************************************
* @Description.:hex_dump 
* @Input Value.:
* 		@prompt
* 		@buf
* 		@len
******************************************************************************/
void hex_dump(const char *prompt, unsigned char *buf, int len)
{
        int i;

        for (i = 0; i < len; i++) {
                if ((i & 0x0f) == 0) { 
                        fprintf(stderr, "\n%s | 0x%04x", prompt, i);
                }    
                fprintf(stderr, " %02x", *buf++);
        }    
        fprintf(stderr, "\n");
}


void daemon_mode(void)
{
    int fr = 0;

    fr = fork();
    if(fr < 0) {
        fprintf(stderr, "fork() failed\n");
        exit(1);
    }
    if(fr > 0) {
        exit(0);
    }

    if(setsid() < 0) {
        fprintf(stderr, "setsid() failed\n");
        exit(1);
    }

    fr = fork();
    if(fr < 0) {
        fprintf(stderr, "fork() failed\n");
        exit(1);
    }
    if(fr > 0) {
        fprintf(stderr, "forked to background (%d)\n", fr);
        exit(0);
    }

    umask(0);

    fr = chdir("/");
    if(fr != 0) {
        fprintf(stderr, "chdir(/) failed\n");
        exit(0);
    }

    close(0);
    close(1);
    close(2);

    open("/dev/null", O_RDWR);

    fr = dup(0);
    fr = dup(0);
}

/******************************************************************************
* @Description.:run_daemon 
* @Input Value.:
* 		@nochdir
* 		@noclose
* @Return Value:
******************************************************************************/
int run_daemon(int nochdir, int noclose)
{
	int pid;
	int fd = -1;

	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		return -1;
	}
	
	if (pid != 0)
		exit(0);

	/* Clean up*/
	/* ioctl(0, TIOCNOTTY, 0); */
	close(0);
	close(1);
	close(2);
	pid = setsid();
	if (pid < 0)
	{
		perror("setpid");
		return pid;
	}
	
	if (!nochdir)
	{
		chdir("/");
	}

	if (!noclose)
	{
		fd = open("/dev/null", O_RDWR);
		if (fd < 0)
		{
			perror("console");
			return fd;
		}
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		/* ioctl(0, TIOCSCTTY, 1); */
	}
	umask(0027);
	return 0;
}
static struct timeval tick_val= {0, 0};
int pipe_exec(char *shell)
{
	FILE *fp;
	char buffer[64];
	int ret = -1;

	fp = popen(shell, "r");
	if (fp != NULL)
	{
		memset(buffer, 0, sizeof(buffer));
		if (fread(buffer, 1, sizeof(buffer)-1, fp) > 0)
		{
			//fprintf(stderr, "CMD:%s, RET:%s", buffer);
			ret = 1;
		}
		else
		{
			ret = 0;
		}

		pclose(fp);
	}
	
	return ret;
}

int LSMOD(char *module)
{
	char cmd[32];

	memset(cmd, 0x0, sizeof(cmd));
	sprintf(cmd, "lsmod | grep %s", module);

	return pipe_exec(cmd);
}

int file_to_buf(char *path, char *buf, int len)
{
	FILE *fp;

	memset(buf, 0, len);

	if ((fp = fopen(path, "r"))) {
		fgets(buf, len, fp);
		fclose(fp);
		return 1;
	}

	return 0;
}

int get_ppp_pid(char *file)
{
	char buf[80];
	int pid = -1;

	if (file_to_buf(file, buf, sizeof(buf))) {
		char tmp[80], tmp1[80];

		snprintf(tmp, sizeof(tmp), "/var/run/%s.pid", buf);
		file_to_buf(tmp, tmp1, sizeof(tmp1));
		pid = atoi(tmp1);
	}
	return pid;
}


char *find_name_by_proc(int pid)
{
	FILE *fp;
	char line[254];
	char filename[80];
	static char name[80];

	snprintf(filename, sizeof(filename), "/proc/%d/status", pid);

	if ((fp = fopen(filename, "r"))) {
		fgets(line, sizeof(line), fp);
		/*
		 * Buffer should contain a string like "Name: binary_name" 
		 */
		sscanf(line, "%*s %s", name);
		fclose(fp);
		return name;
	}

	return "";
}

static int system2(char *command)
{
        return system(command);
}

int sysprintf(const char *fmt, ...)
{
        char varbuf[256];
        va_list args;

        va_start(args, (char *)fmt);
        vsnprintf(varbuf, sizeof(varbuf), fmt, args);
        va_end(args);
        return system2(varbuf);
}

int _evalpid(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;

	switch (pid = fork()) {
	case -1:		/* error */
		perror("fork");
		return errno;
	case 0:		/* child */
		/*
		 * Reset signal handlers set for parent process 
		 */
		for (sig = 0; sig < (_NSIG - 1); sig++)
			signal(sig, SIG_DFL);

		/*  Clean up  */
		ioctl(0, TIOCNOTTY, 0);
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		setsid();

		/*
		 * We want to check the board if exist UART? , add by honor
		 * 2003-12-04 
		 */
		if ((fd = open("/dev/console", O_RDWR)) < 0) {
			(void)open("/dev/null", O_RDONLY);
			(void)open("/dev/null", O_WRONLY);
			(void)open("/dev/null", O_WRONLY);
		} else {
			close(fd);
			(void)open("/dev/console", O_RDONLY);
			(void)open("/dev/console", O_WRONLY);
			(void)open("/dev/console", O_WRONLY);
		}

		/*
		 * Redirect stdout to <path> 
		 */
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				/*
				 * append to <path> 
				 */
				flags |= O_APPEND;
				path += 2;
			} else if (!strncmp(path, ">", 1)) {
				/*
				 * overwrite <path> 
				 */
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}
		}

		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		alarm(timeout);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:		/* parent */
		if (ppid) {
			*ppid = pid;
			return 0;
		} else {
			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

int _eval(char *const argv[],int timeout, int *ppid)
{
       return _evalpid(argv, ">/dev/console", timeout, ppid);
}

int f_exists(const char *path)  // note: anything but a directory
{
        struct stat st;

        return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode));
}


int f_read(const char *path, void *buffer, int max)
{
        int f;
        int n;

        if ((f = open(path, O_RDONLY)) < 0)
                return -1;
        n = read(f, buffer, max);
        close(f);
        return n;
}

int f_read_string(const char *path, char *buffer, int max)
{
        if (max <= 0)
                return -1;
        int n = f_read(path, buffer, max - 1);

        buffer[(n > 0) ? n : 0] = 0;
        return n;
}


char *psname(int pid, char *buffer, int maxlen)
{
        char buf[512];
        char path[64];
        char *p;

        if (maxlen <= 0)
                return NULL;
        *buffer = 0;
        sprintf(path, "/proc/%d/stat", pid);
        if ((f_read_string(path, buf, sizeof(buf)) > 4)
            && ((p = strrchr(buf, ')')) != NULL)) {
                *p = 0;
                if (((p = strchr(buf, '(')) != NULL) && (atoi(buf) == pid)) {
                        strncpy(buffer, p + 1, maxlen);
                }
        }
        return buffer;
}


static int _pidof(const char *name, pid_t ** pids)
{
        const char *p;
        char *e;
        DIR *dir;
        struct dirent *de;
        pid_t i;
        int count;
        char buf[256];

        count = 0;
        *pids = NULL;
        if ((p = strchr(name, '/')) != NULL)
                name = p + 1;
        if ((dir = opendir("/proc")) != NULL) {
                while ((de = readdir(dir)) != NULL) {
                        i = strtol(de->d_name, &e, 10);
                        if (*e != 0)
                                continue;
                        if (strcmp(name, psname(i, buf, sizeof(buf))) == 0) {
                                if ((*pids =
                                     realloc(*pids,
                                             sizeof(pid_t) * (count + 1))) ==
                                    NULL) {
                                        return -1;
                                }
                                (*pids)[count++] = i;
                        }
                }
        }
        closedir(dir);
        return count;
}


int pidof(const char *name)
{
        pid_t *pids;
        pid_t p;

        if (_pidof(name, &pids) > 0) {
                p = *pids;
                free(pids);
                return p;
        }
        return -1;
}


int killall(const char *name, int sig)
{
        pid_t *pids;
        int i;
        int r;

        if ((i = _pidof(name, &pids)) > 0) {
                r = 0;
                do {
                        r |= kill(pids[--i], sig);
                }
                while (i > 0);
                free(pids);
                return r;
        }
        return -2;
}

int kill_pid(int pid)
{
	int deadcnt = 20;
	struct stat s;
	char pfile[32];
	
	if (pid > 0)
	{
		kill(pid, SIGTERM);

		sprintf(pfile, "/proc/%d/stat", pid);
		while (deadcnt--)
		{
			usleep(100*1000);
			if ((stat(pfile, &s) > -1) && (s.st_mode & S_IFREG))
			{
				kill(pid, SIGKILL);
			}
			else
				break;
		}
		return 1;
	}
	
	return 0;
}

int stop_process(char *name)
{
	int deadcounter = 20;

        if (pidof(name) > 0) {
                killall(name, SIGTERM);
                while (pidof(name) > 0 && deadcounter--) {
                        usleep(100*1000);
                }
                if (pidof(name) > 0) {
                        killall(name, SIGKILL);
                }
                return 1;
        }
        return 0;
}

void get_sys_date(struct tm *tm)
{
	time_t timep;
	
	time(&timep);


	tm = localtime(&timep);

}

int get_tick_1hz(void)
{
	struct timeval tv;
	int tick;

	gettimeofday(&tv, NULL);
	tick = (tv.tv_sec - tick_val.tv_sec)*1000 + (tv.tv_usec - tick_val.tv_usec)/1000;

	return tick/1000;
}

void clktick_init(void)
{
	gettimeofday(&tick_val, NULL);
}

int get_tick_1khz(void)
{
	int tick;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	tick = (tv.tv_sec - tick_val.tv_sec)*1000 + (tv.tv_usec - tick_val.tv_usec)/1000;

	return tick;
}

#define GPIO_SYS_PATH	"/sys/class/gpio"

void gpio_export(int gpio, char *dir, int value)
{
	// set gpio-pinmux
	sysprintf("echo \"%d\" >%s/export", gpio, GPIO_SYS_PATH);
	// set gpio-direction and gpio-level: "in" or "out", 1 or 0
	if (!strcmp(dir, "in"))
	{
		sysprintf("echo \"in\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpio);
	}
	else
	{
		sysprintf("echo \"out\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpio);
		sysprintf("echo \"%d\" >%s/gpio%d/value", value, GPIO_SYS_PATH, gpio);
	}
}

void gpio_unexport(int gpio)
{
	sysprintf("echo \"%d\" >%s/unexport", gpio, GPIO_SYS_PATH);
}

void set_gpio_high(int gpionum)
{
//	sysprintf("echo \"out\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpionum);
	sysprintf("echo \"1\" >%s/gpio%d/value", GPIO_SYS_PATH, gpionum);
}

void set_gpio_low(int gpionum)
{
//	sysprintf("echo \"out\" >%s/gpio%d/direction", GPIO_SYS_PATH, gpionum);
	sysprintf("echo \"0\" >%s/gpio%d/value", GPIO_SYS_PATH, gpionum);
}

int get_gpio_level(int gpionum)
{
	return sysprintf("cat %s/gpio%d/value", GPIO_SYS_PATH, gpionum); 
}

