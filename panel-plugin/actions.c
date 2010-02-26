#include <actions.h>

void AssociateCommandChange( GtkEntry *entry, CPUGraph * base )
{
	set_command( base, gtk_entry_get_text( entry ) );
}

GdkColor ChangeColor( CPUGraph *base, GdkColor color, GtkWidget *button )
{
	GtkWidget *dialog;
	GtkColorSelection *colorsel;
	gint response;

	dialog = gtk_color_selection_dialog_new( "Select color" );
	gtk_window_set_transient_for( GTK_WINDOW( dialog ), GTK_WINDOW( base->m_OptionsDialog ) );

	colorsel = GTK_COLOR_SELECTION( GTK_COLOR_SELECTION_DIALOG( dialog )->colorsel );

	gtk_color_selection_set_previous_color( colorsel, &color );
	gtk_color_selection_set_current_color( colorsel, &color );

	gtk_color_selection_set_has_palette( colorsel, TRUE );

	response = gtk_dialog_run( GTK_DIALOG( dialog ) );
	if( response == GTK_RESPONSE_OK )
	{
		gtk_color_selection_get_current_color( colorsel, &color );
		gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &color );
	}

	gtk_widget_destroy( dialog );
	return color;
}

void ChangeColor1( GtkButton * button, CPUGraph * base )
{
	set_foreground_color1( base, ChangeColor( base, base->m_ForeGround1, base->m_Options.m_ColorDA ) );
}

void ChangeColor2( GtkButton * button, CPUGraph * base )
{
	set_foreground_color2( base, ChangeColor( base, base->m_ForeGround2, base->m_Options.m_ColorDA2 ) );
}

void ChangeColor3( GtkButton * button, CPUGraph * base )
{
	set_background_color( base, ChangeColor( base, base->m_BackGround, base->m_Options.m_ColorDA3 ) );
}

void ChangeColor4( GtkButton * button, CPUGraph * base )
{
	set_foreground_color3( base, ChangeColor( base, base->m_ForeGround3, base->m_Options.m_ColorDA5 ) );
}

void ColorModeChange( GtkOptionMenu * om, CPUGraph * base )
{
	set_color_mode( base, gtk_option_menu_get_history( om ) );
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
	gtk_widget_destroy( dlg );
	xfce_panel_plugin_unblock_menu( base->plugin );
	WriteSettings( base->plugin, base );
}

void FrameChange( GtkToggleButton * button, CPUGraph * base )
{
	set_frame( base, gtk_toggle_button_get_active( button ) );
}

void ModeChange( GtkOptionMenu * om, CPUGraph * base )
{
	set_mode( base, gtk_option_menu_get_history( om ) );
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

void WidthChange( GtkSpinButton * sb, CPUGraph *base)
{
	set_width( base, gtk_spin_button_get_value_as_int( sb ) );
}

void TimeScaleChange( GtkToggleButton * button, CPUGraph * base )
{
	set_nonlinear_time( base, gtk_toggle_button_get_active( button ) );
}

void UpdateChange( GtkOptionMenu * om, CPUGraph * base )
{
	set_update_rate( base, gtk_option_menu_get_history( om ) );
}

