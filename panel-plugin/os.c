#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpu.h"

#if defined (__linux__)
#define PROC_STAT "/proc/stat"
#define SYS_DEVICE "/sys/devices/system/cpu/cpu"
#define SCAL_CUR_FREQ "/cpufreq/scaling_cur_freq"
#define SCAL_MIN_FREQ "/cpufreq/scaling_min_freq"
#define SCAL_MAX_FREQ "/cpufreq/scaling_max_freq"
#define PROCMAXLNLEN 256 /* should make it */
#endif

CpuData *cpudata = NULL;
int nrCpus = 0;

void cpuData_free(){
  free(cpudata);
  cpudata = NULL;
  nrCpus = 0;
}

#if defined (__linux__)
int cpuData_init(){
  //fprintf(stderr,"cpuData_init\n");
  FILE *fstat = NULL;
  char cpuStr[PROCMAXLNLEN];
  int i, cpuNr = -1;
  /* Check if previously initalized */
	if(cpudata != NULL) return(-2);

	/* Open proc stat file */
	if(!(fstat = fopen(PROC_STAT, "r"))) return(-1);

	/* Read each cpu line at time */
	do{
    if(!fgets(cpuStr, PROCMAXLNLEN, fstat)) return(cpuNr);
		cpuNr++;
	}
	while(strncmp(cpuStr, "cpu", 3) == 0);
	/* Alloc storage for cpu data stuff */
	cpudata = (CpuData *) calloc(cpuNr, sizeof(CpuData));
	if(cpudata == NULL) return(-3);

  /* init frequency */
  for(i=cpuNr-1; i>=0; i--){
    cpudata[i].scalCurFreq = 0;
    cpudata[i].scalMinFreq = 0;
    cpudata[i].scalMaxFreq = -1;
  }

	fclose(fstat);
	return(nrCpus=cpuNr);
}

CpuData *cpuData_read(){
  //fprintf(stderr,"cpuData_read\n");
  FILE *fStat = NULL;
	char cpuStr[PROCMAXLNLEN];
	unsigned long user, nice, system, idle, used, total;
  unsigned long iowait=0, irq=0, softirq=0;
	int cpuNr = 0;

  /* Check if callable */
	if((cpudata == NULL) || (nrCpus == 0)) return(NULL);

	/* Open proc stat file */
	if(!(fStat = fopen(PROC_STAT, "r"))) return(NULL);

  /* Read each cpu line at time */
	do{
		if(!fgets(cpuStr, PROCMAXLNLEN, fStat)) return(cpudata);
		if(sscanf(cpuStr, "%*s %ld %ld %ld %ld %ld %ld %ld",
              &user, &nice, &system, &idle, &iowait, &irq, &softirq) < 7)
      iowait = irq = softirq = 0;
    used = user + nice + system + irq + softirq;
    total = used + idle + iowait;
		if((total - cpudata[cpuNr].pTotal) != 0){
			cpudata[cpuNr].load = CPU_SCALE * (float)(used - cpudata[cpuNr].pUsed)
				/ (float)(total - cpudata[cpuNr].pTotal);
		}
		else
		{
			cpudata[cpuNr].load = 0;
		}
		cpudata[cpuNr].pUsed = used;
		cpudata[cpuNr].pTotal = total;
		cpuNr++;
  }
	while((cpuNr < nrCpus) && (strncmp(cpuStr, "cpu", 3) == 0));

	fclose(fStat);

  return cpudata;
}

#elif defined (__FreeBSD__)
void cpuData_init(){
  int i, cpuNr = -1;

  /* Check if previously initalized */
	if(cpudata != NULL) return(-2);

  cpuNr = 1;

	/* Alloc storage for cpu data stuff */
	cpudata = (CpuData *) calloc(cpuNr, sizeof(CpuData));
	if(cpudata == NULL) return(-3);

  /* init frequency */
  for(i=cpuNr-1; i>=0; i--){
    cpudata[i].scalCurFreq = 0;
    cpudata[i].scalMinFreq = 0;
    cpudata[i].scalMaxFreq = -1;
  }

	fclose(fstat);
	return(nrCpus=cpuNr);
}

CpuData *cpuData_read(){
	unsigned long user, nice, sys, bsdidle, idle;
	unsigned long used, total;
	long cp_time[CPUSTATES];
	size_t len = sizeof (cp_time);

	long usage;

	if (sysctlbyname ("kern.cp_time", &cp_time, &len, NULL, 0) < 0)
	{
		printf ("Cannot get kern.cp_time.\n");
		return -1;
	}

	user = cp_time[CP_USER];
	nice = cp_time[CP_NICE];
	sys = cp_time[CP_SYS];
	bsdidle = cp_time[CP_IDLE];
	idle = cp_time[CP_IDLE];

	used = user+nice+sys;
	total = used+bsdidle;
	if ((total - cpudata[0].pTotal) != 0)
		cpudata[0].pTotal = (CPU_SCALE.0 * (used - cpudata[0].pTotal))/(total - cpudata[0].pTotal);
	else
		cpudata[0].pTotal = 0;

	cpudata[0].pUsed = used;
	cpudata[0].pTotal = total;

  return cpudata;
}

#elif defined (__NetBSD__)
void cpuData_init(){
  int i, cpuNr = -1;

  /* Check if previously initalized */
	if(cpudata != NULL) return(-2);

  cpuNr = 1;

	/* Alloc storage for cpu data stuff */
	cpudata = (CpuData *) calloc(cpuNr, sizeof(CpuData));
	if(cpudata == NULL) return(-3);

  /* init frequency */
  for(i=cpuNr-1; i>=0; i--){
    cpudata[i].scalCurFreq = 0;
    cpudata[i].scalMinFreq = 0;
    cpudata[i].scalMaxFreq = -1;
  }

	fclose(fstat);
	return(nrCpus=cpuNr);
}


CpuData *cpuData_read(){
	long user, nice, sys, bsdidle, idle;
	long used, total, usage;
	static int mib[] = {CTL_KERN, KERN_CP_TIME };
	u_int64_t cp_time[CPUSTATES];
	size_t len = sizeof (cp_time);

	if (sysctl (mib, 2, &cp_time, &len, NULL, 0) < 0)
	{
		printf ("Cannot get kern.cp_time\n");
		return -1;
	}

	user = cp_time[CP_USER];
	nice = cp_time[CP_NICE];
	sys = cp_time[CP_SYS];
	bsdidle = cp_time[CP_IDLE];
	idle = cp_time[CP_IDLE];

	used = user+nice+sys;
	total = used+bsdidle;

	if (total - cpudata[0].pTotal != 0)
		usage = (CPU_SCALE * (double)(used - cpudata[0].pTotal))/(double)(total - cpudata[0].pTotal);
	else
		usage = 0;

	cpudata[0].pUsed = used;
	cpudata[0].pTotal = total;

  return cpudata;
}

#elif defined (__OpenBSD_)
void cpuData_init(){
  int i, cpuNr = -1;

  /* Check if previously initalized */
	if(cpudata != NULL) return(-2);

  cpuNr = 1;

	/* Alloc storage for cpu data stuff */
	cpudata = (CpuData *) calloc(cpuNr, sizeof(CpuData));
	if(cpudata == NULL) return(-3);

  /* init frequency */
  for(i=cpuNr-1; i>=0; i--){
    cpudata[i].scalCurFreq = 0;
    cpudata[i].scalMinFreq = 0;
    cpudata[i].scalMaxFreq = -1;
  }

	fclose(fstat);
	return(nrCpus=cpuNr);
}


CpuData *cpuData_read(){
  unsigned long user, nice, sys, bsdidle, idle;
  unsigned long used, total, usage;
  static int mib[] = {CTL_KERN, KERN_CP_TIME };
  u_int64_t cp_time[CPUSTATES];
  size_t len = sizeof (cp_time);
  if (sysctl (mib, 2, &cp_time, &len, NULL, 0) < 0){
    printf ("Cannot get kern.cp_time\n");
    return -1;
  }

	user = cp_time[CP_USER];
	nice = cp_time[CP_NICE];
  sys = cp_time[CP_SYS];
  bsdidle = cp_time[CP_INTR];
  idle = cp_time[CP_IDLE];

  used = user+nice+sys;
  total = used+bsdidle;

  if (total - cpudata[0].pTotal != 0)
		usage = (CPU_SCALE * (double)(used - cpudata[0].pTotal))/(double)(total - cpudata[0].pTotal);
	else
    usage = 0;
  cpudata[0].pUsed = used;
  cpudata[0].pTotal = total;

  return cpudata;
}
#else
#error "You're OS is not supported."
#endif
