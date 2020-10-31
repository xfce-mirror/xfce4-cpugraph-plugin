/*  cpu.c
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
#include "cpu.h"
#include "settings.h"
#include "mode.h"
#include "properties.h"

#include <libxfce4ui/libxfce4ui.h>
#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

static void       cpugraph_construct   (XfcePanelPlugin    *plugin);
static CPUGraph  *create_gui           (XfcePanelPlugin    *plugin);
static void       create_bars          (CPUGraph           *base,
                                        GtkOrientation      orientation);
static guint      init_cpu_data        (CpuData           **data);
static void       shutdown             (XfcePanelPlugin    *plugin,
                                        CPUGraph           *base);
static void       delete_bars          (CPUGraph           *base);
static gboolean   size_cb              (XfcePanelPlugin    *plugin,
                                        guint               size,
                                        CPUGraph           *base);
static void       about_cb             (XfcePanelPlugin    *plugin,
                                        CPUGraph           *base);
static void       set_bars_size        (CPUGraph           *base);
static void       mode_cb              (XfcePanelPlugin    *plugin,
                                        XfcePanelPluginMode mode,
                                        CPUGraph           *base);
static gboolean   update_cb            (CPUGraph           *base);
static void       update_tooltip       (CPUGraph           *base);
static gboolean   tooltip_cb           (GtkWidget          *widget,
                                        gint                x,
                                        gint                y,
                                        gboolean            keyboard,
                                        GtkTooltip         *tooltip,
                                        CPUGraph           *base);
static void       draw_area_cb         (GtkWidget          *w,
                                        cairo_t            *cr,
                                        gpointer            data);
static void       draw_bars_cb         (GtkWidget          *w,
                                        cairo_t            *cr,
                                        gpointer            data);
static gboolean   command_cb           (GtkWidget          *w,
                                        GdkEventButton     *event,
                                        CPUGraph           *base);

XFCE_PANEL_PLUGIN_REGISTER (cpugraph_construct);

static void
cpugraph_construct (XfcePanelPlugin *plugin)
{
    CPUGraph *base;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    base = create_gui (plugin);
    read_settings (plugin, base);
    xfce_panel_plugin_menu_show_configure (plugin);

    xfce_panel_plugin_menu_show_about (plugin);

    g_signal_connect (plugin, "about", G_CALLBACK (about_cb), base);
    g_signal_connect (plugin, "free-data", G_CALLBACK (shutdown), base);
    g_signal_connect (plugin, "save", G_CALLBACK (write_settings), base);
    g_signal_connect (plugin, "configure-plugin", G_CALLBACK (create_options), base);
    g_signal_connect (plugin, "size-changed", G_CALLBACK (size_cb), base);
    g_signal_connect (plugin, "mode-changed", G_CALLBACK (mode_cb), base);
}

static CPUGraph *
create_gui (XfcePanelPlugin *plugin)
{
    GtkWidget *frame, *ebox;
    GtkOrientation orientation;
    CPUGraph *base = g_new0 (CPUGraph, 1);

    orientation = xfce_panel_plugin_get_orientation (plugin);
    if ((base->nr_cores = init_cpu_data (&base->cpu_data)) == 0)
        fprintf (stderr,"Cannot init cpu data !\n");

    base->plugin = plugin;

    ebox = gtk_event_box_new ();
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (ebox), FALSE);
    gtk_event_box_set_above_child (GTK_EVENT_BOX (ebox), TRUE);
    gtk_container_add (GTK_CONTAINER (plugin), ebox);
    xfce_panel_plugin_add_action_widget (plugin, ebox);
    g_signal_connect (ebox, "button-press-event", G_CALLBACK (command_cb), base);

    base->box = gtk_box_new (orientation, 0);
    gtk_container_add (GTK_CONTAINER (ebox), base->box);
    gtk_widget_set_has_tooltip (base->box, TRUE);
    g_signal_connect (base->box, "query-tooltip", G_CALLBACK (tooltip_cb), base);

    base->frame_widget = frame = gtk_frame_new (NULL);
    gtk_box_pack_end (GTK_BOX (base->box), frame, TRUE, TRUE, 2);

    base->draw_area = gtk_drawing_area_new ();
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (base->draw_area));
    g_signal_connect_after (base->draw_area, "draw", G_CALLBACK (draw_area_cb), base);

    base->has_bars = FALSE;
    base->has_barcolor = FALSE;
    base->bars.orientation = orientation;

    mode_cb (plugin, (XfcePanelPluginMode) orientation, base);
    gtk_widget_show_all (ebox);

    base->tooltip_text = gtk_label_new (NULL);
    g_object_ref (base->tooltip_text);

    return base;
}

static void
about_cb (XfcePanelPlugin *plugin, CPUGraph *base)
{
    GdkPixbuf *icon;
    const gchar *auth[] = {
        "Alexander Nordfelth <alex.nordfelth@telia.com>", "gatopeich <gatoguan-os@yahoo.com>",
        "lidiriel <lidiriel@coriolys.org>","Angelo Miguel Arrifano <miknix@gmail.com>",
        "Florian Rivoal <frivoal@gmail.com>","Peter Tribble <peter.tribble@gmail.com>", NULL};
    icon = xfce_panel_pixbuf_from_source ("xfce4-cpugraph-plugin", NULL, 32);
    gtk_show_about_dialog (NULL,
        "logo", icon,
        "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
        "version", PACKAGE_VERSION,
        "program-name", PACKAGE_NAME,
        "comments", _("Graphical representation of the CPU load"),
        "website", "https://docs.xfce.org/panel-plugins/xfce4-cpugraph-plugin",
        "copyright", _("Copyright (c) 2003-2019\n"),
        "authors", auth, NULL);

    if (icon)
        g_object_unref (G_OBJECT (icon));
}

static guint
nb_bars (CPUGraph *base)
{
    return base->tracked_core == 0 ? base->nr_cores : 1;
}

static void
create_bars (CPUGraph *base, GtkOrientation orientation)
{
    base->bars.frame = gtk_frame_new (NULL);
    base->bars.draw_area = gtk_drawing_area_new ();
    base->bars.orientation = orientation;
    gtk_container_add (GTK_CONTAINER (base->bars.frame), base->bars.draw_area);
    gtk_box_pack_end (GTK_BOX (base->box), base->bars.frame, TRUE, TRUE, 0);
    g_signal_connect_after (base->bars.draw_area, "draw", G_CALLBACK (draw_bars_cb), base);
    gtk_widget_show_all (base->bars.frame);
}

guint
init_cpu_data (CpuData **data)
{
    guint cpuNr;

    cpuNr = detect_cpu_number ();
    if (cpuNr == 0)
        return 0;

    *data = (CpuData *) g_malloc0 ((cpuNr+1) * sizeof (CpuData));

    return cpuNr;
}

static void
shutdown (XfcePanelPlugin *plugin, CPUGraph *base)
{
    g_free (base->cpu_data);
    delete_bars (base);
    gtk_widget_destroy (base->box);
    gtk_widget_destroy (base->tooltip_text);
    if (base->timeout_id)
        g_source_remove (base->timeout_id);
    g_free (base->history);
    g_free (base->command);
    g_free (base);
}

static void
delete_bars (CPUGraph *base)
{
    if (base->bars.frame)
    {
        gtk_widget_destroy (base->bars.frame);
        base->bars.frame = NULL;
        base->bars.draw_area = NULL;
    }
}

static gboolean
size_cb (XfcePanelPlugin *plugin, guint size, CPUGraph *base)
{
    gint frame_h, frame_v, history;
    GtkOrientation orientation;
    gint shadow_width = base->has_frame ? 2*1 : 0;

    orientation = xfce_panel_plugin_get_orientation (plugin);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        frame_h = base->size + shadow_width;
        frame_v = size;
        history = base->size;
    }
    else
    {
        frame_h = size;
        frame_v = base->size + shadow_width;
        history = size;
    }

    gtk_widget_set_size_request (GTK_WIDGET (base->frame_widget), frame_h, frame_v);

    base->history = (guint *) g_realloc (base->history, history * sizeof (guint));
    if (history > base->history_size)
        memset (base->history + base->history_size, 0, (history - base->history_size) * sizeof (guint));
    base->history_size = history;

    if (base->has_bars) {
        base->bars.orientation = orientation;
        set_bars_size (base);
    }

    set_border (base, base->has_border);

    return TRUE;
}

static void
set_bars_size (CPUGraph *base)
{
    gint h, v;
    gint shadow_width;

    shadow_width = base->has_frame ? 2*1 : 0;

    if (base->bars.orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        h = 6 * nb_bars (base) - 2 + shadow_width;
        v = -1;
    }
    else
    {
        h = -1;
        v = 6 * nb_bars (base) - 2 + shadow_width;
    }

    gtk_widget_set_size_request (base->bars.frame, h, v);
}

static void
mode_cb (XfcePanelPlugin *plugin, XfcePanelPluginMode mode, CPUGraph *base)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (base->box),
                                    xfce_panel_plugin_get_orientation (plugin));

    size_cb (plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

static gboolean
update_cb (CPUGraph *base)
{
    if (!read_cpu_data (base->cpu_data, base->nr_cores))
        return TRUE;

    if (base->tracked_core > base->nr_cores)
        base->cpu_data[0].load = 0;
    else if (base->tracked_core != 0)
        base->cpu_data[0].load = base->cpu_data[base->tracked_core].load;

    if (base->mode == -1)
    {
        /* Disabled mode, skip updating history and drawing the graph */
        return TRUE;
    }

    if (base->non_linear)
    {
        gssize i = base->history_size - 1;
        while (i > 0)
        {
            gint a, b, factor;
            a = base->history[i], b = base->history[i-1];
            if (a < b) a++;
            factor = (i * 2);
            base->history[i--] = (a * (factor-1) + b) / factor;
        }
    }
    else {
        memmove (base->history + 1 , base->history , (base->history_size - 1) * sizeof (guint));
    }
    base->history[0] = base->cpu_data[0].load;

    update_tooltip (base);
    gtk_widget_queue_draw (base->draw_area);
    if (base->bars.draw_area)
        gtk_widget_queue_draw (base->bars.draw_area);

    return TRUE;
}

static void
update_tooltip (CPUGraph *base)
{
    gchar tooltip[32];
    g_snprintf (tooltip, 32, _("Usage: %u%%"), (guint) base->cpu_data[0].load * 100 / CPU_SCALE);
    gtk_label_set_text (GTK_LABEL (base->tooltip_text), tooltip);
}

static gboolean
tooltip_cb (GtkWidget *widget, gint x, gint y, gboolean keyboard, GtkTooltip *tooltip, CPUGraph *base)
{
    gtk_tooltip_set_custom (tooltip, base->tooltip_text);
    return TRUE;
}

static void
draw_area_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    CPUGraph *base = (CPUGraph *) data;
    GtkAllocation alloc;
    gint w, h;

    gtk_widget_get_allocation (base->draw_area, &alloc);
    w = alloc.width;
    h = alloc.height;

    gdk_cairo_set_source_rgba (cr, &base->colors[0]);
    cairo_rectangle (cr, 0, 0, w, h);
    cairo_fill (cr);

    switch (base->mode)
    {
        case 0:
            draw_graph_normal (base, cr, w, h);
            break;
        case 1:
            draw_graph_LED (base, cr, w, h);
            break;
        case 2:
            draw_graph_no_history (base, cr, w, h);
            break;
        case 3:
            draw_graph_grid (base, cr, w, h);
            break;
    }
}

static void
draw_bars_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    CPUGraph *const base = (CPUGraph *) data;
    GtkAllocation alloc;
    gdouble size;
    const gboolean horizontal = (base->bars.orientation == GTK_ORIENTATION_HORIZONTAL);

    gtk_widget_get_allocation (base->bars.draw_area, &alloc);

    gdk_cairo_set_source_rgba (cr, &base->colors[0]);
    cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
    cairo_fill (cr);

    gdk_cairo_set_source_rgba (cr, &base->colors[4]);

    size = (horizontal ? alloc.height : alloc.width);
    if (base->tracked_core != 0 || base->nr_cores == 1)
    {
        gdouble usage = size * base->cpu_data[0].load / CPU_SCALE;
        if (horizontal)
            cairo_rectangle (cr, 0, size-usage, 4, usage);
        else
            cairo_rectangle (cr, 0, 0, usage, 4);
        cairo_fill (cr);
    }
    else
    {
        guint i;
        for (i = 0; i < base->nr_cores; i++)
        {
            gdouble usage = size * base->cpu_data[i+1].load / CPU_SCALE;
            if (horizontal)
                cairo_rectangle (cr, 6*i, size-usage, 4, usage);
            else
                cairo_rectangle (cr, 0, 6*i, usage, 4);
            cairo_fill (cr);
        }
    }
}

static gboolean
command_cb (GtkWidget *w, GdkEventButton *event, CPUGraph *base)
{
    if (event->button == 1 && base->command)
    {
        xfce_spawn_command_line_on_screen (gdk_screen_get_default (),
                                           base->command, base->in_terminal,
                                           base->startup_notification, NULL);
    }
    return FALSE;
}

void
set_startup_notification (CPUGraph *base, gboolean startup_notification)
{
    base->startup_notification = startup_notification;
}

void
set_in_terminal (CPUGraph *base, gboolean in_terminal)
{
    base->in_terminal = in_terminal;
}

void
set_command (CPUGraph *base, const gchar *command)
{
    g_free (base->command);
    base->command = g_strdup (command);
}

void
set_bars (CPUGraph *base, gboolean bars)
{
    if (base->has_bars != bars)
    {
        base->has_bars = bars;
        if (bars)
        {
            create_bars (base, xfce_panel_plugin_get_orientation (base->plugin));
            set_bars_size (base);
        }
        else
            delete_bars (base);
    }
}

void
set_border (CPUGraph *base, gboolean border)
{
    int border_width = (xfce_panel_plugin_get_size (base->plugin) > 26 ? 2 : 1);
    base->has_border = border;
    if (!base->has_border)
        border_width = 0;
    gtk_container_set_border_width (GTK_CONTAINER (base->box), border_width);
}

void
set_frame (CPUGraph *base, gboolean frame)
{
    base->has_frame = frame;
    gtk_frame_set_shadow_type (GTK_FRAME (base->frame_widget), base->has_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    if (base->bars.frame)
        gtk_frame_set_shadow_type (GTK_FRAME (base->bars.frame), base->has_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

void
set_nonlinear_time (CPUGraph *base, gboolean nonlinear)
{
    base->non_linear = nonlinear;
}

void
set_update_rate (CPUGraph *base, guint rate)
{
    guint update;

    base->update_interval = rate;

    if (base->timeout_id)
        g_source_remove (base->timeout_id);

    switch (base->update_interval)
    {
        case 0:
            update = 250;
            break;
        case 1:
            update = 500;
            break;
        case 2:
            update = 750;
            break;
        case 3:
            update = 1000;
            break;
        default:
            update = 3000;
    }
    base->timeout_id = g_timeout_add (update, (GSourceFunc) update_cb, base);
}

void
set_size (CPUGraph *base, guint size)
{
    base->size = size;
    size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

void
set_color_mode (CPUGraph *base, guint color_mode)
{
    base->color_mode = color_mode;
}

void
set_mode (CPUGraph *base, gint mode)
{
    base->mode = mode;

    if (mode == -1)
    {
        /* 'Disabled' mode, hide graph and clear history */
        gtk_widget_hide (base->frame_widget);
        for (gint i = 0; i < base->history_size; i++)
            base->history[i] = 0;
    }
    else
    {
        gtk_widget_show (base->frame_widget);
    }
}

void
set_color (CPUGraph *base, guint number, GdkRGBA color)
{
    base->colors[number] = color;

    if (number == 0)
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_widget_override_background_color (base->draw_area, GTK_STATE_FLAG_INSENSITIVE, &base->colors[0]);
        gtk_widget_override_background_color (base->draw_area, GTK_STATE_FLAG_NORMAL, &base->colors[0]);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
}

void
set_tracked_core (CPUGraph *base, guint core)
{
    gboolean has_bars = base->has_bars;
    if (has_bars)
        set_bars (base, FALSE);
    base->tracked_core = core;
    if (has_bars)
        set_bars (base, TRUE);
}
