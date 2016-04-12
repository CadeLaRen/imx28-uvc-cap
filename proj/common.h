#ifndef __common_h__
#define __common_h__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include "types.h"
#include <sys/time.h>
#include <time.h>
#include <syslog.h>

#define ABS(a) (((a) < 0) ? -(a) : (a))
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#define LENGTH_OF(x) (sizeof(x)/sizeof(x[0]))

#define eval(cmd, args...) ({ \
        char *argv[] = { cmd, ## args, NULL }; \
        _eval(argv, 0, NULL); \
})

#define eval_pid(ppid, cmd, args...) ({	\
        char *argv[] = { cmd, ## args, NULL }; \
		_eval(argv, 0, ppid);\
})

#define eval_time(timeout,cmd, args...) ({ \
        char *argv[] = { 	\
        cmd, ##args, NULL }; \
        _eval(argv, timeout, NULL); \
})

#if 1
//#define DBG(fmt, arg...) \
	do {fprintf(stderr, "DBG>> " fmt, ##arg);} while(0)
#define DBG(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)

#define DBG_ENTER()		do{fprintf(stderr, "DBG>> %s: Enter\n", __func__);}while(0)
#define DBG_EXIT()		do{fprintf(stderr, "DBG>> %s: Exit\n", __func__);}while(0)

#define SYSLOG(fmt, arg...) \
	do {fprintf(stderr, "DBG>> " fmt, ##arg); \
		openlog("VPND SYSLOG", LOG_CONS|LOG_PID, 0); \
		syslog(LOG_USER|LOG_INFO, "SYS_LOG>> " fmt, ##arg); \
		closelog();} while(0)
#else
#define DBG(fmt, arg...) do{;}while(0)
//#define DBG(...)
#endif

#define LOG(...) { char _bf[1024] = {0}; snprintf(_bf, sizeof(_bf)-1, __VA_ARGS__); fprintf(stderr, "%s", _bf); syslog(LOG_INFO, "%s", _bf); }

#define FORK(func) \
{ \
    switch ( fork(  ) ) \
    { \
        case -1: \
            break; \
        case 0: \
            ( void )setsid(  ); \
            func; \
            exit(0); \
            break; \
        default: \
        break; \
    } \
}

void daemon_mode(void);
void hex_dump(const char *prompt, unsigned char *buf, int len);
int pidof(const char *name);
int sysprintf(const char *fmt, ...);
int killall(const char *name, int sig);
int stop_process(char *name);

void clktick_init(void);
int get_tick_1hz(void);
int get_tick_1khz(void);

void get_sys_date(struct tm *tm);

// gpio number
// gpio-operations
void gpio_export(int gpio, char *dir, int value);
void gpio_unexport(int gpio);
void set_gpio_high(int gpionum);
void set_gpio_low(int gpionum);
int get_gpio_level(int gpionum);
int get_gateway(char *gw);

#endif /* __shared_h__ */

