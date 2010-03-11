#include "settings.h"

#define DEFAULT_COMMAND "exo-open --launch TerminalEmulator top"

void read_settings( XfcePanelPlugin * plugin, CPUGraph * base )
{
	const char *value;
	char *file;
	XfceRc *rc;

	int rate = 0;
	gboolean nonlinear = FALSE;
	int size = 70;
	int mode = 0;
	int color_mode = 0;
	gboolean frame = FALSE;
	gboolean border = TRUE;
	const gchar  *associated_command = DEFAULT_COMMAND;

	GdkColor foreground1;
	GdkColor foreground2;
	GdkColor foreground3;
	GdkColor background;

	foreground1.red = 0;
	foreground1.green = 65535;
	foreground1.blue = 0;

	foreground2.red = 65535;
	foreground2.green = 0;
	foreground2.blue = 0;

	foreground3.red = 0;
	foreground3.green = 0;
	foreground3.blue = 65535;

	background.red = 65535;
	background.green = 65535;
	background.blue = 65535;

	if( (file = xfce_panel_plugin_lookup_rc_file( plugin )) != NULL )
	{
		rc = xfce_rc_simple_open( file, TRUE );
		g_free( file );

		if( rc )
		{
			rate =  xfce_rc_read_int_entry (rc, "UpdateInterval", rate );
			nonlinear = xfce_rc_read_int_entry (rc, "TimeScale", nonlinear );
			size = xfce_rc_read_int_entry( rc, "size", size );
			mode = xfce_rc_read_int_entry( rc, "Mode", mode );
			color_mode = xfce_rc_read_int_entry( rc, "ColorMode", color_mode );
			frame = xfce_rc_read_int_entry( rc, "Frame", frame );
			associated_command = xfce_rc_read_entry( rc, "AssociateCommand", associated_command );
			border = xfce_rc_read_int_entry( rc, "Border", border );

			if( (value = xfce_rc_read_entry( rc, "Foreground1", NULL )) )
				gdk_color_parse( value, &foreground1 );
			if( (value = xfce_rc_read_entry( rc, "Foreground2", NULL )) )
				gdk_color_parse( value, &foreground2 );
			if( (value = xfce_rc_read_entry( rc, "Foreground3", NULL )) )
				gdk_color_parse( value, &foreground3 );
			if( (value = xfce_rc_read_entry( rc, "Background", NULL )) )
				gdk_color_parse( value, &background );

			xfce_rc_close( rc );
		}
	}

	set_update_rate( base, rate );
	set_nonlinear_time( base, nonlinear );
	set_size( base, size );
	set_mode( base, mode );
	set_color_mode( base, color_mode );
	set_frame( base, frame );
	set_command( base, associated_command );
	set_border( base, border);
	set_color( base, 1, foreground1 );
	set_color( base, 2, foreground2 );
	set_color( base, 3, foreground3 );
	set_color( base, 0, background );
}

void write_settings( XfcePanelPlugin *plugin, CPUGraph *base )
{
	char value[10];
	XfceRc *rc;
	char *file;

	if( !(file = xfce_panel_plugin_save_location( plugin, TRUE )) )
		return;

	rc = xfce_rc_simple_open( file, FALSE );
	g_free( file );

	if( !rc )
		return;

	xfce_rc_write_int_entry( rc, "UpdateInterval", base->update_interval );

	xfce_rc_write_int_entry( rc, "TimeScale", base->non_linear );

	xfce_rc_write_int_entry( rc, "Size", base->size );

	xfce_rc_write_int_entry( rc, "Mode", base->mode );

	xfce_rc_write_int_entry( rc, "Frame", base->frame );

	xfce_rc_write_int_entry( rc, "Border", base->border );

	xfce_rc_write_entry( rc, "AssociateCommand", base->command ? base->command : "" );

	xfce_rc_write_int_entry( rc, "ColorMode", base->color_mode );

	g_snprintf( value, 8, "#%02X%02X%02X", base->colors[1].red >> 8, base->colors[1].green >> 8, base->colors[1].blue >> 8 );
	xfce_rc_write_entry( rc, "Foreground1", value );

	g_snprintf( value, 8, "#%02X%02X%02X", base->colors[2].red >> 8, base->colors[2].green >> 8, base->colors[2].blue >> 8 );
	xfce_rc_write_entry( rc, "Foreground2", value );

	g_snprintf( value, 8, "#%02X%02X%02X", base->colors[0].red >> 8, base->colors[0].green >> 8, base->colors[0].blue >> 8 );
	xfce_rc_write_entry( rc, "Background", value );

	g_snprintf( value, 8, "#%02X%02X%02X", base->colors[3].red >> 8, base->colors[3].green >> 8, base->colors[3].blue >> 8 );
	xfce_rc_write_entry( rc, "Foreground3", value );

	xfce_rc_close( rc );
}
