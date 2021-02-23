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
#ifndef _XFCE_CPU_H_
#define _XFCE_CPU_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfce4panel/libxfce4panel.h>

#include "os.h"

#define BORDER 8
#define HIGHLIGHT_SMT_BY_DEFAULT FALSE
#define MAX_HISTORY_SIZE (100*1000)
#define MAX_LOAD_THRESHOLD 0.2
#define MAX_SIZE 128
#define MIN_SIZE 10
#define NONLINEAR_MODE_BASE 1.04

typedef enum
{
    MODE_DISABLED = -1,
    MODE_NORMAL = 0,
    MODE_LED = 1,
    MODE_NO_HISTORY = 2,
    MODE_GRID = 3,
} CPUGraphMode;

/* Number of milliseconds between updates */
typedef enum
{
    RATE_FASTEST = 0,
    RATE_FAST = 1,
    RATE_NORMAL = 2,
    RATE_SLOW = 3,
    RATE_SLOWEST = 4,
} CPUGraphUpdateRate;

enum { NUM_COLORS = 6 };

typedef enum
{
    BG_COLOR = 0,
    FG_COLOR1 = 1,
    FG_COLOR2 = 2,
    FG_COLOR3 = 3,
    BARS_COLOR = 4,
    SMT_ISSUES_COLOR = 5, /* NUM_COLORS-1 */
} CPUGraphColorNumber;

typedef struct
{
    gint64 timestamp; /* Microseconds since 1970-01-01 UTC, or zero */
    gfloat value;     /* Range: from 0.0 to 1.0 */
} CpuLoad;

typedef struct
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
    gchar *command;
    GdkRGBA colors[NUM_COLORS];
    guint tracked_core;    /* 0 means "all CPU cores", an x >= 1 means "CPU core x-1" */
    gfloat load_threshold; /* Range: from 0.0 to MAX_LOAD_THRESHOLD */

    /* Boolean settings */
    gboolean command_in_terminal:1;
    gboolean command_startup_notification:1;
    gboolean has_barcolor:1;
    gboolean has_bars:1;
    gboolean has_border:1;
    gboolean has_frame:1;
    gboolean highlight_smt:1;
    gboolean non_linear:1;
    gboolean per_core:1;

    /* Runtime data */
    guint nr_cores;
    guint timeout_id;
    struct {
        gssize cap_pow2;  /* Capacity. A power of 2. */
        gssize size;      /* size <= cap_pow2 */
        gssize mask;      /* Equals to (cap_pow2 - 1) */
        gssize offset;    /* Circular buffer position. Range: from 0 to (cap_pow2 - 1) */
        CpuLoad **data;   /* Circular buffers */
    } history;
    CpuData *cpu_data;
    Topology *topology;
    CpuStats stats;
} CPUGraph;

guint get_update_interval_ms (CPUGraphUpdateRate rate);
void set_bars (CPUGraph * base, gboolean bars);
void set_border (CPUGraph *base, gboolean border);
void set_color (CPUGraph *base, guint number, GdkRGBA color);
void set_color_mode (CPUGraph *base, guint color_mode);
void set_command (CPUGraph *base, const gchar *command);
void set_frame (CPUGraph *base, gboolean frame);
void set_in_terminal (CPUGraph *base, gboolean in_terminal);
void set_load_threshold (CPUGraph *base, gfloat threshold);
void set_mode (CPUGraph *base, CPUGraphMode mode);
void set_nonlinear_time (CPUGraph *base, gboolean nonlinear);
void set_per_core (CPUGraph *base, gboolean per_core);
void set_size (CPUGraph *base, guint width);
void set_smt (CPUGraph * base, gboolean highlight_smt);
void set_startup_notification (CPUGraph *base, gboolean startup_notification);
void set_tracked_core (CPUGraph *base, guint core);
void set_update_rate (CPUGraph *base, CPUGraphUpdateRate rate);

#endif /* !_XFCE_CPU_H_ */
