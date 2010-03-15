#include "cpu.h"
#include "settings.h"
#include "mode.h"
#include "properties.h"

static void cpugraph_construct( XfcePanelPlugin *plugin );
static CPUGraph *create_gui( XfcePanelPlugin *plugin );
static void create_bars( CPUGraph *base );
static guint init_cpu_data( CpuData **data );
static void shutdown( XfcePanelPlugin *plugin, CPUGraph *base );
static void delete_bars( CPUGraph *base );
static gboolean size_cb( XfcePanelPlugin *plugin, int size, CPUGraph *base );
static void set_bars_size( CPUGraph *base, gint size, GtkOrientation orientation );
static void orientation_cb( XfcePanelPlugin *plugin, GtkOrientation orientation, CPUGraph *base );
static void set_bars_orientation( CPUGraph *base, GtkOrientation orientation);
static gboolean update_cb( CPUGraph *base );
static void update_tooltip( CPUGraph *base );
static void draw_area_cb( GtkWidget *da, GdkEventExpose *event, gpointer data );
static void draw_graph( CPUGraph *base );
static gboolean command_cb( GtkWidget *w, GdkEventButton *event, CPUGraph *base );

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL( cpugraph_construct );

static void cpugraph_construct( XfcePanelPlugin *plugin )
{
	CPUGraph *base;

	xfce_textdomain( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8" );

	base = create_gui( plugin );
	read_settings( plugin, base );
	xfce_panel_plugin_menu_show_configure( plugin );

	g_signal_connect( plugin, "free-data", G_CALLBACK( shutdown ), base );
	g_signal_connect( plugin, "save", G_CALLBACK( write_settings ), base );
	g_signal_connect( plugin, "configure-plugin", G_CALLBACK( create_options ), base );
	g_signal_connect( plugin, "size-changed", G_CALLBACK( size_cb ), base );
	g_signal_connect( plugin, "orientation-changed", G_CALLBACK( orientation_cb ), base );
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
	gtk_container_add( GTK_CONTAINER( plugin ), ebox );
	xfce_panel_plugin_add_action_widget( plugin, ebox );
	g_signal_connect( ebox, "button-press-event", G_CALLBACK( command_cb ), base );

	base->box = xfce_hvbox_new(orientation, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ebox), base->box);

	base->frame_widget = frame = gtk_frame_new( NULL );
	gtk_box_pack_end( GTK_BOX(base->box), frame, TRUE, TRUE, 0);

	base->draw_area = gtk_drawing_area_new();
	gtk_container_add( GTK_CONTAINER( frame ), GTK_WIDGET( base->draw_area ) );
	g_signal_connect_after( base->draw_area, "expose-event", G_CALLBACK( draw_area_cb ), base );

	base->has_bars = FALSE;
	base->bars = NULL;

	orientation_cb(plugin, orientation, base);
	gtk_widget_show_all(ebox);

	return base;
}

static void create_bars( CPUGraph *base )
{
	gint i;
	base->bars = (GtkWidget **) g_malloc( sizeof( GtkWidget * ) * base->nr_cores );

	for(i=0; i<base->nr_cores; i++) {
		base->bars[i] = GTK_WIDGET(gtk_progress_bar_new());
		gtk_box_pack_end( GTK_BOX(base->box), base->bars[i], FALSE, FALSE, 0);
		gtk_widget_show( base->bars[i] );
	}
}

guint init_cpu_data( CpuData **data )
{
	guint cpuNr;

	cpuNr = detect_cpu_number();
	if( cpuNr == 0 )
		return 0;

	*data = (CpuData *) g_malloc0( cpuNr * sizeof( CpuData ) );

	return cpuNr;
}

static void shutdown( XfcePanelPlugin * plugin, CPUGraph * base )
{
	g_free( base->cpu_data );
	delete_bars( base );
	gtk_widget_destroy(base->box);
	if( base->timeout_id )
		g_source_remove( base->timeout_id );
	g_free( base->history );
	g_free( base->command );
	g_free( base );
}

static void delete_bars( CPUGraph *base )
{
	gint i;
	if( base->bars )
	{
		for( i=0; i < base->nr_cores; i++ )
		{
			gtk_widget_hide( base->bars[i] );
			gtk_widget_destroy( base->bars[i] );
		}
		g_free( base->bars );
		base->bars = NULL;
	}
}

static gboolean size_cb( XfcePanelPlugin *plugin, int size, CPUGraph *base )
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

	base->history = (int *) g_realloc( base->history, history * sizeof( int ) );
	if( history > base->history_size )
		memset( base->history + base->history_size, 0, (history - base->history_size) * sizeof( int ) );
	base->history_size = history;

	if( base->has_bars )
		set_bars_size( base, size, orientation );

	return TRUE;
}

static void set_bars_size( CPUGraph *base, gint size, GtkOrientation orientation )
{
	gint i;
	gint h, v;
	if( orientation == GTK_ORIENTATION_HORIZONTAL )
	{
		h = BORDER;
		v = size;
	}
	else
	{
		h = size;
		v = BORDER;
	}
	for( i=0; i < base->nr_cores; i++ )
		gtk_widget_set_size_request( GTK_WIDGET(base->bars[i]), h, v );
}

static void orientation_cb( XfcePanelPlugin * plugin, GtkOrientation orientation, CPUGraph *base )
{
	xfce_hvbox_set_orientation( XFCE_HVBOX( base->box ), orientation );
	if( base->has_bars )
		set_bars_orientation( base, orientation );

}

static void set_bars_orientation( CPUGraph *base, GtkOrientation orientation)
{
	GtkProgressBarOrientation barOrientation;
	gint i; 
	if( orientation == GTK_ORIENTATION_HORIZONTAL )
		barOrientation = GTK_PROGRESS_BOTTOM_TO_TOP;
	else
		barOrientation = GTK_PROGRESS_LEFT_TO_RIGHT;

	for( i=0; i<base->nr_cores; i++ )
		gtk_progress_bar_set_orientation( GTK_PROGRESS_BAR( base->bars[i] ), barOrientation );	
}

static gboolean update_cb( CPUGraph * base )
{
	gint i, j, a, b, factor;
	if( !read_cpu_data( base->cpu_data, base->nr_cores ) )
		return TRUE;
	if( base->has_bars )
	{
		if( base->nr_cores == 1 )
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
		j = i + base->history_size;
		while( i > 0 )
		{
			a = base->history[i], b = base->history[i-1];
			if( a < b ) a++;
			factor = (i*2);
			base->history[i--] = (a * (factor-1) + b) / factor;
		}
	} else {
		memmove( base->history + 1 , base->history , (base->history_size - 1) * sizeof( int ) );
	}
	base->history[0] = base->cpu_data[0].load;

	update_tooltip( base );
	gtk_widget_queue_draw( base->draw_area );

	return TRUE;
}

static void update_tooltip( CPUGraph * base )
{
	gchar tooltip[32];
	int pos = g_snprintf( tooltip, 32, "Usage: %d%%", (int)base->cpu_data[0].load*100/CPU_SCALE );
	gtk_widget_set_tooltip_text( base->frame_widget, tooltip );
}

static void draw_area_cb( GtkWidget * da, GdkEventExpose * event, gpointer data )
{
	draw_graph( (CPUGraph *) data );
}

static void draw_graph( CPUGraph * base )
{
	GtkWidget *da = base->draw_area;
	int w, h;

	w = da->allocation.width;
	h = da->allocation.height;

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
		xfce_exec( base->command, base->in_terminal, base->startup_notification, NULL );
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
	base->has_border = border;
	gtk_container_set_border_width( GTK_CONTAINER( base->box ), border ? BORDER / 2 : 0 );
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

void set_update_rate( CPUGraph *base, int rate )
{
	int update;

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
	base->timeout_id = g_timeout_add( update, (GtkFunction) update_cb, base );
}

void set_size( CPUGraph *base, int size )
{
	base->size = size;
	size_cb( base->plugin, xfce_panel_plugin_get_size( base->plugin ), base );
}

void set_color_mode( CPUGraph *base, int color_mode )
{
	base->color_mode = color_mode;
}

void set_mode( CPUGraph *base, int mode )
{
	base->mode = mode;
}

void set_color( CPUGraph *base, int number, GdkColor color )
{
	base->colors[number] = color;
	if( number == 0 )
	{
		gtk_widget_modify_bg( base->draw_area, GTK_STATE_INSENSITIVE, &base->colors[0] );
		gtk_widget_modify_bg( base->draw_area, GTK_STATE_NORMAL, &base->colors[0] );
	}
}
