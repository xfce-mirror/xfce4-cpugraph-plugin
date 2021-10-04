/*  os.cc
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
 *  Copyright (c) 2010 Peter Tribble <peter.tribble@gmail.com>
 *  Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "os.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using xfce4::parse_ulong;

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
detect_cpu_number ()
{
    FILE *fstat = NULL;
    if (!(fstat = fopen (PROC_STAT, "r")))
        return 0;

    guint nb_cpu = 0;
    gchar cpuStr[PROCMAXLNLEN];
    while (fgets (cpuStr, PROCMAXLNLEN, fstat))
    {
        if (strncmp (cpuStr, "cpu", 3) != 0)
            break;

        gchar *s = cpuStr + 3;
        if (!g_ascii_isspace (*s))
        {
            gulong cpu = parse_ulong(&s);
            nb_cpu = MAX(nb_cpu, cpu + 1);
        }
    }

    fclose (fstat);
    return nb_cpu;
}

bool
read_cpu_data (std::vector<CpuData> &data)
{
    if (G_UNLIKELY(data.size() == 0))
        return false;

    const size_t nb_cpu = data.size()-1;
    FILE *fStat;
    gulong used[nb_cpu+1], total[nb_cpu+1];

    if (!(fStat = fopen (PROC_STAT, "r")))
        return false;

    for (guint cpu = 0; cpu < nb_cpu+1; cpu++)
        used[cpu] = total[cpu] = 0;

    while (true)
    {
        gchar cpuStr[PROCMAXLNLEN];
        if (!fgets (cpuStr, PROCMAXLNLEN, fStat))
        {
            fclose (fStat);
            return false;
        }

        if (strncmp (cpuStr, "cpu", 3) != 0)
            break;

        gchar *s = cpuStr + 3;

        guint cpu;
        if (g_ascii_isspace (*s))
            cpu = 0;
        else
            cpu = 1 + parse_ulong (&s);

        gulong user = parse_ulong (&s);
        gulong nice = parse_ulong (&s);
        gulong system = parse_ulong (&s);
        gulong idle = parse_ulong (&s);
        gulong iowait = parse_ulong (&s);
        gulong irq = parse_ulong (&s);
        gulong softirq = parse_ulong (&s);

        if (G_LIKELY (cpu < nb_cpu + 1))
        {
            used[cpu] = user + nice + system + irq + softirq;
            total[cpu] = used[cpu] + idle + iowait;
        }
    }

    fclose (fStat);

    for (guint cpu = 0; cpu < nb_cpu + 1; cpu++)
    {
        if (used[cpu] >= data[cpu].previous_used && total[cpu] > data[cpu].previous_total)
            data[cpu].load = (gfloat) (used[cpu] - data[cpu].previous_used) /
                             (gfloat) (total[cpu] - data[cpu].previous_total);
        else
            data[cpu].load = 0;

        data[cpu].previous_used = used[cpu];
        data[cpu].previous_total = total[cpu];
    }

    return true;
}

#elif defined (__FreeBSD__)
guint
detect_cpu_number ()
{
    static gint mib[] = {CTL_HW, HW_NCPU};
    gint ncpu;
    gsize len = sizeof (gint);

    if (sysctl (mib, 2, &ncpu, &len, NULL, 0) < 0)
        return 0;
    else
        return ncpu;
}

bool
read_cpu_data (std::vector<CpuData> &data)
{
    if (G_UNLIKELY(data.size() == 0))
        return false;

    const size_t nb_cpu = data.size()-1;
    glong used, total;
    glong *cp_time;
    glong *cp_time1;
    guint i;
    unsigned int max_cpu;
    gsize len = sizeof (max_cpu);

    data[0].load = 0;
    if (sysctlbyname ("kern.smp.maxid", &max_cpu, &len, NULL, 0) < 0)
        return false;

    max_cpu++; /* max_cpu is 0-based */
    if (max_cpu < nb_cpu)
        return false; /* should not happen */
    len = sizeof (glong) * max_cpu * CPUSTATES;
    cp_time = (glong *) g_malloc (len);

    if (sysctlbyname ("kern.cp_times", cp_time, &len, NULL, 0) < 0) {
        g_free (cp_time);
        return false;
    }

    for (i = 1; i <= nb_cpu; i++)
    {
        cp_time1 = &cp_time[CPUSTATES * (i - 1)];
        used = cp_time1[CP_USER] + cp_time1[CP_NICE] + cp_time1[CP_SYS] + cp_time1[CP_INTR];
        total = used + cp_time1[CP_IDLE];

        if (used >= (gint64) data[i].previous_used && total > (gint64) data[i].previous_total)
            data[i].load = (gfloat) (used - data[i].previous_used) /
                           (gfloat) (total - data[i].previous_total);
        else
            data[i].load = 0;

        data[i].previous_used = used;
        data[i].previous_total = total;
        data[0].load += data[i].load;
    }

    data[0].load /= nb_cpu;
    g_free (cp_time);
    return true;
}

#elif defined (__NetBSD__)
guint
detect_cpu_number ()
{
    static gint mib[] = {CTL_HW, HW_NCPU};
    gint ncpu;
    gsize len = sizeof (gint);

    if (sysctl (mib, 2, &ncpu, &len, NULL, 0) < 0)
        return 0;
    else
        return ncpu;
}

bool
read_cpu_data (std::vector<CpuData> &data)
{
    if (G_UNLIKELY(data.size() == 0))
        return false;

    const size_t nb_cpu = data.size()-1;
    guint64 cp_time[CPUSTATES * nb_cpu];
    gsize len = nb_cpu * CPUSTATES * sizeof (guint64);
    gint mib[] = {CTL_KERN, KERN_CP_TIME};

    if (sysctl (mib, 2, &cp_time, &len, NULL, 0) < 0)
        return false;

    data[0].load = 0;
    for (guint i = 1; i <= nb_cpu; i++)
    {
        guint64 *cp_time1 = cp_time + CPUSTATES * (i - 1);
        guint64 used = cp_time1[CP_USER] + cp_time1[CP_NICE] + cp_time1[CP_SYS] + cp_time1[CP_INTR];
        guint64 total = used + cp_time1[CP_IDLE];

        if (used >= data[i].previous_used && total > data[i].previous_total)
            data[i].load = (gfloat) (used - data[i].previous_used) /
                           (gfloat) (total - data[i].previous_total);
        else
            data[i].load = 0;

        data[i].previous_used = used;
        data[i].previous_total = total;
        data[0].load += data[i].load;
    }

    data[0].load /= nb_cpu;
    return true;
}

#elif defined (__OpenBSD__)
guint
detect_cpu_number ()
{
    static gint mib[] = {CTL_HW, HW_NCPU};
    gint ncpu;
    gsize len = sizeof (gint);

    if (sysctl (mib, 2, &ncpu, &len, NULL, 0) < 0)
        return 0;
    else
        return ncpu;
}

bool
read_cpu_data (std::vector<CpuData> &data)
{
    if (G_UNLIKELY(data.size() == 0))
        return false;

    const size_t nb_cpu = data.size()-1;
    guint64 cp_time[CPUSTATES];
    data[0].load = 0;

    for (guint i = 1; i <= nb_cpu; i++)
    {
        gsize len = CPUSTATES * sizeof (guint64);
        gint mib[] = {CTL_KERN, KERN_CPTIME2, gint(i) - 1};

        if (sysctl (mib, 3, &cp_time, &len, NULL, 0) < 0)
            return false;

        guint64 used = cp_time[CP_USER] + cp_time[CP_NICE] + cp_time[CP_SYS] + cp_time[CP_INTR];
        guint64 total = used + cp_time[CP_IDLE];

        if (used >= data[i].previous_used && total > data[i].previous_total)
            data[i].load = (gfloat) (used - data[i].previous_used) /
                           (gfloat) (total - data[i].previous_total);
        else
            data[i].load = 0;

        data[i].previous_used = used;
        data[i].previous_total = total;
        data[0].load += data[i].load;
    }

    data[0].load /= nb_cpu;
    return true;
}

#elif defined (__sun__)
static void
init_stats ()
{
    kc = kstat_open ();
}

guint
detect_cpu_number ()
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

bool
read_cpu_data (std::vector<CpuData> &data)
{
    if (G_UNLIKELY(data.size() == 0))
        return false;

    const size_t nb_cpu = data.size()-1;
    kstat_t *ksp;
    kstat_named_t *knp;
    data[0].load = 0;

    if (!kc)
        init_stats ();

    gint i = 1;

    for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next)
    {
        if (!g_strcmp0 (ksp->ks_module, "cpu") && !g_strcmp0 (ksp->ks_name, "sys"))
        {
            kstat_read (kc, ksp, NULL);
            knp = kstat_data_lookup (ksp, "cpu_nsec_user");
            guint64 used = knp->value.ul;
            knp = kstat_data_lookup (ksp, "cpu_nsec_intr");
            used += knp->value.ul;
            knp = kstat_data_lookup (ksp, "cpu_nsec_kernel");
            used += knp->value.ul;
            knp = kstat_data_lookup (ksp, "cpu_nsec_idle");
            guint64 total = used + knp->value.ul;

            if (used >= data[i].previous_used && total > data[i].previous_total)
                data[i].load = (gfloat) (used - data[i].previous_used) /
                               (gfloat) (total - data[i].previous_total);
            else
                data[i].load = 0;

            data[i].previous_used = used;
            data[i].previous_total = total;
            data[0].load += data[i].load;
            i++;
        }
    }

    data[0].load /= nb_cpu;
    return true;
}
#else
#error "Your OS is not supported."
#endif



#if defined (__linux__)

#define SYSFS_BASE "/sys/devices/system/cpu"

Ptr0<Topology>
read_topology ()
{
    std::unordered_set<gint> core_ids;
    std::unordered_map<guint, gint> logical_cpu_2_core;
    gint max_core_id = -1;

    guint num_online_logical_cpus = 0;
    for (guint logical_cpu = 0; true; logical_cpu++)
    {
        /* See also: https://www.kernel.org/doc/html/latest/admin-guide/cputopology.html */

        if (!xfce4::is_directory (xfce4::sprintf ("%s/cpu%d", SYSFS_BASE, logical_cpu)))
            break;

        std::string file_contents;
        if (xfce4::read_file (xfce4::sprintf ("%s/cpu%d/topology/core_id", SYSFS_BASE, logical_cpu), file_contents))
        {
            auto core_id_opt = xfce4::parse_long (file_contents, 10);
            if (core_id_opt.has_value())
            {
                auto core_id = core_id_opt.value();
                if (G_LIKELY (core_id >= 0 && core_id <= G_MAXINT))
                {
                    num_online_logical_cpus++;
                    core_ids.insert(core_id);
                    logical_cpu_2_core[logical_cpu] = core_id;
                    if (max_core_id < core_id)
                        max_core_id = core_id;
                }
                else
                    return nullptr;
            }
            else
                return nullptr;
        }
        else
        {
            /* The CPU is probably offline */
            logical_cpu_2_core[logical_cpu] = -1;
        }
    }

    const size_t num_cores = core_ids.size();
    const size_t num_logical_cpus = logical_cpu_2_core.size();

    /* Perform some sanity checks */
    if (G_UNLIKELY (max_core_id < 0 || max_core_id > G_MAXINT-1 || !(num_cores <= num_logical_cpus)))
        return nullptr;

    if (!logical_cpu_2_core.empty())
    {
        auto t = xfce4::make<Topology>();

        /* Fill-in the topology data */

        t->num_logical_cpus = num_logical_cpus;
        t->num_online_logical_cpus = num_online_logical_cpus;
        t->num_cores = num_cores;
        t->logical_cpu_2_core.resize(num_logical_cpus);
        {
            for (const auto &i : logical_cpu_2_core)
            {
                guint logical_cpu = i.first;
                gint core_id = i.second;
                if (core_id != -1)
                {
                    t->logical_cpu_2_core[logical_cpu] = core_id;
                    t->cores[core_id].logical_cpus.push_back(logical_cpu);
                }
                else
                {
                    t->logical_cpu_2_core[logical_cpu] = -1;
                }
                g_info ("thread %u is in core %d", logical_cpu, t->logical_cpu_2_core[logical_cpu]);
            }
        }

        t->num_online_cores = 0;
        t->smt = false;
        for (const auto &i : t->cores)
        {
            const Topology::CpuCore &core = i.second;
            if (!core.logical_cpus.empty())
                t->num_online_cores++;
            if (core.logical_cpus.size() > 1)
                t->smt = true;
        }
        t->smt_ratio = t->num_online_logical_cpus / (gdouble) t->num_online_cores;

        g_info ("num_logical_cpus: %u total, %u online", t->num_logical_cpus, t->num_online_logical_cpus);
        g_info ("num_cores: %u total, %u online", t->num_cores, t->num_online_cores);
        g_info ("smt: %s, ratio=%.3f", t->smt ? "active" : "inactive", t->smt_ratio);

        return t;
    }
    else
        return nullptr;
}

#else

Ptr0<Topology>
read_topology ()
{
    return nullptr;
}

#endif
