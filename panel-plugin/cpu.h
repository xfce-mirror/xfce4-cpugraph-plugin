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
#include <string>
#include <vector>
#include "xfce4++/util.h"

#include "os.h"

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
    MODE_DISABLED = -1,
    MODE_NORMAL = 0,
    MODE_LED = 1,
    MODE_NO_HISTORY = 2,
    MODE_GRID = 3,
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

enum { NUM_COLORS = 6 };

enum CPUGraphColorNumber
{
    BG_COLOR = 0,
    FG_COLOR1 = 1,
    FG_COLOR2 = 2,
    FG_COLOR3 = 3,
    BARS_COLOR = 4,
    SMT_ISSUES_COLOR = 5, /* NUM_COLORS-1 */
};

struct CpuLoad
{
    gint64 timestamp; /* Microseconds since 1970-01-01 UTC, or zero */
    gfloat value;     /* Range: from 0.0 to 1.0 */
};

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
        gssize mask;                /* Equals to (cap_pow2 - 1) */
        gssize offset;              /* Circular buffer position. Range: from 0 to (cap_pow2 - 1) */
        std::vector<CpuLoad*> data; /* Circular buffers */
    } history;
    std::vector<CpuData> cpu_data;  /* size == nr_cores+1 */
    xfce4::Ptr0<Topology> topology;
    CpuStats stats;

    ~CPUGraph();

    void set_bars (bool bars);
    void set_border (bool border);
    void set_color (CPUGraphColorNumber number, const xfce4::RGBA &color);
    void set_color_mode (guint color_mode);
    void set_command (const gchar *command);
    void set_frame (bool frame);
    void set_in_terminal (bool in_terminal);
    void set_load_threshold (gfloat threshold);
    void set_mode (CPUGraphMode mode);
    void set_nonlinear_time (bool nonlinear);
    void set_per_core (bool per_core);
    void set_per_core_spacing (guint spacing);
    void set_size (guint width);
    void set_smt (bool highlight_smt);
    void set_startup_notification (bool startup_notification);
    void set_tracked_core (guint core);
    void set_update_rate (CPUGraphUpdateRate rate);
};

guint get_update_interval_ms (CPUGraphUpdateRate rate);

#endif /* _XFCE_CPUGRAPH_CPU_H_ */
