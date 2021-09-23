/*  cpu.cc
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

#include "cpu.h"
#include "settings.h"
#include "mode.h"
#include "plugin.h"
#include "properties.h"

#include <libxfce4ui/libxfce4ui.h>
#include <math.h>
#include "xfce4++/util.h"

using xfce4::PluginSize;
using xfce4::Propagation;
using xfce4::TooltipTime;

/* vim: !sort -k3 */
static void          about_cb       ();
static Propagation   command_cb     (GdkEventButton *event, const Ptr<CPUGraph> &base);
static void          create_bars    (const Ptr<CPUGraph> &base, GtkOrientation orientation);
static Ptr<CPUGraph> create_gui     (XfcePanelPlugin *plugin);
static void          delete_bars    (const Ptr<CPUGraph> &base);
static Propagation   draw_area_cb   (cairo_t *cr, const Ptr<CPUGraph> &base);
static Propagation   draw_bars_cb   (cairo_t *cr, const Ptr<CPUGraph> &base);
static void          mode_cb        (XfcePanelPlugin *plugin, const Ptr<CPUGraph> &base);
static guint         nb_bars        (const Ptr<const CPUGraph> &base);
static void          set_bars_size  (const Ptr<CPUGraph> &base);
static void          shutdown       (const Ptr<CPUGraph> &base);
static PluginSize    size_cb        (XfcePanelPlugin *plugin, guint size, const Ptr<CPUGraph> &base);
static TooltipTime   tooltip_cb     (GtkTooltip *tooltip, const Ptr<CPUGraph> &base);
static void          update_tooltip (const Ptr<CPUGraph> &base);

void
cpugraph_construct (XfcePanelPlugin *plugin)
{
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    Ptr<CPUGraph> base = create_gui (plugin);

    read_settings (plugin, base);

    xfce_panel_plugin_menu_show_about (plugin);
    xfce_panel_plugin_menu_show_configure (plugin);

    xfce4::connect_about           (plugin, [base](XfcePanelPlugin *p) { about_cb(); });
    xfce4::connect_free_data       (plugin, [base](XfcePanelPlugin *p) { shutdown(base); });
    xfce4::connect_save            (plugin, [base](XfcePanelPlugin *p) { write_settings(p, base); });
    xfce4::connect_configure_plugin(plugin, [base](XfcePanelPlugin *p) { create_options(p, base); });
    xfce4::connect_mode_changed    (plugin, [base](XfcePanelPlugin *p, XfcePanelPluginMode mode) { mode_cb(p, base); });
    xfce4::connect_size_changed    (plugin, [base](XfcePanelPlugin *p, guint size) { return size_cb(p, size, base); });
}

static guint
init_cpu_data (std::vector<CpuData> &data)
{
    guint cpuNr = detect_cpu_number ();
    if (cpuNr != 0)
        data.resize(cpuNr+1);
    return cpuNr;
}

static Ptr<CPUGraph>
create_gui (XfcePanelPlugin *plugin)
{
    GtkWidget *frame, *ebox;
    GtkOrientation orientation;
    auto base = xfce4::make<CPUGraph>();

    orientation = xfce_panel_plugin_get_orientation (plugin);
    if ((base->nr_cores = init_cpu_data (base->cpu_data)) == 0)
        fprintf (stderr,"Cannot init cpu data !\n");

    /* Read CPU data twice in order to initialize
     * cpu_data[].previous_used and cpu_data[].previous_total
     * with the current HWMs. HWM = High Water Mark. */
    read_cpu_data (base->cpu_data);
    read_cpu_data (base->cpu_data);

    base->topology = read_topology ();

    base->plugin = plugin;

    base->ebox = ebox = gtk_event_box_new ();
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (ebox), FALSE);
    gtk_event_box_set_above_child (GTK_EVENT_BOX (ebox), TRUE);
    gtk_container_add (GTK_CONTAINER (plugin), ebox);
    xfce_panel_plugin_add_action_widget (plugin, ebox);
    xfce4::connect_button_press (ebox, [base](GtkWidget*, GdkEventButton *event) -> Propagation {
        return command_cb (event, base);
    });

    base->box = gtk_box_new (orientation, 0);
    gtk_container_add (GTK_CONTAINER (ebox), base->box);
    gtk_widget_set_has_tooltip (base->box, TRUE);
    xfce4::connect_query_tooltip (base->box, [base](GtkWidget *widget, gint x, gint y, bool keyboard, GtkTooltip *tooltip) {
        return tooltip_cb (tooltip, base);
    });

    base->frame_widget = frame = gtk_frame_new (NULL);
    gtk_box_pack_end (GTK_BOX (base->box), frame, TRUE, TRUE, 2);

    base->draw_area = gtk_drawing_area_new ();
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (base->draw_area));
    xfce4::connect_after_draw (base->draw_area, [base](cairo_t *cr) { return draw_area_cb (cr, base); });

    base->has_bars = false;
    base->has_barcolor = false;
    base->bars.orientation = orientation;
    base->highlight_smt = HIGHLIGHT_SMT_BY_DEFAULT;
    base->per_core_spacing = PER_CORE_SPACING_DEFAULT;

    mode_cb (plugin, base);
    gtk_widget_show_all (ebox);

    base->tooltip_text = gtk_label_new (NULL);
    g_object_ref (base->tooltip_text);

    return base;
}

static void
about_cb ()
{
    /* List of authors (in alphabetical order) */
    const gchar *auth[] = {
        "Agustin Ferrin Pozuelo <gatoguan-os@yahoo.com>",
        "Alexander Nordfelth <alex.nordfelth@telia.com>",
        "Angelo Miguel Arrifano <miknix@gmail.com>",
        "Florian Rivoal <frivoal@gmail.com>",
        "Jan Ziak <0xe2.0x9a.0x9b@xfce.org>",
        "Ludovic Mercier <lidiriel@coriolys.org>",
        "Peter Tribble <peter.tribble@gmail.com>",
        NULL
    };

    gtk_show_about_dialog (NULL,
        "logo-icon-name", "org.xfce.panel.cpugraph",
        "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
        "version", PACKAGE_VERSION,
        "program-name", PACKAGE_NAME,
        "comments", _("Graphical representation of the CPU load"),
        "website", "https://docs.xfce.org/panel-plugins/xfce4-cpugraph-plugin",
        "copyright", _("Copyright (c) 2003-2021\n"),
        "authors", auth, NULL);
}

static void
ebox_revalidate (const Ptr<CPUGraph> &base)
{
    gtk_event_box_set_above_child (GTK_EVENT_BOX (base->ebox), FALSE);
    gtk_event_box_set_above_child (GTK_EVENT_BOX (base->ebox), TRUE);
}

static guint
nb_bars (const Ptr<const CPUGraph> &base)
{
    return base->tracked_core == 0 ? base->nr_cores : 1;
}

static void
create_bars (const Ptr<CPUGraph> &base, GtkOrientation orientation)
{
    base->bars.frame = gtk_frame_new (NULL);
    base->bars.draw_area = gtk_drawing_area_new ();
    base->bars.orientation = orientation;
    CPUGraph::set_frame (base, base->has_frame);
    gtk_container_add (GTK_CONTAINER (base->bars.frame), base->bars.draw_area);
    gtk_box_pack_end (GTK_BOX (base->box), base->bars.frame, TRUE, TRUE, 0);
    xfce4::connect_after_draw (base->bars.draw_area, [base](cairo_t *cr) { return draw_bars_cb(cr, base); });
    gtk_widget_show_all (base->bars.frame);
    ebox_revalidate (base);
}

CPUGraph::~CPUGraph()
{
    g_info ("%s", __PRETTY_FUNCTION__);
    for (auto hist_data : history.data)
        g_free (hist_data);
}

static void
shutdown (const Ptr<CPUGraph> &base)
{
    delete_bars (base);
    gtk_widget_destroy (base->ebox);
    base->ebox = NULL;
    g_object_unref (base->tooltip_text);
    base->tooltip_text = NULL;
    if (base->timeout_id)
    {
        g_source_remove (base->timeout_id);
        base->timeout_id = 0;
    }
}

static void
queue_draw (const Ptr<CPUGraph> &base)
{
    if (base->mode != MODE_DISABLED)
        gtk_widget_queue_draw (base->draw_area);
    if (base->bars.draw_area)
        gtk_widget_queue_draw (base->bars.draw_area);
}

static void
delete_bars (const Ptr<CPUGraph> &base)
{
    if (base->bars.frame)
    {
        gtk_widget_destroy (base->bars.frame);
        base->bars.frame = NULL;
        base->bars.draw_area = NULL;
    }
}

static void
resize_history (const Ptr<CPUGraph> &base, gssize history_size)
{
    const guint fastest = get_update_interval_ms (RATE_FASTEST);
    const guint slowest = get_update_interval_ms (RATE_SLOWEST);
    const gssize old_cap_pow2 = base->history.cap_pow2;

    gssize cap_pow2 = 1;
    while (cap_pow2 < MAX_SIZE * slowest / fastest)
        cap_pow2 <<= 1;
    while (cap_pow2 < history_size * slowest / fastest)
        cap_pow2 <<= 1;

    if (cap_pow2 != old_cap_pow2)
    {
        const std::vector<CpuLoad*> old_data = std::move(base->history.data);
        const gssize old_mask = base->history.mask;
        const gssize old_offset = base->history.offset;

        base->history.cap_pow2 = cap_pow2;
        base->history.data.resize(base->nr_cores + 1);
        for (guint core = 0; core < base->nr_cores + 1; core++)
            base->history.data[core] = (CpuLoad*) g_malloc0 (cap_pow2 * sizeof (CpuLoad));
        base->history.mask = cap_pow2 - 1;
        base->history.offset = 0;
        if (!old_data.empty())
        {
            for (guint core = 0; core < base->nr_cores + 1; core++)
            {
                for (gssize i = 0; i < old_cap_pow2 && i < cap_pow2; i++)
                    base->history.data[core][i] = old_data[core][(old_offset + i) & old_mask];
                g_free (old_data[core]);
            }
        }
    }

    base->history.size = history_size;
}

static PluginSize
size_cb (XfcePanelPlugin *plugin, guint plugin_size, const Ptr<CPUGraph> &base)
{
    gint frame_h, frame_v, size;
    gssize history;
    GtkOrientation orientation;
    guint border_width;
    const gint shadow_width = base->has_frame ? 2*1 : 0;

    size = base->size;
    if (base->per_core && base->nr_cores >= 2)
    {
        size *= base->nr_cores;
        size += (base->nr_cores - 1) * base->per_core_spacing;
    }

    orientation = xfce_panel_plugin_get_orientation (plugin);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        frame_h = size + shadow_width;
        frame_v = plugin_size;
        history = base->size;
    }
    else
    {
        frame_h = plugin_size;
        frame_v = size + shadow_width;
        history = plugin_size;
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

    if (base->has_border)
        border_width = (xfce_panel_plugin_get_size (base->plugin) > 26 ? 2 : 1);
    else
        border_width = 0;
    gtk_container_set_border_width (GTK_CONTAINER (base->box), border_width);

    base->set_border (base, base->has_border);

    return xfce4::RECTANGLE;
}

static void
set_bars_size (const Ptr<CPUGraph> &base)
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
mode_cb (XfcePanelPlugin *plugin, const Ptr<CPUGraph> &base)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (base->box), xfce_panel_plugin_get_orientation (plugin));

    size_cb (plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

static void
detect_smt_issues (const Ptr<CPUGraph> &base)
{
    const bool debug = false;
    gfloat actual_load[base->nr_cores];
    bool movement[base->nr_cores];
    bool suboptimal[base->nr_cores];

    for (guint i = 0; i < base->nr_cores; i++)
    {
        actual_load[i] = base->cpu_data[i+1].load;
        suboptimal[i] = false;
        movement[i] = false;
        if (debug)
            g_info ("actual_load[%u] = %g", i, actual_load[i]);
    }

    if (base->topology && base->topology->smt)
    {
        /* Use <Topology> instead of <const Topology>.
         * The non-const version results in less efficient C++ code,
         * but it is less prone to generate an exception or a crash
         * than the const version due to an unforseen programming bug. */
        const Ptr0<Topology> topo = base->topology;

        gfloat optimal_load[base->nr_cores];
        gfloat actual_num_instr_executed[base->nr_cores];
        gfloat optimal_num_instr_executed[base->nr_cores];
        bool smt_incident = false;

        /* Initialize CPU load arrays.
         * The array optimal_load[] will be updated
         * if a suboptimal SMT thread placement is detected. */
        for (guint i = 0; i < base->nr_cores; i++)
        {
            const gfloat load = actual_load[i];
            optimal_load[i] = load;
            actual_num_instr_executed[i] = load;
            optimal_num_instr_executed[i] = load;
        }

        const gfloat THRESHOLD = 1.0 + 0.1;       /* A lower bound (this core) */
        const gfloat THRESHOLD_OTHER = 1.0 - 0.1; /* An upper bound (some other core) */

        for (guint i = 0; i < base->nr_cores; i++)
        {
            if (G_LIKELY (i < topo->num_logical_cpus))
            {
                const gint core = topo->logical_cpu_2_core[i];
                if (G_LIKELY (core != -1) && topo->cores[core].logical_cpus.size() >= 2)
                {
                    /* _Approximate_ slowdown if two threads
                     * are executed on the same physical core
                     * instead of being executed on separate cores.
                     * This number has been determined by measuring
                     * the slowdown of running two instances of
                     * "stress-ng --cpu=1" on a Ryzen 3700X CPU. */
                    const gfloat SMT_SLOWDOWN = 0.25f;

                retry:
                    gfloat combined_usage = 0;
                    for (guint cpu : topo->cores[core].logical_cpus)
                    {
                        if (G_LIKELY (cpu < base->nr_cores))
                            combined_usage += optimal_load[cpu];
                    }
                    if (combined_usage > THRESHOLD)
                    {
                        /* Attempt to find a free CPU *core* different from `core`
                         * that might have had executed the workload
                         * without resorting to SMT/hyperthreading */
                        for (const auto &core_iterator : topo->cores)
                        {
                            guint other_core = core_iterator.first;
                            if (other_core != (guint) core)
                            {
                                gfloat combined_usage_other = 0.0;
                                for (guint other_cpu : topo->cores[other_core].logical_cpus)
                                {
                                    if (G_LIKELY (other_cpu < base->nr_cores))
                                        combined_usage_other += optimal_load[other_cpu];
                                }
                                if (combined_usage_other < THRESHOLD_OTHER)
                                {
                                    /* The thread might have been executed on 'other_core',
                                     * instead of on 'core', where it might have enjoyed
                                     * a much higher IPC (instructions per clock) ratio */

                                    smt_incident = true;
                                    for (guint cpu : topo->cores[core].logical_cpus)
                                    {
                                        if (G_LIKELY (cpu < base->nr_cores))
                                            suboptimal[cpu] = true;
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
                                        for (guint other_cpu : topo->cores[other_core].logical_cpus)
                                        {
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
                                            movement[other_cpu_min] = true;
                                            optimal_num_instr_executed[other_cpu_min] += (1.0f + SMT_SLOWDOWN) * load_to_move;

                                            /* Decrease combined_usage by load_to_move */
                                            for (guint j = topo->cores[core].logical_cpus.size(); load_to_move > 0 && j != 0;)
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
                                                        movement[cpu] = true;
                                                    }
                                                    else
                                                    {
                                                        const gfloat diff = optimal_load[cpu];

                                                        optimal_load[cpu] = 0;
                                                        load_to_move -= diff;

                                                        /* The move negates the SMT slowdown for the work remaining on the original CPU core */
                                                        optimal_num_instr_executed[cpu] -= 1.0f * diff;         /* Moved work */
                                                        optimal_num_instr_executed[cpu] += SMT_SLOWDOWN * diff; /* Remaining work (speedup) */
                                                        movement[cpu] = true;
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
        for (guint i = 0; i < base->nr_cores; i++)
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
            for (guint i = 0; i < base->nr_cores; i++)
            {
                if (movement[i] || suboptimal[i])
                {
                    base->stats.num_instructions_executed.during_smt_incidents.actual += actual_num_instr_executed[i];
                    base->stats.num_instructions_executed.during_smt_incidents.optimal += optimal_num_instr_executed[i];
                }
            }
        }

        /* At this point, the values in suboptimal[] are based on values in optimal_load.
         * This can falsely mark a CPU as suboptimal if the algoritm moved some work to the CPU from other CPUs.
         * Fix false positives in suboptimal[] based on values in actual_load.
         *
         * It is uncertain whether this correction should be performed before or after instruction counter updates.
         */
        for (const auto &core_iterator : topo->cores)
        {
            const Topology::CpuCore &core = core_iterator.second;

            bool positive = false;
            for (guint cpu : core.logical_cpus)
                if (G_LIKELY (cpu < base->nr_cores))
                    positive |= suboptimal[cpu];

            if (positive)
            {
                gfloat actual_combined_usage = 0;
                for (guint cpu : core_iterator.second.logical_cpus)
                    actual_combined_usage += actual_load[cpu];

                bool false_positive = !(actual_combined_usage > THRESHOLD);
                if (false_positive)
                    for (guint cpu : core.logical_cpus)
                        if (G_LIKELY (cpu < base->nr_cores))
                            suboptimal[cpu] = false;
            }
        }
    }

    for (guint i = 0; i < base->nr_cores; i++)
        base->cpu_data[i+1].smt_highlight = suboptimal[i];
}

static bool
update_cb (const Ptr<CPUGraph> &base)
{
    if (!read_cpu_data (base->cpu_data))
        return TRUE;

    detect_smt_issues (base);

    if (!base->history.data.empty())
    {
        const gint64 timestamp = g_get_real_time ();

        /* Prepend the current CPU load to the history */
        base->history.offset = (base->history.offset - 1) & base->history.mask;
        for (guint core = 0; core < base->nr_cores + 1; core++)
        {
            CpuLoad load;
            load.timestamp = timestamp;
            load.value = base->cpu_data[core].load;
            base->history.data[core][base->history.offset] = load;
        }
    }

    queue_draw (base);
    update_tooltip (base);

    return TRUE;
}

static void
update_tooltip (const Ptr<CPUGraph> &base)
{
    auto tooltip = xfce4::sprintf (_("Usage: %u%%"), (guint) roundf (base->cpu_data[0].load * 100));
    if (gtk_label_get_text (GTK_LABEL (base->tooltip_text)) != tooltip)
        gtk_label_set_text (GTK_LABEL (base->tooltip_text), tooltip.c_str());
}

static TooltipTime
tooltip_cb (GtkTooltip *tooltip, const Ptr<CPUGraph> &base)
{
    gtk_tooltip_set_custom (tooltip, base->tooltip_text);
    return xfce4::NOW;
}

static Propagation
draw_area_cb (cairo_t *cr, const Ptr<CPUGraph> &base)
{
    GtkAllocation alloc;
    gint w, h;
    void (*draw) (const Ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core) = NULL;

    gtk_widget_get_allocation (base->draw_area, &alloc);
    w = alloc.width;
    h = alloc.height;

    switch (base->mode)
    {
        case MODE_DISABLED:
            break;
        case MODE_NORMAL:
            draw = draw_graph_normal;
            break;
        case MODE_LED:
            draw = draw_graph_LED;
            break;
        case MODE_NO_HISTORY:
            draw = draw_graph_no_history;
            break;
        case MODE_GRID:
            draw = draw_graph_grid;
            break;
    }

    if (draw)
    {
        if (!base->per_core || base->nr_cores == 1)
        {
            guint core;

            if (!base->colors[BG_COLOR].isTransparent())
            {
                xfce4::cairo_set_source (cr, base->colors[BG_COLOR]);
                cairo_rectangle (cr, 0, 0, w, h);
                cairo_fill (cr);
            }

            core = base->tracked_core;
            if (G_UNLIKELY (core > base->nr_cores + 1))
                core = 0;
            draw (base, cr, w, h, core);
        }
        else
        {
            bool horizontal;
            gint w1, h1;

            horizontal = (xfce_panel_plugin_get_orientation (base->plugin) == GTK_ORIENTATION_HORIZONTAL);
            if (horizontal)
            {
                w1 = base->size;
                h1 = h;
            }
            else
            {
                w1 = w;
                h1 = base->size;
            }

            for (guint core = 0; core < base->nr_cores; core++)
            {
                cairo_save (cr);
                {
                    cairo_rectangle_t translation = {};
                    *(horizontal ? &translation.x : &translation.y) = core * (base->size + base->per_core_spacing);
                    cairo_translate (cr, translation.x, translation.y);

                    if (!base->colors[BG_COLOR].isTransparent())
                    {
                        xfce4::cairo_set_source (cr, base->colors[BG_COLOR]);
                        cairo_rectangle (cr, 0, 0, w1, h1);
                        cairo_fill (cr);
                    }

                    cairo_rectangle (cr, 0, 0, w1, h1);
                    cairo_clip (cr);
                    draw (base, cr, w1, h1, core+1);
                }
                cairo_restore (cr);
            }
        }
    }
    return xfce4::PROPAGATE;
}

static Propagation
draw_bars_cb (cairo_t *cr, const Ptr<CPUGraph> &base)
{
    GtkAllocation alloc;
    gfloat size;
    const bool horizontal = (base->bars.orientation == GTK_ORIENTATION_HORIZONTAL);

    gtk_widget_get_allocation (base->bars.draw_area, &alloc);

    if (!base->colors[BG_COLOR].isTransparent())
    {
        xfce4::cairo_set_source (cr, base->colors[BG_COLOR]);
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

        xfce4::cairo_set_source (cr, base->colors[BARS_COLOR]);
        if (horizontal)
            cairo_rectangle (cr, 0, size-usage, 4, usage);
        else
            cairo_rectangle (cr, 0, 0, usage, 4);
        cairo_fill (cr);
    }
    else
    {
        const xfce4::RGBA *active_color = NULL;
        bool fill = false;
        for (guint i = 0; i < base->nr_cores; i++)
        {
            const bool highlight = base->highlight_smt && base->cpu_data[i+1].smt_highlight;

            gfloat usage = base->cpu_data[i+1].load;
            if (usage < base->load_threshold)
                usage = 0;
            usage *= size;

            /* Suboptimally placed threads on SMT CPUs are optionally painted using a different color. */
            const xfce4::RGBA *color = &base->colors[highlight ? SMT_ISSUES_COLOR : BARS_COLOR];
            if (active_color != color)
            {
                if (fill)
                {
                    cairo_fill (cr);
                    fill = false;
                }
                xfce4::cairo_set_source (cr, *color);
                active_color = color;
            }

            if (horizontal)
                cairo_rectangle (cr, 6*i, size-usage, 4, usage);
            else
                cairo_rectangle (cr, 0, 6*i, usage, 4);
            fill = true;
        }
        if (fill)
            cairo_fill (cr);
    }
    return xfce4::PROPAGATE;
}

static const gchar*
default_command (bool *in_terminal, bool *startup_notification)
{
    gchar *s = g_find_program_in_path ("xfce4-taskmanager");
    if (s)
    {
        g_free (s);
        *in_terminal = false;
        *startup_notification = true;
        return "xfce4-taskmanager";
    }
    else
    {
        *in_terminal = true;
        *startup_notification = false;

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

static Propagation
command_cb (GdkEventButton *event, const Ptr<CPUGraph> &base)
{
    if (event->button == 1)
    {
        std::string command;
        bool in_terminal, startup_notification;

        if (!base->command.empty())
        {
            command = base->command;
            in_terminal = base->command_in_terminal;
            startup_notification = base->command_startup_notification;
        }
        else
        {
            command = default_command (&in_terminal, &startup_notification);
        }

        xfce_spawn_command_line_on_screen (gdk_screen_get_default (),
                                           command.c_str(), in_terminal,
                                           startup_notification, NULL);
    }
    return xfce4::STOP;
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
CPUGraph::set_startup_notification (const Ptr<CPUGraph> &base, bool startup_notification)
{
    base->command_startup_notification = startup_notification;
}

void
CPUGraph::set_in_terminal (const Ptr<CPUGraph> &base, bool in_terminal)
{
    base->command_in_terminal = in_terminal;
}

void
CPUGraph::set_command (const Ptr<CPUGraph> &base, const std::string &command)
{
    base->command = xfce4::trim (command);
}

void
CPUGraph::set_bars (const Ptr<CPUGraph> &base, bool has_bars)
{
    if (base->has_bars != has_bars)
    {
        base->has_bars = has_bars;
        if (base->has_bars)
        {
            create_bars (base, xfce_panel_plugin_get_orientation (base->plugin));
            set_bars_size (base);
        }
        else
            delete_bars (base);
    }
}

void
CPUGraph::set_border (const Ptr<CPUGraph> &base, bool has_border)
{
    if (base->has_border != has_border)
    {
        base->has_border = has_border;
        size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
    }
}

void
CPUGraph::set_frame (const Ptr<CPUGraph> &base, bool has_frame)
{
    base->has_frame = has_frame;
    gtk_frame_set_shadow_type (GTK_FRAME (base->frame_widget), has_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    if (base->bars.frame)
        gtk_frame_set_shadow_type (GTK_FRAME (base->bars.frame), has_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

void
CPUGraph::set_nonlinear_time (const Ptr<CPUGraph> &base, bool non_linear)
{
    if (base->non_linear != non_linear)
    {
        base->non_linear = non_linear;
        queue_draw (base);
    }
}

void
CPUGraph::set_per_core (const Ptr<CPUGraph> &base, bool per_core)
{
    if (base->per_core != per_core)
    {
        base->per_core = per_core;
        size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
    }
}

void
CPUGraph::set_per_core_spacing (const Ptr<CPUGraph> &base, guint spacing)
{
    /* Use <=, instead of <, supresses a compiler warning */
    if (G_UNLIKELY (spacing <= PER_CORE_SPACING_MIN))
        spacing = PER_CORE_SPACING_MIN;
    if (G_UNLIKELY (spacing > PER_CORE_SPACING_MAX))
        spacing = PER_CORE_SPACING_MAX;

    if (base->per_core_spacing != spacing)
    {
        base->per_core_spacing = spacing;
        size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
    }
}

void
CPUGraph::set_smt (const Ptr<CPUGraph> &base, bool highlight_smt)
{
    base->highlight_smt = highlight_smt;
}

void
CPUGraph::set_update_rate (const Ptr<CPUGraph> &base, CPUGraphUpdateRate rate)
{
    bool change = (base->update_interval != rate);
    bool init = (base->timeout_id == 0);

    if (change || init)
    {
        guint interval = get_update_interval_ms (rate);

        base->update_interval = rate;
        if (base->timeout_id)
            g_source_remove (base->timeout_id);
        base->timeout_id = xfce4::timeout_add (interval, [base]() -> bool { return update_cb(base); });

        if (change && !init)
            queue_draw (base);
    }
}

void
CPUGraph::set_size (const Ptr<CPUGraph> &base, guint size)
{
    if (G_UNLIKELY (size < MIN_SIZE))
        size = MIN_SIZE;
    if (G_UNLIKELY (size > MAX_SIZE))
        size = MAX_SIZE;

    base->size = size;
    size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

void
CPUGraph::set_color_mode (const Ptr<CPUGraph> &base, guint color_mode)
{
    if (base->color_mode != color_mode)
    {
        base->color_mode = color_mode;
        queue_draw (base);
    }
}

void
CPUGraph::set_mode (const Ptr<CPUGraph> &base, CPUGraphMode mode)
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
CPUGraph::set_color (const Ptr<CPUGraph> &base, CPUGraphColorNumber number, const xfce4::RGBA &color)
{
    if (!base->colors[number].equals(color))
    {
        base->colors[number] = color;
        queue_draw (base);
    }
}

void
CPUGraph::set_tracked_core (const Ptr<CPUGraph> &base, guint core)
{
    if (G_UNLIKELY (core > base->nr_cores + 1))
        core = 0;

    if (base->tracked_core != core)
    {
        const bool had_bars = base->has_bars;
        if (had_bars)
            set_bars (base, false);
        base->tracked_core = core;
        if (had_bars)
            set_bars (base, true);
    }
}

void
CPUGraph::set_load_threshold (const Ptr<CPUGraph> &base, gfloat threshold)
{
    if (threshold < 0)
        threshold = 0;
    if (threshold > MAX_LOAD_THRESHOLD)
        threshold = MAX_LOAD_THRESHOLD;
    base->load_threshold = threshold;
}
