#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "os.h"

int scaling_cur_freq=0;
int scaling_min_freq=-1;
int scaling_max_freq=0;

#if defined (__linux__)
long GetCPUUsage(int *cpu_busy_prev, int *cpu_total_prev)
{
	long busy, nice, system, idle, total;
    long iowait=0, irq=0, softirq=0; /* New in Linux 2.6 */
	long usage;

	static FILE *sfp = NULL;
    if(!sfp) {
        if( !(sfp = fopen("/proc/stat", "r")))
	{
		printf ("Could'nt read from /proc/stat");
		return -1;
	}
    }

	if( 7 > fscanf(sfp, "%*s %ld %ld %ld %ld %ld %ld %ld",
                    &busy, &nice, &system, &idle, &iowait, &irq, &softirq))
        iowait = irq = softirq = 0;
    rewind(sfp);
    fflush(sfp);

	busy += nice+system+irq+softirq;
	total = busy+idle+iowait;

	if( total > *cpu_total_prev )
		usage = CPU_SCALE * (busy - *cpu_busy_prev) / (total - *cpu_total_prev);
	else
		usage = 0;

	*cpu_busy_prev = busy;
	*cpu_total_prev = total;

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

	return usage;
}
#elif defined (__FreeBSD__)
long GetCPUUsage (int *oldusage, int *oldtotal)
{
	long user, nice, sys, bsdidle, idle;
	long used, total;
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
	if ((total - (long)*oldtotal) != 0)
		usage = (CPU_SCALE.0 * (used - (long)*oldusage))/(total - (long)*oldtotal);
	else
		usage = 0;

	*oldusage = (int)used;
	*oldtotal = (int)total;
	
	return usage;
}
#elif defined (__NetBSD__)
long GetCPUUsage (int *oldusage, int *oldtotal)
{
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

	if (total - (long)*oldtotal != 0)
		usage = (CPU_SCALE * (double)(used - (long)*oldusage))/(double)(total - (long)*oldtotal);
	else
		usage = 0;
	
	*oldusage = (int)used;
	*oldtotal = (int)total;
	
	return usage;
}
#elif defined (__OpenBSD_)
long GetCPUUsage (int *oldusage, int *oldtotal)
{
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
        bsdidle = cp_time[CP_INTR];
        idle = cp_time[CP_IDLE];

        used = user+nice+sys;
        total = used+bsdidle;

        if (total - (long)*oldtotal != 0)
		usage = (CPU_SCALE * (double)(used - (long)*oldusage))/(double)(total - (long)*oldtotal);
	else
                usage = 0;
        *oldusage = (int)used;
        *oldtotal = (int)total;
	
        return usage;							
}
#else
#error "You're OS is not supported."
#endif
