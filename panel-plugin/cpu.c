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
#include <math.h>
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

    /* Read CPU data twice in order to initialize
     * cpu_data[].previous_used and cpu_data[].previous_total
     * with the current HWMs. HWM = High Water Mark. */
    read_cpu_data (base->cpu_data, base->nr_cores);
    read_cpu_data (base->cpu_data, base->nr_cores);

    base->topology = read_topology ();

    base->plugin = plugin;

    base->ebox = ebox = gtk_event_box_new ();
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
    base->highlight_smt = HIGHLIGHT_SMT_BY_DEFAULT;

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
    icon = xfce_panel_pixbuf_from_source ("xfce4-cpugraph-plugin", NULL, 48);
    gtk_show_about_dialog (NULL,
        "logo", icon,
        "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
        "version", PACKAGE_VERSION,
        "program-name", PACKAGE_NAME,
        "comments", _("Graphical representation of the CPU load"),
        "website", "https://docs.xfce.org/panel-plugins/xfce4-cpugraph-plugin",
        "copyright", _("Copyright (c) 2003-2021\n"),
        "authors", auth, NULL);

    if (icon)
        g_object_unref (G_OBJECT (icon));
}

static void
ebox_revalidate (CPUGraph *base)
{
    gtk_event_box_set_above_child (GTK_EVENT_BOX (base->ebox), FALSE);
    gtk_event_box_set_above_child (GTK_EVENT_BOX (base->ebox), TRUE);
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
    set_frame (base, base->has_frame);
    gtk_container_add (GTK_CONTAINER (base->bars.frame), base->bars.draw_area);
    gtk_box_pack_end (GTK_BOX (base->box), base->bars.frame, TRUE, TRUE, 0);
    g_signal_connect_after (base->bars.draw_area, "draw", G_CALLBACK (draw_bars_cb), base);
    gtk_widget_show_all (base->bars.frame);
    ebox_revalidate (base);
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
    g_free (base->topology);
    delete_bars (base);
    gtk_widget_destroy (base->ebox);
    gtk_widget_destroy (base->tooltip_text);
    if (base->timeout_id)
        g_source_remove (base->timeout_id);
    g_free (base->history.data);
    g_free (base->command);
    g_free (base);
}

static void
queue_draw (CPUGraph *base)
{
    if (base->mode != MODE_DISABLED)
        gtk_widget_queue_draw (base->draw_area);
    if (base->bars.draw_area)
        gtk_widget_queue_draw (base->bars.draw_area);
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

static void
clear_history (CPUGraph *base)
{
    gssize i;

    for (i = 0; i < base->history.cap_pow2; i++)
        base->history.data[i] = (CpuLoad) {};

    queue_draw (base);
}

static void
resize_history (CPUGraph *base, gssize history_size)
{
    const guint fastest = get_update_interval_ms (RATE_FASTEST);
    const guint slowest = get_update_interval_ms (RATE_SLOWEST);
    gssize cap_pow2, old_cap_pow2, old_mask, old_offset;
    CpuLoad *old_data;

    old_cap_pow2 = base->history.cap_pow2;
    old_data = base->history.data;
    old_mask = base->history.mask;
    old_offset = base->history.offset;

    cap_pow2 = 1;
    while (cap_pow2 < MAX_SIZE * slowest / fastest)
        cap_pow2 <<= 1;
    while (cap_pow2 < history_size * slowest / fastest)
        cap_pow2 <<= 1;

    if (cap_pow2 != old_cap_pow2)
    {
        gssize i;
        base->history.cap_pow2 = cap_pow2;
        base->history.data = (CpuLoad*) g_malloc0 (cap_pow2 * sizeof (CpuLoad));
        base->history.mask = cap_pow2 - 1;
        base->history.offset = 0;
        if (old_data != NULL)
            for (i = 0; i < old_cap_pow2 && i < cap_pow2; i++)
                base->history.data[i] = old_data[(old_offset + i) & old_mask];
        g_free (old_data);
    }

    base->history.size = history_size;
}

static gboolean
size_cb (XfcePanelPlugin *plugin, guint size, CPUGraph *base)
{
    gint frame_h, frame_v;
    gssize history;
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

    /* Expand history size for the non-linear time-scale mode.
     *   128 * pow(1.04, 128) = 19385.5175366781
     *   163 * pow(1.04, 163) = 97414.11965601446
     */
    history = ceil (history * pow(NONLINEAR_MODE_BASE, history));
    if (G_UNLIKELY (history < 0 || history > MAX_HISTORY_SIZE))
        history = MAX_HISTORY_SIZE;

    if (history > base->history.cap_pow2)
        resize_history (base, history);
    else
        base->history.size = history;

    gtk_widget_set_size_request (GTK_WIDGET (base->frame_widget), frame_h, frame_v);
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

static void
detect_smt_issues (CPUGraph *base)
{
    const gboolean debug = FALSE;
    gfloat actual_load[base->nr_cores];
    gboolean movement[base->nr_cores];
    gboolean suboptimal[base->nr_cores];
    guint i;

    for (i = 0; i < base->nr_cores; i++)
    {
        actual_load[i] = base->cpu_data[i+1].load;
        suboptimal[i] = FALSE;
        movement[i] = FALSE;
        if (debug)
            g_info ("actual_load[%u] = %g", i, actual_load[i]);
    }

    if (base->topology && base->topology->smt)
    {
        Topology *const topo = base->topology;
        gfloat optimal_load[base->nr_cores];
        gfloat actual_num_instr_executed[base->nr_cores];
        gfloat optimal_num_instr_executed[base->nr_cores];
        gboolean smt_incident = FALSE;

        /* Initialize CPU load arrays.
         * The array optimal_load[] will be updated
         * if a suboptimal SMT thread placement is detected. */
        for (i = 0; i < base->nr_cores; i++)
        {
            const gfloat load = actual_load[i];
            optimal_load[i] = load;
            actual_num_instr_executed[i] = load;
            optimal_num_instr_executed[i] = load;
        }

        for (i = 0; i < base->nr_cores; i++)
        {
            if (G_LIKELY (i < topo->num_all_logical_cpus))
            {
                const gint core = topo->logical_cpu_2_core[i];
                if (G_LIKELY (core != -1) && topo->cores[core].num_logical_cpus >= 2)
                {
                    const gfloat THRESHOLD = 1.0 + 0.1;       /* A lower bound (this core) */
                    const gfloat THRESHOLD_OTHER = 1.0 - 0.1; /* An upper bound (some other core) */

                    /* _Approximate_ slowdown if two threads
                     * are executed on the same physical core
                     * instead of being executed on separate cores.
                     * This number has been determined by measuring
                     * the slowdown of running two instances of
                     * "stress-ng --cpu=1" on a Ryzen 3700X CPU. */
                    const gfloat SMT_SLOWDOWN = 0.25f;

                    gfloat combined_usage;
                    guint j;

                retry:
                    combined_usage = 0;
                    for (j = 0; j < topo->cores[core].num_logical_cpus; j++)
                    {
                        guint cpu = topo->cores[core].logical_cpus[j];
                        if (G_LIKELY (cpu < base->nr_cores))
                            combined_usage += optimal_load[cpu];
                    }
                    if (combined_usage > THRESHOLD)
                    {
                        /* Attempt to find a free CPU *core* different from `core`
                         * that might have had executed the workload
                         * without resorting to SMT/hyperthreading */
                        guint other_core;
                        for (other_core = 0; other_core < topo->num_all_cores; other_core++)
                        {
                            if (other_core != (guint) core)
                            {
                                gfloat combined_usage_other = 0.0;
                                for (j = 0; j < topo->cores[other_core].num_logical_cpus; j++)
                                {
                                    guint other_cpu = topo->cores[other_core].logical_cpus[j];
                                    if (G_LIKELY (other_cpu < base->nr_cores))
                                        combined_usage_other += optimal_load[other_cpu];
                                }
                                if (combined_usage_other < THRESHOLD_OTHER)
                                {
                                    /* The thread might have been executed on 'other_core',
                                     * instead of on 'core', where it might have enjoyed
                                     * a much higher IPC (instructions per clock) ratio */

                                    smt_incident = TRUE;
                                    for (j = 0; j < topo->cores[other_core].num_logical_cpus; j++)
                                    {
                                        guint cpu = topo->cores[core].logical_cpus[j];
                                        if (G_LIKELY (cpu < base->nr_cores))
                                            suboptimal[cpu] = TRUE;
                                    }

                                    /*
                                     * 1.001 and 0.999 are used instead of 1.0 to make sure that:
                                     *  - the algorithm always terminates
                                     *  - the algorithm terminates quickly
                                     *  - it skips unimportant differences such as 1e-5
                                     */

                                    if (G_LIKELY (combined_usage > 1.001f))
                                    {
                                        /* Move as much of excess load to the other core as possible */
                                        const gfloat excess_load = combined_usage - 1.0f;
                                        gint other_cpu_min = -1;
                                        for (j = 0; j < topo->cores[other_core].num_logical_cpus; j++)
                                        {
                                            guint other_cpu = topo->cores[other_core].logical_cpus[j];
                                            if (G_LIKELY (other_cpu < base->nr_cores))
                                                if (optimal_load[other_cpu] < 0.999f)
                                                    if (other_cpu_min == -1 || optimal_load[other_cpu_min] > optimal_load[other_cpu])
                                                        other_cpu_min = other_cpu;
                                        }
                                        if (G_LIKELY (other_cpu_min != -1))
                                        {
                                            gfloat load_to_move;

                                            load_to_move = excess_load;
                                            if (load_to_move > 1.0f - optimal_load[other_cpu_min])
                                                load_to_move = 1.0f - optimal_load[other_cpu_min];

                                            if (debug)
                                                g_info ("load_to_move = %g", load_to_move);

                                            optimal_load[other_cpu_min] += load_to_move;

                                            /* The move negates the SMT slowdown for the work moved onto the underutilized target CPU core */
                                            movement[other_cpu_min] = TRUE;
                                            optimal_num_instr_executed[other_cpu_min] += (1.0f + SMT_SLOWDOWN) * load_to_move;

                                            /* Decrease combined_usage by load_to_move */
                                            for (j = topo->cores[core].num_logical_cpus; load_to_move > 0 && j != 0;)
                                            {
                                                guint cpu = topo->cores[core].logical_cpus[--j];
                                                if (G_LIKELY (cpu < base->nr_cores))
                                                {
                                                    if (optimal_load[cpu] >= load_to_move)
                                                    {
                                                        const gfloat diff = load_to_move;

                                                        optimal_load[cpu] -= diff;
                                                        load_to_move = 0;

                                                        /* The move negates the SMT slowdown for the work remaining on the original CPU core */
                                                        optimal_num_instr_executed[cpu] -= 1.0f * diff;         /* Moved work */
                                                        optimal_num_instr_executed[cpu] += SMT_SLOWDOWN * diff; /* Remaining work (speedup) */
                                                        movement[cpu] = TRUE;
                                                    }
                                                    else
                                                    {
                                                        const gfloat diff = optimal_load[cpu];

                                                        optimal_load[cpu] = 0;
                                                        load_to_move -= diff;

                                                        /* The move negates the SMT slowdown for the work remaining on the original CPU core */
                                                        optimal_num_instr_executed[cpu] -= 1.0f * diff;         /* Moved work */
                                                        optimal_num_instr_executed[cpu] += SMT_SLOWDOWN * diff; /* Remaining work (speedup) */
                                                        movement[cpu] = TRUE;
                                                    }
                                                }
                                            }

                                            /* At this point: load_to_move should be zero or very close to zero */
                                            g_warn_if_fail (load_to_move < 0.001f);

                                            goto retry;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /* Update instruction counters */
        for (i = 0; i < base->nr_cores; i++)
        {
            base->stats.num_instructions_executed.total.actual += actual_num_instr_executed[i];
            base->stats.num_instructions_executed.total.optimal += optimal_num_instr_executed[i];
        }

        /* Suboptimal SMT scheduling cases are actually quite rare (at least in Linux):
         * - They are impossible to happen if the CPU is under full load
         * - They are rare if the CPU is running one single-threaded task
         * - They tend to occur especially when the CPU is running about nr_cores/2 threads
         */
        if (G_UNLIKELY (smt_incident))
        {
            base->stats.num_smt_incidents++;

            /* Update instruction counters */
            for (i = 0; i < base->nr_cores; i++)
            {
                if (movement[i] || suboptimal[i])
                {
                    base->stats.num_instructions_executed.during_smt_incidents.actual += actual_num_instr_executed[i];
                    base->stats.num_instructions_executed.during_smt_incidents.optimal += optimal_num_instr_executed[i];
                }
            }
        }
    }

    for (i = 0; i < base->nr_cores; i++)
        base->cpu_data[i+1].smt_highlight = suboptimal[i];
}

static gboolean
update_cb (gpointer user_data)
{
    CPUGraph *base = user_data;

    if (!read_cpu_data (base->cpu_data, base->nr_cores))
        return TRUE;

    detect_smt_issues (base);

    if (base->tracked_core > base->nr_cores)
        base->cpu_data[0].load = 0;
    else if (base->tracked_core != 0 && G_LIKELY (base->tracked_core < base->nr_cores + 1))
        base->cpu_data[0].load = base->cpu_data[base->tracked_core].load;

    if (base->history.data != NULL)
    {
        CpuLoad load;

        /* Prepend a datapoint to the history */
        base->history.offset = (base->history.offset - 1) & base->history.mask;
        load.timestamp = g_get_real_time ();
        load.value = base->cpu_data[0].load;
        base->history.data[base->history.offset] = load;
    }

    queue_draw (base);
    update_tooltip (base);

    return TRUE;
}

static void
update_tooltip (CPUGraph *base)
{
    gchar tooltip[32];
    g_snprintf (tooltip, 32, _("Usage: %u%%"), (guint) roundf (base->cpu_data[0].load * 100));
    if (strcmp (gtk_label_get_text (GTK_LABEL (base->tooltip_text)), tooltip))
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

    if (base->colors[BG_COLOR].alpha != 0)
    {
        gdk_cairo_set_source_rgba (cr, &base->colors[BG_COLOR]);
        cairo_rectangle (cr, 0, 0, w, h);
        cairo_fill (cr);
    }

    switch (base->mode)
    {
        case MODE_DISABLED:
            break;
        case MODE_NORMAL:
            draw_graph_normal (base, cr, w, h);
            break;
        case MODE_LED:
            draw_graph_LED (base, cr, w, h);
            break;
        case MODE_NO_HISTORY:
            draw_graph_no_history (base, cr, w, h);
            break;
        case MODE_GRID:
            draw_graph_grid (base, cr, w, h);
            break;
    }
}

static void
draw_bars_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    CPUGraph *const base = (CPUGraph *) data;
    GtkAllocation alloc;
    gfloat size;
    const gboolean horizontal = (base->bars.orientation == GTK_ORIENTATION_HORIZONTAL);

    gtk_widget_get_allocation (base->bars.draw_area, &alloc);

    if (base->colors[BG_COLOR].alpha != 0)
    {
        gdk_cairo_set_source_rgba (cr, &base->colors[BG_COLOR]);
        cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
        cairo_fill (cr);
    }

    size = (horizontal ? alloc.height : alloc.width);
    if (base->tracked_core != 0 || base->nr_cores == 1)
    {
        gfloat usage = base->cpu_data[0].load;
        if (usage < base->load_threshold)
            usage = 0;
        usage *= size;

        gdk_cairo_set_source_rgba (cr, &base->colors[BARS_COLOR]);
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
            const gboolean highlight = base->highlight_smt && base->cpu_data[i+1].smt_highlight;
            gfloat usage;

            usage = base->cpu_data[i+1].load;
            if (usage < base->load_threshold)
                usage = 0;
            usage *= size;

            /* Suboptimally placed threads on SMT CPUs are optionally painted using a different color. */
            gdk_cairo_set_source_rgba (cr, &base->colors[highlight ? SMT_ISSUES_COLOR : BARS_COLOR]);

            if (horizontal)
                cairo_rectangle (cr, 6*i, size-usage, 4, usage);
            else
                cairo_rectangle (cr, 0, 6*i, usage, 4);
            cairo_fill (cr);
        }
    }
}

static const gchar*
default_command (gboolean *in_terminal, gboolean *startup_notification)
{
    gchar *s = g_find_program_in_path ("xfce4-taskmanager");
    if (s)
    {
        g_free (s);
        *in_terminal = FALSE;
        *startup_notification = TRUE;
        return "xfce4-taskmanager";
    }
    else
    {
        *in_terminal = TRUE;
        *startup_notification = FALSE;

        s = g_find_program_in_path ("htop");
        if (s)
        {
            g_free (s);
            return "htop";
        }
        else
        {
            return "top";
        }
    }
}

static gboolean
command_cb (GtkWidget *w, GdkEventButton *event, CPUGraph *base)
{
    if (event->button == 1)
    {
        const gchar *command;
        gboolean in_terminal, startup_notification;

        if (base->command)
        {
            command = base->command;
            in_terminal = base->in_terminal;
            startup_notification = base->startup_notification;
        }
        else
        {
            command = default_command (&in_terminal, &startup_notification);
        }

        xfce_spawn_command_line_on_screen (gdk_screen_get_default (),
                                           command, in_terminal,
                                           startup_notification, NULL);
    }
    return FALSE;
}

/**
 * get_update_interval_ms:
 *
 * Returns: update interval in milliseconds.
 */
guint
get_update_interval_ms (CPUGraphUpdateRate rate)
{
    switch (rate)
    {
        case RATE_FASTEST:
            return 250;
        case RATE_FAST:
            return 500;
        case RATE_NORMAL:
            return 750;
        case RATE_SLOW:
            return 1000;
        case RATE_SLOWEST:
            return 3000;
        default:
            return 750;
    }
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
    g_strstrip (base->command);
    if (strlen (base->command) == 0)
    {
        g_free (base->command);
        base->command = NULL;
    }
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
    size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

void
set_nonlinear_time (CPUGraph *base, gboolean nonlinear)
{
    if (base->non_linear != nonlinear)
    {
        base->non_linear = nonlinear;
        queue_draw (base);
    }
}

void
set_smt (CPUGraph *base, gboolean highlight_smt)
{
    base->highlight_smt = highlight_smt;
}

void
set_update_rate (CPUGraph *base, CPUGraphUpdateRate rate)
{
    gboolean change = (base->update_interval != rate);
    gboolean init = (base->timeout_id == 0);

    if (change || init)
    {
        guint interval = get_update_interval_ms (rate);

        base->update_interval = rate;
        if (base->timeout_id)
            g_source_remove (base->timeout_id);
        base->timeout_id = g_timeout_add (interval, update_cb, base);

        if (change && !init)
            queue_draw (base);
    }
}

void
set_size (CPUGraph *base, guint size)
{
    if (G_UNLIKELY (size < MIN_SIZE))
        size = MIN_SIZE;
    if (G_UNLIKELY (size > MAX_SIZE))
        size = MAX_SIZE;

    base->size = size;
    size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

void
set_color_mode (CPUGraph *base, guint color_mode)
{
    if (base->color_mode != color_mode)
    {
        base->color_mode = color_mode;
        queue_draw (base);
    }
}

void
set_mode (CPUGraph *base, CPUGraphMode mode)
{
    base->mode = mode;

    if (mode == MODE_DISABLED)
    {
        gtk_widget_hide (base->frame_widget);
    }
    else
    {
        gtk_widget_show (base->frame_widget);
        ebox_revalidate (base);
    }
}

void
set_color (CPUGraph *base, guint number, GdkRGBA color)
{
    if (!gdk_rgba_equal (&base->colors[number], &color))
    {
        base->colors[number] = color;
        queue_draw (base);
    }
}

void
set_tracked_core (CPUGraph *base, guint core)
{
    if (base->tracked_core != core)
    {
        gboolean has_bars = base->has_bars;
        if (has_bars)
            set_bars (base, FALSE);
        base->tracked_core = core;
        if (has_bars)
            set_bars (base, TRUE);

        clear_history (base);
    }
}

void
set_load_threshold (CPUGraph *base, gfloat threshold)
{
    if (threshold < 0)
        threshold = 0;
    if (threshold > MAX_LOAD_THRESHOLD)
        threshold = MAX_LOAD_THRESHOLD;
    base->load_threshold = threshold;
}
