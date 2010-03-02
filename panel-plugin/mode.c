/*  mode.c
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "mode.h"

static guint16 _lerp( double t, guint16 a, guint16 b )
{
	return (guint16) (a + t * (b - a));
}

static void MixAndApplyColors( double ratio, GdkColor *color1, GdkColor *color2, GdkGC *target )
{
	GdkColor color;
	color.red = _lerp (ratio, color1->red, color2->red);
	color.green = _lerp (ratio, color1->green, color2->green);
	color.blue = _lerp (ratio, color1->blue, color2->blue);
	gdk_gc_set_rgb_fg_color( target, &color );
}

void drawGraphModeNormal( CPUGraph *base, GtkWidget *da, int w, int h )
{
	int x, y;
	long usage;
	double t;
	int tmp;
	GdkGC *fg1 = gdk_gc_new( da->window );

	if( base->m_ColorMode == 0 )
		gdk_gc_set_rgb_fg_color( fg1, &base->colors[1] );

	for( x = 0; x < w; x++ )
	{
		usage = h * base->m_History[w - 1- x] / CPU_SCALE;

		if( usage == 0 ) continue;

		if( base->m_ColorMode == 0 )
		{
			gdk_draw_line( da->window, fg1, x, h-usage, x, h-1 );
		}
		else
		{
			tmp = 0;
			for( y = h-1; y >= h - usage; y--, tmp++ )
			{
				t = (base->m_ColorMode == 1) ?
					(tmp / (double) (h)) :
					(tmp / (double) (usage));
				MixAndApplyColors( t, &base->colors[1], &base->colors[2], fg1);
				gdk_draw_point( da->window, fg1, x, y );
			}
		}
	}
	g_object_unref( fg1 );
}

void drawGraphModeLED( CPUGraph *base, GtkWidget *da, int w, int h )
{
	int nrx = (w + 1) / 3;
	int nry = (h + 1) / 2;
	int x, y;

	GdkGC *fg1 = gdk_gc_new( da->window );
	GdkGC *fg2 = gdk_gc_new( da->window );
	gdk_gc_set_rgb_fg_color( fg1, &base->colors[1] );
	gdk_gc_set_rgb_fg_color( fg2, &base->colors[2] );

	for( x = 0; x * 3 < w; x++ )
	{
		int idx = nrx-x;
		int limit = nry - nry * base->m_History[idx]/CPU_SCALE;
		for( y = 0; y * 2 < h; y++ )
		{
			if( base->m_ColorMode != 0 && y < limit )
			{
				double t = (base->m_ColorMode == 1) ?
				           (y / (double)nry) :
				           (y / (double)limit);
				MixAndApplyColors( t, &base->colors[3], &base->colors[2], fg2);
			}
			gdk_draw_rectangle (da->window, y >= limit ? fg1 : fg2, TRUE, x * 3, y * 2, 2, 1);
		}
	}
	g_object_unref( fg1 );
	g_object_unref( fg2 );
}

void drawGraphModeNoHistory( CPUGraph *base, GtkWidget *da, int w, int h )
{
	int y;
	long usage = h * base->m_History[0] / CPU_SCALE;
	int tmp = 0;
	double t;
	GdkGC *fg1 = gdk_gc_new( da->window );

	if( base->m_ColorMode == 0 )
	{
		gdk_gc_set_rgb_fg_color( fg1, &base->colors[1] );
		gdk_draw_rectangle( da->window, fg1, TRUE, 0, h-usage, w, usage );
	}
	else
	{
		for( y = h-1; y > h - 1 - usage; y-- )
		{
			t = (base->m_ColorMode == 1) ?
				(tmp / (double) (h)) :
				(tmp / (double) (usage));
			MixAndApplyColors( t, &base->colors[1], &base->colors[2], fg1 );
			tmp++;
			gdk_draw_line( da->window, fg1, 0, y, w-1, y );
		}
	}
	g_object_unref( fg1 );
}

typedef struct
{
	long x;
	long y;
} point;

void drawGraphGrid( CPUGraph *base, GtkWidget *da, int w, int h )
{
	int x, y;
	long usage;
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
		usage = h * base->m_History[w - 1- x] / CPU_SCALE;
		current.x = x;
		current.y = h - usage;
		gdk_draw_line( da->window, fg1, current.x, current.y, last.x, last.y );
		last = current;
	}
	g_object_unref( fg1 );
}
