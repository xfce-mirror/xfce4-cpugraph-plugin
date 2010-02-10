#include <actions.h>

void AssociateCommandChange( GtkEntry *entry, CPUGraph * base )
{
	g_free (base->m_AssociateCommand );
	base->m_AssociateCommand = g_strdup( gtk_entry_get_text( entry ) );
}

void ChangeColor( int color, CPUGraph * base )
{
	GtkWidget *dialog;
	GtkColorSelection *colorsel;
	gint response;

	dialog = gtk_color_selection_dialog_new( "Select color" );
	gtk_window_set_transient_for( GTK_WINDOW( dialog ), GTK_WINDOW( base->m_OptionsDialog ) );

	colorsel = GTK_COLOR_SELECTION( GTK_COLOR_SELECTION_DIALOG( dialog )->colorsel );

	if( color == 0 )
	{
		gtk_color_selection_set_previous_color( colorsel, &base->m_ForeGround1 );
		gtk_color_selection_set_current_color( colorsel, &base->m_ForeGround1 );
	}
	else if( color == 1 )
	{
		gtk_color_selection_set_previous_color( colorsel, &base->m_ForeGround2 );
		gtk_color_selection_set_current_color( colorsel, &base->m_ForeGround2 );
	}
	else if( color == 2 )
	{
		gtk_color_selection_set_previous_color( colorsel, &base->m_BackGround );
		gtk_color_selection_set_current_color( colorsel, &base->m_BackGround );
	}
	else if( color == 3 )
	{
		gtk_color_selection_set_previous_color( colorsel, &base->m_ForeGround3 );
		gtk_color_selection_set_current_color( colorsel, &base->m_ForeGround3 );
	}

	gtk_color_selection_set_has_palette( colorsel, TRUE );

	response = gtk_dialog_run( GTK_DIALOG( dialog ) );
	if( response == GTK_RESPONSE_OK )
	{
		if( color == 0 )
		{
			gtk_color_selection_get_current_color( colorsel, &base->m_ForeGround1 );
			gtk_widget_modify_bg( base->m_Options.m_ColorDA, GTK_STATE_NORMAL, &base->m_ForeGround1 );
		}
		else if( color == 1 )
		{
			gtk_color_selection_get_current_color( colorsel, &base->m_ForeGround2 );
			gtk_widget_modify_bg( base->m_Options.m_ColorDA2, GTK_STATE_NORMAL, &base->m_ForeGround2 );
		}
		else if( color == 2 )
		{
			gtk_color_selection_get_current_color( colorsel, &base->m_BackGround );
			gtk_widget_modify_bg( base->m_Options.m_ColorDA3, GTK_STATE_NORMAL, &base->m_BackGround );
		}
		else if( color == 3 )
		{
			gtk_color_selection_get_current_color( colorsel, &base->m_ForeGround3 );
			gtk_widget_modify_bg( base->m_Options.m_ColorDA5, GTK_STATE_NORMAL, &base->m_ForeGround3 );
		}
	}

	gtk_widget_destroy( dialog );

}

void ChangeColor1( GtkButton * button, CPUGraph * base )
{
	ChangeColor( 0, base );
}

void ChangeColor2( GtkButton * button, CPUGraph * base )
{
	ChangeColor( 1, base );
}

void ChangeColor3( GtkButton * button, CPUGraph * base )
{
	ChangeColor( 2, base );
}

void ChangeColor4( GtkButton * button, CPUGraph * base )
{
	ChangeColor( 3, base );
}

void ColorModeChange( GtkOptionMenu * om, CPUGraph * base )
{
	base->m_ColorMode = gtk_option_menu_get_history( om );
	if( base->m_ColorMode == 0 )
	{
		if( base->m_Mode == 0 || base->m_Mode == 2 )
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), FALSE );
		else
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
	}
	else if( base->m_ColorMode == 1 )
	{
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );
		if( base->m_Mode == 1 )
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), TRUE );
		else
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
	}
	else if( base->m_ColorMode == 2 )
	{
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );
		if( base->m_Mode == 1 )
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), TRUE );
		else
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
	}
	else if( base->m_ColorMode == 3 )
	{
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );
		if( base->m_Mode == 1 )
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), TRUE );
		else
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
	}
}

void DialogResponse( GtkWidget *dlg, int response, CPUGraph *base )
{
	ApplyChanges( base );
	gtk_widget_destroy( dlg );
	xfce_panel_plugin_unblock_menu( base->plugin );
	WriteSettings( base->plugin, base );
}

void ApplyChanges( CPUGraph * base )
{
	int update;

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

	UserSetSize( base );
	SetHistorySize( base, base->m_Width );
}

void FrameChange( GtkToggleButton * button, CPUGraph * base )
{
	base->m_Frame = gtk_toggle_button_get_active( button );
	gtk_frame_set_shadow_type( GTK_FRAME( base->m_FrameWidget ), base->m_Frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE );
}

void ModeChange( GtkOptionMenu * om, CPUGraph * base )
{
	base->m_Mode = gtk_option_menu_get_history( om );
	if( base->m_Mode == 0 )
	{
		if( base->m_ColorMode > 0 )
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );
		else
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
	}
	else if( base->m_Mode == 1 )
	{
		if( base->m_ColorMode > 0 )
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), TRUE );
		else
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );
	}
	else if( base->m_Mode == 2 )
	{
		if( base->m_ColorMode > 0 )
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );
		else
			gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
	}
}

void SpinChange( GtkSpinButton * sb, int *value )
{
	(*value) = gtk_spin_button_get_value_as_int( sb );
}

void TimeScaleChange( GtkToggleButton * button, CPUGraph * base )
{
	base->m_TimeScale = gtk_toggle_button_get_active( button );
}

void UpdateChange( GtkOptionMenu * om, CPUGraph * base )
{
	base->m_UpdateInterval = gtk_option_menu_get_history( om );
}

void SetHistorySize( CPUGraph * base, int size )
{
	int i;
	base->m_History = (long *) g_realloc( base->m_History, 2 * size * sizeof( long ) );

	base->m_CpuData = cpuData_read();
	base->m_CpuData[0].pUsed = 0;
	base->m_CpuData[0].pTotal = 0;
	long usage = base->m_CpuData[0].load;
	for( i = size - 1; i >= base->m_Values; i-- )
	{
		base->m_History[i] = usage;
		base->m_History[i+size] = base->m_CpuData[0].scalCurFreq;
	}
	base->m_Values = size;
}
