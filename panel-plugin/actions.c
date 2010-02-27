#include <actions.h>

void AssociateCommandChange( GtkEntry *entry, CPUGraph * base )
{
	set_command( base, gtk_entry_get_text( entry ) );
}

void ChangeColor1( GtkColorButton * button, CPUGraph * base )
{
	GdkColor color;
	gtk_color_button_get_color( button, &color );
	set_foreground_color1( base, color );
}

void ChangeColor2( GtkColorButton * button, CPUGraph * base )
{
	GdkColor color;
	gtk_color_button_get_color( button, &color );
	set_foreground_color2( base, color );
}

void ChangeColor3( GtkColorButton * button, CPUGraph * base )
{
	GdkColor color;
	gtk_color_button_get_color( button, &color );
	set_background_color( base, color );
}

void ChangeColor4( GtkColorButton * button, CPUGraph * base )
{
	GdkColor color;
	gtk_color_button_get_color( button, &color );
	set_foreground_color3( base, color );
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

