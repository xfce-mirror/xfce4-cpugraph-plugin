/*  cpu_os.c
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

#define PROC_STAT "/proc/stat"
#define PROCMAXLNLEN 256 /* should make it */

int scaling_cur_freq=0;
int scaling_min_freq=-1;
int scaling_max_freq=0;

cpuLoadData *data = NULL;
int nrCpus = 0;

#if defined (__linux__)
int cpuLoadMon_init()
{
	FILE *fStat = NULL;
	char  cpuStr[PROCMAXLNLEN];
	int   cpuNr = -1;

	/* Check if previously initalized */
	if(data != NULL) return(-2);

	/* Open proc stat file */
	if(!(fStat = fopen(PROC_STAT, "r"))) return(-1);

	/* Read each cpu line at time */
	do
	{
		if(!fgets(cpuStr, PROCMAXLNLEN, fStat)) return(cpuNr);
		cpuNr++;
	}
	while(strncmp(cpuStr, "cpu", 3) == 0);

	/* Alloc storage for cpu data stuff */
	data = (cpuLoadData *) calloc(cpuNr, sizeof(cpuLoadData));
	if(data == NULL) return(-3);

	fclose(fStat);
	return(nrCpus=cpuNr);
}

void cpuLoadMon_free()
{
	/* free cpu data */
	free(data);
	data = NULL;

	/* Reset values */
	nrCpus = 0;
}


cpuLoadData *cpuLoadMon_read(){
	FILE *fStat = NULL;
	char cpuStr[PROCMAXLNLEN];
	unsigned long user, nice, system, idle;
	unsigned long used, total;
	int cpuNr = 0;


	/* Check if callable */
	if((data == NULL) || (nrCpus == 0)) return(NULL);

	/* Open proc stat file */
	if(!(fStat = fopen(PROC_STAT, "r"))) return(NULL);

	/* Read each cpu line at time */
	do
	{
		if(!fgets(cpuStr, PROCMAXLNLEN, fStat)) return(data);
		sscanf(cpuStr, "%*s %ld %ld %ld %ld", &user, &nice, &system, &idle);

		used = user + nice + system;
		total = used + idle;
		if((total - data[cpuNr].pTotal) != 0)
		{
			data[cpuNr].value = (float)(used - data[cpuNr].pUsed)
				/ (float)(total - data[cpuNr].pTotal);
		}
		else
		{
			data[cpuNr].value = 0;
		}
		data[cpuNr].pUsed = used;
		data[cpuNr].pTotal = total;
		cpuNr++;
	}
	while((cpuNr < nrCpus) && (strncmp(cpuStr, "cpu", 3) == 0));

	fclose(fStat);

  /* init current, min and max frequency */
  /* TODO integrate with usage data */
  if( -1 != scaling_max_freq)
  {
    FILE *fp;
    if( scaling_max_freq )
    {
      fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
      if( NULL != fp )
      {
        fscanf(fp, "%d", &scaling_cur_freq);
        fclose(fp);
      }
    } else {
      scaling_max_freq = -1;
      fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq", "r");
      if( NULL != fp )
      {
        if (1 == fscanf(fp, "%d", &scaling_min_freq) )
        {
          fclose(fp);
          fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", "r");
          if( NULL != fp)
          {
            fscanf(fp, "%d", &scaling_max_freq);
            if( scaling_max_freq < 1 ) scaling_max_freq = -1;
            fclose(fp);
          }
        } else {
          fclose(fp);
        }
      }
    }
  }


	return(data);
}
/* TODO FreeBSD */
/* TODO NetBSD */
/* TODO OpenBSD */
#else
#error "Your're OS is not supported"
#endif
