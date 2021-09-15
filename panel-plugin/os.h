/*  os.h
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
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
#ifndef _XFCE_CPUGRAPH_OS_H_
#define _XFCE_CPUGRAPH_OS_H_

#include <glib.h>
#include <vector>
#include "xfce4++/util.h"

using xfce4::Ptr0;

struct CpuData
{
    gfloat load; /* Range: from 0.0 to 1.0 */
    guint64 previous_used;
    guint64 previous_total;
    bool smt_highlight;
};

struct CpuStats
{
    guint num_smt_incidents;
    struct {
        /* Estimated performance loss (%): 100 * (optimal - actual) / actual */
        struct {
            gdouble actual, optimal;
        } during_smt_incidents;
        struct {
            gdouble actual, optimal;
        } total;
    } num_instructions_executed;
};

/* All pointers in this data structure are internal to a single memory allocation
 * and thus the whole data structure can be deallocated using a single call to g_free().
 * Consequenly, pointers exported/read from this data structure are invalid after the deallocation. */
struct Topology
{
    guint num_all_logical_cpus;
    guint num_online_logical_cpus;
    guint num_all_cores;                  /* Range: <1, num_all_logical_cpus> */
    guint num_online_cores;               /* Range: <1, num_online_logical_cpus> */
    std::vector<gint> logical_cpu_2_core; /* Maps a logical CPU to its core, or to -1 if offline */
    struct CpuCore {
        guint num_logical_cpus;           /* Number of logical CPUs in this core. Might be zero. */
        std::vector<guint> logical_cpus;  /* logical_cpus[i] range: <0, num_all_logical_cpus> */
    };
    std::vector<CpuCore> cores;
    bool smt;           /* Simultaneous multi-threading (hyper-threading) */
    gdouble smt_ratio;  /* Equals to (num_online_logical_cpus / num_online_cores), >= 1.0 */
};

guint detect_cpu_number ();
bool read_cpu_data (std::vector<CpuData> &data);
Ptr0<Topology> read_topology ();

#endif /* _XFCE_CPUGRAPH_OS_H */
