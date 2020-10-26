/*  os.c
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
 *  Copyright (c) 2010 Peter Tribble <peter.tribble@gmail.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "os.h"

#include <errno.h>
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

#if defined (__sun__)
#include <kstat.h>
static kstat_ctl_t *kc;
#endif

#if defined (__linux__) || defined (__FreeBSD_kernel__)
guint
detect_cpu_number (void)
{
    guint nb_lines= 0;
    FILE *fstat = NULL;
    gchar cpuStr[PROCMAXLNLEN];

    if (!(fstat = fopen (PROC_STAT, "r")))
        return 0;

    while (fgets (cpuStr, PROCMAXLNLEN, fstat))
    {
        if (strncmp (cpuStr, "cpu", 3) == 0)
            nb_lines++;
        else
            break;
    }

    fclose (fstat);

    return nb_lines > 1 ? nb_lines - 1 : 0;
}

static gulong
parse_ulong (gchar **s)
{
    guint64 v;

    errno = 0;
    v = g_ascii_strtoull (*s, s, 0);
    if (errno || v != (gulong) v)
        v = 0;

    return v;
}

gboolean
read_cpu_data (CpuData *data, guint nb_cpu)
{
    FILE *fStat;
    gchar cpuStr[PROCMAXLNLEN];
    gulong cpu, used[nb_cpu+1], total[nb_cpu+1];

    if (!(fStat = fopen (PROC_STAT, "r")))
        return FALSE;

    for (cpu = 0; cpu < nb_cpu+1; cpu++)
        used[cpu] = total[cpu] = 0;

    while (TRUE)
    {
        gchar *s;
        gulong user, nice, system, idle, iowait, irq, softirq;

        if (!fgets (cpuStr, PROCMAXLNLEN, fStat))
        {
            fclose (fStat);
            return FALSE;
        }

        if (strncmp (cpuStr, "cpu", 3) != 0)
            break;

        s = cpuStr + 3;

        if (g_ascii_isspace (*s))
            cpu = 0;
        else
            cpu = 1 + parse_ulong (&s);

        user = parse_ulong (&s);
        nice = parse_ulong (&s);
        system = parse_ulong (&s);
        idle = parse_ulong (&s);
        iowait = parse_ulong (&s);
        irq = parse_ulong (&s);
        softirq = parse_ulong (&s);

        if (cpu < nb_cpu + 1)
        {
            used[cpu] = user + nice + system + irq + softirq;
            total[cpu] = used[cpu] + idle + iowait;
        }
    }

    for (cpu = 0; cpu < nb_cpu + 1; cpu++)
    {
        if ((total[cpu] - data[cpu].previous_total) != 0)
        {
            data[cpu].load = CPU_SCALE * (used[cpu] - data[cpu].previous_used) /
                              (total[cpu] - data[cpu].previous_total);
        }
        else
        {
            data[cpu].load = 0;
        }
        data[cpu].previous_used = used[cpu];
        data[cpu].previous_total = total[cpu];
    }

    fclose (fStat);

    return TRUE;
}

#elif defined (__FreeBSD__)
guint
detect_cpu_number (void)
{
    static gint mib[] = {CTL_HW, HW_NCPU};
    gint ncpu;
    gsize len = sizeof (gint);

    if (sysctl (mib, 2, &ncpu, &len, NULL, 0) < 0)
        return 0;
    else
        return ncpu;
}

gboolean
read_cpu_data (CpuData *data, guint nb_cpu)
{
    glong used, total;
    glong *cp_time;
    glong *cp_time1;
    gint i;
    unsigned int max_cpu;
    gsize len = sizeof (max_cpu);

    data[0].load = 0;
    if (sysctlbyname ("kern.smp.maxid", &max_cpu, &len, NULL, 0) < 0)
        return FALSE;

    max_cpu++; /* max_cpu is 0-based */
    if (max_cpu < nb_cpu)
        return FALSE; /* should not happen */
    len = sizeof (glong) * max_cpu * CPUSTATES;
    cp_time = (glong *) g_malloc (len);

    if (sysctlbyname ("kern.cp_times", cp_time, &len, NULL, 0) < 0) {
        g_free (cp_time);
        return FALSE;
    }

    for (i = 1; i <= nb_cpu; i++)
    {
        cp_time1 = &cp_time[CPUSTATES * (i - 1)];
        used = cp_time1[CP_USER] + cp_time1[CP_NICE] + cp_time1[CP_SYS] + cp_time1[CP_INTR];
        total = used + cp_time1[CP_IDLE];

        if ((total - data[i].previous_total) != 0)
            data[i].load = (CPU_SCALE * (used - data[i].previous_used)) /
                           (total - data[i].previous_total);
        else
            data[i].load = 0;

        data[i].previous_used = used;
        data[i].previous_total = total;
        data[0].load += data[i].load;
    }

    data[0].load /= nb_cpu;
    g_free (cp_time);
    return TRUE;
}

#elif defined (__NetBSD__)
guint
detect_cpu_number (void)
{
    static gint mib[] = {CTL_HW, HW_NCPU};
    gint ncpu;
    gsize len = sizeof (gint);

    if (sysctl (mib, 2, &ncpu, &len, NULL, 0) < 0)
        return 0;
    else
        return ncpu;
}

gboolean
read_cpu_data (CpuData *data, guint nb_cpu)
{
    guint64 used, total;
    guint64 cp_time[CPUSTATES * nb_cpu];
    guint64 *cp_time1;
    gint i;
    gsize len = nb_cpu * CPUSTATES * sizeof (guint64);
    gint mib[] = {CTL_KERN, KERN_CP_TIME};

    if (sysctl (mib, 2, &cp_time, &len, NULL, 0) < 0)
        return FALSE;

    data[0].load = 0;
    for (i = 1; i <= nb_cpu; i++)
    {
        cp_time1 = cp_time + CPUSTATES * (i - 1);
        used = cp_time1[CP_USER] + cp_time1[CP_NICE] + cp_time1[CP_SYS] + cp_time1[CP_INTR];
        total = used + cp_time1[CP_IDLE];

        if (total - data[i].previous_total != 0)
            data[i].load = (CPU_SCALE * (used - data[i].previous_used)) /
                           (total - data[i].previous_total);
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
guint
detect_cpu_number (void)
{
    static gint mib[] = {CTL_HW, HW_NCPU};
    gint ncpu;
    gsize len = sizeof (gint);

    if (sysctl (mib, 2, &ncpu, &len, NULL, 0) < 0)
        return 0;
    else
        return ncpu;
}

gboolean
read_cpu_data (CpuData *data, guint nb_cpu)
{
    guint64 used, total;
    guint64 cp_time[CPUSTATES];
    gint i;
    data[0].load = 0;

    for (i = 1; i <= nb_cpu; i++)
    {
        gsize len = CPUSTATES * sizeof (guint64);
        gint mib[] = {CTL_KERN, KERN_CPTIME2, i - 1};

        if (sysctl (mib, 3, &cp_time, &len, NULL, 0) < 0)
            return FALSE;

        used = cp_time[CP_USER] + cp_time[CP_NICE] + cp_time[CP_SYS] + cp_time[CP_INTR];
        total = used + cp_time[CP_IDLE];

        if (total - data[i].previous_total != 0)
            data[i].load = (CPU_SCALE * (used - data[i].previous_used)) /
                           (total - data[i].previous_total);
        else
            data[i].load = 0;

        data[i].previous_used = used;
        data[i].previous_total = total;
        data[0].load += data[i].load;
    }

    data[0].load /= nb_cpu;
    return TRUE;
}

#elif defined (__sun__)
static void
init_stats ()
{
    kc = kstat_open ();
}

guint
detect_cpu_number (void)
{
    kstat_t *ksp;
    kstat_named_t *knp;

    if (!kc)
        init_stats ();

    if (!(ksp = kstat_lookup (kc, "unix", 0, "system_misc")))
        return 0;

    kstat_read (kc, ksp, NULL);
    knp = kstat_data_lookup (ksp, "ncpus");

    return knp->value.ui32;
}

gboolean
read_cpu_data (CpuData *data, guint nb_cpu)
{
    kstat_t *ksp;
    kstat_named_t *knp;
    guint64 used, total;
    gint i;
    data[0].load = 0;

    if (!kc)
        init_stats ();

    i = 1;

    for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next)
    {
        if (!g_strcmp0 (ksp->ks_module, "cpu") && !g_strcmp0 (ksp->ks_name, "sys"))
        {
            kstat_read (kc, ksp, NULL);
            knp = kstat_data_lookup (ksp, "cpu_nsec_user");
            used = knp->value.ul;
            knp = kstat_data_lookup (ksp, "cpu_nsec_intr");
            used += knp->value.ul;
            knp = kstat_data_lookup (ksp, "cpu_nsec_kernel");
            used += knp->value.ul;
            knp = kstat_data_lookup (ksp, "cpu_nsec_idle");
            total = used + knp->value.ul;

            if (total - data[i].previous_total != 0)
                data[i].load = (CPU_SCALE * (used - data[i].previous_used)) /
                               (total - data[i].previous_total);
            else
                data[i].load = 0;

            data[i].previous_used = used;
            data[i].previous_total = total;
            data[0].load += data[i].load;
            i++;
        }
    }

    data[0].load /= nb_cpu;
    return TRUE;
}
#else
#error "Your OS is not supported."
#endif
