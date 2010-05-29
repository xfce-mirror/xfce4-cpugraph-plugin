#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "os.h"

#include <stdio.h>
#include <string.h>

#if defined (__linux__) || defined (__FreeBSD_kernel__)
#define PROC_STAT "/proc/stat"
#define PROCMAXLNLEN 256 /* should make it */
#endif

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
#endif

#if defined (__OpenBSD__)
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#endif

#if defined (__linux__) || defined (__FreeBSD_kernel__)
guint detect_cpu_number()
{
	guint nb_lines= 0;
	FILE *fstat = NULL;
	gchar cpuStr[PROCMAXLNLEN];

	if( !(fstat = fopen( PROC_STAT, "r" )) )
		return 0;

	while( fgets( cpuStr, PROCMAXLNLEN, fstat ) )
	{
		if( strncmp( cpuStr, "cpu", 3 ) == 0 )
			nb_lines++;
		else
			break;
	}

	fclose( fstat );

	return nb_lines > 1 ? nb_lines - 1 : 0;
}

gboolean read_cpu_data( CpuData *data, guint nb_cpu)
{
	FILE *fStat;
	gchar cpuStr[PROCMAXLNLEN];
	gulong user, nice, system, idle, used, total, iowait, irq, softirq;
	guint line;

	if( !(fStat = fopen( PROC_STAT, "r" )) )
		return FALSE;

	for( line = 0; line < nb_cpu + 1; line++ )
	{
		if( !fgets( cpuStr, PROCMAXLNLEN, fStat ) ||
		    strncmp( cpuStr, "cpu", 3 ) != 0
		  )
		{
			fclose( fStat );
			return FALSE;
		}
		if( sscanf( cpuStr, "%*s %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle, &iowait, &irq, &softirq ) < 7 )
			iowait = irq = softirq = 0;
		used = user + nice + system + irq + softirq;
		total = used + idle + iowait;
		if( (total - data[line].previous_total) != 0 )
		{
			data[line].load = CPU_SCALE * (used - data[line].previous_used) /
			                      (total - data[line].previous_total);
		}
		else
		{
			data[line].load = 0;
		}
		data[line].previous_used = used;
		data[line].previous_total = total;
	}

	fclose( fStat );

	return TRUE;
}

#elif defined (__FreeBSD__)
guint detect_cpu_number()
{
	return 1;
}

gboolean read_cpu_data( CpuData *data, guint nb_cpu)
{
	guint user, nice, sys, bsdidle, idle;
	guint used, total;
	gint cp_time[CPUSTATES];
	gsize len = sizeof( cp_time );

	guint usage;

	if( sysctlbyname( "kern.cp_time", &cp_time, &len, NULL, 0 ) < 0 )
	{
		return FALSE1;
	}

	user = cp_time[CP_USER];
	nice = cp_time[CP_NICE];
	sys = cp_time[CP_SYS];
	bsdidle = cp_time[CP_IDLE];
	idle = cp_time[CP_IDLE];

	used = user+nice+sys;
	total = used+bsdidle;
	if( (total - data[0].previous_total) != 0 )
		data[0].load = (CPU_SCALE * (used - data[0].previous_total))/(total - data[0].previous_total);
	else
		data[0].load = 0;

	data[0].previous_used = used;
	data[0].previous_total = total;

	return TRUE;
}

#elif defined (__NetBSD__)
guint detect_cpu_number()
{
	static gint mib[] = {CTL_HW, HW_NCPU};
	gint ncpu;
	gsize len = sizeof( gint );
	if( sysctl( mib, 2, &ncpu, &len, NULL, 0 ) < 0 )
		return 0;
	else
		return ncpu;
}

gboolean read_cpu_data( CpuData *data, guint nb_cpu)
{
	guint64 used, total;
	guint64 cp_time[CPUSTATES * nb_cpu];
	guint64 *cp_time1;
	gint i;
	gsize len = nb_cpu * CPUSTATES * sizeof( guint64 );
	gint mib[] = {CTL_KERN, KERN_CP_TIME};
	if( sysctl( mib, 2, &cp_time, &len, NULL, 0 ) < 0 )
		return FALSE;

	data[0].load = 0;
	for( i = 1 ; i <= nb_cpu ; i++ )
	{
		cp_time1 = cp_time + CPUSTATE * (i - 1)
		used = cp_time1[CP_USER] + cp_time1[CP_NICE] + cp_time1[CP_SYS] + cp_time1[CP_INTR];
		total = used + cp_time1[CP_IDLE];

		if( total - data[i].previous_total != 0 )
			data[i].load = (CPU_SCALE * (used - data[i].previous_used))/(total - data[i].previous_total);
		else
			data[i].load = 0;
		data[i].previous_used = used;
		data[i].previous_total = total;
		data[0].load += data[i].load;
	}
	data[0].load /= nb_cpu;
	return TRUE;
}

#elif defined (__OpenBSD__)
guint detect_cpu_number()
{
	static gint mib[] = {CTL_HW, HW_NCPU};
	gint ncpu;
	gsize len = sizeof( gint );
	if( sysctl( mib, 2, &ncpu, &len, NULL, 0 ) < 0 )
		return 0;
	else
		return ncpu;
}

gboolean read_cpu_data( CpuData *data, guint nb_cpu)
{
	guint64 used, total;
	guint64 cp_time[CPUSTATES];
	gint i;
	data[0].load = 0;
	for( i = 1 ; i <= nb_cpu ; i++ )
	{
		gsize len = CPUSTATES * sizeof( guint64 );
		gint mib[] = {CTL_KERN, KERN_CPTIME2, i - 1};
		if( sysctl( mib, 3, &cp_time, &len, NULL, 0 ) < 0 )
			return FALSE;

		used = cp_time[CP_USER] + cp_time[CP_NICE] + cp_time[CP_SYS] + cp_time[CP_INTR];
		total = used + cp_time[CP_IDLE];

		if( total - data[i].previous_total != 0 )
			data[i].load = (CPU_SCALE * (used - data[i].previous_used))/(total - data[i].previous_total);
		else
			data[i].load = 0;
		data[i].previous_used = used;
		data[i].previous_total = total;
		data[0].load += data[i].load;
	}
	data[0].load /= nb_cpu;
	return TRUE;
}
#else
#error "Your OS is not supported."
#endif
