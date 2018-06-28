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
#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

static void cpugraph_construct( XfcePanelPlugin *plugin );
static CPUGraph *create_gui( XfcePanelPlugin *plugin );
static void create_bars( CPUGraph *base );
static guint init_cpu_data( CpuData **data );
static void shutdown( XfcePanelPlugin *plugin, CPUGraph *base );
static void delete_bars( CPUGraph *base );
static gboolean size_cb( XfcePanelPlugin *plugin, guint size, CPUGraph *base );
static void about_cb( XfcePanelPlugin *plugin, CPUGraph *base );
static void set_bars_size( CPUGraph *base, gint size, GtkOrientation orientation );
static void mode_cb( XfcePanelPlugin *plugin, XfcePanelPluginMode mode, CPUGraph *base );
static void set_bars_orientation( CPUGraph *base, GtkOrientation orientation);
static gboolean update_cb( CPUGraph *base );
static void update_tooltip( CPUGraph *base );
static gboolean tooltip_cb( GtkWidget *widget, gint x, gint y, gboolean keyboard, GtkTooltip * tooltip, CPUGraph *base);
static void draw_area_cb( GtkWidget *da, GdkEventExpose *event, gpointer data );
static void draw_graph( CPUGraph *base );
static gboolean command_cb( GtkWidget *w, GdkEventButton *event, CPUGraph *base );

XFCE_PANEL_PLUGIN_REGISTER( cpugraph_construct );

static void cpugraph_construct( XfcePanelPlugin *plugin )
{
	CPUGraph *base;

	xfce_textdomain( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8" );

	base = create_gui( plugin );
	read_settings( plugin, base );
	xfce_panel_plugin_menu_show_configure( plugin );

	xfce_panel_plugin_menu_show_about( plugin );

	g_signal_connect( plugin, "about", G_CALLBACK (about_cb), base );
	g_signal_connect( plugin, "free-data", G_CALLBACK( shutdown ), base );
	g_signal_connect( plugin, "save", G_CALLBACK( write_settings ), base );
	g_signal_connect( plugin, "configure-plugin", G_CALLBACK( create_options ), base );
	g_signal_connect( plugin, "size-changed", G_CALLBACK( size_cb ), base );
	g_signal_connect( plugin, "mode-changed", G_CALLBACK( mode_cb ), base );
}

static CPUGraph * create_gui( XfcePanelPlugin * plugin )
{
	GtkWidget *frame, *ebox;
	GtkOrientation orientation;
	CPUGraph *base = g_new0( CPUGraph, 1 );

	orientation = xfce_panel_plugin_get_orientation(plugin);
	if( (base->nr_cores = init_cpu_data( &base->cpu_data )) == 0)
		fprintf(stderr,"Cannot init cpu data !\n");

	base->plugin = plugin;

	ebox = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(ebox), FALSE);
	gtk_event_box_set_above_child(GTK_EVENT_BOX(ebox), TRUE);
	gtk_container_add( GTK_CONTAINER( plugin ), ebox );
	xfce_panel_plugin_add_action_widget( plugin, ebox );
	g_signal_connect( ebox, "button-press-event", G_CALLBACK( command_cb ), base );

	base->box = gtk_box_new(orientation, 0);
	gtk_container_add(GTK_CONTAINER(ebox), base->box);
	gtk_widget_set_has_tooltip( base->box, TRUE);
	g_signal_connect( base->box, "query-tooltip", G_CALLBACK( tooltip_cb ), base );

	base->frame_widget = frame = gtk_frame_new( NULL );
	gtk_box_pack_end( GTK_BOX(base->box), frame, TRUE, TRUE, 0);

	base->draw_area = gtk_drawing_area_new();
	gtk_container_add( GTK_CONTAINER( frame ), GTK_WIDGET( base->draw_area ) );
	g_signal_connect_after( base->draw_area, "draw", G_CALLBACK( draw_area_cb ), base );

	base->has_bars = FALSE;
	base->has_barcolor = FALSE;
	base->bars = NULL;

	mode_cb(plugin, orientation, base);
	gtk_widget_show_all(ebox);

	base->tooltip_text = gtk_label_new( NULL );
	g_object_ref( base->tooltip_text );

	return base;
}

static void
about_cb( XfcePanelPlugin *plugin, CPUGraph *base )
{
	GdkPixbuf *icon;
	const gchar *auth[] = {
		"Alexander Nordfelth <alex.nordfelth@telia.com>", "gatopeich <gatoguan-os@yahoo.com>",
		"lidiriel <lidiriel@coriolys.org>","Angelo Miguel Arrifano <miknix@gmail.com>",
		"Florian Rivoal <frivoal@gmail.com>","Peter Tribble <peter.tribble@gmail.com>", NULL};
	icon = xfce_panel_pixbuf_from_source("xfce4-cpugraph-plugin", NULL, 32);
	gtk_show_about_dialog(NULL,
		"logo", icon,
		"license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
		"version", PACKAGE_VERSION,
		"program-name", PACKAGE_NAME,
		"comments", _("Graphical representation of the CPU load"),
		"website", "http://goodies.xfce.org/projects/panel-plugins/xfce4-cpugraph-plugin",
		"copyright", _("Copyright (c) 2003-2012\n"),
		"authors", auth, NULL);

	if(icon)
		g_object_unref(G_OBJECT(icon));
}

static guint nb_bars( CPUGraph * base )
{
	return base->tracked_core == 0 ? base->nr_cores : 1;
}
static void create_bars( CPUGraph *base )
{
	guint i;
	guint n;
	n = nb_bars( base );
	base->bars = (GtkWidget **) g_malloc( sizeof( GtkWidget * ) * n );

	for( i=0; i< n; i++ )
	{
		base->bars[i] = GTK_WIDGET(gtk_progress_bar_new());
		/* Set bar colors */
		if (base->has_barcolor) {
			gtk_widget_override_background_color(base->bars[i], GTK_STATE_PRELIGHT, &base->colors[4]);
			gtk_widget_override_background_color(base->bars[i], GTK_STATE_SELECTED, &base->colors[4]);
			gtk_widget_override_color(base->bars[i], GTK_STATE_SELECTED, &base->colors[4]);
		}
		gtk_box_pack_end( GTK_BOX(base->box), base->bars[i], FALSE, FALSE, 0 );
		gtk_widget_show( base->bars[i] );
	}
}

guint init_cpu_data( CpuData **data )
{
	guint cpuNr;

	cpuNr = detect_cpu_number();
	if( cpuNr == 0 )
		return 0;

	*data = (CpuData *) g_malloc0( (cpuNr+1) * sizeof( CpuData ) );

	return cpuNr;
}

static void shutdown( XfcePanelPlugin * plugin, CPUGraph * base )
{
	g_free( base->cpu_data );
	delete_bars( base );
	gtk_widget_destroy(base->box);
	gtk_widget_destroy(base->tooltip_text);
	if( base->timeout_id )
		g_source_remove( base->timeout_id );
	g_free( base->history );
	g_free( base->command );
	g_free( base );
}

static void delete_bars( CPUGraph *base )
{
	guint i;
	guint n;
	if( base->bars )
	{
		n = nb_bars( base );
		for( i=0; i < n; i++ )
		{
			gtk_widget_hide( base->bars[i] );
			gtk_widget_destroy( base->bars[i] );
		}
		g_free( base->bars );
		base->bars = NULL;
	}
}

static gboolean size_cb( XfcePanelPlugin *plugin, guint size, CPUGraph *base )
{
	gint frame_h, frame_v, history;
	GtkOrientation orientation;
	
	orientation = xfce_panel_plugin_get_orientation( plugin );

	if( orientation == GTK_ORIENTATION_HORIZONTAL )
	{
		frame_h = base->size;
		frame_v = size;
	       	history = base->size;
	}
	else
	{
		frame_h = size;
		frame_v = base->size;
	       	history = size;
	}

	gtk_widget_set_size_request( GTK_WIDGET( base->frame_widget ), frame_h, frame_v );

	base->history = (guint *) g_realloc( base->history, history * sizeof( guint ) );
	if( history > base->history_size )
		memset( base->history + base->history_size, 0, (history - base->history_size) * sizeof( guint ) );
	base->history_size = history;

	if( base->has_bars )
		set_bars_size( base, size, orientation );
	set_border( base, base->has_border );

	return TRUE;
}

static void set_bars_size( CPUGraph *base, gint size, GtkOrientation orientation )
{
	guint i;
	guint n;
	gint h, v;
	if( orientation == GTK_ORIENTATION_HORIZONTAL )
	{
		h = 8;
		v = -1;
	}
	else
	{
		h = -1;
		v = 8;
	}
	n = nb_bars( base );
	for( i=0; i < n ; i++ )
		gtk_widget_set_size_request( GTK_WIDGET(base->bars[i]), h, v );
}

static void mode_cb( XfcePanelPlugin * plugin, XfcePanelPluginMode mode, CPUGraph *base )
{
	GtkOrientation orientation = (mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL) ?
		GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

	gtk_orientable_set_orientation( GTK_ORIENTABLE( base->box ), xfce_panel_plugin_get_orientation (plugin));

	if( base->has_bars )
		set_bars_orientation( base, orientation );

	size_cb( plugin, xfce_panel_plugin_get_size( base->plugin ), base);
}

static void set_bars_orientation( CPUGraph *base, GtkOrientation orientation)
{
	guint i, n;

	n = nb_bars( base );
	for( i=0; i < n; i++ )
		gtk_orientable_set_orientation( GTK_ORIENTABLE( base->bars[i] ), orientation );	
}

static gboolean update_cb( CPUGraph * base )
{
	gint i, a, b, factor;

	if( !read_cpu_data( base->cpu_data, base->nr_cores ) )
		return TRUE;

	if( base->tracked_core > base->nr_cores )
		base->cpu_data[0].load = 0;
	else if( base->tracked_core != 0 )
		base->cpu_data[0].load = base->cpu_data[base->tracked_core].load;

	if( base->has_bars )
	{
		if( base->tracked_core != 0 || base->nr_cores == 1 )
		{
			gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(base->bars[0]),
					(gdouble)base->cpu_data[0].load / CPU_SCALE
					);
		}
		else
		{
			for( i=0; i<base->nr_cores; i++ )
				gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(base->bars[i]),
						(gdouble)base->cpu_data[i+1].load / CPU_SCALE
						);
		}
	}

	if( base->non_linear )
	{
		i = base->history_size - 1;
		while( i > 0 )
		{
			a = base->history[i], b = base->history[i-1];
			if( a < b ) a++;
			factor = (i*2);
			base->history[i--] = (a * (factor-1) + b) / factor;
		}
	} else {
		memmove( base->history + 1 , base->history , (base->history_size - 1) * sizeof( guint ) );
	}
	base->history[0] = base->cpu_data[0].load;

	update_tooltip( base );
	gtk_widget_queue_draw( base->draw_area );

	return TRUE;
}

static void update_tooltip( CPUGraph * base )
{
	gchar tooltip[32];
	g_snprintf( tooltip, 32, _("Usage: %u%%"), (guint)base->cpu_data[0].load*100/CPU_SCALE );
	gtk_label_set_text( GTK_LABEL(base->tooltip_text), tooltip );
}

static gboolean tooltip_cb( GtkWidget *widget, gint x, gint y, gboolean keyboard, GtkTooltip * tooltip, CPUGraph *base)
{
	gtk_tooltip_set_custom( tooltip, base->tooltip_text );
	return TRUE;
}

static void draw_area_cb( GtkWidget * da, GdkEventExpose * event, gpointer data )
{
	draw_graph( (CPUGraph *) data );
}

static void draw_graph( CPUGraph * base )
{
	GtkWidget *da = base->draw_area;
	GtkAllocation alloc;
	gint w, h;

	gtk_widget_get_allocation( da, &alloc );
	w = alloc.width;
	h = alloc.height;

	switch( base->mode )
	{
		case 0:
			draw_graph_normal( base, da, w, h );
			break;
		case 1:
			draw_graph_LED( base, da, w, h );
			break;
		case 2:
			draw_graph_no_history( base, da, w, h );
			break;
		case 3:
			draw_graph_grid(base, da, w, h);
			break;
	}
}

static gboolean command_cb( GtkWidget *w,GdkEventButton *event, CPUGraph *base )
{
	if( event->button == 1 && base->command )
	{
		xfce_spawn_command_line_on_screen( gdk_screen_get_default(), base->command, base->in_terminal, base->startup_notification, NULL );
	}
	return FALSE;
}

void set_startup_notification( CPUGraph *base, gboolean startup_notification )
{
	base->startup_notification = startup_notification;
}

void set_in_terminal( CPUGraph *base, gboolean in_terminal )
{
	base->in_terminal = in_terminal;
}

void set_command( CPUGraph *base, const gchar *command )
{
	g_free( base->command );
	base->command = g_strdup( command );
}

void set_bars( CPUGraph * base, gboolean bars)
{
	GtkOrientation orientation;
	if( base->has_bars != bars )
	{
		base->has_bars = bars;
		if(bars)
		{
			orientation = xfce_panel_plugin_get_orientation( base->plugin );
			create_bars( base );
			set_bars_orientation( base, orientation );
			set_bars_size( base, xfce_panel_plugin_get_size( base->plugin ), orientation );
		}
		else
			delete_bars( base );
	}
}

void set_border( CPUGraph *base, gboolean border )
{
	int border_width = (xfce_panel_plugin_get_size( base->plugin ) > 26 ? 2 : 1);
	base->has_border = border;
	if (!base->has_border)
		border_width = 0;
	gtk_container_set_border_width( GTK_CONTAINER( base->box ), border_width);
}

void set_frame( CPUGraph *base, gboolean frame )
{
	base->has_frame = frame;
	gtk_frame_set_shadow_type( GTK_FRAME( base->frame_widget ), base->has_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE );
}

void set_nonlinear_time( CPUGraph *base, gboolean nonlinear )
{
	base->non_linear = nonlinear;
}

void set_update_rate( CPUGraph *base, guint rate )
{
	guint update;

	base->update_interval = rate;

	if( base->timeout_id )
		g_source_remove( base->timeout_id );
	switch( base->update_interval )
	{
		case 0:
			update = 250;
			break;
		case 1:
			update = 500;
			break;
		case 2:
			update = 750;
			break;
		default:
			update = 1000;
	}
	base->timeout_id = g_timeout_add( update, (GSourceFunc) update_cb, base );
}

void set_size( CPUGraph *base, guint size )
{
	base->size = size;
	size_cb( base->plugin, xfce_panel_plugin_get_size( base->plugin ), base );
}

void set_color_mode( CPUGraph *base, guint color_mode )
{
	base->color_mode = color_mode;
}

void set_mode( CPUGraph *base, guint mode )
{
	base->mode = mode;
}

void set_color( CPUGraph *base, guint number, GdkRGBA color )
{
	guint i, n;

	base->colors[number] = color;
	if( number == 0 )
	{
		gtk_widget_override_background_color( base->draw_area, GTK_STATE_INSENSITIVE, &base->colors[0] );
		gtk_widget_override_background_color( base->draw_area, GTK_STATE_NORMAL, &base->colors[0] );
	}
	if( number == 4 && base->has_bars && base->has_barcolor )
	{
		n = nb_bars( base );

		for( i=0; i< n; i++ )
		{
			/* Set bar colors */
			gtk_widget_override_background_color(base->bars[i], GTK_STATE_PRELIGHT, &base->colors[4]);
			gtk_widget_override_background_color(base->bars[i], GTK_STATE_SELECTED, &base->colors[4]);
			gtk_widget_override_color(base->bars[i], GTK_STATE_SELECTED, &base->colors[4]);
		}
	}
}

void set_tracked_core( CPUGraph *base, guint core )
{
	gboolean has_bars = base->has_bars;
	if( has_bars)
		set_bars( base, FALSE );
	base->tracked_core = core;
	if( has_bars)
		set_bars( base, TRUE );
}
