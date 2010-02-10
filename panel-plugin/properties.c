#include <actions.h>

void CreateOptions( XfcePanelPlugin *plugin, CPUGraph *base )
{
	GtkWidget *dlg, *header;
	GtkBox *vbox, *vbox2, *hbox;
	GtkWidget *label;
	GtkSizeGroup *sg;
	SOptions *op = &base->m_Options;

	xfce_panel_plugin_block_menu( plugin );

	dlg = gtk_dialog_new_with_buttons( _("Configure CPU Graph"),
	                                   GTK_WINDOW( gtk_widget_get_toplevel( GTK_WIDGET( plugin ) ) ),
	                                   GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
	                                   GTK_STOCK_CLOSE,
	                                   GTK_RESPONSE_OK,
	                                   NULL
					 );

	base->m_OptionsDialog = dlg;

	g_signal_connect( dlg, "response", G_CALLBACK( DialogResponse ), base );

	gtk_container_set_border_width( GTK_CONTAINER( dlg ), 2 );

	header = xfce_create_header( NULL, _("CPU Graph") );
	gtk_widget_set_size_request( GTK_BIN( header )->child, -1, 32 );
	gtk_container_set_border_width( GTK_CONTAINER( header ), BORDER - 2 );
	gtk_widget_show( header );
	gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dlg )->vbox ), header, FALSE, TRUE, 0 );

	vbox = GTK_BOX( gtk_vbox_new( FALSE, BORDER ) );
	gtk_container_set_border_width( GTK_CONTAINER( vbox ), BORDER );
	gtk_widget_show( GTK_WIDGET( vbox ) );

	sg = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );

	/* Update Interval */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );
	label = gtk_label_new( _("Update Interval: ") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );

	op->m_UpdateOption = gtk_option_menu_new();
	gtk_widget_show( op->m_UpdateOption );
	gtk_box_pack_start( GTK_BOX( hbox ), op->m_UpdateOption, FALSE, FALSE, 0 );

	op->m_UpdateMenu = gtk_menu_new();
	gtk_option_menu_set_menu( GTK_OPTION_MENU( op->m_UpdateOption ), op->m_UpdateMenu );

	op->m_UpdateMenuItem = gtk_menu_item_new_with_label( _("Fastest (~250ms)") );
	gtk_widget_show( op->m_UpdateMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_UpdateMenu ), op->m_UpdateMenuItem );

	op->m_UpdateMenuItem = gtk_menu_item_new_with_label( _("Fast (~500ms)") );
	gtk_widget_show( op->m_UpdateMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_UpdateMenu ), op->m_UpdateMenuItem );

	op->m_UpdateMenuItem = gtk_menu_item_new_with_label( _("Normal (~750ms)") );
	gtk_widget_show( op->m_UpdateMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_UpdateMenu ), op->m_UpdateMenuItem );

	op->m_UpdateMenuItem = gtk_menu_item_new_with_label( _("Slow (~1s)") );
	gtk_widget_show( op->m_UpdateMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_UpdateMenu ), op->m_UpdateMenuItem );

	gtk_option_menu_set_history( GTK_OPTION_MENU( op->m_UpdateOption ), base->m_UpdateInterval );

	g_signal_connect( op->m_UpdateOption, "changed", G_CALLBACK( UpdateChange ), base );

	/* Width */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	if( xfce_panel_plugin_get_orientation( plugin ) == GTK_ORIENTATION_HORIZONTAL )
		label = gtk_label_new( _("Width:") );
	else
		label = gtk_label_new( _("Height:") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );

	op->m_Width = gtk_spin_button_new_with_range( 10, 128, 1 );
	gtk_spin_button_set_value( GTK_SPIN_BUTTON( op->m_Width ), base->m_Width );
	gtk_widget_show( op->m_Width );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_Width ), FALSE, FALSE, 0 );
	g_signal_connect( op->m_Width, "value-changed", G_CALLBACK( SpinChange ), &base->m_Width );

	/* TimeScale */
	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	op->m_TimeScale = gtk_check_button_new_with_mnemonic( _("Non-linear time-scale") );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( op->m_TimeScale ), base->m_TimeScale );
	gtk_widget_show( op->m_TimeScale );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_TimeScale ), FALSE, FALSE, 0 );
	g_signal_connect( op->m_TimeScale, "toggled", G_CALLBACK( TimeScaleChange ), base );
	gtk_size_group_add_widget( sg, op->m_TimeScale );

	/* Frame */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	op->m_GraphFrame = gtk_check_button_new_with_mnemonic( _("Show frame") );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( op->m_GraphFrame ), base->m_Frame );
	gtk_widget_show( op->m_GraphFrame );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_GraphFrame ), FALSE, FALSE, 0 );
	g_signal_connect( op->m_GraphFrame, "toggled", G_CALLBACK( FrameChange ), base );
	gtk_size_group_add_widget( sg, op->m_GraphFrame );

	vbox2 = GTK_BOX( gtk_vbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( vbox2 ) );
	gtk_container_set_border_width( GTK_CONTAINER( vbox2 ), 8 );

	/* Associate Command */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );
	label = gtk_label_new( _("Associated command :") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
	op->m_AssociateCommand = gtk_entry_new();
	gtk_entry_set_max_length( GTK_ENTRY(op->m_AssociateCommand), 32 );
	gtk_entry_set_text( GTK_ENTRY(op->m_AssociateCommand), base->m_AssociateCommand );
	gtk_widget_show( op->m_AssociateCommand );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_AssociateCommand ), FALSE, FALSE, 0 );
	g_signal_connect( op->m_AssociateCommand, "changed", G_CALLBACK( AssociateCommandChange ), base );

	/* Foreground 1 */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	label = gtk_label_new( _("Color 1:") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );

	op->m_FG1 = gtk_button_new();
	op->m_ColorDA = gtk_drawing_area_new();

	gtk_widget_modify_bg( op->m_ColorDA, GTK_STATE_NORMAL, &base->m_ForeGround1 );
	gtk_widget_set_size_request( op->m_ColorDA, 12, 12 );
	gtk_container_add( GTK_CONTAINER( op->m_FG1 ), op->m_ColorDA );
	gtk_widget_show( GTK_WIDGET( op->m_FG1 ) );
	gtk_widget_show( GTK_WIDGET( op->m_ColorDA ) );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_FG1 ), FALSE, FALSE, 0 );

	g_signal_connect( op->m_FG1, "clicked", G_CALLBACK( ChangeColor1 ), base );

	/* Foreground2 */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	label = gtk_label_new( _("Color 2:") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );

	op->m_FG2 = gtk_button_new();
	op->m_ColorDA2 = gtk_drawing_area_new();

	gtk_widget_modify_bg( op->m_ColorDA2, GTK_STATE_NORMAL, &base->m_ForeGround2 );
	gtk_widget_set_size_request( op->m_ColorDA2, 12, 12 );
	gtk_container_add( GTK_CONTAINER( op->m_FG2 ), op->m_ColorDA2 );
	gtk_widget_show( GTK_WIDGET( op->m_FG2 ) );
	gtk_widget_show( GTK_WIDGET( op->m_ColorDA2 ) );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_FG2 ), FALSE, FALSE, 0 );

	g_signal_connect( op->m_FG2, "clicked", G_CALLBACK( ChangeColor2 ), base );

	if( base->m_Mode == 1 )
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG2 ), TRUE );

	/* Foreground3 */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	label = gtk_label_new( _("Color 3:") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
	op->m_FG3 = gtk_button_new();
	op->m_ColorDA5 = gtk_drawing_area_new();
	gtk_widget_modify_bg( op->m_ColorDA5, GTK_STATE_NORMAL, &base->m_ForeGround3 );
	gtk_widget_set_size_request( op->m_ColorDA5, 12, 12 );
	gtk_container_add( GTK_CONTAINER( op->m_FG3 ), op->m_ColorDA5 );
	gtk_widget_show( GTK_WIDGET( op->m_FG3 ) );
	gtk_widget_show( GTK_WIDGET( op->m_ColorDA5 ) );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_FG3 ), FALSE, FALSE, 0 );
	g_signal_connect( op->m_FG3, "clicked", G_CALLBACK( ChangeColor4 ), base );

	if( base->m_Mode == 0 || base->m_Mode == 2 || base->m_ColorMode == 0 )
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), FALSE );
	else
		gtk_widget_set_sensitive( GTK_WIDGET( base->m_Options.m_FG3 ), TRUE );


	/* Background */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	label = gtk_label_new( _("Background:") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );

	op->m_BG = gtk_button_new();
	op->m_ColorDA3 = gtk_drawing_area_new();

	gtk_widget_modify_bg( op->m_ColorDA3, GTK_STATE_NORMAL, &base->m_BackGround );
	gtk_widget_set_size_request( op->m_ColorDA3, 12, 12 );
	gtk_container_add( GTK_CONTAINER( op->m_BG ), op->m_ColorDA3 );
	gtk_widget_show( GTK_WIDGET( op->m_BG ) );
	gtk_widget_show( GTK_WIDGET( op->m_ColorDA3 ) );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_BG ), FALSE, FALSE, 0 );

	g_signal_connect( op->m_BG, "clicked", G_CALLBACK( ChangeColor3 ), base );

	/* Modes */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );

	label = gtk_label_new( _("Mode:") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );

	op->m_OptionMenu = gtk_option_menu_new();
	gtk_widget_show( op->m_OptionMenu );
	gtk_box_pack_start( GTK_BOX( hbox ), op->m_OptionMenu, FALSE, FALSE, 0 );

	op->m_Menu = gtk_menu_new();
	gtk_option_menu_set_menu( GTK_OPTION_MENU( op->m_OptionMenu ), op->m_Menu );

	op->m_MenuItem = gtk_menu_item_new_with_label( _("Normal") );
	gtk_widget_show( op->m_MenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_Menu ), op->m_MenuItem );

	op->m_MenuItem = gtk_menu_item_new_with_label( _("LED") );
	gtk_widget_show( op->m_MenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_Menu ), op->m_MenuItem );

	op->m_MenuItem = gtk_menu_item_new_with_label( _("No history") );
	gtk_widget_show( op->m_MenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_Menu ), op->m_MenuItem );

	gtk_option_menu_set_history( GTK_OPTION_MENU( op->m_OptionMenu ), base->m_Mode );

	g_signal_connect( op->m_OptionMenu, "changed", G_CALLBACK( ModeChange ), base );

	/* Color mode */

	hbox = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( hbox ) );
	gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( hbox ), FALSE, FALSE, 0 );
	label = gtk_label_new( _("Color mode: ") );
	gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
	gtk_size_group_add_widget( sg, label );
	gtk_widget_show( label );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );

	op->m_ModeOption = gtk_option_menu_new();
	gtk_widget_show( op->m_ModeOption );
	gtk_box_pack_start( GTK_BOX( hbox ), op->m_ModeOption, FALSE, FALSE, 0 );

	op->m_ModeMenu = gtk_menu_new();
	gtk_option_menu_set_menu( GTK_OPTION_MENU( op->m_ModeOption ), op->m_ModeMenu );

	op->m_ModeMenuItem = gtk_menu_item_new_with_label( _("None") );
	gtk_widget_show( op->m_ModeMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_ModeMenu ), op->m_ModeMenuItem );

	op->m_ModeMenuItem = gtk_menu_item_new_with_label( _("Gradient") );
	gtk_widget_show( op->m_ModeMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_ModeMenu ), op->m_ModeMenuItem );

	op->m_ModeMenuItem = gtk_menu_item_new_with_label( _("Fire") );
	gtk_widget_show( op->m_ModeMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_ModeMenu ), op->m_ModeMenuItem );

	op->m_ModeMenuItem = gtk_menu_item_new_with_label( "cpufreq" );
	gtk_widget_show( op->m_ModeMenuItem );
	gtk_menu_shell_append( GTK_MENU_SHELL( op->m_ModeMenu ), op->m_ModeMenuItem );

	gtk_option_menu_set_history( GTK_OPTION_MENU( op->m_ModeOption ), base->m_ColorMode );

	g_signal_connect( op->m_ModeOption, "changed", G_CALLBACK( ColorModeChange ), base );

	gtk_widget_show_all( GTK_WIDGET( hbox ) );

	op->m_Notebook = gtk_notebook_new();
	gtk_container_set_border_width( GTK_CONTAINER( op->m_Notebook ), BORDER - 2 );
	label = gtk_label_new( _("Appearance") );
	gtk_notebook_append_page( GTK_NOTEBOOK( op->m_Notebook ), GTK_WIDGET( vbox2 ), GTK_WIDGET( label ) );
	label = gtk_label_new( _("Advanced") );
	gtk_notebook_append_page( GTK_NOTEBOOK( op->m_Notebook ), GTK_WIDGET( vbox ), GTK_WIDGET( label ) );
	gtk_widget_show( op->m_Notebook );

	gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dlg )->vbox), GTK_WIDGET( op->m_Notebook ), TRUE, TRUE, 0 );

	gtk_widget_show( dlg );
}
