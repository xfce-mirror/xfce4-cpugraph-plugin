/*  settings.c
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
#include "settings.h"
#include <libxfce4ui/libxfce4ui.h>
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
    char *file;
    XfceRc *rc;

    CPUGraphUpdateRate rate = RATE_NORMAL;
    CPUGraphMode mode = MODE_NORMAL;
    guint color_mode = 0;
    gboolean bars = TRUE;
    gboolean border = TRUE;
    gboolean frame = FALSE;
    gboolean highlight_smt = HIGHLIGHT_SMT_BY_DEFAULT;
    gboolean nonlinear = FALSE;
    gboolean per_core = FALSE;
    guint per_core_spacing = PER_CORE_SPACING_DEFAULT;
    guint tracked_core = 0;

    GdkRGBA colors[NUM_COLORS];
    guint size;
    gchar *command = NULL;
    gboolean in_terminal = TRUE;
    gboolean startup_notification = FALSE;
    guint i, load_threshold = 0;

    for (i = 0; i < NUM_COLORS; i++)
        colors[i] = default_colors[i];

    size = xfce_panel_plugin_get_size (plugin);

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc)
        {
            const gchar *value;

            rate =  xfce_rc_read_int_entry (rc, "UpdateInterval", rate);
            nonlinear = xfce_rc_read_int_entry (rc, "TimeScale", nonlinear);
            size = xfce_rc_read_int_entry (rc, "Size", size);
            mode = xfce_rc_read_int_entry (rc, "Mode", mode);
            color_mode = xfce_rc_read_int_entry (rc, "ColorMode", color_mode);
            frame = xfce_rc_read_int_entry (rc, "Frame", frame);
            in_terminal = xfce_rc_read_int_entry (rc, "InTerminal", in_terminal);
            startup_notification = xfce_rc_read_int_entry (rc, "StartupNotification", startup_notification);
            border = xfce_rc_read_int_entry (rc, "Border", border);
            bars = xfce_rc_read_int_entry (rc, "Bars", bars);
            highlight_smt = xfce_rc_read_int_entry (rc, "SmtIssues", highlight_smt);
            per_core = xfce_rc_read_int_entry (rc, "PerCore", per_core);
            per_core_spacing = xfce_rc_read_int_entry (rc, "PerCoreSpacing", per_core_spacing);
            tracked_core = xfce_rc_read_int_entry (rc, "TrackedCore", tracked_core);
            load_threshold = xfce_rc_read_int_entry (rc, "LoadThreshold", load_threshold);

            if ((value = xfce_rc_read_entry (rc, "Command", NULL))) {
                command = g_strdup (value);
            }

            for (i = 0; i < NUM_COLORS; i++)
            {
                if ((value = xfce_rc_read_entry (rc, color_keys[i], NULL)))
                {
                    gdk_rgba_parse (&colors[i], value);
                    if (i == BARS_COLOR)
                        base->has_barcolor = TRUE;
                }
            }

            xfce_rc_close (rc);
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
    }

    set_bars (base, bars);
    set_border (base, border);
    for (i = 0; i < NUM_COLORS; i++)
        set_color (base, i, colors[i]);
    set_color_mode (base, color_mode);
    if (command)
        set_command (base, command);
    set_in_terminal (base, in_terminal);
    set_frame (base, frame);
    set_load_threshold (base, load_threshold * 0.01f);
    set_mode (base, mode);
    set_nonlinear_time (base, nonlinear);
    set_per_core (base, per_core);
    set_per_core_spacing (base, per_core_spacing);
    set_size (base, size);
    set_smt (base, highlight_smt);
    set_startup_notification (base, startup_notification);
    set_tracked_core (base, tracked_core);
    set_update_rate (base, rate);
    g_free (command);
}

void
write_settings (XfcePanelPlugin *plugin, CPUGraph *base)
{
    XfceRc *rc;
    char *file;
    guint i;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;

    xfce_rc_write_int_entry (rc, "UpdateInterval", base->update_interval);
    xfce_rc_write_int_entry (rc, "TimeScale", base->non_linear ? 1 : 0);
    xfce_rc_write_int_entry (rc, "Size", base->size);
    xfce_rc_write_int_entry (rc, "Mode", base->mode);
    xfce_rc_write_int_entry (rc, "Frame", base->has_frame ? 1 : 0);
    xfce_rc_write_int_entry (rc, "Border", base->has_border ? 1 : 0);
    xfce_rc_write_int_entry (rc, "Bars", base->has_bars ? 1 : 0);
    xfce_rc_write_int_entry (rc, "PerCore", base->per_core ? 1 : 0);
    xfce_rc_write_int_entry (rc, "TrackedCore", base->tracked_core);
    if (base->command)
        xfce_rc_write_entry (rc, "Command", base->command);
    else
        xfce_rc_delete_entry (rc, "Command", FALSE);
    xfce_rc_write_int_entry (rc, "InTerminal", base->command_in_terminal ? 1 : 0);
    xfce_rc_write_int_entry (rc, "StartupNotification", base->command_startup_notification ? 1 : 0);
    xfce_rc_write_int_entry (rc, "ColorMode", base->color_mode);
    if (base->load_threshold != 0)
        xfce_rc_write_int_entry (rc, "LoadThreshold", (gint) roundf (100 * base->load_threshold));
    else
        xfce_rc_delete_entry (rc, "LoadThreshold", FALSE);

    for (i = 0; i < NUM_COLORS; i++)
    {
        const gchar *key = color_keys[i];

        if(i == BARS_COLOR && !base->has_barcolor)
            key = NULL;

        if (key)
        {
            gchar *rgba = gdk_rgba_to_string (&base->colors[i]);
            gchar *rgba_default = gdk_rgba_to_string (&default_colors[i]);

            if (strcmp (rgba, rgba_default) != 0)
                xfce_rc_write_entry (rc, key, rgba);
            else
                xfce_rc_delete_entry (rc, key, FALSE);

            g_free (rgba);
            g_free (rgba_default);
        }
    }

    if (base->highlight_smt != HIGHLIGHT_SMT_BY_DEFAULT)
        xfce_rc_write_int_entry (rc, "SmtIssues", base->highlight_smt ? 1 : 0);
    else
        xfce_rc_delete_entry (rc, "SmtIssues", FALSE);

    if (base->per_core_spacing != PER_CORE_SPACING_DEFAULT)
        xfce_rc_write_int_entry (rc, "PerCoreSpacing", base->per_core_spacing);
    else
        xfce_rc_delete_entry (rc, "PerCoreSpacing", FALSE);

    xfce_rc_close (rc);
}
