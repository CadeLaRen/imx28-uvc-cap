/*
 * NVRAM variable manipulation
 *
 * Copyright 2007, Broadcom Corporation
 * Copyright 2009, OpenWrt.org
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */

#ifndef _nvram_h_
#define _nvram_h_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <stdarg.h>


struct nvram_header {
	uint32_t magic;
	uint32_t len;
	uint32_t crc_ver_init;	/* 0:7 crc, 8:15 ver, 16:31 sdram_init */
	uint32_t config_refresh;	/* 0:15 sdram_config, 16:31 sdram_refresh */
	uint32_t config_ncdl;	/* ncdl values for memc */
} __attribute__((__packed__));

struct nvram_tuple {
	char *name;
	char *value;
	struct nvram_tuple *next;
};

struct nvram_handle {
	int fd;
	char *mmap;
	unsigned long length;
	struct nvram_tuple *nvram_hash[257];
	struct nvram_tuple *nvram_dead;
};

typedef struct nvram_handle nvram_handle_t;
typedef struct nvram_header nvram_header_t;
typedef struct nvram_tuple  nvram_tuple_t;


/* Get nvram header. */
nvram_header_t * nvram_header(nvram_handle_t *h);

/* Set the value of an NVRAM variable */
int _nvram_set(nvram_handle_t *h, const char *name, const char *value);

/* Get the value of an NVRAM variable. */
char * _nvram_get(nvram_handle_t *h, const char *name);

/* Unset the value of an NVRAM variable. */
int _nvram_unset(nvram_handle_t *h, const char *name);

/* Get all NVRAM variables. */
nvram_tuple_t * _nvram_getall(nvram_handle_t *h);

/* Regenerate NVRAM. */
int _nvram_commit(nvram_handle_t *h);

/* Open NVRAM and obtain a handle. */
nvram_handle_t * _nvram_open(const char *file, int rdonly);

/* Close NVRAM and free memory. */
int _nvram_close(nvram_handle_t *h);

/* Get the value of an NVRAM variable in a safe way, use "" instead of NULL. */
#define _nvram_safe_get(h, name) (nvram_get(h, name) ? : "")

/* Computes a crc8 over the input data. */
uint8_t hndcrc8 (uint8_t * pdata, uint32_t nbytes, uint8_t crc);

/* Returns the crc value of the nvram. */
uint8_t nvram_calc_crc(nvram_header_t * nvh);

/* Determine NVRAM device node. */
char * nvram_find_mtd(void);

/* Copy NVRAM contents to staging file. */
int nvram_to_staging(void);

/* Copy staging file to NVRAM device. */
int staging_to_nvram(void);

/* Check NVRAM staging file. */
char * nvram_find_staging(void);


/* Staging file for NVRAM */
#define NVRAM_LOCK          "/tmp/.nvram_lock"
#define NVRAM_STAGING	"/tmp/.nvram"
#define NVRAM_MTD           "/etc/nvram"
#define NVRAM_RO			1
#define NVRAM_RW			0

/* Helper macros */
#define NVRAM_ARRAYSIZE(a)	sizeof(a)/sizeof(a[0])
#define	NVRAM_ROUNDUP(x, y)	((((x)+((y)-1))/(y))*(y))

/* NVRAM constants */
#define NVRAM_COUNT         0x10000
#define NVRAM_SPACE			0x8000
#define NVRAM_START(x)		x - NVRAM_SPACE
#define NVRAM_MAGIC			0x48534C46	/* 'FLSH' */
#define NVRAM_VERSION		1

#define NVRAM_CRC_START_POSITION	9 /* magic, len, crc8 to be skipped */

char *nvram_get(const char *name);
int  nvram_set(const char *name, const char *value);
int  nvram_unset(const char *name);
int  nvram_commit(void);
char *nvram_safe_get(const char *name);
int  nvram_match(char *name, char *match);
int  nvram2file(char *varname, char *filename);
int  nvram_getall(char *buf,int count);
void nvram_open(void);
char * nvram_find_nvram(void);
char *nvram_default_get(char *var, char *def);
int	 nvram_nmatch(char *match, const char *fmt, ...);
void fwritenvram(char *var, FILE * fp);

#endif /* _nvram_h_ */