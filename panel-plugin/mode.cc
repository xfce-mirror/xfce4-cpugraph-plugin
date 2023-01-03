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
#include "FukushimaLambertW.h"
#include "LambertW.h"

struct Point
{
    gfloat x, y;
    Point() : x(0), y(0) {}
    Point(gfloat _x, gfloat _y) : x(_x), y(_y) {}
};

static xfce4::RGBA
mix_colors (gdouble ratio, const xfce4::RGBA &color1, const xfce4::RGBA &color2)
{
    return color1 + ratio * (color2 - color1);
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
nearest_loads (const Ptr<const CPUGraph> &base, const guint core, const gint64 start, const gint64 step, const gssize _count, gfloat *out)
{
    const gssize history_cap_pow2 = base->history.cap_pow2;
    const CpuLoad *history_data = base->history.data[core];
    const gssize history_mask = base->history.mask();
    const gssize history_offset = base->history.offset;
    const gssize count =_count * base->scale;

    if (!base->non_linear)
    {
        gssize i, j;
        for (i = 0, j = 0; i < count; i++)
        {
            const gint64 timestamp = start + i * step;
            CpuLoad nearest = {};
            for (; j < history_cap_pow2; j++)
            {
                CpuLoad load = history_data[(history_offset + j) & history_mask];

                if (load.timestamp == 0)
                    goto end;

                if (nearest.timestamp == 0)
                {
                    nearest = load;
                }
                else
                {
                    gint64 delta = labs (load.timestamp - timestamp);
                    if (delta < labs (nearest.timestamp - timestamp))
                    {
                        nearest = load;
                    }
                    else if (delta > labs (nearest.timestamp - timestamp))
                    {
                        if (j > 0)
                            j--;
                        break;
                    }
                }
            }
            out[i] = nearest.value;
        }

    end:
        for (; i < count; i++)
            out[i] = 0;
    }
    else
    {
        double pows[count + 1];
        pows[0] = 1;
        pows[1] = base->scale == 1 ? NONLINEAR_MODE_BASE : pow (NONLINEAR_MODE_BASE, 1.0/base->scale);
        for (int p = 1; p < (count + 1) / 2; p++) // Multiply first factors half by itself
        {
            pows[p*2+0] = pows[p] * pows[p+0];
            pows[p*2+1] = pows[p] * pows[p+1];    // and by self+1 (for odd factors)
        }
        if (!( count % 1))                        // Handle trailing even factor
            pows[count] = pows[count / 2] * pows[count / 2];

        // Premultiplied sub-step per device pixels, when in scale
        const double f_step = step / (double)base->scale;

        for (gssize i = 0; i < count; i++)
        {
            /* Note: step < 0, therefore: timestamp1 < timestamp0 */
            const gint64 timestamp0 = base->scale > 1 ? (start + f_step * (i+0) * pows[i+0]) : (start + step * (i+0) * pows[i+0]);
            const gint64 timestamp1 = base->scale > 1 ? (start + f_step * (i+1) * pows[i+1]) : (start + step * (i+0) * pows[i+0]);
            gfloat sum = 0;
            gint num_loads = 0;

            for (gssize j = 0; j < history_cap_pow2; j++)
            {
                CpuLoad load = history_data[(history_offset + j) & history_mask];
                if (load.timestamp > timestamp1 && load.timestamp <= timestamp0)
                {
                    sum += load.value;
                    num_loads++;
                }
                else if (load.timestamp < timestamp1)
                    break;
            }

            /* count==0 in the following cases:
             *  - Both timestamps are pointing to a time before the CPU load measurements have started
             *  - Both timestamps are pointing to a time before the history has been cleared
             *  - There has been a change in base->update_interval,
             *    for example from RATE_SLOWEST to RATE_FASTEST
             */

            if (num_loads != 0)
                out[i] = sum / num_loads;
            else
                out[i] = -1;
        }

        for (gssize i = 0; i < count; i++)
        {
            if (out[i] == -1)
            {
                gfloat prev = -1, next = -1;

                for (gssize j = 0; j < i; j++)
                    if (out[j] != -1)
                    {
                        prev = out[j];
                        break;
                    }

                for (gssize j = i+1; j < count; j++)
                    if (out[j] != -1)
                    {
                        next = out[j];
                        break;
                    }

                if (prev != -1 && next != -1)
                    out[i] = (prev + next) / 2;
                else
                    out[i] = 0;
            }
        }
    }
}

void
draw_graph_normal (const Ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    const gint64 step = 1000 * (gint64) get_update_interval_ms (base->update_interval);
    gfloat nearest[w * base->scale];

    if (base->color_mode == 0)
        xfce4::cairo_set_source (cr, base->colors[FG_COLOR1]);

    gint64 t0 = base->history.data[core][base->history.offset].timestamp;
    nearest_loads (base, core, t0, -step, w, nearest);

    for (gint x = 0; x < w * base->scale; x++)
    {
        gfloat load = nearest[w * base->scale - 1 - x];
        if (load < base->load_threshold)
            load = 0;
        gfloat usage = h * load;

        if (usage == 0)
            continue;

        if (base->color_mode == 0)
        {
            /* draw line */
            cairo_rectangle (cr, x / (double)base->scale, h - usage, 1 / (double)base->scale, usage);
            cairo_fill (cr);
        }
        else
        {
            const gint h_usage = h - (gint) roundf (usage);
            gint tmp = 0;
            for (gint y = h - 1; y >= h_usage; y--, tmp++)
            {
                gfloat t = tmp / (base->color_mode == 1 ? (gfloat) h : usage);
                xfce4::cairo_set_source (cr, mix_colors (t, base->colors[FG_COLOR1], base->colors[FG_COLOR2]));
                /* draw point */
                cairo_rectangle (cr, x / (double)base->scale, y, 1 / (double)base->scale, 1);
                cairo_fill (cr);
            }
        }
    }
}

void
draw_graph_LED (const Ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    const gint nrx = (w + 2) / 3;
    const gint nry = (h + 1) / 2;
    const xfce4::RGBA *active_color = NULL;
    const gint64 step = 1000 * (gint64) get_update_interval_ms (base->update_interval);
    gfloat nearest[nrx * base->scale];

    gint64 t0 = base->history.data[core][base->history.offset].timestamp;
    nearest_loads (base, core, t0, -step, nrx, nearest);

    for (gint x = 0; x * 3 < w; x++)
    {
        const gint idx = nrx - x - 1;
        gint limit;

        if (G_LIKELY (idx >= 0 && idx < nrx))
        {
            gfloat load = 0;
            for (int i=0; i < base->scale; i++)
                load += nearest[idx * 3 + i];
            load /= base->scale;
            if (load < base->load_threshold)
                load = 0;
            limit = nry - (gint) roundf (nry * load);
        }
        else
            limit = nry - 0;

        for (gint y = 0; y * 2 < h; y++)
        {
            if (base->color_mode != 0 && y < limit)
            {
                gfloat t = y / (gfloat) (base->color_mode == 1 ? nry : limit);
                xfce4::cairo_set_source (cr, mix_colors (t, base->colors[FG_COLOR3], base->colors[FG_COLOR2]));
                active_color = NULL;
            }
            else
            {
                const xfce4::RGBA *color = (y >= limit ? &base->colors[FG_COLOR1] : &base->colors[FG_COLOR2]);
                if (active_color != color)
                {
                    xfce4::cairo_set_source (cr, *color);
                    active_color = color;
                }
            }

            /* draw rectangle */
            cairo_rectangle (cr, x * 3, y * 2, 2, 1);
            cairo_fill (cr);
        }
    }
}

void
draw_graph_no_history (const Ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    gfloat usage = base->history.data[core][base->history.offset].value;

    if (usage < base->load_threshold)
        usage = 0;

    usage *= h;

    if (base->color_mode == 0)
    {
        xfce4::cairo_set_source (cr, base->colors[FG_COLOR1]);
        cairo_rectangle (cr, 0, h - usage, w, usage);
        cairo_fill (cr);
    }
    else
    {
        const gint h_usage = h - (gint) roundf (usage);
        gint tmp = 0;
        for (gint y = h - 1; y >= h_usage; y--, tmp++)
        {
            gfloat t = tmp / (base->color_mode == 1 ? (gfloat) h : usage);
            xfce4::cairo_set_source (cr, mix_colors (t, base->colors[FG_COLOR1], base->colors[FG_COLOR2]));
            /* draw line */
            cairo_rectangle (cr, 0, y, w, 1);
            cairo_fill (cr);
        }
    }
}

void
draw_graph_grid (const Ptr<CPUGraph> &base, cairo_t *cr, gint w, gint h, guint core)
{
    if (G_UNLIKELY (core >= base->history.data.size()))
        return;

    const gfloat thickness = 1.75f / base->scale;
    const gint64 step = 1000 * (gint64) get_update_interval_ms (base->update_interval);
    gfloat nearest[w * base->scale];

    gint64 t0 = base->history.data[core][base->history.offset].timestamp;
    nearest_loads (base, core, t0, -step, w, nearest);

    cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_BEVEL);

    /* Paint the grid using a single call to cairo_stroke() */
    if (G_LIKELY (!base->colors[FG_COLOR1].isTransparent())) {
        cairo_save (cr);
        cairo_set_line_width (cr, 1);
        xfce4::cairo_set_source (cr, base->colors[FG_COLOR1]);
        
        double ln1_04 = 0;
        for (gint x = 0; x < w; x += 6)
        {
            double x1 = x;

            if (base->non_linear && x)
            {
                if (ln1_04 == 0)
                    ln1_04 = log (1.04);
                // Start with logarithmic scale.
                // First non-zero tick is at 6px.
                x1 = pow (6 * (1.04*1.04*1.04) * (1.04*1.04*1.04), x1 / 12 + 0.5);
                // Convert tick position to 'x*pow(c,x)' kind of space
                x1 = (x1 > 399049500000000000000000000000 ? utl::LambertW (0, x1 * ln1_04) : Fukushima::LambertW (0, x1 * ln1_04)) / ln1_04;
                x1 -= remainder (x1, (1.0 / base->scale));
                if (x1 >= w)
                    break;
            }

            /* draw vertical line */
            cairo_move_to (cr, w - 1 - x1 + 0.5, 0.5);
            cairo_rel_line_to (cr, 0, h-1);
        }
        for (gint y = h-1; y >= 0; y -= 4)
        {
            /* draw horizontal line */
            cairo_move_to (cr, 0.5, y + 0.5);
            cairo_rel_line_to (cr, w-1, 0);
        }
        cairo_stroke (cr);
        cairo_restore (cr);
    }

    /* Paint a line on top of the grid, using a single call to cairo_stroke() */
    if (G_LIKELY (!base->colors[2].isTransparent())) {
        cairo_save (cr);
        cairo_set_line_width (cr, thickness);
        xfce4::cairo_set_source (cr, base->colors[2]);
        for (gint x = 0; x < w * base->scale; x++)
        {
            gfloat load = nearest[w * base->scale - 1 - x];
            if (load < base->load_threshold)
                load = 0;
            gfloat usage = h * load;

            Point current(x / (double)base->scale, h + (thickness-1)/2 - usage);
            if (x == 0)
                /* start new polyline */
                cairo_move_to (cr, current.x + 0.5, current.y + 0.5);
            else
                /* add polyline segment */
                cairo_line_to (cr, current.x + 0.5, current.y + 0.5);
        }
        cairo_stroke (cr);
        cairo_restore (cr);
    }
}
