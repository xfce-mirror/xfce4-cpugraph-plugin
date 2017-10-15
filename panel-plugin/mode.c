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

static guint16 _lerp( gdouble t, guint16 a, guint16 b )
{
	return (guint16) (a + t * (b - a));
}

static void mix_colors( gdouble ratio, GdkRGBA *color1, GdkRGBA *color2, cairo_t *target )
{
	GdkRGBA color;
	color.red = _lerp (ratio, color1->red, color2->red);
	color.green = _lerp (ratio, color1->green, color2->green);
	color.blue = _lerp (ratio, color1->blue, color2->blue);
	color.alpha = 1.0;
	gdk_cairo_set_source_rgba( target, &color );
}

void draw_graph_normal( CPUGraph *base, GtkWidget *da, gint w, gint h )
{
	gint x, y;
	gint usage;
	gdouble t;
	gint tmp;
	cairo_t *fg1 = gdk_cairo_create( gtk_widget_get_window( da ) );

	if( base->color_mode == 0 )
		gdk_cairo_set_source_rgba( fg1, &base->colors[1] );

	for( x = 0; x < w; x++ )
	{
		usage = h * base->history[w - 1- x] / CPU_SCALE;

		if( usage == 0 ) continue;

		if( base->color_mode == 0 )
		{
			/* draw line */
			cairo_set_line_cap( fg1, CAIRO_LINE_CAP_SQUARE );
			cairo_move_to( fg1, x, h - usage );
			cairo_line_to( fg1, x, h - 1 );
			cairo_stroke( fg1 );
		}
		else
		{
			tmp = 0;
			for( y = h-1; y >= h - usage; y--, tmp++ )
			{
				t = (base->color_mode == 1) ?
					(tmp / (gdouble) (h)) :
					(tmp / (gdouble) (usage));
				mix_colors( t, &base->colors[1], &base->colors[2], fg1 );
				/* draw point */
				cairo_set_line_cap( fg1, CAIRO_LINE_CAP_SQUARE );
				cairo_move_to( fg1, x, y );
				cairo_stroke( fg1 );
			}
		}
	}
	cairo_destroy( fg1 );
}

void draw_graph_LED( CPUGraph *base, GtkWidget *da, gint w, gint h )
{
	gint nrx = (w + 1) / 3;
	gint nry = (h + 1) / 2;
	gint x, y;
	gint idx;
	gint limit;

	cairo_t *fg1 = gdk_cairo_create( gtk_widget_get_window( da ) );
	cairo_t *fg2 = gdk_cairo_create( gtk_widget_get_window( da ) );
	gdk_cairo_set_source_rgba( fg1, &base->colors[1] );
	gdk_cairo_set_source_rgba( fg2, &base->colors[2] );

	for( x = 0; x * 3 < w; x++ )
	{
		idx = nrx-x;
		limit = nry - nry * base->history[idx]/CPU_SCALE;
		for( y = 0; y * 2 < h; y++ )
		{
			if( base->color_mode != 0 && y < limit )
			{
				gdouble t = (base->color_mode == 1) ?
				           (y / (gdouble)nry) :
				           (y / (gdouble)limit);
				mix_colors( t, &base->colors[3], &base->colors[2], fg2 );
			}
			/* draw rectangle */
			cairo_t *fg = y >= limit ? fg1 : fg2;
			cairo_rectangle( fg, x * 3, y * 2, 2, 1 );
			cairo_fill( fg );
		}
	}
	cairo_destroy( fg1 );
	cairo_destroy( fg2 );
}

void draw_graph_no_history( CPUGraph *base, GtkWidget *da, gint w, gint h )
{
	gint y;
	gint usage = h * base->history[0] / CPU_SCALE;
	gint tmp = 0;
	gdouble t;
	cairo_t *fg1 = gdk_cairo_create( gtk_widget_get_window( da ) );

	if( base->color_mode == 0 )
	{
		gdk_cairo_set_source_rgba( fg1, &base->colors[1] );
		cairo_rectangle( fg1, 0, h - usage, w, usage );
		cairo_fill( fg1 );
	}
	else
	{
		for( y = h-1; y > h - 1 - usage; y-- )
		{
			t = (base->color_mode == 1) ?
				(tmp / (gdouble) (h)) :
				(tmp / (gdouble) (usage));
			mix_colors( t, &base->colors[1], &base->colors[2], fg1 );
			tmp++;
			/* draw line */
			cairo_set_line_cap( fg1, CAIRO_LINE_CAP_SQUARE );
			cairo_move_to( fg1, 0, y );
			cairo_line_to( fg1, w -1, y );
			cairo_stroke( fg1 );
		}
	}
	cairo_destroy( fg1 );
}

typedef struct
{
	gint x;
	gint y;
} point;

void draw_graph_grid( CPUGraph *base, GtkWidget *da, gint w, gint h )
{
	gint x, y;
	gint usage;
	point last, current;
	last.x = 0;
	last.y = h;
	cairo_t *fg1 = gdk_cairo_create( gtk_widget_get_window( da ) );

	gdk_cairo_set_source_rgba( fg1, &base->colors[1] );
	for( x = 0; x * 6 < w; x++ )
	{
		/* draw line */
		cairo_set_line_cap( fg1, CAIRO_LINE_CAP_SQUARE );
		cairo_move_to( fg1, x * 6, 0 );
		cairo_line_to( fg1, x * 6,  h - 1 );
		cairo_stroke( fg1 );
	}
	for( y = 0; y * 4 < h; y++ )
	{
		/* draw line */
		cairo_set_line_cap( fg1, CAIRO_LINE_CAP_SQUARE );
		cairo_move_to( fg1, 0, y * 4 );
		cairo_line_to( fg1, w - 1,  y * 4 );
		cairo_stroke( fg1 );
	}

	gdk_cairo_set_source_rgba( fg1, &base->colors[2] );
	for( x = 0; x < w; x++ )
	{
		usage = h * base->history[w - 1- x] / CPU_SCALE;
		current.x = x;
		current.y = h - usage;
		/* draw line */
		cairo_set_line_cap( fg1, CAIRO_LINE_CAP_SQUARE );
		cairo_move_to( fg1, current.x, current.y );
		cairo_line_to( fg1, last.x,  last.y );
		cairo_stroke( fg1 );
		last = current;
	}
	cairo_destroy( fg1 );
}
