#include "os.h"

#if defined (__linux__)
long CPULinux::GetCPUUsage (void)
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

	if (total - m_OldTotal != 0)
		usage = (100*(double)(used-m_OldUsage))/(double)(total - m_OldTotal);
	else
		usage = 0;

	m_OldUsage = used;
	m_OldTotal = total;

	return usage;
}
#endif
#if defined (__FreeBSD__)
long CPUFreeBSD::GetCPUUsage (void)
{
	long user, nice, sys, bsdidle, idle;
	long used, total;
	long cp_time[CPUSTATES];
	size_t len = sizeof (cp_time);

	long usage;

	if (sysctlbyname ("kern.cp_time", &cp_time, &len, NULL, 0) < 0)
	{
		g_warning ("Cannot get kern.cp_time");
		return -1;
	}

	user = cp_time[CP_USER];
	nice = cp_time[CP_NICE];
	sys = cp_time[CP_SYS];
	bsdidle = cp_time[CP_IDLE];
	idle = cp_time[CP_IDLE];

	used = user+nice+sys;
	total = used+bsdidle;
	if ((total - m_OldTotal) != 0)
		usage = (100 * (double)(used - m_OldUsage))/(double)(total - m_OldTotal);
	else
		usage = 0;

	m_OlsUsage = used;
	m_OldTotal = total;
	return usage;
}
#endif
#if defined (__NetBSD__)
long CPUNetBSD::GetCPUUsage (void)
{
	long user, nice, sys, bsdidle, idle;
	long used, total, usage;
	static int mib[] = {CTL_KERN, KERN_CP_TIME };
	u_int64_t cp_time[CPUSTATES];
	size_t len = sizeof (cp_time);

	if (sysctl (mib, 2, &cp_time, &len, NULL, 0) < 0)
	{
		g_warning ("Cannot get kern.cp_time");
		return -1;
	}

	user = cp_time[CP_USER];
	nice = cp_time[CP_NICE];
	sys = cp_time[CP_SYS];
	bsdidle = cp_time[CP_IDLE];
	idle = cp_time[CP_IDLE];

	used = user+nice+sys;
	total = used+bsdidle;

	if (total - m_OldTotal != 0)
		usage = (100 * (double)(used - m_OldUsage))/(double)(total - m_OldTotal);
	else
		usage = 0;
	
	m_OldUsage = used;
	m_OldTotal = total;
	return usage;
}
#endif
#if defined (__OpenBSD_)
long CPUOpenBSD::GetCPUUsage (void)
{
	 long user, nice, sys, bsdidle, idle;
         long used, total, usage;
         static int mib[] = {CTL_KERN, KERN_CP_TIME };
         u_int64_t cp_time[CPUSTATES];
         size_t len = sizeof (cp_time);
         if (sysctl (mib, 2, &cp_time, &len, NULL, 0) < 0)
         {
                 g_warning ("Cannot get kern.cp_time");
                 return -1;
         }

	user = cp_time[CP_USER];
	nice = cp_time[CP_NICE];
        sys = cp_time[CP_SYS];
        bsdidle = cp_time[CP_INTR];
        idle = cp_time[CP_IDLE];

        used = user+nice+sys;
        total = used+bsdidle;

        if (total - m_OldTotal != 0)
		usage = (100 * (double)(used - m_OldUsage))/(double)(total - m_OldTotal);
	else
                usage = 0;
        m_OldUsage = used;
        m_OldTotal = total;
        return usage;							
}	
#endif
