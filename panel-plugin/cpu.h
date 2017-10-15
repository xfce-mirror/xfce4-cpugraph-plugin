/*  cpu.h
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
#ifndef _XFCE_CPU_H_
#define _XFCE_CPU_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfce4panel/xfce-panel-plugin.h>

#include "os.h"

#define BORDER  8

typedef struct
{
	/* GUI components */
	XfcePanelPlugin *plugin;
	GtkWidget *frame_widget;
	GtkWidget *draw_area;
	GtkWidget *box;
	GtkWidget **bars;
	GtkWidget *color_buttons[5];
	GtkWidget *tooltip_text;

	/* Settings */
	guint update_interval; /* Number of ms between updates. */
	gboolean non_linear;
	guint size;
	guint mode;
	guint color_mode;
	gboolean has_frame;
	gboolean has_border;
	gboolean has_bars;
	gboolean has_barcolor;
	gchar  *command;
	gboolean in_terminal;
	gboolean startup_notification;
	GdkRGBA colors[5];
	guint tracked_core;

	/* Runtime data */
	guint nr_cores;
	guint timeout_id;
	guint *history;
	gssize history_size;
	CpuData *cpu_data;
} CPUGraph;

void set_startup_notification( CPUGraph *base, gboolean startup_notification );
void set_in_terminal( CPUGraph *base, gboolean in_terminal );
void set_command( CPUGraph *base, const gchar *command );
void set_bars( CPUGraph * base, gboolean bars);
void set_border( CPUGraph *base, gboolean border);
void set_frame( CPUGraph *base, gboolean frame );
void set_nonlinear_time( CPUGraph *base, gboolean nonlinear );
void set_update_rate( CPUGraph *base, guint rate );
void set_size( CPUGraph *base, guint width );
void set_color_mode( CPUGraph *base, guint color_mode );
void set_mode( CPUGraph *base, guint mode );
void set_color( CPUGraph *base, guint number, GdkRGBA color );
void set_tracked_core( CPUGraph *base, guint core );
#endif /* !_XFCE_CPU_H_ */
