/*  cpu_os.h
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _CPU_OS_H
#define _CPU_OS_H

#define CPU_SCALE 1;/*00000*/

extern int scaling_cur_freq;
extern int scaling_max_freq;
extern int scaling_min_freq;

typedef struct s_cpuLoadData
{
	float         value;  /* cpu utilization % */
	unsigned long pUsed;  /* Previous value of used cpu time */
	unsigned long pTotal; /* Previous value of total cpu time */
} cpuLoadData;

int cpuLoadMon_init();

void cpuLoadMon_free();

cpuLoadData *cpuLoadMon_read();

#endif /* _CPU_OS_H */
