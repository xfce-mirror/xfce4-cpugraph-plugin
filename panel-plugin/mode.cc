/*  mode.cc
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

#include <cairo/cairo.h>
#include <math.h>
#include <stdlib.h>
#include "mode.h"

using namespace std;

struct Point
{
    gfloat x, y;
    Point() : x(0), y(0) {}
    Point(gfloat _x, gfloat _y) : x(_x), y(_y) {}
};

static xfce4::RGBA
mix_colors (gdouble ratio, const xfce4::RGBA &color1, const xfce4::RGBA &color2)
{
    return color1 + (color2 - color1) * ratio;
}

template<typename Vector>
static void
ensure_vector_size (Vector &arr, gint size)
{
    if (G_UNLIKELY (size < 0))
        size = 0;

    if (arr.size() != (guint) size)
    {
        arr.clear();
        arr.shrink_to_fit();
        arr.resize(size);
    }
}

/**
 * nearest_loads:
 * @start: microseconds since 1970-01-01 UTC
 * @step: step in microseconds, has to be a negative value
 * @count: number of steps
 * @out: output of this function
 *
 * Get CPU loads near (close to) a specified range of timestamps.
 * The timestampts range from 'timestamp' to timestamp+step*(count-1).
 */
static void
nearest_loads (const shared_ptr<CPUGraph> &base, const guint core, const gint64 start, const gint64 step, const gssize count, const CpuLoad **out)
{
    const gssize history_cap_pow2 = base->history.cap_pow2;
    const CpuLoad *history_data = base->history.data[core].get();
    const gssize history_mask = base->history.mask();
    const gssize history_offset = base->history.offset;

    if (!base->non_linear)
    {
        for (gssize i = 0, j = 0; i < count; i++)
        {
            const gint64 timestamp = start + i * step;
            const CpuLoad *nearest = nullptr;
            for (; j < history_cap_pow2; j++)
            {
                const CpuLoad &load = history_data[(history_offset + j) & history_mask];

                if (load.timestamp == 0)
                {
                    for (; i < count; i++)
                        out[i] = nullptr;
                    return;
                }

                if (!nearest)
                {
                    nearest = &load;
                }
                else
                {
                    gint64 delta = labs (load.timestamp - timestamp);
                    if (delta < labs (nearest->timestamp - timestamp))
                    {
                        nearest = &load;
                    }
                    else if (delta > labs (nearest->timestamp - timestamp))
                    {
                        if (j > 0)
                            j--;
                        break;
                    }
                }
            }
            out[i] = nearest;
        }
    }
    else
    {
        auto &cache = base->non_linear_cache;
        ensure_vector_size (cache, count);
        for (gssize i = 0; i < count; i++)
        {
            /* Note: step < 0, therefore: timestamp1 < timestamp0 */
            const gint64 timestamp0 = start + (i+0) * pow (NONLINEAR_MODE_BASE, i+0) * step;
            const gint64 timestamp1 = start + (i+1) * pow (NONLINEAR_MODE_BASE, i+1) * step;
            gfloat sum_value = 0.0f;
            gfloat sum_user = 0.0f;
            gfloat sum_system = 0.0f;
            gfloat sum_nice = 0.0f;
            gfloat sum_iowait = 0.0f;
            gint num_loads = 0;

            for (gssize j = 0; j < history_cap_pow2; j++)
            {
                const CpuLoad &load = history_data[(history_offset + j) & history_mask];
                if (load.timestamp > timestamp1 && load.timestamp <= timestamp0)
                {
                    sum_value += load.value;
                    sum_system += load.system;
                    sum_user += load.user;
                    sum_nice += load.nice;
                    sum_iowait += load.iowait;
                    num_loads++;
                }
                else if (load.timestamp < timestamp1)
                    break;
            }

            /* num_loads==0 in the following cases:
             *  - Both timestamps are pointing to a time before the CPU load measurements have started
             *  - Both timestamps are pointing to a time before the history has been cleared
             *  - There has been a change in base->update_interval,
             *    for example from RATE_SLOWEST to RATE_FASTEST
             */

            if (num_loads != 0)
            {
                cache[i].value = sum_value / num_loads;
                cache[i].system = sum_system / num_loads;
                cache[i].user = sum_user / num_loads;
                cache[i].nice = sum_nice / num_loads;
                cache[i].iowait = sum_iowait / num_loads;
                out[i] = &cache[i];
            }
            else
            {
                out[i] = nullptr;
            }
        }

        for (gssize i = 0; i < count; i++)
        {
            if (!out[i])
            {
                const CpuLoad *prev = nullptr, *next = nullptr;

                for (gssize j = 0; j < i; j++)
                    if (out[j])
                    {
                        prev = out[j];
                        break;
                    }

                for (gssize j = i+1; j < count; j++)
                    if (out[j])
                    {
                        next = out[j];
                        break;
                    }

                if (prev && next)
                {
                    cache[i].value = (prev->value + next->value) / 2.0f;
                    cache[i].system = (prev->system + next->system) / 2.0f;
                    cache[i].user = (prev->user + next->user) / 2.0f;
                    cache[i].nice = (prev->nice + next->nice) / 2.0f;
                    cache[i].iowait = (prev->iowait + next->iowait) / 2.0f;
                }
                else
                {
                    cache[i] = {};
                }

                out[i] = &cache[i];
            }
        }
    }
}

static void
draw_graph_helper (const shared_ptr<CPUGraph> &base, const CpuLoad &load, cairo_t *cr, gint i, gint span, gint breadth)
{
    if (load.value < base->load_threshold)
        return;

    const gfloat usage = breadth * load.value;

    if (usage == 0.0f)
        return;

    bool horizontal = (xfce_panel_plugin_get_orientation (base->plugin) == GTK_ORIENTATION_HORIZONTAL);

    if (base->color_mode == COLOR_MODE_DETAILED)
    {
        gfloat j_offset = 0.0f;
        auto draw = [&](gfloat value, CPUGraphColorNumber color) {
            if (value > 0.0f)
            {
                xfce4::cairo_set_source_rgba (cr, base->colors[color]);
                if (horizontal) {
                    cairo_rectangle (cr, i, breadth - value - j_offset, span, value);
                }
                else {
                    cairo_rectangle (cr, breadth - value - j_offset, i, value, span);
                }
                cairo_fill (cr);
                j_offset += value;
            }
        };
        draw(breadth * load.system, FG_COLOR_SYSTEM);
        draw(breadth * load.user, FG_COLOR_USER);
        draw(breadth * load.nice, FG_COLOR_NICE);
        draw(breadth * load.iowait, FG_COLOR_IOWAIT);
    }
    else if (base->color_mode == COLOR_MODE_SOLID)
    {
        xfce4::cairo_set_source_rgba (cr, base->colors[FG_COLOR1]);
        if (horizontal) {
            cairo_rectangle (cr, i, breadth - usage, span, usage);
        }
        else {
            cairo_rectangle (cr, breadth - usage, i, usage, span);
        }
        cairo_fill (cr);
    }
    else
    {
        const gint breadth_usage = breadth - (gint) roundf (usage);
        for (gint j = breadth - 1, tmp = 0; j >= breadth_usage; j--, tmp++)
        {
            gfloat t = tmp / (base->color_mode == COLOR_MODE_GRADIENT ? (gfloat) breadth : usage);
            xfce4::cairo_set_source_rgba (cr, mix_colors (t, base->colors[FG_COLOR1], base->colors[FG_COLOR2]));
            if (horizontal) {
                cairo_rectangle (cr, i, j, span, 1);
            }
            else {
                cairo_rectangle (cr, j, i, 1, span);
            }
            cairo_fill (cr);
        }
    }
}

void
draw_graph_normal (const shared_ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    const gint64 step = 1000 * (gint64) get_update_interval_ms (base->update_interval);
    auto &nearest = base->nearest_cache;

    bool horizontal = (xfce_panel_plugin_get_orientation (base->plugin) == GTK_ORIENTATION_HORIZONTAL);

    gint span = horizontal ? w : h;

    ensure_vector_size (nearest, span);

    gint64 t0 = base->history.data[core][base->history.offset].timestamp;
    nearest_loads (base, core, t0, -step, span, nearest.data());

    for (gint i = 0; i < span; i++)
    {
        if (const CpuLoad *loadPtr = nearest[horizontal ? span - 1 - i : i])
            draw_graph_helper (base, *loadPtr, cr, i, 1, horizontal ? h : w);
    }
}

void
draw_graph_LED (const shared_ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    bool horizontal = (xfce_panel_plugin_get_orientation (base->plugin) == GTK_ORIENTATION_HORIZONTAL);

    gint span = horizontal ? w : h;
    gint breadth = horizontal ? h : w;

    const gint nri = (span + 2) / 3;
    const gint nrj = (breadth + 1) / 2;
    const xfce4::RGBA *active_color = NULL;
    const gint64 step = 1000 * (gint64) get_update_interval_ms (base->update_interval);
    auto &nearest = base->nearest_cache;
    ensure_vector_size (nearest, span);

    gint64 t0 = base->history.data[core][base->history.offset].timestamp;
    nearest_loads (base, core, t0, -step, nri, nearest.data());

    for (gint i = 0; i * 3 < span; i++)
    {
        const gint idi = horizontal ? nri - i - 1 : i;
        gint limit = nrj;

        if (G_LIKELY (idi >= 0 && idi < nri))
        {
            if (const CpuLoad *loadPtr = nearest[idi])
            {
                if (loadPtr->value >= base->load_threshold)
                    limit = nrj - (gint) roundf (nrj * loadPtr->value);
            }
        }

        for (gint j = 0; j * 2 < breadth; j++)
        {
            if (base->color_mode != COLOR_MODE_SOLID && j < limit)
            {
                gfloat t = j / (gfloat) (base->color_mode == COLOR_MODE_GRADIENT ? nrj : limit);
                xfce4::cairo_set_source_rgba (cr, mix_colors (t, base->colors[FG_COLOR3], base->colors[FG_COLOR2]));
                active_color = NULL;
            }
            else
            {
                const xfce4::RGBA *color = (j >= limit ? &base->colors[FG_COLOR1] : &base->colors[FG_COLOR2]);
                if (active_color != color)
                {
                    xfce4::cairo_set_source_rgba (cr, *color);
                    active_color = color;
                }
            }

            /* draw rectangle */
            if (horizontal) {
                cairo_rectangle (cr, i * 3, j * 2, 2, 1);
            }
            else {
                cairo_rectangle (cr, j * 2, i * 3, 1, 2);
            }
            cairo_fill (cr);
        }
    }
}

void
draw_graph_no_history (const shared_ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    bool horizontal = (xfce_panel_plugin_get_orientation (base->plugin) == GTK_ORIENTATION_HORIZONTAL);

    const CpuLoad &load = base->history.data[core][base->history.offset];
    draw_graph_helper (base, load, cr, 0, horizontal ? w : h, horizontal ? h : w);
}

void
draw_graph_grid (const shared_ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    bool horizontal = (xfce_panel_plugin_get_orientation (base->plugin) == GTK_ORIENTATION_HORIZONTAL);

    gint span = horizontal ? w : h;
    gint breadth = horizontal ? h : w;

    const gfloat thickness = 1.75f;
    const gint64 step = 1000 * (gint64) get_update_interval_ms (base->update_interval);
    auto &nearest = base->nearest_cache;
    ensure_vector_size (nearest, span);

    gint64 t0 = base->history.data[core][base->history.offset].timestamp;
    nearest_loads (base, core, t0, -step, span, nearest.data());

    cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

    /* Paint the grid using a single call to cairo_stroke() */
    if (G_LIKELY (!base->colors[FG_COLOR1].is_transparent())) {
        cairo_save (cr);
        cairo_set_line_width (cr, 1);
        xfce4::cairo_set_source_rgba (cr, base->colors[FG_COLOR1]);
        for (gint i = 0; i < span; i += 6)
        {
            gint i1 = i;

            if (base->non_linear)
            {
                i1 *= pow (1.02, i1);
                if (i1 >= span)
                    break;
            }

            /* draw perpendicular line */
            if (horizontal) {
                cairo_move_to (cr, span - 1 - i1 + 0.5, 0.5);
                cairo_line_to (cr, span - 1 - i1 + 0.5, breadth - 1 + 0.5);
            }
            else {
                cairo_move_to (cr, 0.5, span - 1 - i1 + 0.5);
                cairo_line_to (cr, breadth - 1 + 0.5, span - 1 - i1 + 0.5);
            }
        }
        for (gint j = 0; j < breadth; j += 4)
        {
            /* draw parallel line */
            if (horizontal) {
                cairo_move_to (cr, 0.5, breadth - 1 - j + 0.5);
                cairo_line_to (cr, span - 1  + 0.5, breadth - 1 - j + 0.5);
            }
            else {
                cairo_move_to (cr, breadth - 1 - j + 0.5, 0.5);
                cairo_line_to (cr, breadth - 1 - j + 0.5, span - 1  + 0.5);
            }
        }
        cairo_stroke (cr);
        cairo_restore (cr);
    }

    /* Paint a line on top of the grid, using a single call to cairo_stroke() */
    if (G_LIKELY (!base->colors[2].is_transparent())) {
        Point last;

        cairo_save (cr);
        cairo_set_line_width (cr, thickness);
        xfce4::cairo_set_source_rgba (cr, base->colors[2]);
        for (gint i = 0; i < span; i++)
        {
            gfloat usage = 0.0f;
            if (const CpuLoad *loadPtr = nearest[horizontal ? span - 1 - i : i])
            {
                if (loadPtr->value >= base->load_threshold)
                    usage = breadth * loadPtr->value;
            }

            Point current;
            if (horizontal) {
                current = Point(i, breadth + (thickness-1)/2 - usage);
            }
            else {
                current = Point(breadth + (thickness-1)/2 - usage, i);
            }

            if (i == 0)
                last = current;

            /* draw line */
            cairo_move_to (cr, last.x + 0.5, last.y + 0.5);
            cairo_line_to (cr, current.x + 0.5, current.y + 0.5);
            last = current;
        }
        cairo_stroke (cr);
        cairo_restore (cr);
    }
}
