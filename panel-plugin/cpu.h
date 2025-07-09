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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>
#include <string>
#include <vector>
#include "../xfce4cpp/rgba.hh"
#include "../xfce4cpp/timer.hh"

#include "os.h"

constexpr auto BORDER = 8;
constexpr auto BAR_SPACE = 2;
constexpr auto STATS_SMT_BY_DEFAULT = false;
constexpr auto HIGHLIGHT_SMT_BY_DEFAULT = false;
constexpr auto MAX_HISTORY_SIZE = 100 * 1000;
constexpr auto MAX_LOAD_THRESHOLD = 0.2;
constexpr auto MAX_SIZE = 128;
constexpr auto MIN_SIZE = 1;
constexpr auto NONLINEAR_MODE_BASE = 1.04;
constexpr auto PER_CORE_SPACING_DEFAULT = 1;
constexpr auto PER_CORE_SPACING_MAX = 3;
constexpr auto PER_CORE_SPACING_MIN = 0;

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

struct CPUGraph final : public std::enable_shared_from_this<CPUGraph>
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
    gint tooltip_last_value;
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
    bool command_in_terminal;
    bool command_startup_notification;
    bool has_barcolor;
    bool has_bars;
    bool bars_perpendicular;
    bool has_border;
    bool has_frame;
    bool stats_smt;
    bool highlight_smt;
    bool non_linear;
    bool per_core;

    /* Runtime data */
    std::unordered_map<guint, guint> cpu_to_index_cache;
    std::unordered_map<guint, guint> cpu_to_index;
    std::unordered_map<guint, guint> index_to_cpu;
    guint nr_cores;
    xfce4::SourceTag timeout_id;
    struct {
        gssize cap_pow2;            /* Capacity. A power of 2. */
        gssize size;                /* size <= cap_pow2 */
        gssize offset;              /* Circular buffer position. Range: from 0 to (cap_pow2 - 1) */
        std::vector<std::unique_ptr<CpuLoad[]>> data; /* Circular buffers */
        gssize mask() const         { return cap_pow2 - 1; }
    } history;
    std::unordered_map<guint, CpuData> cpu_data;  /* size == nr_cores+1 */
    std::unique_ptr<Topology> topology;
    CpuStats stats;
    std::vector<const CpuLoad *> nearest_cache;
    std::vector<CpuLoad> non_linear_cache;
    struct {
        std::vector<bool> movement;
        std::vector<bool> suboptimal;
        std::vector<gfloat> actual_load;
        std::vector<gfloat> optimal_load;
        std::vector<gfloat> actual_num_instr_executed;
        std::vector<gfloat> optimal_num_instr_executed;
    } smt;

    ~CPUGraph();

    // Called outside of "cpu.cc"
    void set_bars                 (bool has_bars_arg);
    void set_bars_perpendicular   (bool bars_perpendicular_arg);
    void set_border               (bool has_border_arg);
    void set_color                (CPUGraphColorNumber number, const xfce4::RGBA &color);
    void set_color_mode           (guint color_mode_arg);
    void set_command              (const std::string_view &command_arg);
    void set_frame                (bool has_frame_arg);
    void set_in_terminal          (bool in_terminal);
    void set_load_threshold       (gfloat threshold);
    void set_mode                 (CPUGraphMode mode_arg);
    void set_nonlinear_time       (bool non_linear_arg);
    void set_per_core             (bool per_core_arg);
    void set_per_core_spacing     (guint spacing);
    void set_size                 (guint size_arg);
    void set_stats_smt            (bool stats_smt_arg);
    void set_smt                  (bool highlight_smt_arg);
    void set_startup_notification (bool startup_notification);
    void set_tracked_core         (guint core);
    void set_update_rate          (CPUGraphUpdateRate rate);
    void maybe_clear_smt_stats    ();

    // Called inside "cpu.cc"
    bool  is_smt_issues_enabled   () const;
    void  detect_smt_issues       ();
    void  create_bars             (GtkOrientation orientation);
    void  delete_bars             ();
    void  set_bars_size           ();
    guint nb_bars                 ();
    void  ebox_revalidate         ();
};

guint get_update_interval_ms (CPUGraphUpdateRate rate);

#endif /* _XFCE_CPUGRAPH_CPU_H_ */
