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

void
read_settings (XfcePanelPlugin *plugin, CPUGraph *base)
{
    char *file;
    XfceRc *rc;

    guint rate = 0;
    gboolean nonlinear = FALSE;
    gint mode = 0;
    guint color_mode = 0;
    gboolean frame = FALSE;
    gboolean border = TRUE;
    gboolean bars = TRUE;
    guint tracked_core = 0;

    GdkRGBA foreground1;
    GdkRGBA foreground2;
    GdkRGBA foreground3;
    GdkRGBA background;
    GdkRGBA barscolor;
    guint size;
    gchar *command = NULL;
    gboolean in_terminal = TRUE;
    gboolean startup_notification = FALSE;

    foreground1.red = 0.0;
    foreground1.green = 1.0;
    foreground1.blue = 0.0;
    foreground1.alpha = 1.0;

    foreground2.red = 1.0;
    foreground2.green = 0.0;
    foreground2.blue = 0.0;
    foreground2.alpha = 1.0;

    foreground3.red = 0.0;
    foreground3.green = 0.0;
    foreground3.blue = 1.0;
    foreground3.alpha = 1.0;

    background.red = 1.0;
    background.green = 1.0;
    background.blue = 1.0;
    background.alpha = 0.0;

    barscolor.red = 1.0;
    barscolor.green = 0.73048;
    barscolor.blue = 0.0;
    barscolor.alpha = 1.0;

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
            tracked_core = xfce_rc_read_int_entry (rc, "TrackedCore", tracked_core);

            if ((value = xfce_rc_read_entry (rc, "Command", NULL))) {
                command = g_strdup (value);
            }

            if ((value = xfce_rc_read_entry (rc, "Foreground1", NULL)))
                gdk_rgba_parse (&foreground1, value);
            if ((value = xfce_rc_read_entry (rc, "Foreground2", NULL)))
                gdk_rgba_parse (&foreground2, value);
            if ((value = xfce_rc_read_entry (rc, "Foreground3", NULL)))
                gdk_rgba_parse (&foreground3, value);
            if ((value = xfce_rc_read_entry (rc, "Background", NULL)))
                gdk_rgba_parse (&background, value);
            if ((value = xfce_rc_read_entry (rc, "BarsColor", NULL))) {
                gdk_rgba_parse (&barscolor, value);
                base->has_barcolor = TRUE;
            }

            xfce_rc_close (rc);
        }
    }

    set_update_rate (base, rate);
    set_nonlinear_time (base, nonlinear);
    set_size (base, size);
    set_mode (base, mode);
    set_color_mode (base, color_mode);
    set_frame (base, frame);
    if (command)
        set_command (base, command);
    set_in_terminal (base, in_terminal);
    set_startup_notification (base, startup_notification);
    set_border (base, border);
    set_tracked_core (base, tracked_core);
    set_bars (base, bars);
    set_color (base, 1, foreground1);
    set_color (base, 2, foreground2);
    set_color (base, 3, foreground3);
    set_color (base, 0, background);
    set_color (base, 4, barscolor);
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
    xfce_rc_write_int_entry (rc, "TimeScale", base->non_linear);
    xfce_rc_write_int_entry (rc, "Size", base->size);
    xfce_rc_write_int_entry (rc, "Mode", base->mode);
    xfce_rc_write_int_entry (rc, "Frame", base->has_frame);
    xfce_rc_write_int_entry (rc, "Border", base->has_border);
    xfce_rc_write_int_entry (rc, "Bars", base->has_bars);
    xfce_rc_write_int_entry (rc, "TrackedCore", base->tracked_core);
    if (base->command)
        xfce_rc_write_entry (rc, "Command", base->command);
    else
        xfce_rc_delete_entry (rc, "Command", FALSE);
    xfce_rc_write_int_entry (rc, "InTerminal", base->in_terminal);
    xfce_rc_write_int_entry (rc, "StartupNotification", base->startup_notification);
    xfce_rc_write_int_entry (rc, "ColorMode", base->color_mode);

    for (i = 0; i < 5; i++)
    {
        gchar *rgba = gdk_rgba_to_string (&(base->colors[i]));
        switch (i)
        {
            case 0: xfce_rc_write_entry (rc, "Background", rgba); break;
            case 1: xfce_rc_write_entry (rc, "Foreground1", rgba); break;
            case 2: xfce_rc_write_entry (rc, "Foreground2", rgba); break;
            case 3: xfce_rc_write_entry (rc, "Foreground3", rgba); break;
            case 4:
                if (base->has_barcolor)
                    xfce_rc_write_entry (rc, "BarsColor", rgba);
                break;
        }
        g_free (rgba);
    }

    xfce_rc_close (rc);
}
