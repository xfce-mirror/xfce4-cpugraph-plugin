#include "os.h"

#if defined (__linux__)
long GetCPUUsage (int *oldusage, int *oldtotal)
{
	long cpu, nice, system, idle, used, total;
	long usage;
	FILE *fp;
	fp = fopen ("/proc/stat", "r");
	if (!fp)
	{
		printf ("Could'nt read from /proc/stat");
		return -1;
	}

	fscanf (fp, "%*s %ld %ld %ld %ld", &cpu, &nice, &system, &idle);
	fclose (fp);

	used = cpu+nice+system;
	total = used+idle;

	if (total - (long)*oldtotal != 0)
		usage = (100*(double)(used-(long)*oldusage))/(double)(total - (long)*oldtotal);
	else
		usage = 0;

	*oldusage = (int)used;
	*oldtotal = (int)total;

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
		usage = (100 * (double)(used - (long)*oldusage))/(double)(total - (long)*oldtotal);
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
		usage = (100 * (double)(used - (long)*oldusage))/(double)(total - (long)*oldtotal);
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
		usage = (100 * (double)(used - (long)*oldusage))/(double)(total - (long)*oldtotal);
	else
                usage = 0;
        *oldusage = (int)used;
        *oldtotal = (int)total;
	
        return usage;							
}
#else
#error "You're OS is not supported."
#endif
