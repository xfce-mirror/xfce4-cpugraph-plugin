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
#include <unordered_map>
#include <vector>
#include <memory>

struct CpuData
{
    /* Range of float values: from 0.0 to 1.0 */

    gfloat load; /* Overall CPU load: user + system + nice */

    guint64 previous_used;
    guint64 previous_total;

    bool smt_highlight;

    /* Detailed CPU load */
    gfloat system;
    gfloat user;
    gfloat nice;
    gfloat iowait;

    guint64 previous_system;
    guint64 previous_user;
    guint64 previous_nice;
    guint64 previous_iowait;
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

struct Topology
{
    guint num_logical_cpus;
    guint num_online_logical_cpus;
    guint num_cores;                      /* Range: <1, num_logical_cpus> */
    guint num_online_cores;               /* Range: <1, num_online_logical_cpus> */
    std::vector<gint> logical_cpu_2_core; /* Maps a logical CPU to its core, or to -1 if offline */

    struct CpuCore {
        std::vector<guint> logical_cpus;  /* Logical CPUs in this core. Empty if the core is offline. */
    };

    /* Maps a core ID to a CpuCore. The core ID can be larger than num_cores,
     * for example in case of a 6-core CPU on an 8-core die (2 cores are disabled). */
    std::unordered_map<guint, CpuCore> cores;

    bool smt;           /* Simultaneous multi-threading (hyper-threading) */
    gdouble smt_ratio;  /* Equals to (num_online_logical_cpus / num_online_cores), >= 1.0 */
};

void read_cpu_data (std::unordered_map<guint, CpuData> &data, std::unordered_map<guint, guint> &cpu_to_index);
std::unique_ptr<Topology> read_topology ();

#endif /* _XFCE_CPUGRAPH_OS_H */
