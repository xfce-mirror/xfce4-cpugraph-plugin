#include <actions.h>

void ReadSettings( XfcePanelPlugin * plugin, CPUGraph * base )
{
	const char *value;
	char *file;
	XfceRc *rc;
	int update;

	base->m_Width = 70;

	base->m_ForeGround1.red = 0;
	base->m_ForeGround1.green = 65535;
	base->m_ForeGround1.blue = 0;

	base->m_ForeGround2.red = 65535;
	base->m_ForeGround2.green = 0;
	base->m_ForeGround2.blue = 0;

	base->m_ForeGround3.red = 0;
	base->m_ForeGround3.green = 0;
	base->m_ForeGround3.blue = 65535;

	base->m_BackGround.red = 65535;
	base->m_BackGround.green = 65535;
	base->m_BackGround.blue = 65535;

	base->m_TimeScale = 0;
	base->m_Frame = 0;
	base->m_AssociateCommand = NULL;
	base->m_ColorMode = 0;
	base->m_Mode = 0;

	if( (file = xfce_panel_plugin_lookup_rc_file( plugin )) != NULL )
	{
		rc = xfce_rc_simple_open( file, TRUE );
		g_free( file );

		if( rc )
		{
			base->m_UpdateInterval =
				xfce_rc_read_int_entry (rc, "UpdateInterval",
						base->m_UpdateInterval);

			base->m_TimeScale =
				xfce_rc_read_int_entry (rc, "TimeScale",
						base->m_TimeScale);

			base->m_Width =
				xfce_rc_read_int_entry( rc, "Width", base->m_Width );

			base->m_Mode = xfce_rc_read_int_entry( rc, "Mode", base->m_Mode );

			base->m_Frame =
				xfce_rc_read_int_entry( rc, "Frame", base->m_Frame );

			if( value = xfce_rc_read_entry( rc, "AssociateCommand", DEFAULT_COMMAND ) ) {
				base->m_AssociateCommand = g_strdup(value);
			}
			else
			{
				base->m_AssociateCommand = g_strdup( DEFAULT_COMMAND );
			}

			base->m_ColorMode =
				xfce_rc_read_int_entry( rc, "ColorMode", base->m_ColorMode );

			if( (value = xfce_rc_read_entry( rc, "Foreground1", NULL )) )
			{
				gdk_color_parse( value, &base->m_ForeGround1 );
			}
			if( (value = xfce_rc_read_entry( rc, "Foreground2", NULL )) )
			{
				gdk_color_parse( value, &base->m_ForeGround2 );
			}
			if( (value = xfce_rc_read_entry( rc, "Background", NULL )) )
			{
				gdk_color_parse( value, &base->m_BackGround );
			}
			if( (value = xfce_rc_read_entry( rc, "Foreground3", NULL )) )
			{
				gdk_color_parse( value, &base->m_ForeGround3 );
			}

			xfce_rc_close( rc );
		}
	}
	else
	{
		base->m_AssociateCommand = g_strdup( DEFAULT_COMMAND );
	}
	SetHistorySize( base, base->m_Width );
	if( base->m_TimeoutID )
		g_source_remove( base->m_TimeoutID );
	switch( base->m_UpdateInterval )
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
	base->m_TimeoutID = g_timeout_add( update, (GtkFunction) UpdateCPU, base );
	gtk_frame_set_shadow_type( GTK_FRAME(base->m_FrameWidget ), base->m_Frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE );
}

void WriteSettings( XfcePanelPlugin *plugin, CPUGraph *base )
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

	xfce_rc_write_int_entry( rc, "UpdateInterval", base->m_UpdateInterval );

	xfce_rc_write_int_entry( rc, "TimeScale", base->m_TimeScale );

	xfce_rc_write_int_entry( rc, "Width", base->m_Width );

	xfce_rc_write_int_entry( rc, "Mode", base->m_Mode );

	xfce_rc_write_int_entry( rc, "Frame", base->m_Frame );

	xfce_rc_write_entry( rc, "AssociateCommand", base->m_AssociateCommand ? base->m_AssociateCommand : "" );

	xfce_rc_write_int_entry( rc, "ColorMode", base->m_ColorMode );

	g_snprintf( value, 8, "#%02X%02X%02X", base->m_ForeGround1.red >> 8, base->m_ForeGround1.green >> 8, base->m_ForeGround1.blue >> 8 );
	xfce_rc_write_entry( rc, "Foreground1", value );

	g_snprintf( value, 8, "#%02X%02X%02X", base->m_ForeGround2.red >> 8, base->m_ForeGround2.green >> 8, base->m_ForeGround2.blue >> 8 );
	xfce_rc_write_entry( rc, "Foreground2", value );

	g_snprintf( value, 8, "#%02X%02X%02X", base->m_BackGround.red >> 8, base->m_BackGround.green >> 8, base->m_BackGround.blue >> 8 );
	xfce_rc_write_entry( rc, "Background", value );

	g_snprintf( value, 8, "#%02X%02X%02X", base->m_ForeGround3.red >> 8, base->m_ForeGround3.green >> 8, base->m_ForeGround3.blue >> 8 );
	xfce_rc_write_entry( rc, "Foreground3", value );

	xfce_rc_close( rc );
}
