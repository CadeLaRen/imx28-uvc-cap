/*
 * Command line interface for libnvram
 *
 * Copyright 2009, Jo-Philipp Wich <xm@subsignal.org>
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The libnvram code is based on Broadcom code for Linux 2.4.x .
 *
 */

#include "nvram.h"

static nvram_handle_t * nvram_open_staging(void)
{
	int fd;
	if(nvram_find_staging()==NULL){
		if (nvram_find_nvram() == NULL)
		{

			fd = open(NVRAM_MTD, O_RDWR|O_CREAT, 0666);
			if (fd < 0) {
				return NULL;
			}

			lseek(fd, 0x10000, SEEK_SET);
			write(fd, "0", 1);
			close(fd);
		}
		nvram_to_staging();
	}
	return _nvram_open(NVRAM_STAGING, NVRAM_RW);
}

static int do_show(nvram_handle_t *nvram)
{
	nvram_tuple_t *t;
	int stat = 1;

	if( (t = _nvram_getall(nvram)) != NULL )
	{
		while( t )
		{
			printf("%s=%s\n", t->name, t->value);
			t = t->next;
		}

		stat = 0;
	}

	return stat;
}

static int do_get(nvram_handle_t *nvram, const char *var)
{
	const char *val;
	int stat = 1;

	if( (val = _nvram_get(nvram, var)) != NULL )
	{
		printf("%s\n", val);
		stat = 0;
	}

	return stat;
}

static int do_unset(nvram_handle_t *nvram, const char *var)
{
	return _nvram_unset(nvram, var);
}

static int do_set(nvram_handle_t *nvram, const char *pair)
{

    char *val = strstr(pair, "=");
	char var[strlen(pair)];
	int stat = 1;

	if( val != NULL )
	{
		memset(var, 0, sizeof(var));
		strncpy(var, pair, (int)(val-pair));
		stat = _nvram_set(nvram, var, (char *)(val + 1));
	}

	return stat;
}

static int do_info(nvram_handle_t *nvram)
{
	nvram_header_t *hdr = nvram_header(nvram);

	/* CRC8 over the last 11 bytes of the header and data bytes */
	uint8_t crc = hndcrc8((unsigned char *) &hdr[0] + NVRAM_CRC_START_POSITION,
		hdr->len - NVRAM_CRC_START_POSITION, 0xff);

	/* Show info */
	printf("Magic:         0x%08X\n",   hdr->magic);
	printf("Length:        0x%08X\n",   hdr->len);

	printf("CRC8:          0x%02X (calculated: 0x%02X)\n",
		hdr->crc_ver_init & 0xFF, crc);

	printf("Version:       0x%02X\n",   (hdr->crc_ver_init >> 8) & 0xFF);
	printf("SDRAM init:    0x%04X\n",   (hdr->crc_ver_init >> 16) & 0xFFFF);
	printf("SDRAM config:  0x%04X\n",   hdr->config_refresh & 0xFFFF);
	printf("SDRAM refresh: 0x%04X\n",   (hdr->config_refresh >> 16) & 0xFFFF);
	printf("NCDL values:   0x%08X\n\n", hdr->config_ncdl);

	printf("%i bytes used / %i bytes available (%.2f%%)\n",
		hdr->len, NVRAM_SPACE - hdr->len,
		(100.00 / (double)NVRAM_SPACE) * (double)hdr->len);

	return 0;
}


int main( int argc, const char *argv[] )
{
	nvram_handle_t *nvram;
	int commit = 0;
	int write = 0;
	int stat = 1;
	int done = 0;
	int i;

	/* Ugly... iterate over arguments to see whether we can expect a write */
	for( i = 1; i < argc; i++ )
		if( ( !strcmp(argv[i], "set")   && ++i < argc ) ||
			( !strcmp(argv[i], "unset") && ++i < argc ) ||
			!strcmp(argv[i], "commit") )
		{
			write = 1;
			break;
		}


  LOCK_NVRAM();
  nvram=nvram_open_staging();
   if( nvram != NULL && argc > 1 )
	{
		for( i = 1; i < argc; i++ )
		{
			if( !strcmp(argv[i], "show") )
			{
				stat = do_show(nvram);
				done++;
			}
			else if( !strcmp(argv[i], "info") )
			{
				stat = do_info(nvram);
				done++;
			}
			else if( !strcmp(argv[i], "get") && ++i < argc )
			{
				stat = do_get(nvram, argv[i]);
				done++;
			}
			else if( !strcmp(argv[i], "unset") && ++i < argc )
			{
				stat = do_unset(nvram, argv[i]);
				done++;
			}
			else if( !strcmp(argv[i], "set") && ++i < argc )
			{
			
				stat = do_set(nvram, argv[i]);
				
				done++;
				
			}
			else if( !strcmp(argv[i], "commit") )
			{
				commit = 1;
				done++;
			}
			else
			{
				fprintf(stderr, "Unknown option '%s' !\n", argv[i]);
				done = 0;
				break;
			}
		}

		if( write )
				stat = _nvram_commit(nvram);

		nvram_close(nvram);

		if( commit )
			stat = staging_to_nvram();
	}

	if( !nvram )
	{
		printf("nvram error\n");
		stat = 1;
	}
	else if( !done )
	{
		fprintf(stderr,
			"Usage:\n"
			"	nvram show\n"
			"	nvram info\n"
			"	nvram get variable\n"
			"	nvram set variable=value [set ...]\n"
			"	nvram unset variable [unset ...]\n"
			"	nvram commit\n"
		);

		stat = 1;
	}

	UNLOCK_NVRAM();
	return stat;
}
