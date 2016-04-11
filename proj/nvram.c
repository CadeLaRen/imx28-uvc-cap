/*
 * NVRAM variable manipulation (common)
 *
 * Copyright 2004, Broadcom Corporation
 * Copyright 2009, OpenWrt.org
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */

#include "nvram.h"

#define TRACE(msg) \
	printf("%s(%i) in %s(): %s\n", \
			__FILE__, __LINE__, __FUNCTION__, msg ? msg : "?")

static const uint8_t crc8_table[256] = {
	0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B,
	0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
	0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF,
	0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
	0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14,
	0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
	0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80,
	0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
	0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95,
	0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
	0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01,
	0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
	0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA,
	0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
	0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E,
	0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
	0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0,
	0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
	0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54,
	0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
	0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF,
	0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
	0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B,
	0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
	0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E,
	0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
	0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA,
	0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
	0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41,
	0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
	0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5,
	0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F
};

uint8_t hndcrc8 (
	uint8_t * pdata,  /* pointer to array of data to process */
	uint32_t nbytes,  /* number of input data bytes to process */
	uint8_t crc       /* either CRC8_INIT_VALUE or previous return value */
) {
	while (nbytes-- > 0)
		crc = crc8_table[(crc ^ *pdata++) & 0xff];

	return crc;
}
size_t nvram_erase_size = NVRAM_COUNT;

nvram_handle_t *_nvram;
static int lock_fd=0;
struct flock* file_lock(short type, short whence)
{
	static struct flock ret;
	ret.l_type = type ;
	ret.l_start = 0;
	ret.l_whence = whence;
	ret.l_len = 0;
	ret.l_pid = getpid();
	return &ret;
}
void lock(int fd)
{
	struct flock lock;
	lock.l_type=F_WRLCK;
	lock.l_whence=SEEK_SET;
	lock.l_start=0;
	lock.l_len=0;
	lock.l_pid=getpid();
	fcntl(fd,F_SETLKW,&lock);
}

void unlock(int fd)
{
	struct flock lock;
	lock.l_type=F_UNLCK;
	lock.l_whence=SEEK_SET;
	lock.l_start=0;
	lock.l_len=0;
	lock.l_pid=getpid();
	fcntl(fd,F_SETLKW,&lock);
}


void LOCK_NVRAM()
{



	lock_fd=open(NVRAM_LOCK,O_CREAT|O_TRUNC|O_RDWR,0666);
	if(lock_fd<=0){
		fprintf(stderr,"LOCK_NVRAM open %s error\n",NVRAM_LOCK);
		return;
	}
	lock(lock_fd);

}

void UNLOCK_NVRAM()
{


	if(lock_fd<=0){
		fprintf(stderr,"UNLOCK_NVRAM open %serror\n",NVRAM_LOCK);
		return;
	}
	unlock(lock_fd);
	close(lock_fd);
	lock_fd=0;

}



/*
 * -- Helper functions --
 */

/* String hash */
static uint32_t hash(const char *s)
{
	uint32_t hash = 0;

	while (*s)
		hash = 31 * hash + *s++;

	return hash;
}

/* Free all tuples. */
static void _nvram_free(nvram_handle_t *h)
{
	int32_t i;
	nvram_tuple_t *t, *next;
	/* Free hash table */
	for (i = 0; i < NVRAM_ARRAYSIZE(h->nvram_hash); i++) {
		for (t = h->nvram_hash[i]; t; t = next) {
			next = t->next;
			free(t);
		}
		h->nvram_hash[i] = NULL;
	}
	/* Free dead table */
	for (t = h->nvram_dead; t; t = next) {
		next = t->next;
		free(t);
	}
	h->nvram_dead = NULL;
}

/* (Re)allocate NVRAM tuples. */
static nvram_tuple_t * _nvram_realloc( nvram_handle_t *h, nvram_tuple_t *t,
		const char *name, const char *value )
{
	if ((strlen(value) + 1) > NVRAM_SPACE)
		return NULL;
	if (!t) {
		if (!(t = malloc(sizeof(nvram_tuple_t) + strlen(name) + 1)))
			return NULL;
		/* Copy name */
		t->name = (char *) &t[1];
		strcpy(t->name, name);
		t->value = NULL;
	}

	/* Copy value */
	if (!t->value || strcmp(t->value, value))
	{
		if(!(t->value = (char *) realloc(t->value, strlen(value)+1)))
			return NULL;

		strcpy(t->value, value);
		t->value[strlen(value)] = '\0';
	}

	return t;
}

/* (Re)initialize the hash table. */
static int _nvram_rehash(nvram_handle_t *h)
{
	nvram_header_t *header =(nvram_header_t *) &h->mmap[0]; 
	char *name, *value, *eq;

	_nvram_free(h);
	name = (char *) &header[1];
	for (; *name; name = value + strlen(value) + 1)
	{
		if (!(eq = strchr(name, '=')))  //。第一个形参就是要搜索的字符串，第二个是被搜索的字符。
			//如果找到了该字符就返回该字符第一次出现的内存地址。如果没有找到就返回NULL（也就是0）。
			break;
		*eq = '\0';
		value = eq + 1;
		_nvram_set(h, name, value);
		*eq = '=';
	}
	return 0;
}
/*
 * -- Public functions --
 */

/* Get nvram header. */
nvram_header_t * nvram_header(nvram_handle_t *h)
{
	return (nvram_header_t *) &h->mmap[0];
}

/* Get the value of an NVRAM variable. */
char * _nvram_get(nvram_handle_t *h, const char *name)
{
	uint32_t i;
	nvram_tuple_t *t;
	char *value;

	if (!name)
		return NULL;

	/* Hash the name */
	i = hash(name) % NVRAM_ARRAYSIZE(h->nvram_hash);

	/* Find the associated tuple in the hash table */
	for (t = h->nvram_hash[i]; t && strcmp(t->name, name); t = t->next);

	value = t ? t->value : NULL;

	return value;
}

/* Set the value of an NVRAM variable. */
int _nvram_set(nvram_handle_t *h, const char *name, const char *value)
{
	uint32_t i;
	nvram_tuple_t *t, *u, **prev;

	/* Hash the name */
	i = hash(name) % NVRAM_ARRAYSIZE(h->nvram_hash);  //h->nvram_hash 指向一个名字，值，下一跳的地方
	/* Find the associated tuple in the hash table */
	for (prev = &h->nvram_hash[i], t = *prev; t && strcmp(t->name, name); prev = &t->next, t = *prev);

	/* (Re)allocate tuple */
	if (!(u = _nvram_realloc(h, t, name, value)))
		return -12; /* -ENOMEM */

	/* Value reallocated */
	if (t && t == u)
		return 0;

	/* Move old tuple to the dead table */
	if (t) {
		*prev = t->next;
		t->next = h->nvram_dead;
		h->nvram_dead = t;
	}

	/* Add new tuple to the hash table */
	u->next = h->nvram_hash[i];
	h->nvram_hash[i] = u;
	return 0;
}

/* Unset the value of an NVRAM variable. */
int _nvram_unset(nvram_handle_t *h, const char *name)
{
	uint32_t i;
	nvram_tuple_t *t, **prev;

	if (!name)
		return 0;

	/* Hash the name */
	i = hash(name) % NVRAM_ARRAYSIZE(h->nvram_hash);

	/* Find the associated tuple in the hash table */
	for (prev = &h->nvram_hash[i], t = *prev;
			t && strcmp(t->name, name); prev = &t->next, t = *prev);

	/* Move it to the dead table */
	if (t) {
		*prev = t->next;
		t->next = h->nvram_dead;
		h->nvram_dead = t;
	}

	return 0;
}

/* Get all NVRAM variables. */
nvram_tuple_t * _nvram_getall(nvram_handle_t *h)
{
	int i;
	nvram_tuple_t *t, *l, *x;

	l = NULL;

	for (i = 0; i < NVRAM_ARRAYSIZE(h->nvram_hash); i++) {
		for (t = h->nvram_hash[i]; t; t = t->next) {
			if( (x = (nvram_tuple_t *) malloc(sizeof(nvram_tuple_t))) != NULL )
			{
				x->name  = t->name;
				x->value = t->value;
				x->next  = l;
				l = x;
			}
			else
			{
				break;
			}
		}
	}

	return l;
}

/* Regenerate NVRAM. */
int _nvram_commit(nvram_handle_t *h)
{
	nvram_header_t *header = nvram_header(h);
	char *init, *config, *refresh, *ncdl;
	char *ptr, *end;
	int i;
	nvram_tuple_t *t;
	nvram_header_t tmp;
	uint8_t crc;

	/* Regenerate header */
	header->magic = NVRAM_MAGIC;
	header->crc_ver_init = (NVRAM_VERSION << 8);
	header->crc_ver_init |= 0x419 << 16;
	header->config_refresh = 0x0000;
	header->config_refresh |= 0x8040 << 16;
	header->config_ncdl = 0;

	/* Clear data area */
	ptr = (char *) header + sizeof(nvram_header_t);
	memset(ptr, 0xFF, NVRAM_SPACE - sizeof(nvram_header_t));
	memset(&tmp, 0, sizeof(nvram_header_t));

	/* Leave space for a double NUL at the end */
	end = (char *) header + NVRAM_SPACE - 2;

	/* Write out all tuples */
	for (i = 0; i < NVRAM_ARRAYSIZE(h->nvram_hash); i++) {
		for (t = h->nvram_hash[i]; t; t = t->next) {
			if ((ptr + strlen(t->name) + 1 + strlen(t->value) + 1) > end)
				break;
			ptr += sprintf(ptr, "%s=%s", t->name, t->value) + 1;
		}
	}

	/* End with a double NULL and pad to 4 bytes */
	*ptr = '\0';
	ptr++;

	if( (int)ptr % 4 )
		memset(ptr, 0, 4 - ((int)ptr % 4));

	ptr++;

	/* Set new length */
	header->len = NVRAM_ROUNDUP(ptr - (char *) header, 4);

	/* Little-endian CRC8 over the last 11 bytes of the header */
	tmp.crc_ver_init   = header->crc_ver_init;
	tmp.config_refresh = header->config_refresh;
	tmp.config_ncdl    = header->config_ncdl;
	crc = hndcrc8((unsigned char *) &tmp + NVRAM_CRC_START_POSITION,
			sizeof(nvram_header_t) - NVRAM_CRC_START_POSITION, 0xff);

	/* Continue CRC8 over data bytes */
	crc = hndcrc8((unsigned char *) &header[0] + sizeof(nvram_header_t),
			header->len - sizeof(nvram_header_t), crc);

	/* Set new CRC8 */
	header->crc_ver_init |= crc;

	/* Write out */
	msync(h->mmap, h->length, MS_SYNC);
	fsync(h->fd);

	/* Reinitialize hash table */
	return _nvram_rehash(h);
}

/* Open NVRAM and obtain a handle. */
nvram_handle_t * _nvram_open(const char *file, int rdonly)
{
	int fd;
	char *mtd = NULL;
	nvram_handle_t *h;
	nvram_header_t *header = NULL;
	if( (fd = open(file ? file : mtd, O_RDWR)) > -1 )
	{

		char *mmap_area = (char *) mmap(
				NULL, nvram_erase_size, PROT_READ | PROT_WRITE,
				( rdonly == NVRAM_RO ) ? MAP_PRIVATE : MAP_SHARED, fd, 0);

		if( mmap_area != MAP_FAILED )
		{
			if((h = (nvram_handle_t *) malloc(sizeof(nvram_handle_t))) != NULL)
			{
				h->fd     = fd;
				h->mmap   = mmap_area;
				h->length = nvram_erase_size;
				header = nvram_header(h);
				_nvram_rehash(h);  
				return h;
			}
		}
	}
	return NULL;
}

/* Close NVRAM and free memory. */
int nvram_close(nvram_handle_t *h)
{
	_nvram_free(h);
	munmap(h->mmap, h->length);
	close(h->fd);
	free(h);

	return 0;
}

/* Determine NVRAM device node. */
char * nvram_find_mtd(void)
{
	return NVRAM_MTD;
}

/* Check NVRAM staging file. */
char * nvram_find_staging(void)
{


	struct stat s;
	if( (stat(NVRAM_STAGING, &s) > -1) && (s.st_mode & S_IFREG) )
	{

		return NVRAM_STAGING;
	}
	return NULL;
}
char * nvram_find_nvram(void)
{
	struct stat s;

	if( (stat(NVRAM_MTD, &s) > -1) && (s.st_mode & S_IFREG) )
	{
		return NVRAM_MTD;
	}

	return NULL;
}

// Copy NVRAM contents to staging file.
//mtd-->/tmp/.nvram
int nvram_to_staging(void)
{
	int fdmtd, fdstg, stat;
	char *mtd = nvram_find_mtd();
	char buf[nvram_erase_size];
	stat = -1;
	if( (mtd != NULL) && (nvram_erase_size > 0) )
	{
		if( (fdmtd = open(mtd, O_RDONLY)) > -1 )
		{
			if( read(fdmtd, buf, sizeof(buf)) == sizeof(buf) )
			{
				if((fdstg = open(NVRAM_STAGING, O_WRONLY | O_CREAT, 0666)) > -1)
				{
					write(fdstg, buf, sizeof(buf));
					fsync(fdstg);
					close(fdstg);
					stat = 0;
				}
			}
			close(fdmtd);
		}

	}
	// free(mtd);
	return stat;
}

/* Copy staging file to NVRAM device. */
// /tmp/.nvram -->mtd
int staging_to_nvram(void)
{
	int fdmtd, fdstg, stat;
	char *mtd = nvram_find_mtd();
	char buf[nvram_erase_size];
	stat = -1;

	if( (mtd != NULL) && (nvram_erase_size > 0) )
	{
		if( (fdstg = open(NVRAM_STAGING, O_RDONLY)) > -1 )
		{
			if( read(fdstg, buf, sizeof(buf)) == sizeof(buf) )
				close(fdstg);
			{
				if( (fdmtd = open(mtd, O_WRONLY | O_SYNC)) > -1 )
				{
					write(fdmtd, buf, sizeof(buf));
					fsync(fdmtd);
					close(fdmtd);
					stat = 0;
				}
			}
		}
	}
	return stat;
}

static nvram_handle_t * _lib_nvram_open_rdonly(void)
{
	const char *file = nvram_find_staging();
	if(!(file!=NULL))  file=nvram_find_mtd();
	if( file != NULL ) return _nvram_open(file, NVRAM_RO);
	return NULL;
}

static nvram_handle_t * _lib_nvram_open_staging(void)
{
	if( nvram_find_staging() != NULL || nvram_to_staging() == 0 )
		return _nvram_open(NVRAM_STAGING, NVRAM_RW);
	return NULL;
}

char *nvram_get(const char *name)
{
	LOCK_NVRAM();
	char *value;
	if(!(_nvram!=NULL)) _nvram =  _lib_nvram_open_staging();
	if(!(_nvram!=NULL))  {
		UNLOCK_NVRAM();
		return NULL;
	}
	if((value=_nvram_get(_nvram,name))!=NULL)
	{

	}
	UNLOCK_NVRAM();
	return value;
}

int nvram_set(const char *name, const char *value)
{
	LOCK_NVRAM();
	int stat;
	if(!(_nvram!=NULL)) _nvram =  _lib_nvram_open_staging();
	if(!(_nvram!=NULL)) {
		UNLOCK_NVRAM();
		return -1;
	}
	stat=_nvram_set(_nvram, name,value);
	UNLOCK_NVRAM();
	return stat;
}

int nvram_unset(const char *name)
{
	LOCK_NVRAM();
	int stat;
	if(!(_nvram!=NULL)) _nvram = _lib_nvram_open_staging();
	if(!(_nvram!=NULL)) return -1;
	stat= _nvram_unset(_nvram, name);
	UNLOCK_NVRAM();
	return stat;	
}

int nvram_commit(void)
{
	LOCK_NVRAM();
	int stat=0;
	if(!(_nvram!=NULL)) _nvram =  _lib_nvram_open_staging();
	stat= _nvram_commit(_nvram);
	stat = staging_to_nvram();
	UNLOCK_NVRAM();
	return stat;
}


char *nvram_safe_get(const char *name)
{
	return nvram_get(name) ? : "";
}

int nvram_match(char *name, char *match)
{
	const char *value = nvram_get(name);
	return (value && !strcmp(value, match));
}

char *nvram_default_get(char *var, char *def)
{
	char *v = nvram_get(var);
	if (v == NULL || strlen(v) == 0) {
		nvram_set(var, def);
		return def;
	}
	return nvram_safe_get(var);
}

int nvram2file(char *varname, char *filename)
{
	FILE *fp;
	int i = 0, j = 0;
	char *buf;
	char mem[10000];

	if (!(fp = fopen(filename, "wb")))
		return 0;

	buf = strdup(nvram_safe_get(varname));
	while (buf[i] && j < sizeof(mem) - 3) {
		if (buf[i] == '~') {
			mem[j] = 0;
			j++;
			i++;
		} else {
			mem[j] = buf[i];
			j++;
			i++;
		}
	}
	if (j <= 0)
		return j;
	j = fwrite(mem, 1, j, fp);
	fclose(fp);
	free(buf);
	return j;
}

void nvram_open(void)           // dummy
{

}
int nvram_getall(char *buf,int count)
{
	return 0;

}

char *nvram_nget(const char *fmt, ...)
{
	char varbuf[64];
	va_list args;
	va_start(args, (char *)fmt);
	vsnprintf(varbuf, sizeof(varbuf), fmt, args);
	va_end(args);
	return nvram_safe_get(varbuf);
}

int nvram_invmatch(char *name, char *invmatch)
{
	const char *value = nvram_get(name);
	return (value && strcmp(value, invmatch));
}

int nvram_nmatch(char *match, const char *fmt, ...)
{
	char varbuf[64];
	va_list args;
	va_start(args, (char *)fmt);
	vsnprintf(varbuf, sizeof(varbuf), fmt, args);
	va_end(args);
	return nvram_match(varbuf, match);
}

void fwritenvram(char *var, FILE * fp)
{
	int i;
	if (fp == NULL)
		return;
	char *host_key = nvram_safe_get(var);
	int len = strlen(host_key);
	for (i = 0; i < len; i++)
		if (host_key[i] != '\r')
			fprintf(fp, "%c", host_key[i]);
}

