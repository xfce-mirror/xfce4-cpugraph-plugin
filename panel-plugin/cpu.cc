/*  cpu.cc
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
#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include "cpu.h"
#include "settings.h"
#include "mode.h"
#include "plugin.h"
#include "properties.h"

#include <libxfce4ui/libxfce4ui.h>
#include <math.h>
#include "../xfce4cpp/connections.hh"

#ifdef HAVE_MALLOC_TRIM
#include <malloc.h>
#endif

using xfce4::PluginShape;
using xfce4::Propagation;
using xfce4::TooltipTime;

using namespace std;

/* vim: !sort -k3 */
static void                 about_cb       ();
static Propagation          command_cb     (GdkEventButton *event, const shared_ptr<CPUGraph> &base);
static shared_ptr<CPUGraph> create_gui     (XfcePanelPlugin *plugin);
static Propagation          draw_area_cb   (cairo_t *cr, const shared_ptr<CPUGraph> &base);
static Propagation          draw_bars_cb   (cairo_t *cr, const shared_ptr<CPUGraph> &base);
static void                 mode_cb        (XfcePanelPlugin *plugin, const shared_ptr<CPUGraph> &base);
static void                 shutdown       (const shared_ptr<CPUGraph> &base);
static PluginShape          size_cb        (XfcePanelPlugin *plugin, guint size, const shared_ptr<CPUGraph> &base);
static TooltipTime          tooltip_cb     (GtkTooltip *tooltip, const shared_ptr<CPUGraph> &base);
static void                 update_tooltip (const shared_ptr<CPUGraph> &base, bool force);

void
cpugraph_construct (XfcePanelPlugin *plugin)
{
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    auto base = create_gui (plugin);

    Settings::init (plugin, base);

    Settings::read (plugin, base);

    xfce_panel_plugin_menu_show_about (plugin);
    xfce_panel_plugin_menu_show_configure (plugin);

    xfce4::connect_about           (plugin, [base](XfcePanelPlugin *p) { about_cb (); });
    xfce4::connect_free_data       (plugin, [base](XfcePanelPlugin *p) { shutdown (base); });
    xfce4::connect_save            (plugin, [base](XfcePanelPlugin *p) { Settings::write (p, base); });
    xfce4::connect_configure_plugin(plugin, [base](XfcePanelPlugin *p) { create_options (p, base); });
    xfce4::connect_mode_changed    (plugin, [base](XfcePanelPlugin *p, XfcePanelPluginMode mode) { mode_cb (p, base); });
    xfce4::connect_size_changed    (plugin, [base](XfcePanelPlugin *p, guint size) { return size_cb (p, size, base); });
}

static void
init_cpu_data (const shared_ptr<CPUGraph> &base, bool read)
{
    if (read)
    {
        read_cpu_data (base->cpu_data, base->cpu_to_index);

        /* Read CPU data twice in order to initialize
         * cpu_data[].previous_used and cpu_data[].previous_total
         * with the current HWMs. HWM = High Water Mark. */
        read_cpu_data (base->cpu_data, base->cpu_to_index);
    }

    base->nr_cores = base->cpu_to_index.size();

    base->index_to_cpu.clear();
    for (const auto [cpu, idx] : base->cpu_to_index)
        base->index_to_cpu[idx] = cpu;
    // CPU at index 0 (overall usage) has always index 0 - no need to map it:
    // base->cpu_data[0] is always the same as base->cpu_data[base->index_to_cpu[0]]

    if (base->nr_cores == 0)
        fprintf (stderr, "Cannot init cpu data !\n");

    base->topology = read_topology ();
}

static shared_ptr<CPUGraph>
create_gui (XfcePanelPlugin *plugin)
{
    GtkWidget *frame, *ebox;
    GtkOrientation orientation;
    auto base = make_shared<CPUGraph>();

    orientation = xfce_panel_plugin_get_orientation (plugin);

    init_cpu_data (base, true);

    base->plugin = plugin;

    base->ebox = ebox = gtk_event_box_new ();
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (ebox), false);
    gtk_event_box_set_above_child (GTK_EVENT_BOX (ebox), true);
    gtk_container_add (GTK_CONTAINER (plugin), ebox);
    xfce_panel_plugin_add_action_widget (plugin, ebox);
    xfce4::connect_button_press (ebox, [base](GtkWidget*, GdkEventButton *event) -> Propagation {
        return command_cb (event, base);
    });

    base->box = gtk_box_new (orientation, 0);
    gtk_container_add (GTK_CONTAINER (ebox), base->box);
    gtk_widget_set_has_tooltip (base->box, true);
    xfce4::connect_query_tooltip (base->box, [base](GtkWidget *widget, gint x, gint y, bool keyboard, GtkTooltip *tooltip) {
        return tooltip_cb (tooltip, base);
    });

    base->frame_widget = frame = gtk_frame_new (nullptr);
    gtk_box_pack_end (GTK_BOX (base->box), frame, true, true, 2);

    base->draw_area = gtk_drawing_area_new ();
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (base->draw_area));
    xfce4::connect_after_draw (base->draw_area, [base](cairo_t *cr) { return draw_area_cb (cr, base); });

    base->has_bars = false;
    base->has_barcolor = false;
    base->bars.orientation = orientation;
    base->stats_smt = STATS_SMT_BY_DEFAULT;
    base->highlight_smt = HIGHLIGHT_SMT_BY_DEFAULT;
    base->per_core_spacing = PER_CORE_SPACING_DEFAULT;

    mode_cb (plugin, base);
    gtk_widget_show_all (ebox);

    base->tooltip_last_value = -1;
    base->tooltip_text = gtk_label_new (nullptr);
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
        "Błażej Szczygieł <mumei6102@gmail.com>",
        "Florian Rivoal <frivoal@gmail.com>",
        "Jan Ziak <0xe2.0x9a.0x9b@xfce.org>",
        "Ludovic Mercier <lidiriel@coriolys.org>",
        "Peter Tribble <peter.tribble@gmail.com>",
        nullptr
    };

    gtk_show_about_dialog (nullptr,
        "logo-icon-name", "org.xfce.panel.cpugraph",
        "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
        "version", VERSION_FULL,
        "program-name", PACKAGE_NAME,
        "comments", _("Graphical representation of the CPU load"),
        "website", "https://docs.xfce.org/panel-plugins/xfce4-cpugraph-plugin",
        "copyright", "Copyright \302\251 2004-" COPYRIGHT_YEAR " The Xfce development team",
        "authors", auth, nullptr);
}

void
CPUGraph::ebox_revalidate ()
{
    gtk_event_box_set_above_child (GTK_EVENT_BOX (ebox), false);
    gtk_event_box_set_above_child (GTK_EVENT_BOX (ebox), true);
}

guint
CPUGraph::nb_bars ()
{
    return tracked_core == 0 ? nr_cores : 1;
}

void
CPUGraph::create_bars (GtkOrientation orientation)
{
    bars.frame = gtk_frame_new (nullptr);
    bars.draw_area = gtk_drawing_area_new ();
    bars.orientation = orientation;
    set_frame (has_frame);
    gtk_container_add (GTK_CONTAINER (bars.frame), bars.draw_area);
    gtk_box_pack_end (GTK_BOX (box), bars.frame, true, true, 0);
    xfce4::connect_after_draw (bars.draw_area, [base = shared_from_this ()](cairo_t *cr) { return draw_bars_cb(cr, base); });
    gtk_widget_show_all (bars.frame);
    ebox_revalidate ();
}

CPUGraph::~CPUGraph ()
{
    g_info ("%s", __PRETTY_FUNCTION__);
    if (channel)
    {
        g_object_unref (channel);
        Settings::finalize ();
    }
}

bool
CPUGraph::is_smt_issues_enabled () const
{
    return stats_smt || (has_bars && highlight_smt);
}

static void
shutdown (const shared_ptr<CPUGraph> &base)
{
    base->delete_bars ();
    gtk_widget_destroy (base->ebox);
    base->ebox = nullptr;
    g_object_unref (base->tooltip_text);
    base->tooltip_text = nullptr;
    xfce4::source_remove (base->timeout_id);
}

void
CPUGraph::delete_bars ()
{
    if (bars.frame)
    {
        gtk_widget_destroy (bars.frame);
        bars.frame = nullptr;
        bars.draw_area = nullptr;
    }
}

static void
queue_draw (const shared_ptr<CPUGraph> &base)
{
    if (base->mode != MODE_DISABLED)
        gtk_widget_queue_draw (base->draw_area);
    if (base->bars.draw_area)
        gtk_widget_queue_draw (base->bars.draw_area);
}

static void
resize_history (const shared_ptr<CPUGraph> &base, gssize history_size)
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
        const auto old_data = std::move(base->history.data);
        const gssize old_mask = base->history.mask();
        const gssize old_offset = base->history.offset;

        base->history.cap_pow2 = cap_pow2;
        base->history.data.resize(base->nr_cores + 1);
        base->history.offset = 0;
        for (guint core = 0; core < base->nr_cores + 1; core++)
        {
            base->history.data[core] = make_unique<CpuLoad[]> (cap_pow2);
            if (!old_data.empty())
            {
                for (gssize i = 0; i < old_cap_pow2 && i < cap_pow2; i++)
                    base->history.data[core][i] = old_data[core][(old_offset + i) & old_mask];
            }
        }

#ifdef HAVE_MALLOC_TRIM
        malloc_trim (0);
#endif
    }

    base->history.size = history_size;
}

static PluginShape
size_cb (XfcePanelPlugin *plugin, guint plugin_size, const shared_ptr<CPUGraph> &base)
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
        base->set_bars_size ();
    }

    if (base->has_border)
        border_width = (xfce_panel_plugin_get_size (base->plugin) > 26 ? 2 : 1);
    else
        border_width = 0;
    gtk_container_set_border_width (GTK_CONTAINER (base->box), border_width);

    base->set_border (base->has_border);

    return xfce4::PluginShape::Rectangle();
}

void
CPUGraph::set_bars_size ()
{
    gint h, v;
    gint shadow_width;

    shadow_width = has_frame ? 2*1 : 0;

    if (bars.orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        h = size + shadow_width;
        v = -1;
    }
    else
    {
        h = -1;
        v = size + shadow_width;
    }

    gtk_widget_set_size_request (bars.frame, h, v);
}

static void
mode_cb (XfcePanelPlugin *plugin, const shared_ptr<CPUGraph> &base)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (base->box), xfce_panel_plugin_get_orientation (plugin));

    size_cb (plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

void
CPUGraph::detect_smt_issues ()
{
    constexpr bool debug = false;

    auto ensure_resized = [this](auto &&arr) {
        if (G_UNLIKELY (arr.size() < nr_cores))
        {
            arr.clear();
            arr.resize(nr_cores);
        }
    };
    ensure_resized(smt.movement);
    ensure_resized(smt.suboptimal);
    ensure_resized(smt.actual_load);
    ensure_resized(smt.optimal_load);
    ensure_resized(smt.actual_num_instr_executed);
    ensure_resized(smt.optimal_num_instr_executed);

    /* Initialize CPU load arrays.
     * The array optimal_load[] will be updated
     * if a suboptimal SMT thread placement is detected. */
    for (guint i = 0; i < nr_cores; i++)
    {
        const gfloat load = cpu_data[index_to_cpu[i+1]].load;
        smt.suboptimal[i] = false;
        smt.movement[i] = false;
        smt.actual_load[i] = load;
        smt.optimal_load[i] = load;
        smt.actual_num_instr_executed[i] = load;
        smt.optimal_num_instr_executed[i] = load;
        if constexpr (debug)
            g_info ("actual_load[%u] = %g", i, load);
    }

    const auto topo = topology.get();

    bool smt_incident = false;

    constexpr gfloat THRESHOLD = 1.0 + 0.1;       /* A lower bound (this core) */
    constexpr gfloat THRESHOLD_OTHER = 1.0 - 0.1; /* An upper bound (some other core) */

    for (guint i = 0; i < nr_cores; i++)
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
                    if (G_LIKELY (cpu < nr_cores))
                        combined_usage += smt.optimal_load[cpu];
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
                                if (G_LIKELY (other_cpu < nr_cores))
                                    combined_usage_other += smt.optimal_load[other_cpu];
                            }
                            if (combined_usage_other < THRESHOLD_OTHER)
                            {
                                /* The thread might have been executed on 'other_core',
                                 * instead of on 'core', where it might have enjoyed
                                 * a much higher IPC (instructions per clock) ratio */

                                smt_incident = true;
                                for (guint cpu : topo->cores[core].logical_cpus)
                                {
                                    if (G_LIKELY (cpu < nr_cores))
                                        smt.suboptimal[cpu] = true;
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
                                        if (G_LIKELY (other_cpu < nr_cores))
                                            if (smt.optimal_load[other_cpu] < 0.999f)
                                                if (other_cpu_min == -1 || smt.optimal_load[other_cpu_min] > smt.optimal_load[other_cpu])
                                                    other_cpu_min = other_cpu;
                                    }
                                    if (G_LIKELY (other_cpu_min != -1))
                                    {
                                        gfloat load_to_move;

                                        load_to_move = excess_load;
                                        if (load_to_move > 1.0f - smt.optimal_load[other_cpu_min])
                                            load_to_move = 1.0f - smt.optimal_load[other_cpu_min];

                                        if constexpr (debug)
                                            g_info ("load_to_move = %g", load_to_move);

                                        smt.optimal_load[other_cpu_min] += load_to_move;

                                        /* The move negates the SMT slowdown for the work moved onto the underutilized target CPU core */
                                        smt.movement[other_cpu_min] = true;
                                        smt.optimal_num_instr_executed[other_cpu_min] += (1.0f + SMT_SLOWDOWN) * load_to_move;

                                        /* Decrease combined_usage by load_to_move */
                                        for (guint j = topo->cores[core].logical_cpus.size(); load_to_move > 0 && j != 0;)
                                        {
                                            guint cpu = topo->cores[core].logical_cpus[--j];
                                            if (G_LIKELY (cpu < nr_cores))
                                            {
                                                if (smt.optimal_load[cpu] >= load_to_move)
                                                {
                                                    const gfloat diff = load_to_move;

                                                    smt.optimal_load[cpu] -= diff;
                                                    load_to_move = 0;

                                                    /* The move negates the SMT slowdown for the work remaining on the original CPU core */
                                                    smt.optimal_num_instr_executed[cpu] -= 1.0f * diff;         /* Moved work */
                                                    smt.optimal_num_instr_executed[cpu] += SMT_SLOWDOWN * diff; /* Remaining work (speedup) */
                                                    smt.movement[cpu] = true;
                                                }
                                                else
                                                {
                                                    const gfloat diff = smt.optimal_load[cpu];

                                                    smt.optimal_load[cpu] = 0;
                                                    load_to_move -= diff;

                                                    /* The move negates the SMT slowdown for the work remaining on the original CPU core */
                                                    smt.optimal_num_instr_executed[cpu] -= 1.0f * diff;         /* Moved work */
                                                    smt.optimal_num_instr_executed[cpu] += SMT_SLOWDOWN * diff; /* Remaining work (speedup) */
                                                    smt.movement[cpu] = true;
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
    for (guint i = 0; i < nr_cores; i++)
    {
        stats.num_instructions_executed.total.actual += smt.actual_num_instr_executed[i];
        stats.num_instructions_executed.total.optimal += smt.optimal_num_instr_executed[i];
    }

    /* Suboptimal SMT scheduling cases are actually quite rare (at least in Linux):
     * - They are impossible to happen if the CPU is under full load
     * - They are rare if the CPU is running one single-threaded task
     * - They tend to occur especially when the CPU is running about nr_cores/2 threads
     */
    if (G_UNLIKELY (smt_incident))
    {
        stats.num_smt_incidents++;

        /* Update instruction counters */
        for (guint i = 0; i < nr_cores; i++)
        {
            if (smt.movement[i] || smt.suboptimal[i])
            {
                stats.num_instructions_executed.during_smt_incidents.actual += smt.actual_num_instr_executed[i];
                stats.num_instructions_executed.during_smt_incidents.optimal += smt.optimal_num_instr_executed[i];
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
            if (G_LIKELY (cpu < nr_cores))
                positive |= smt.suboptimal[cpu];

        if (positive)
        {
            gfloat actual_combined_usage = 0;
            for (guint cpu : core_iterator.second.logical_cpus)
                actual_combined_usage += smt.actual_load[cpu];

            bool false_positive = !(actual_combined_usage > THRESHOLD);
            if (false_positive)
                for (guint cpu : core.logical_cpus)
                    if (G_LIKELY (cpu < nr_cores))
                        smt.suboptimal[cpu] = false;
        }
    }

    for (guint i = 0; i < nr_cores; i++)
        cpu_data[index_to_cpu[i+1]].smt_highlight = smt.suboptimal[i];
}

static xfce4::TimeoutResponse
update_cb (const shared_ptr<CPUGraph> &base)
{
    read_cpu_data (base->cpu_data, base->cpu_to_index_cache);

    if (base->cpu_to_index_cache.empty()) // Read Error
    {
        return xfce4::TimeoutResponse::Again();
    }
    else if (base->cpu_to_index_cache != base->cpu_to_index) // CPU layout changed
    {
        const auto old_cpus = std::move (base->cpu_to_index);

        // Init CPU data
        base->cpu_to_index = std::move (base->cpu_to_index_cache);
        init_cpu_data (base, false);

        if (!base->history.data.empty () && old_cpus != base->cpu_to_index)
        {
            // There are changes in CPU layout - (reorder / allocate new / free old) per-core history

            auto old_history_data = std::move (base->history.data);

            base->history.data.resize (base->nr_cores + 1);

            // Copy old history pointer for overall CPU usage
            base->history.data[0] = std::move(old_history_data[0]);

            // Copy old history pointers to the new CPU layout with the new CPUs order
            for (const auto [cpu, idx] : base->cpu_to_index)
            {
                const auto old_it = old_cpus.find (cpu);
                if (old_it == old_cpus.end ())
                    continue; // CPU disappeared, ignore

                const auto old_idx = old_it->second;
                base->history.data[idx] = std::move(old_history_data[old_idx]);
            }

            // Allocate memory for all new CPUs
            for (auto &&data : base->history.data)
            {
                if (!data)
                    data = make_unique<CpuLoad[]> (base->history.cap_pow2);
            }
        }

        // Update GUI and init history if necessary
        size_cb (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
    }

    if (base->topology && base->topology->smt && base->is_smt_issues_enabled ())
        base->detect_smt_issues ();

    if (!base->history.data.empty())
    {
        const gint64 timestamp = g_get_real_time ();

        /* Prepend the current CPU load to the history */
        base->history.offset = (base->history.offset - 1) & base->history.mask();
        for (guint core = 0; core < base->nr_cores + 1; core++)
        {
            const CpuData &cpu_core_data = base->cpu_data[base->index_to_cpu[core]];
            CpuLoad &load = base->history.data[core][base->history.offset];
            load.timestamp = timestamp;
            load.value = cpu_core_data.load;
            load.system = cpu_core_data.system;
            load.user = cpu_core_data.user;
            load.nice = cpu_core_data.nice;
            load.iowait = cpu_core_data.iowait;
        }
    }

    queue_draw (base);
    update_tooltip (base, false);

    return xfce4::TimeoutResponse::Again();
}

static void
update_tooltip (const shared_ptr<CPUGraph> &base, bool force)
{
    const gint value = round(base->cpu_data[0].load * 1000.0f);
    if (base->tooltip_last_value != value && (force || gtk_widget_get_mapped (base->tooltip_text)))
    {
        auto tooltip = xfce4::sprintf (_("CPU usage: %.1f%%"), value / 10.0f);
        gtk_label_set_text (GTK_LABEL (base->tooltip_text), tooltip.c_str());
        base->tooltip_last_value = value;
    }
}

static TooltipTime
tooltip_cb (GtkTooltip *tooltip, const shared_ptr<CPUGraph> &base)
{
    update_tooltip (base, true);
    gtk_tooltip_set_custom (tooltip, base->tooltip_text);
    return xfce4::TooltipTime::Now();
}

static Propagation
draw_area_cb (cairo_t *cr, const shared_ptr<CPUGraph> &base)
{
    GtkAllocation alloc;
    gint w, h;
    void (*draw) (const shared_ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core) = nullptr;

    gtk_widget_get_allocation (base->draw_area, &alloc);
    w = alloc.width;
    h = alloc.height;

    switch (base->mode)
    {
        case MODE_DISABLED:
            break;
        case MODE_NORMAL:
            if (base->size > 1)
                draw = draw_graph_normal;
            else
                draw = draw_graph_no_history;
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

            if (!base->colors[BG_COLOR].is_transparent())
            {
                xfce4::cairo_set_source_rgba (cr, base->colors[BG_COLOR]);
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

                    if (!base->colors[BG_COLOR].is_transparent())
                    {
                        xfce4::cairo_set_source_rgba (cr, base->colors[BG_COLOR]);
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
    return xfce4::Propagation::Propagate();
}

static Propagation
draw_bars_cb (cairo_t *cr, const shared_ptr<CPUGraph> &base)
{
    GtkAllocation alloc;
    const bool horizontal = (base->bars.orientation == GTK_ORIENTATION_HORIZONTAL) ^ base->bars_perpendicular;

    gtk_widget_get_allocation (base->bars.draw_area, &alloc);

    if (!base->colors[BG_COLOR].is_transparent())
    {
        xfce4::cairo_set_source_rgba (cr, base->colors[BG_COLOR]);
        cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
        cairo_fill (cr);
    }

    gfloat length = (horizontal ? alloc.width : alloc.height);
    gfloat breadth = (horizontal ? alloc.height : alloc.width);
    if (base->tracked_core != 0 || base->nr_cores == 1)
    {
        gfloat usage = base->cpu_data[0].load;
        if (usage < base->load_threshold)
            usage = 0;
        usage *= length;

        xfce4::cairo_set_source_rgba (cr, base->colors[BARS_COLOR]);
        if (horizontal)
            cairo_rectangle (cr, 0, 0, usage, breadth);
        else
            cairo_rectangle (cr, 0, length-usage, breadth, usage);
        cairo_fill (cr);
    }
    else
    {
        breadth -= BAR_SPACE * (base->nr_cores - 1);
        breadth /= base->nr_cores;

        const xfce4::RGBA *active_color = nullptr;
        bool fill = false;
        for (guint i = 0; i < base->nr_cores; i++)
        {
            const auto &cpu_data_i_plus_1 = base->cpu_data[base->index_to_cpu[i+1]];

            const bool highlight = base->highlight_smt && cpu_data_i_plus_1.smt_highlight;

            gfloat usage = cpu_data_i_plus_1.load;
            if (usage < base->load_threshold)
                usage = 0;
            usage *= length;

            /* Suboptimally placed threads on SMT CPUs are optionally painted using a different color. */
            const xfce4::RGBA *color = &base->colors[highlight ? SMT_ISSUES_COLOR : BARS_COLOR];
            if (active_color != color)
            {
                if (fill)
                {
                    cairo_fill (cr);
                    fill = false;
                }
                xfce4::cairo_set_source_rgba (cr, *color);
                active_color = color;
            }

            if (horizontal)
                cairo_rectangle (cr, 0, (breadth+BAR_SPACE)*i, usage, breadth);
            else
                cairo_rectangle (cr, (breadth+BAR_SPACE)*i, length-usage, breadth, usage);
            fill = true;
        }
        if (fill)
            cairo_fill (cr);
    }
    return xfce4::Propagation::Propagate();
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
command_cb (GdkEventButton *event, const shared_ptr<CPUGraph> &base)
{
    if (event->button == 1)
    {
        string command;
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

        xfce_spawn_command_line (gdk_screen_get_default (),
                                 command.c_str(), in_terminal,
                                 startup_notification, true, nullptr);
    }
    return xfce4::Propagation::Stop();
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
CPUGraph::set_startup_notification (bool startup_notification)
{
    command_startup_notification = startup_notification;
}

void
CPUGraph::set_in_terminal (bool in_terminal)
{
    command_in_terminal = in_terminal;
}

void
CPUGraph::set_command (const string_view &command_arg)
{
    command = xfce4::trim (command_arg);
}

void
CPUGraph::set_bars (bool has_bars_arg)
{
    if (has_bars != has_bars_arg)
    {
        has_bars = has_bars_arg;
        if (has_bars)
        {
            create_bars (xfce_panel_plugin_get_orientation (plugin));
            set_bars_size ();
        }
        else
        {
            delete_bars ();
        }
    }
}

void
CPUGraph::set_bars_perpendicular (bool bars_perpendicular_arg)
{
    if (bars_perpendicular != bars_perpendicular_arg)
    {
        bars_perpendicular = bars_perpendicular_arg;
        if (has_bars)
        {
            delete_bars ();
            create_bars (xfce_panel_plugin_get_orientation (plugin));
            set_bars_size ();
        }
    }
}

void
CPUGraph::set_border (bool has_border_arg)
{
    if (has_border != has_border_arg)
    {
        has_border = has_border_arg;
        size_cb (plugin, xfce_panel_plugin_get_size (plugin), shared_from_this ());
    }
}

void
CPUGraph::set_frame (bool has_frame_arg)
{
    has_frame = has_frame_arg;
    gtk_frame_set_shadow_type (GTK_FRAME (frame_widget), has_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    if (bars.frame)
        gtk_frame_set_shadow_type (GTK_FRAME (bars.frame), has_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    size_cb (plugin, xfce_panel_plugin_get_size (plugin), shared_from_this ());
}

void
CPUGraph::set_nonlinear_time (bool non_linear_arg)
{
    if (non_linear != non_linear_arg)
    {
        non_linear = non_linear_arg;
        if (!non_linear)
            non_linear_cache = {};
        queue_draw (shared_from_this ());
    }
}

void
CPUGraph::set_per_core (bool per_core_arg)
{
    if (per_core != per_core_arg)
    {
        per_core = per_core_arg;
        size_cb (plugin, xfce_panel_plugin_get_size (plugin), shared_from_this ());
    }
}

void
CPUGraph::set_per_core_spacing (guint spacing)
{
    /* Use <=, instead of <, supresses a compiler warning */
    if (G_UNLIKELY (spacing <= PER_CORE_SPACING_MIN))
        spacing = PER_CORE_SPACING_MIN;
    if (G_UNLIKELY (spacing > PER_CORE_SPACING_MAX))
        spacing = PER_CORE_SPACING_MAX;

    if (per_core_spacing != spacing)
    {
        per_core_spacing = spacing;
        size_cb (plugin, xfce_panel_plugin_get_size (plugin), shared_from_this ());
    }
}

void
CPUGraph::set_stats_smt (bool stats_smt_arg)
{
    stats_smt = stats_smt_arg;
}

void
CPUGraph::set_smt (bool highlight_smt_arg)
{
    highlight_smt = highlight_smt_arg;
}

void
CPUGraph::set_update_rate (CPUGraphUpdateRate rate)
{
    bool change = (update_interval != rate);
    bool init = timeout_id.expired();

    if (change || init)
    {
        guint interval = get_update_interval_ms (rate);

        update_interval = rate;
        xfce4::source_remove (timeout_id);
        timeout_id = xfce4::timeout_add (interval, [base = shared_from_this ()]() { return update_cb (base); });

        if (change && !init)
            queue_draw (shared_from_this ());
    }
}

void
CPUGraph::maybe_clear_smt_stats ()
{
    if (!is_smt_issues_enabled ())
        stats = {};
}

void
CPUGraph::set_size (guint size_arg)
{
    if (G_UNLIKELY (size < MIN_SIZE))
        size = MIN_SIZE;
    if (G_UNLIKELY (size > MAX_SIZE))
        size = MAX_SIZE;

    size = size_arg;
    size_cb (plugin, xfce_panel_plugin_get_size (plugin), shared_from_this ());
}

void
CPUGraph::set_color_mode (guint color_mode_arg)
{
    if (color_mode != color_mode_arg)
    {
        color_mode = color_mode_arg;
        queue_draw (shared_from_this ());
    }
}

void
CPUGraph::set_mode (CPUGraphMode mode_arg)
{
    mode = mode_arg;
    nearest_cache = {};
    non_linear_cache = {};
    if (mode == MODE_DISABLED)
    {
        gtk_widget_hide (frame_widget);
    }
    else
    {
        gtk_widget_show (frame_widget);
        ebox_revalidate ();
    }
}

void
CPUGraph::set_color (CPUGraphColorNumber number, const xfce4::RGBA &color)
{
    if (colors[number] != color)
    {
        colors[number] = color;
        queue_draw (shared_from_this ());
    }
}

void
CPUGraph::set_tracked_core (guint core)
{
    if (G_UNLIKELY (core > nr_cores + 1))
        core = 0;

    if (tracked_core != core)
    {
        const bool had_bars = has_bars;
        if (had_bars)
            set_bars (false);
        tracked_core = core;
        if (had_bars)
            set_bars (true);
    }
}

void
CPUGraph::set_load_threshold (gfloat threshold)
{
    if (threshold < 0)
        threshold = 0;
    if (threshold > MAX_LOAD_THRESHOLD)
        threshold = MAX_LOAD_THRESHOLD;
    load_threshold = threshold;
}
