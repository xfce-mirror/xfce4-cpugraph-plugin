#include "cpu.h"
#include "settings.h"
#include "mode.h"
#include "properties.h"

static void cpugraph_construct( XfcePanelPlugin *plugin );
static CPUGraph *create_gui( XfcePanelPlugin *plugin );
static guint init_cpu_data( CpuData **data );
static void shutdown( XfcePanelPlugin *plugin, CPUGraph *base );
static gboolean size_cb( XfcePanelPlugin *plugin, int size, CPUGraph *base );
static void orientation_cb( XfcePanelPlugin *plugin, GtkOrientation orientation, CPUGraph *base );
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
	gint i;
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

	base->m_Box = xfce_hvbox_new(orientation, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ebox), base->m_Box);

	base->m_pBar = (GtkWidget **) g_malloc( sizeof( GtkWidget * ) * base->nr_cores );

	for(i=0; i<base->nr_cores; i++) {
		base->m_pBar[i] = GTK_WIDGET(gtk_progress_bar_new());
		gtk_box_pack_start( GTK_BOX(base->m_Box), base->m_pBar[i], FALSE, FALSE, 0);
	}

	base->m_FrameWidget = frame = gtk_frame_new( NULL );
	gtk_box_pack_start( GTK_BOX(base->m_Box), frame, TRUE, TRUE, 0);

	base->m_DrawArea = gtk_drawing_area_new();
	gtk_container_add( GTK_CONTAINER( frame ), GTK_WIDGET( base->m_DrawArea ) );
	g_signal_connect_after( base->m_DrawArea, "expose-event", G_CALLBACK( draw_area_cb ), base );

	orientation_cb(plugin, orientation, base);

	gtk_widget_show_all(ebox);

	return base;
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
	gint i;
	g_free( base->cpu_data );
	base->cpu_data = NULL;

	for(i=0; i<base->nr_cores-1; i++)
		gtk_widget_destroy(base->m_pBar[i]);
	g_free( base->m_pBar );

	gtk_widget_destroy(base->m_Box);

	if( base->timeout_id )
		g_source_remove( base->timeout_id );

	g_free( base->history );

	g_free( base->command );
	g_free( base );
}

static gboolean size_cb( XfcePanelPlugin *plugin, int size, CPUGraph *base )
{
	gint i;
	gint frame_h, frame_v, bar_h, bar_v, history;

	gtk_container_set_border_width( GTK_CONTAINER( base->m_FrameWidget ), MIN( size ,base->size ) > 26 ? 2 : 0 );

	if( xfce_panel_plugin_get_orientation( plugin ) == GTK_ORIENTATION_HORIZONTAL )
	{
		frame_h = base->size;
		frame_v = size;
		bar_h = BORDER;
		bar_v = size;
	       	history = base->size;
	}
	else
	{
		frame_h = size;
		frame_v = base->size;
		bar_h = size;
		bar_v = BORDER;
	       	history = size;
	}

	gtk_widget_set_size_request( GTK_WIDGET( base->m_FrameWidget ), frame_h, frame_v );

	for( i=0; i<base->nr_cores; i++ )
		gtk_widget_set_size_request( GTK_WIDGET(base->m_pBar[i]), bar_h, bar_v );

	base->history = (int *) g_realloc( base->history, history * sizeof( int ) );
	if( history > base->history_size )
		memset( base->history + base->history_size, 0, (history - base->history_size) * sizeof( int ) );
	base->history_size = history;

	return TRUE;
}

static void orientation_cb( XfcePanelPlugin * plugin, GtkOrientation orientation, CPUGraph *base )
{
	GtkProgressBarOrientation barOrientation;
	gpointer p_pBar[base->nr_cores];
	gint i; 

	xfce_hvbox_set_orientation( XFCE_HVBOX( base->m_Box ), orientation );

	if( orientation == GTK_ORIENTATION_HORIZONTAL )
		barOrientation = GTK_PROGRESS_BOTTOM_TO_TOP;
	else
		barOrientation = GTK_PROGRESS_LEFT_TO_RIGHT;

	for( i=0; i<base->nr_cores; i++ )
	{
		gtk_progress_bar_set_orientation( GTK_PROGRESS_BAR( base->m_pBar[i] ), barOrientation );	
	}

}

static gboolean update_cb( CPUGraph * base )
{
	gint i;
	if( !read_cpu_data( base->cpu_data, base->nr_cores ) )
		return TRUE;
	if( base->nr_cores == 1 )
	{
		gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(base->m_pBar[0]),
		                               (gdouble)base->cpu_data[0].load / CPU_SCALE
		                             );
	}
	else
	{
		for( i=0; i<base->nr_cores; i++ )
			gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(base->m_pBar[i]),
			                               (gdouble)base->cpu_data[i+1].load / CPU_SCALE
			                             );
	}

	if( base->non_linear )
	{
		int i = base->history_size - 1;
		int j = i + base->history_size;
		while( i > 0 )
		{
			int a, b;

			a = base->history[i], b = base->history[i-1];
			if( a < b ) a++;
			int factor = (i*2);
			base->history[i--] = (a * (factor-1) + b) / factor;
		}
	} else {
		memmove( base->history + 1 , base->history , (base->history_size - 1) * sizeof( int ) );
	}
	base->history[0] = base->cpu_data[0].load;

	/* Tooltip */
	update_tooltip( base );

	/* Draw the graph. */
	gtk_widget_queue_draw( base->m_DrawArea );
	return TRUE;
}

static void update_tooltip( CPUGraph * base )
{
	gchar tooltip[32];
	int pos = g_snprintf( tooltip, 32, "Usage: %d%%", (int)base->cpu_data[0].load*100/CPU_SCALE );
	gtk_widget_set_tooltip_text( base->m_FrameWidget, tooltip );
}

static void draw_area_cb( GtkWidget * da, GdkEventExpose * event, gpointer data )
{
	CPUGraph *base = (CPUGraph *) data;

	draw_graph( base );
}

static void draw_graph( CPUGraph * base )
{
	GtkWidget *da = base->m_DrawArea;
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
	if( event->button == 1 )
	{
		GString *cmd;
		if( strlen(base->command) == 0 )
		{
			return FALSE;
		}
		cmd = g_string_new( base->command );
		xfce_exec( cmd->str, FALSE, FALSE, NULL );
		g_string_free( cmd, TRUE );
	}
	return FALSE;
}

void set_command( CPUGraph *base, const gchar *command )
{
	g_free( base->command );
	base->command = g_strdup( command );
}

void set_frame( CPUGraph *base, gboolean frame )
{
	base->frame = frame;
	gtk_frame_set_shadow_type( GTK_FRAME( base->m_FrameWidget ), base->frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE );
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
		gtk_widget_modify_bg( base->m_DrawArea, GTK_STATE_INSENSITIVE, &base->colors[0] );
		gtk_widget_modify_bg( base->m_DrawArea, GTK_STATE_NORMAL, &base->colors[0] );
	}
}
