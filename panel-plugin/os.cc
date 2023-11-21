/*  os.cc
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
 *  Copyright (c) 2010 Peter Tribble <peter.tribble@gmail.com>
 *  Copyright (c) 2021-2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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
#include "../xfce4cpp/string-utils.hh"
#include "../xfce4cpp/io.hh"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

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
void
read_cpu_data (unordered_map<guint, CpuData> &data, unordered_map<guint, guint> &cpu_to_index)
{
    FILE *fStat;

    cpu_to_index.clear();

    if (!(fStat = fopen (PROC_STAT, "r")))
        return;

    guint cpu_it = 0;

    while (true)
    {
        gchar cpuStr[PROCMAXLNLEN];
        if (!fgets (cpuStr, PROCMAXLNLEN, fStat))
        {
            fclose (fStat);
            cpu_to_index.clear();
            return;
        }

        if (strncmp (cpuStr, "cpu", 3) != 0)
            break;

        gchar *s = cpuStr + 3;

        guint cpu;
        if (g_ascii_isspace (*s))
        {
            cpu = 0;
        }
        else
        {
            cpu = g_ascii_strtoull (s, &s, 0) + 1;
            cpu_to_index[cpu] = ++cpu_it; // Map logical system CPU to array index
        }

        gulong user = g_ascii_strtoull (s, &s, 0);
        gulong nice = g_ascii_strtoull (s, &s, 0);
        gulong system = g_ascii_strtoull (s, &s, 0);
        gulong idle = g_ascii_strtoull (s, &s, 0);
        gulong iowait = g_ascii_strtoull (s, &s, 0);
        gulong irq = g_ascii_strtoull (s, &s, 0);
        gulong softirq = g_ascii_strtoull (s, &s, 0);

        auto &data_cpu = data[cpu];

        system += irq + softirq;
        gulong total = system + user + nice + iowait + idle;

        const bool total_greater = total > data_cpu.previous_total;
        const gfloat divider = (gfloat) (total - data_cpu.previous_total);

        if (total_greater && system >= data_cpu.previous_system)
            data_cpu.system = (system - data_cpu.previous_system) / divider;
        else
            data_cpu.system = 0.0f;

        if (total_greater && user >= data_cpu.previous_user)
            data_cpu.user = (user - data_cpu.previous_user) / divider;
        else
            data_cpu.user = 0.0f;

        if (total_greater && nice >= data_cpu.previous_nice)
            data_cpu.nice = (nice - data_cpu.previous_nice) / divider;
        else
            data_cpu.nice = 0.0f;

        if (total_greater && iowait >= data_cpu.previous_iowait)
            data_cpu.iowait = (iowait - data_cpu.previous_iowait) / divider;
        else
            data_cpu.iowait = 0.0f;

        data_cpu.load = data_cpu.user + data_cpu.system + data_cpu.nice;

        data_cpu.previous_system = system;
        data_cpu.previous_user = user;
        data_cpu.previous_nice = nice;
        data_cpu.previous_iowait = iowait;
        data_cpu.previous_total = total;
    }

    fclose (fStat);
}

#elif defined (__FreeBSD__)
static guint
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

static bool
read_cpu_data (unordered_map<guint, CpuData> &data)
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
static guint
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

static bool
read_cpu_data (unordered_map<guint, CpuData> &data)
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
static guint
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

static bool
read_cpu_data (unordered_map<guint, CpuData> &data)
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

static guint
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

static bool
read_cpu_data (unordered_map<guint, CpuData> &data)
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


#if !(defined (__linux__) || defined (__FreeBSD_kernel__))
void
read_cpu_data (unordered_map<guint, CpuData> &data, unordered_map<guint, guint> &cpu_to_index)
{
    static guint n_cpus = detect_cpu_number ();

    if (data.empty())
    {
        data.reserve (n_cpus + 1);
        for (guint i = 0; i < n_cpus + 1; ++i)
        {
            data[i] = CpuData ();
        }
    }

    if (read_cpu_data (data))
    {
        if (cpu_to_index.empty ())
        {
            cpu_to_index.reserve (n_cpus);
            for (guint i = 0; i < n_cpus; ++i)
            {
                cpu_to_index[i] = i;
            }
        }
    }
    else
    {
        cpu_to_index.clear();
    }
}
#endif


static unique_ptr<Topology>
read_topology_linux ()
{
    unordered_set<gint> core_ids;
    unordered_map<guint, gint> logical_cpu_2_core;
    gint max_core_id = -1;

    guint num_online_logical_cpus = 0;
    for (guint logical_cpu = 0; true; logical_cpu++)
    {
        /* See also: https://www.kernel.org/doc/html/latest/admin-guide/cputopology.html */

        const char *sysfs_base = "/sys/devices/system/cpu";

        if (!xfce4::is_directory (xfce4::sprintf ("%s/cpu%d", sysfs_base, logical_cpu)))
            break;

        xfce4::g_string_view file_contents;
        if (xfce4::read_file (xfce4::sprintf ("%s/cpu%d/topology/core_id", sysfs_base, logical_cpu), file_contents))
        {
            const auto core_id = xfce4::parse_long (file_contents, 10);
            if (core_id.has_value() && G_LIKELY (core_id.value() >= 0 && core_id.value() <= G_MAXINT))
            {
                num_online_logical_cpus++;
                core_ids.insert(core_id.value());
                logical_cpu_2_core[logical_cpu] = core_id.value();
                if (max_core_id < core_id)
                    max_core_id = core_id.value();
            }
            else
            {
                return nullptr;
            }
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
        auto t = make_unique<Topology>();

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

unique_ptr<Topology>
read_topology ()
{
    bool is_linux = false;

#if defined __linux__
    is_linux = true;
#endif

    if (is_linux)
        return read_topology_linux();
    else
        return nullptr;
}
