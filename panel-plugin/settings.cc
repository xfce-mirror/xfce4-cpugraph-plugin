/*  settings.cc
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
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

#include "settings.h"
#include "../xfce4cpp/rc.hh"
#include <xfconf/xfconf.h>
#include <cmath>

using namespace std;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"

static const xfce4::RGBA g_default_colors[NUM_COLORS] =
{
    [BG_COLOR]         = {1.0, 1.0, 1.0, 0.0},
    [FG_COLOR1]        = {0.0, 1.0, 0.0, 1.0},
    [FG_COLOR2]        = {1.0, 0.0, 0.0, 1.0},
    [FG_COLOR3]        = {0.0, 0.0, 1.0, 1.0},
    [BARS_COLOR]       = {1.0, 0.73048, 0.0, 1.0},
    [SMT_ISSUES_COLOR] = {0.9, 0, 0, 1},
    [FG_COLOR_SYSTEM]  = {0.9, 0.1, 0.1, 1.0},
    [FG_COLOR_USER]    = {0.1, 0.4, 0.9, 1.0},
    [FG_COLOR_NICE]    = {0.9, 0.8, 0.2, 1.0},
    [FG_COLOR_IOWAIT]  = {0.2, 0.9, 0.4, 1.0},
};

static const gchar *const g_color_keys[NUM_COLORS][2] =
{
    [BG_COLOR]         = {"/background", "Background"},
    [FG_COLOR1]        = {"/foreground-1", "Foreground1"},
    [FG_COLOR2]        = {"/foreground-2", "Foreground2"},
    [FG_COLOR3]        = {"/foreground-3", "Foreground3"},
    [BARS_COLOR]       = {"/bars-color", "BarsColor"},
    [SMT_ISSUES_COLOR] = {"/smt-issues-color", "SmtIssuesColor"},
    [FG_COLOR_SYSTEM]  = {"/foreground-system", "ForegroundSystem"},
    [FG_COLOR_USER]    = {"/foreground-user", "ForegroundUser"},
    [FG_COLOR_NICE]    = {"/foreground-nice", "ForegroundNice"},
    [FG_COLOR_IOWAIT]  = {"/foreground-iowait", "ForegroundIOwait"},
};

#pragma clang diagnostic pop

#pragma GCC diagnostic pop

constexpr auto g_update_interval = "/update-interval";
constexpr auto g_time_scale = "/time-scale";
constexpr auto g_size = "/size";
constexpr auto g_mode = "/mode";
constexpr auto g_color_mode = "/color-mode";
constexpr auto g_frame = "/frame";
constexpr auto g_border = "/border";
constexpr auto g_bars = "/bars";
constexpr auto g_bars_perpendicular = "/bars-perpendicular";
constexpr auto g_per_core = "/per-core";
constexpr auto g_tracked_core = "/tracked-core";
constexpr auto g_in_terminal = "/in-terminal";
constexpr auto g_startup_notification = "/startup-notification";
constexpr auto g_load_threshold = "/load-threshold";
constexpr auto g_smt_stats = "/smt-stats";
constexpr auto g_smt_issues = "/smt-issues";
constexpr auto g_per_core_spacing = "/per-core-spacing";
constexpr auto g_command = "/command";

void
Settings::init (XfcePanelPlugin *plugin, const shared_ptr<CPUGraph> &base)
{
    if (!xfconf_init (nullptr))
    {
        g_critical ("Could not initialize xfconf.");
        return;
    }

    base->channel = xfconf_channel_new_with_property_base (
        "xfce4-panel",
        xfce_panel_plugin_get_property_base (plugin)
    );
}

void Settings::finalize()
{
    xfconf_shutdown ();
}

void
Settings::read (XfcePanelPlugin *plugin, const shared_ptr<CPUGraph> &base)
{
    CPUGraphUpdateRate rate = RATE_NORMAL;
    CPUGraphMode mode = MODE_NORMAL;
    guint color_mode = 0;
    bool bars = true;
    bool bars_perpendicular = false;
    bool border = true;
    bool frame = false;
    bool stats_smt = STATS_SMT_BY_DEFAULT;
    bool highlight_smt = HIGHLIGHT_SMT_BY_DEFAULT;
    bool nonlinear = false;
    bool per_core = false;
    guint per_core_spacing = PER_CORE_SPACING_DEFAULT;
    guint tracked_core = 0;

    xfce4::RGBA colors[NUM_COLORS];
    string command;
    bool in_terminal = true;
    bool startup_notification = false;
    guint load_threshold = 0;

    for (guint i = 0; i < NUM_COLORS; i++)
        colors[i] = g_default_colors[i];

    gint size = xfce_panel_plugin_get_size (plugin);

    if (const auto chn = base->channel)
    {
        bool migrate = false;

        // Migrate settings if new settings are empty, but old settings exists
        if (const auto rc_file = xfce_panel_plugin_lookup_rc_file (plugin))
        {
            migrate = true;
            if (auto table = xfconf_channel_get_properties (chn, nullptr))
            {
                migrate = (g_hash_table_size (table) <= 1);
                g_hash_table_unref (table);
            }
            if (migrate)
            {
                if (const auto rc = xfce4::Rc::simple_open (rc_file, true))
                {
                    rate = (CPUGraphUpdateRate) rc->read_int_entry ("UpdateInterval", rate);
                    nonlinear = rc->read_int_entry ("TimeScale", nonlinear);
                    size = rc->read_int_entry ("Size", size);
                    mode = (CPUGraphMode) (rc->read_int_entry ("Mode", mode - 1) + 1); // 'Disabled' mode was introduced in 1.1.0 as '-1', in 1.2.8 it was changed to 0.
                    color_mode = rc->read_int_entry ("ColorMode", color_mode);
                    frame = rc->read_int_entry ("Frame", frame);
                    in_terminal = rc->read_int_entry ("InTerminal", in_terminal);
                    startup_notification = rc->read_int_entry ("StartupNotification", startup_notification);
                    border = rc->read_int_entry ("Border", border);
                    bars = rc->read_int_entry ("Bars", bars);
                    bars_perpendicular = rc->read_int_entry ("Perpendicular Bars", bars_perpendicular);
                    highlight_smt = rc->read_int_entry ("SmtIssues", highlight_smt);
                    per_core = rc->read_int_entry ("PerCore", per_core);
                    per_core_spacing = rc->read_int_entry ("PerCoreSpacing", per_core_spacing);
                    tracked_core = rc->read_int_entry ("TrackedCore", tracked_core);
                    load_threshold = rc->read_int_entry ("LoadThreshold", load_threshold);

                    command = rc->read_entry ("Command");

                    for (guint i = 0; i < NUM_COLORS; i++)
                    {
                        if (const auto value = rc->read_entry (g_color_keys[i][1]); !value.empty())
                        {
                            xfce4::RGBA::parse (colors[i], value);
                            if (i == BARS_COLOR)
                                base->has_barcolor = true;
                        }
                    }
                }
                else
                {
                    migrate = false;
                }
            }
            g_free (rc_file);
        }

        if (!migrate)
        {
            rate = (CPUGraphUpdateRate) xfconf_channel_get_int (chn, g_update_interval, rate);
            nonlinear = xfconf_channel_get_int (chn, g_time_scale, nonlinear);
            size = xfconf_channel_get_int (chn, g_size, size);
            mode = (CPUGraphMode) xfconf_channel_get_int (chn, g_mode, mode);
            color_mode = xfconf_channel_get_int (chn, g_color_mode, color_mode);
            frame = xfconf_channel_get_int (chn, g_frame, frame);
            border = xfconf_channel_get_int (chn, g_border, border);
            bars = xfconf_channel_get_int (chn, g_bars, bars);
            bars_perpendicular = xfconf_channel_get_int (chn, g_bars_perpendicular, bars_perpendicular);
            per_core = xfconf_channel_get_int (chn, g_per_core, per_core);
            tracked_core = xfconf_channel_get_int (chn, g_tracked_core, tracked_core);
            in_terminal = xfconf_channel_get_int (chn, g_in_terminal, in_terminal);
            startup_notification = xfconf_channel_get_int (chn, g_startup_notification, startup_notification);
            load_threshold = xfconf_channel_get_int (chn, g_load_threshold, load_threshold);
            stats_smt = xfconf_channel_get_int (chn, g_smt_stats, stats_smt);
            highlight_smt = xfconf_channel_get_int (chn, g_smt_issues, highlight_smt);
            per_core_spacing = xfconf_channel_get_int (chn, g_per_core_spacing, per_core_spacing);

            if (auto value = xfconf_channel_get_string (chn, g_command, nullptr))
            {
                command = value;
                g_free (value);
            }

            for (guint i = 0; i < NUM_COLORS; i++)
            {
                xfce4::RGBA rgba;
                if (xfconf_channel_get_array (chn, g_color_keys[i][0], G_TYPE_DOUBLE, &rgba.red, G_TYPE_DOUBLE, &rgba.green, G_TYPE_DOUBLE, &rgba.blue, G_TYPE_DOUBLE, &rgba.alpha, G_TYPE_INVALID))
                {
                    colors[i] = rgba;
                    if (i == BARS_COLOR)
                        base->has_barcolor = true;
                }
            }
        }
    }

    // Validate settings
    {
        switch (mode)
        {
            case MODE_DISABLED:
            case MODE_NORMAL:
            case MODE_LED:
            case MODE_NO_HISTORY:
            case MODE_GRID:
                break;
            default:
                mode = MODE_NORMAL;
        }

        if (mode == MODE_DISABLED && !bars)
            mode = MODE_NORMAL;

        switch (rate)
        {
            case RATE_FASTEST:
            case RATE_FAST:
            case RATE_NORMAL:
            case RATE_SLOW:
            case RATE_SLOWEST:
                break;
            default:
                rate = RATE_NORMAL;
        }

        if (G_UNLIKELY (size <= 0))
            size = 10;
    }

    base->set_bars (bars);
    base->set_bars_perpendicular (bars_perpendicular);
    base->set_border (border);
    for (guint i = 0; i < NUM_COLORS; i++)
        base->set_color ((CPUGraphColorNumber) i, colors[i]);
    base->set_color_mode (color_mode);
    base->set_command (command);
    base->set_in_terminal (in_terminal);
    base->set_frame (frame);
    base->set_load_threshold (load_threshold * 0.01f);
    base->set_mode (mode);
    base->set_nonlinear_time (nonlinear);
    base->set_per_core (per_core);
    base->set_per_core_spacing (per_core_spacing);
    base->set_size (size);
    base->set_stats_smt (stats_smt);
    base->set_smt (highlight_smt);
    base->set_startup_notification (startup_notification);
    base->set_tracked_core (tracked_core);
    base->set_update_rate (rate);
}

void
Settings::write (XfcePanelPlugin *plugin, const shared_ptr<const CPUGraph> &base)
{
    const auto chn = base->channel;
    if (!chn)
        return;

    xfconf_channel_set_int (chn, g_update_interval, base->update_interval);
    xfconf_channel_set_int (chn, g_time_scale, base->non_linear);
    xfconf_channel_set_int (chn, g_size, base->size);
    xfconf_channel_set_int (chn, g_mode, base->mode);
    xfconf_channel_set_int (chn, g_color_mode, base->color_mode);
    xfconf_channel_set_int (chn, g_frame, base->has_frame);
    xfconf_channel_set_int (chn, g_border, base->has_border);
    xfconf_channel_set_int (chn, g_bars, base->has_bars);
    xfconf_channel_set_int (chn, g_bars_perpendicular, base->bars_perpendicular);
    xfconf_channel_set_int (chn, g_per_core, base->per_core);
    xfconf_channel_set_int (chn, g_tracked_core, base->tracked_core);
    xfconf_channel_set_int (chn, g_in_terminal, base->command_in_terminal);
    xfconf_channel_set_int (chn, g_startup_notification, base->command_startup_notification);
    xfconf_channel_set_int (chn, g_load_threshold, round (100.0f * base->load_threshold));
    xfconf_channel_set_int (chn, g_smt_stats, base->stats_smt);
    xfconf_channel_set_int (chn, g_smt_issues, base->highlight_smt);
    xfconf_channel_set_int (chn, g_per_core_spacing, base->per_core_spacing);

    xfconf_channel_set_string (chn, g_command, base->command.c_str());

    for (guint i = 0; i < NUM_COLORS; i++)
    {
        if (i != BARS_COLOR || base->has_barcolor)
        {
            const auto rgba = base->colors[i];
            xfconf_channel_set_array (chn, g_color_keys[i][0], G_TYPE_DOUBLE, &rgba.red, G_TYPE_DOUBLE, &rgba.green, G_TYPE_DOUBLE, &rgba.blue, G_TYPE_DOUBLE, &rgba.alpha, G_TYPE_INVALID);
        }
    }
}
