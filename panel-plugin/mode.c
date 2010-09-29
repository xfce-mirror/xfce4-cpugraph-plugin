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

#include "mode.h"

static guint16 _lerp( gdouble t, guint16 a, guint16 b )
{
	return (guint16) (a + t * (b - a));
}

static void mix_colors( gdouble ratio, GdkColor *color1, GdkColor *color2, GdkGC *target )
{
	GdkColor color;
	color.red = _lerp (ratio, color1->red, color2->red);
	color.green = _lerp (ratio, color1->green, color2->green);
	color.blue = _lerp (ratio, color1->blue, color2->blue);
	gdk_gc_set_rgb_fg_color( target, &color );
}

void draw_graph_normal( CPUGraph *base, GtkWidget *da, gint w, gint h )
{
	gint x, y;
	gint usage;
	gdouble t;
	gint tmp;
	GdkGC *fg1 = gdk_gc_new( da->window );

	if( base->color_mode == 0 )
		gdk_gc_set_rgb_fg_color( fg1, &base->colors[1] );

	for( x = 0; x < w; x++ )
	{
		usage = h * base->history[w - 1- x] / CPU_SCALE;

		if( usage == 0 ) continue;

		if( base->color_mode == 0 )
		{
			gdk_draw_line( da->window, fg1, x, h-usage, x, h-1 );
		}
		else
		{
			tmp = 0;
			for( y = h-1; y >= h - usage; y--, tmp++ )
			{
				t = (base->color_mode == 1) ?
					(tmp / (gdouble) (h)) :
					(tmp / (gdouble) (usage));
				mix_colors( t, &base->colors[1], &base->colors[2], fg1);
				gdk_draw_point( da->window, fg1, x, y );
			}
		}
	}
	g_object_unref( fg1 );
}

void draw_graph_LED( CPUGraph *base, GtkWidget *da, gint w, gint h )
{
	gint nrx = (w + 1) / 3;
	gint nry = (h + 1) / 2;
	gint x, y;
	gint idx;
	gint limit;

	GdkGC *fg1 = gdk_gc_new( da->window );
	GdkGC *fg2 = gdk_gc_new( da->window );
	gdk_gc_set_rgb_fg_color( fg1, &base->colors[1] );
	gdk_gc_set_rgb_fg_color( fg2, &base->colors[2] );

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
				mix_colors( t, &base->colors[3], &base->colors[2], fg2);
			}
			gdk_draw_rectangle (da->window, y >= limit ? fg1 : fg2, TRUE, x * 3, y * 2, 2, 1);
		}
	}
	g_object_unref( fg1 );
	g_object_unref( fg2 );
}

void draw_graph_no_history( CPUGraph *base, GtkWidget *da, gint w, gint h )
{
	gint y;
	gint usage = h * base->history[0] / CPU_SCALE;
	gint tmp = 0;
	gdouble t;
	GdkGC *fg1 = gdk_gc_new( da->window );

	if( base->color_mode == 0 )
	{
		gdk_gc_set_rgb_fg_color( fg1, &base->colors[1] );
		gdk_draw_rectangle( da->window, fg1, TRUE, 0, h-usage, w, usage );
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
			gdk_draw_line( da->window, fg1, 0, y, w-1, y );
		}
	}
	g_object_unref( fg1 );
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
	GdkGC *fg1 = gdk_gc_new( da->window );

	gdk_gc_set_rgb_fg_color( fg1, &base->colors[1] );
	for( x = 0; x * 6 < w; x++ )
	{
		gdk_draw_line( da->window, fg1, x*6, 0, x*6, h-1 );
	}
	for( y = 0; y * 4 < h; y++ )
	{
		gdk_draw_line( da->window, fg1, 0, y*4, w-1, y*4 );
	}

	gdk_gc_set_rgb_fg_color( fg1, &base->colors[2] );
	for( x = 0; x < w; x++ )
	{
		usage = h * base->history[w - 1- x] / CPU_SCALE;
		current.x = x;
		current.y = h - usage;
		gdk_draw_line( da->window, fg1, current.x, current.y, last.x, last.y );
		last = current;
	}
	g_object_unref( fg1 );
}
