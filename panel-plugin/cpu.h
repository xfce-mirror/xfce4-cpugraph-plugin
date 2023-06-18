/*  cpu.h
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
#ifndef _XFCE_CPUGRAPH_CPU_H_
#define _XFCE_CPUGRAPH_CPU_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>
#include <string>
#include <vector>
#include "xfce4++/util.h"

#include "os.h"

using xfce4::Ptr;
using xfce4::Ptr0;

#define BORDER 8
#define HIGHLIGHT_SMT_BY_DEFAULT false
#define MAX_HISTORY_SIZE (100*1000)
#define MAX_LOAD_THRESHOLD 0.2
#define MAX_SIZE 128
#define MIN_SIZE 10
#define NONLINEAR_MODE_BASE 1.04
#define PER_CORE_SPACING_DEFAULT 1
#define PER_CORE_SPACING_MAX 3
#define PER_CORE_SPACING_MIN 0

enum CPUGraphMode
{
    MODE_DISABLED = 0,
    MODE_NORMAL = 1,
    MODE_LED = 2,
    MODE_NO_HISTORY = 3,
    MODE_GRID = 4,
};

enum CPUGraphColorMode
{
    COLOR_MODE_SOLID = 0,
    COLOR_MODE_GRADIENT = 1,
    COLOR_MODE_FIRE = 2,
    COLOR_MODE_DETAILED = 3,
};

/* Number of milliseconds between updates */
enum CPUGraphUpdateRate
{
    RATE_FASTEST = 0,
    RATE_FAST = 1,
    RATE_NORMAL = 2,
    RATE_SLOW = 3,
    RATE_SLOWEST = 4,
};

enum CPUGraphColorNumber
{
    BG_COLOR = 0,
    FG_COLOR1 = 1,
    FG_COLOR2 = 2,
    FG_COLOR3 = 3,
    BARS_COLOR = 4,
    SMT_ISSUES_COLOR = 5,
    FG_COLOR_SYSTEM = 6,
    FG_COLOR_USER = 7,
    FG_COLOR_NICE = 8,
    FG_COLOR_IOWAIT = 9,

    NUM_COLORS = 10,
};

struct CpuLoad
{
    gint64 timestamp; /* Microseconds since 1970-01-01 UTC, or zero */

    /* Range of float values: from 0.0 to 1.0 */

    /* Overall CPU load: user + system + nice */
    gfloat value;

    /* Detailed CPU load */
    gfloat system;
    gfloat user;
    gfloat nice;
    gfloat iowait;
} __attribute__((packed));

struct CPUGraph
{
    /* GUI components */
    XfcePanelPlugin *plugin;
    GtkWidget *frame_widget;
    GtkWidget *draw_area;
    GtkWidget *box;
    GtkWidget *ebox;
    struct {
        /* Widget pointers are NULL if bars are disabled */
        GtkWidget *frame;
        GtkWidget *draw_area;
        GtkOrientation orientation;
    } bars;
    GtkWidget *tooltip_text;

    /* Settings */
    XfconfChannel *channel;
    CPUGraphUpdateRate update_interval;
    guint size;
    CPUGraphMode mode;
    guint color_mode;
    std::string command;
    xfce4::RGBA colors[NUM_COLORS];
    guint tracked_core;    /* 0 means "all CPU cores", an x >= 1 means "CPU core x-1" */
    gfloat load_threshold; /* Range: from 0.0 to MAX_LOAD_THRESHOLD */
    guint per_core_spacing;

    /* Boolean settings */
    bool command_in_terminal:1;
    bool command_startup_notification:1;
    bool has_barcolor:1;
    bool has_bars:1;
    bool has_border:1;
    bool has_frame:1;
    bool highlight_smt:1;
    bool non_linear:1;
    bool per_core:1;

    /* Runtime data */
    guint nr_cores;
    guint timeout_id;
    struct {
        gssize cap_pow2;            /* Capacity. A power of 2. */
        gssize size;                /* size <= cap_pow2 */
        gssize offset;              /* Circular buffer position. Range: from 0 to (cap_pow2 - 1) */
        std::vector<CpuLoad*> data; /* Circular buffers */
        gssize mask() const         { return cap_pow2 - 1; }
    } history;
    std::vector<CpuData> cpu_data;  /* size == nr_cores+1 */
    Ptr0<Topology> topology;
    CpuStats stats;
    std::vector<const CpuLoad *> nearest_cache;
    std::vector<CpuLoad> non_linear_cache;

    ~CPUGraph();

    static void set_bars                 (const Ptr<CPUGraph> &base, bool bars);
    static void set_border               (const Ptr<CPUGraph> &base, bool border);
    static void set_color                (const Ptr<CPUGraph> &base, CPUGraphColorNumber number, const xfce4::RGBA &color);
    static void set_color_mode           (const Ptr<CPUGraph> &base, guint color_mode);
    static void set_command              (const Ptr<CPUGraph> &base, const std::string &command);
    static void set_frame                (const Ptr<CPUGraph> &base, bool frame);
    static void set_in_terminal          (const Ptr<CPUGraph> &base, bool in_terminal);
    static void set_load_threshold       (const Ptr<CPUGraph> &base, gfloat threshold);
    static void set_mode                 (const Ptr<CPUGraph> &base, CPUGraphMode mode);
    static void set_nonlinear_time       (const Ptr<CPUGraph> &base, bool nonlinear);
    static void set_per_core             (const Ptr<CPUGraph> &base, bool per_core);
    static void set_per_core_spacing     (const Ptr<CPUGraph> &base, guint spacing);
    static void set_size                 (const Ptr<CPUGraph> &base, guint width);
    static void set_smt                  (const Ptr<CPUGraph> &base, bool highlight_smt);
    static void set_startup_notification (const Ptr<CPUGraph> &base, bool startup_notification);
    static void set_tracked_core         (const Ptr<CPUGraph> &base, guint core);
    static void set_update_rate          (const Ptr<CPUGraph> &base, CPUGraphUpdateRate rate);
};

guint get_update_interval_ms (CPUGraphUpdateRate rate);

#endif /* _XFCE_CPUGRAPH_CPU_H_ */
