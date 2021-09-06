/*  settings.cc
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
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

#include "settings.h"
#include "xfce4++/util/rc.h"
#include <math.h>

static const GdkRGBA default_colors[NUM_COLORS] =
{
    [BG_COLOR]         = {1.0, 1.0, 1.0, 0.0},
    [FG_COLOR1]        = {0.0, 1.0, 0.0, 1.0},
    [FG_COLOR2]        = {1.0, 0.0, 0.0, 1.0},
    [FG_COLOR3]        = {0.0, 0.0, 1.0, 1.0},
    [BARS_COLOR]       = {1.0, 0.73048, 0.0, 1.0},
    [SMT_ISSUES_COLOR] = {0.9, 0, 0, 1},
};

static const gchar *const color_keys[NUM_COLORS] =
{
    [BG_COLOR]         = "Background",
    [FG_COLOR1]        = "Foreground1",
    [FG_COLOR2]        = "Foreground2",
    [FG_COLOR3]        = "Foreground3",
    [BARS_COLOR]       = "BarsColor",
    [SMT_ISSUES_COLOR] = "SmtIssuesColor",
};

void
read_settings (XfcePanelPlugin *plugin, CPUGraph *base)
{
    CPUGraphUpdateRate rate = RATE_NORMAL;
    CPUGraphMode mode = MODE_NORMAL;
    guint color_mode = 0;
    bool bars = true;
    bool border = true;
    bool frame = false;
    bool highlight_smt = HIGHLIGHT_SMT_BY_DEFAULT;
    bool nonlinear = false;
    bool per_core = false;
    guint per_core_spacing = PER_CORE_SPACING_DEFAULT;
    guint tracked_core = 0;

    GdkRGBA colors[NUM_COLORS];
    gchar *command = NULL;
    bool in_terminal = true;
    bool startup_notification = false;
    guint load_threshold = 0;

    for (guint i = 0; i < NUM_COLORS; i++)
        colors[i] = default_colors[i];

    gint size = xfce_panel_plugin_get_size (plugin);

    char *file;
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        const auto rc = xfce4::Rc::simple_open (file, true);
        g_free (file);

        if (rc)
        {
            xfce4::Ptr0<std::string> value;

            rate = (CPUGraphUpdateRate) rc->read_int_entry ("UpdateInterval", rate);
            nonlinear = rc->read_int_entry ("TimeScale", nonlinear);
            size = rc->read_int_entry ("Size", size);
            mode = (CPUGraphMode) rc->read_int_entry ("Mode", mode);
            color_mode = rc->read_int_entry ("ColorMode", color_mode);
            frame = rc->read_int_entry ("Frame", frame);
            in_terminal = rc->read_int_entry ("InTerminal", in_terminal);
            startup_notification = rc->read_int_entry ("StartupNotification", startup_notification);
            border = rc->read_int_entry ("Border", border);
            bars = rc->read_int_entry ("Bars", bars);
            highlight_smt = rc->read_int_entry ("SmtIssues", highlight_smt);
            per_core = rc->read_int_entry ("PerCore", per_core);
            per_core_spacing = rc->read_int_entry ("PerCoreSpacing", per_core_spacing);
            tracked_core = rc->read_int_entry ("TrackedCore", tracked_core);
            load_threshold = rc->read_int_entry ("LoadThreshold", load_threshold);

            if ((value = rc->read_entry ("Command", NULL))) {
                command = g_strdup (value->c_str());
            }

            for (guint i = 0; i < NUM_COLORS; i++)
            {
                if ((value = rc->read_entry (color_keys[i], NULL)))
                {
                    gdk_rgba_parse (&colors[i], value->c_str());
                    if (i == BARS_COLOR)
                        base->has_barcolor = true;
                }
            }

            rc->close ();
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
    base->set_border (border);
    for (guint i = 0; i < NUM_COLORS; i++)
        base->set_color ((CPUGraphColorNumber) i, colors[i]);
    base->set_color_mode (color_mode);
    if (command)
        base->set_command (command);
    base->set_in_terminal (in_terminal);
    base->set_frame (frame);
    base->set_load_threshold (load_threshold * 0.01f);
    base->set_mode (mode);
    base->set_nonlinear_time (nonlinear);
    base->set_per_core (per_core);
    base->set_per_core_spacing (per_core_spacing);
    base->set_size (size);
    base->set_smt (highlight_smt);
    base->set_startup_notification (startup_notification);
    base->set_tracked_core (tracked_core);
    base->set_update_rate (rate);
    g_free (command);
}

void
write_settings (XfcePanelPlugin *plugin, CPUGraph *base)
{
    char *file;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    const auto rc = xfce4::Rc::simple_open (file, false);
    g_free (file);
    file = NULL;

    if (!rc)
        return;

    rc->write_int_entry ("UpdateInterval", base->update_interval);
    rc->write_int_entry ("TimeScale", base->non_linear ? 1 : 0);
    rc->write_int_entry ("Size", base->size);
    rc->write_int_entry ("Mode", base->mode);
    rc->write_int_entry ("Frame", base->has_frame ? 1 : 0);
    rc->write_int_entry ("Border", base->has_border ? 1 : 0);
    rc->write_int_entry ("Bars", base->has_bars ? 1 : 0);
    rc->write_int_entry ("PerCore", base->per_core ? 1 : 0);
    rc->write_int_entry ("TrackedCore", base->tracked_core);
    if (base->command)
        rc->write_entry ("Command", base->command);
    else
        rc->delete_entry ("Command", false);
    rc->write_int_entry ("InTerminal", base->command_in_terminal ? 1 : 0);
    rc->write_int_entry ("StartupNotification", base->command_startup_notification ? 1 : 0);
    rc->write_int_entry ("ColorMode", base->color_mode);
    if (base->load_threshold != 0)
        rc->write_int_entry ("LoadThreshold", gint (roundf (100 * base->load_threshold)));
    else
        rc->delete_entry ("LoadThreshold", false);

    for (guint i = 0; i < NUM_COLORS; i++)
    {
        const gchar *key = color_keys[i];

        if(i == BARS_COLOR && !base->has_barcolor)
            key = NULL;

        if (key)
        {
            gchar *rgba = gdk_rgba_to_string (&base->colors[i]);
            gchar *rgba_default = gdk_rgba_to_string (&default_colors[i]);

            if (strcmp (rgba, rgba_default) != 0)
                rc->write_entry (key, rgba);
            else
                rc->delete_entry (key, false);

            g_free (rgba);
            g_free (rgba_default);
        }
    }

    if (base->highlight_smt != HIGHLIGHT_SMT_BY_DEFAULT)
        rc->write_int_entry ("SmtIssues", base->highlight_smt ? 1 : 0);
    else
        rc->delete_entry ("SmtIssues", false);

    if (base->per_core_spacing != PER_CORE_SPACING_DEFAULT)
        rc->write_int_entry ("PerCoreSpacing", base->per_core_spacing);
    else
        rc->delete_entry ("PerCoreSpacing", false);

    rc->close ();
}
