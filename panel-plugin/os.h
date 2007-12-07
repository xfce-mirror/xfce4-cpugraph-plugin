#ifndef __OS_H__
#define __OS_H__

#include <stdio.h>

#if defined (__FreeBSD__)
#include <osreldate.h>
#include <sys/types.h>
#if __FreeBSD_version < 500101
#include <sys/dkstat.h>
#else
#include <sys/resource.h>
#endif
#include <sys/sysctl.h>
#include <devstat.h>
#include <fcntl.h>
#include <nlist.h>
#endif

#if defined (__NetBSD__)
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <fcntl.h>
#include <nlist.h>
#endif

#if defined (__OpenBSD__)
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/dkstat.h>
#include <fcntl.h>
#include <nlist.h>
#endif

#define CPU_SCALE 100000

typedef struct s_cpuData{
  float         load; /* cpu utilization */
  unsigned long pUsed; /* Previous value of used cpu time */
	unsigned long pTotal; /* Previous value of total cpu time */
  long scalCurFreq;
  long scalMinFreq;
  long scalMaxFreq;
} CpuData;


int cpuData_init();
void cpuData_free();
CpuData *cpuData_read();
void setFrequencyScaling(int cpuId);

#endif
