#include <actions.h>

void AssociateCommandChange( GtkEntry *entry, CPUGraph * base )
{
	set_command( base, gtk_entry_get_text( entry ) );
}

void ChangeColor( GtkColorButton * button, CPUGraph * base, int number)
{
	GdkColor color;
	gtk_color_button_get_color( button, &color );
	set_color( base, number, color );
}

void ChangeColor1( GtkColorButton * button, CPUGraph * base )
{
	ChangeColor( button, base, 1);
}

void ChangeColor2( GtkColorButton * button, CPUGraph * base )
{
	ChangeColor( button, base, 2);
}

void ChangeColor3( GtkColorButton * button, CPUGraph * base )
{
	ChangeColor( button, base, 3);
}

void ChangeColor0( GtkColorButton * button, CPUGraph * base )
{
	ChangeColor( button, base, 0);
}

void select_active_colors( CPUGraph * base )
{
	if( base->m_ColorMode != 0 || base->m_Mode == 1 || base->m_Mode == 3 )
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[2] ), TRUE );
	else
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[2] ), FALSE );

	if( base->m_ColorMode != 0 && base->m_Mode == 1 )
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[3] ), TRUE );
	else
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[3] ), FALSE );
}

void ModeChange( GtkOptionMenu * om, CPUGraph * base )
{
	set_mode( base, gtk_option_menu_get_history( om ) );
	select_active_colors( base );
}

void ColorModeChange( GtkOptionMenu * om, CPUGraph * base )
{
	set_color_mode( base, gtk_option_menu_get_history( om ) );
	select_active_colors( base );
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

