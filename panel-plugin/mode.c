/*  mode.c
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

#include <cairo/cairo.h>
#include "mode.h"

typedef struct
{
    gint x;
    gint y;
} point;

static gdouble
_lerp (gdouble t, gdouble a, gdouble b)
{
    return (gdouble) (a + t * (b - a));
}

static void
mix_colors (gdouble ratio, GdkRGBA *color1, GdkRGBA *color2, cairo_t *target)
{
    GdkRGBA color;
    color.red = _lerp (ratio, color1->red, color2->red);
    color.green = _lerp (ratio, color1->green, color2->green);
    color.blue = _lerp (ratio, color1->blue, color2->blue);
    color.alpha = 1.0;
    gdk_cairo_set_source_rgba (target, &color);
}

void
draw_graph_normal (CPUGraph *base, cairo_t *cr, gint w, gint h)
{
    gint x, y;
    gint usage;
    gdouble t;
    gint tmp;

    if (base->color_mode == 0)
        gdk_cairo_set_source_rgba (cr, &base->colors[1]);

    for (x = 0; x < w; x++)
    {
        usage = h * base->history[w - 1- x] / CPU_SCALE;

        if (usage == 0)
            continue;

        if (base->color_mode == 0)
        {
            /* draw line */
            cairo_rectangle (cr, x, h - usage, 1, usage);
            cairo_fill (cr);
        }
        else
        {
            tmp = 0;
            for (y = h - 1; y >= h - usage; y--, tmp++)
            {
                t = (base->color_mode == 1) ?
                    (tmp / (gdouble) (h)) :
                    (tmp / (gdouble) (usage));
                mix_colors (t, &base->colors[1], &base->colors[2], cr);
                /* draw point */
                cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
                cairo_move_to (cr, x, y);
                cairo_close_path (cr);
                cairo_stroke (cr);
            }
        }
    }
}

void
draw_graph_LED (CPUGraph *base, cairo_t *cr, gint w, gint h)
{
    gint nrx = (w + 1) / 3;
    gint nry = (h + 1) / 2;
    gint x, y;
    gint idx;
    gint limit;

    for (x = 0; x * 3 < w; x++)
    {
        idx = nrx - x;
        limit = nry - nry * base->history[idx] / CPU_SCALE;

        for (y = 0; y * 2 < h; y++)
        {
            if (base->color_mode != 0 && y < limit)
            {
                gdouble t = (base->color_mode == 1) ?
                            (y / (gdouble) nry) :
                            (y / (gdouble) limit);
                mix_colors (t, &base->colors[3], &base->colors[2], cr);
            }
            else
            {
                gdk_cairo_set_source_rgba (cr, y >= limit ? &base->colors[1] : &base->colors[2]);
            }

            /* draw rectangle */
            cairo_rectangle (cr, x * 3, y * 2, 2, 1);
            cairo_fill (cr);
        }
    }
}

void
draw_graph_no_history (CPUGraph *base, cairo_t *cr, gint w, gint h)
{
    gint y;
    gint usage = h * base->history[0] / CPU_SCALE;
    gint tmp = 0;
    gdouble t;

    if (base->color_mode == 0)
    {
        gdk_cairo_set_source_rgba (cr, &base->colors[1]);
        cairo_rectangle (cr, 0, h - usage, w, usage);
        cairo_fill (cr);
    }
    else
    {
        for (y = h - 1; y > h - 1 - usage; y--)
        {
            t = (base->color_mode == 1) ?
                (tmp / (gdouble) (h)) :
                (tmp / (gdouble) (usage));
            mix_colors (t, &base->colors[1], &base->colors[2], cr);
            tmp++;

            /* draw line */
            cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
            cairo_move_to (cr, 0, y);
            cairo_line_to (cr, w -1, y);
            cairo_stroke (cr);
        }
    }
}

void
draw_graph_grid (CPUGraph *base, cairo_t *cr, gint w, gint h)
{
    gint x, y;
    gint usage;
    point last, current;
    last.x = 0;
    last.y = h;

    gdk_cairo_set_source_rgba (cr, &base->colors[1]);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
    cairo_set_line_width (cr, 1);

    for (x = 0; x * 6 < w; x++)
    {
        /* draw line */
        cairo_move_to (cr, x * 6 + 0.5, 0.5);
        cairo_line_to (cr, x * 6 + 0.5, h - 1 + 0.5);
        cairo_stroke (cr);
    }

    for (y = 0; y * 4 < h; y++)
    {
        /* draw line */
        cairo_move_to (cr, 0.5, y * 4 + 0.5);
        cairo_line_to (cr, w - 1  + 0.5, y * 4 + 0.5);
        cairo_stroke (cr);
    }

    gdk_cairo_set_source_rgba (cr, &base->colors[2]);

    for (x = 0; x < w; x++)
    {
        usage = h * base->history[w - 1 - x] / CPU_SCALE;
        current.x = x;
        current.y = h - usage;

        /* draw line */
        cairo_move_to (cr, current.x + 0.5, current.y + 0.5);
        cairo_line_to (cr, last.x + 0.5, last.y + 0.5);
        cairo_stroke (cr);
        last = current;
    }
}
