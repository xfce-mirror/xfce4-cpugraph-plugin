#include <actions.h>

static GtkBox *CreateTab();
static GtkBox *CreateOptionLine( GtkBox *tab, GtkSizeGroup *sg, const char *name );
static void CreateCheckBox( GtkBox *tab, GtkSizeGroup *sg, const char *name, int init, void (callback)( GtkToggleButton *, CPUGraph *), void *cb_data );
static void CreateDropDown( GtkBox *tab, GtkSizeGroup *sg, const char * name, const char **items, size_t nb_items, int init, void (callback)( GtkOptionMenu *, CPUGraph * ), void * cb_data);

static void SetupUpdateIntervalOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );
static void SetupWidthOption( GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, CPUGraph *base );
static void SetupAssociateCommandOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );
static void SetupForeground1Option( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base );
static void SetupForeground2Option( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base );
static void SetupForeground3Option( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base );
static void SetupBackgroundOption( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base );
static void SetupModesOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );
static void SetupColormodeOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );


void CreateOptions( XfcePanelPlugin *plugin, CPUGraph *base )
{
	GtkWidget *dlg, *header;
	GtkBox *vbox, *vbox2;
	GtkWidget *label;
	GtkSizeGroup *sg;
	SOptions *op = &base->m_Options;
	GtkWidget *Notebook;

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

	vbox = CreateTab();

	sg = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );

	SetupUpdateIntervalOption( vbox, sg, base );

	SetupWidthOption( vbox, sg, plugin, base );

	CreateCheckBox( vbox, sg, _("Non-linear time-scale"), base->m_TimeScale, TimeScaleChange, base );

	CreateCheckBox( vbox, sg, _("Show frame"), base->m_Frame, FrameChange, base );

	SetupAssociateCommandOption( vbox, sg, base );

	vbox2 = CreateTab();

	SetupForeground1Option( vbox2, sg, op, base );

	SetupForeground2Option( vbox2, sg, op, base );

	SetupForeground3Option( vbox2, sg, op, base );

	SetupBackgroundOption( vbox2, sg, op, base );

	SetupModesOption( vbox2, sg, base );

	SetupColormodeOption( vbox2, sg, base );

	Notebook = gtk_notebook_new();
	gtk_container_set_border_width( GTK_CONTAINER( Notebook ), BORDER - 2 );
	label = gtk_label_new( _("Appearance") );
	gtk_notebook_append_page( GTK_NOTEBOOK( Notebook ), GTK_WIDGET( vbox2 ), GTK_WIDGET( label ) );
	label = gtk_label_new( _("Advanced") );
	gtk_notebook_append_page( GTK_NOTEBOOK( Notebook ), GTK_WIDGET( vbox ), GTK_WIDGET( label ) );
	gtk_widget_show( Notebook );

	gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dlg )->vbox), GTK_WIDGET( Notebook ), TRUE, TRUE, 0 );

	gtk_widget_show( dlg );
}

static GtkBox *CreateTab()
{
	GtkBox *tab;
	tab = GTK_BOX( gtk_vbox_new( FALSE, BORDER ) );
	gtk_container_set_border_width( GTK_CONTAINER( tab ), BORDER );
	gtk_widget_show( GTK_WIDGET( tab ) );
	return tab;
}

static GtkBox *CreateOptionLine( GtkBox *tab, GtkSizeGroup *sg, const char *name )
{
	GtkBox *line;
	GtkWidget *label;

	line = GTK_BOX( gtk_hbox_new( FALSE, BORDER ) );
	gtk_widget_show( GTK_WIDGET( line ) );
	gtk_box_pack_start( GTK_BOX( tab ), GTK_WIDGET( line ), FALSE, FALSE, 0 );
	
	if( name )
	{
		label = gtk_label_new( name );
		gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
		gtk_size_group_add_widget( sg, label );
		gtk_widget_show( label );
		gtk_box_pack_start( GTK_BOX( line ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
	}

	return line;
}

static void CreateCheckBox( GtkBox *tab, GtkSizeGroup *sg, const char *name, int init, void (callback)( GtkToggleButton *, CPUGraph *), void *cb_data )
{
	GtkBox *hbox;
	GtkWidget * checkBox;

	hbox = CreateOptionLine( tab, sg, NULL );

	checkBox = gtk_check_button_new_with_mnemonic( name );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( checkBox ), init );
	gtk_widget_show( checkBox );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( checkBox ), FALSE, FALSE, 0 );
	g_signal_connect( checkBox, "toggled", G_CALLBACK( callback ), cb_data );
	gtk_size_group_add_widget( sg, checkBox );
}

static void CreateDropDown( GtkBox *tab, GtkSizeGroup *sg, const char * name, const char ** items, size_t nb_items, int init, void (callback)( GtkOptionMenu *, CPUGraph * ), void * cb_data)
{
	GtkBox *hbox;
	GtkWidget *Option;
	GtkWidget *Menu;
	GtkWidget *MenuItem;
	int i;

	hbox = CreateOptionLine( tab, sg, name );

	Option = gtk_option_menu_new();
	gtk_widget_show( Option );
	gtk_box_pack_start( GTK_BOX( hbox ), Option, FALSE, FALSE, 0 );

	Menu = gtk_menu_new();
	gtk_option_menu_set_menu( GTK_OPTION_MENU( Option ), Menu );

	for( i = 0; i < nb_items; i++ )
	{
		MenuItem = gtk_menu_item_new_with_label( items[i] );
		gtk_widget_show( MenuItem );
		gtk_menu_shell_append( GTK_MENU_SHELL( Menu ), MenuItem );
	}

	gtk_option_menu_set_history( GTK_OPTION_MENU( Option ), init );

	g_signal_connect( Option, "changed", G_CALLBACK( callback ), cb_data );
}

static void SetupUpdateIntervalOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	const char *items[] = { _("Fastest (~250ms)"),
	                        _("Fast (~500ms)"),
	                        _("Normal (~750ms)"),
	                        _("Slow (~1s)")
	                      };
	size_t nb_items = sizeof( items ) / sizeof( char* );

	CreateDropDown( vbox, sg, _("Update Interval: "), items, nb_items, base->m_UpdateInterval, UpdateChange, base);
}

static void SetupWidthOption( GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, CPUGraph *base )
{
	GtkBox *hbox;
	GtkWidget *Size;

	if( xfce_panel_plugin_get_orientation( plugin ) == GTK_ORIENTATION_HORIZONTAL )
		hbox = CreateOptionLine( vbox, sg, _("Width:") );
	else
		hbox = CreateOptionLine( vbox, sg, _("Height:") );

	Size = gtk_spin_button_new_with_range( 10, 128, 1 );
	gtk_spin_button_set_value( GTK_SPIN_BUTTON( Size ), base->m_Width );
	gtk_widget_show( Size );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( Size ), FALSE, FALSE, 0 );
	g_signal_connect( Size, "value-changed", G_CALLBACK( SpinChange ), &base->m_Width );
}

static void SetupAssociateCommandOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	GtkBox *hbox;
	GtkWidget *AssociateCommand;

	hbox = CreateOptionLine( vbox, sg, _("Associated command :") );

	AssociateCommand = gtk_entry_new();
	gtk_entry_set_text( GTK_ENTRY(AssociateCommand), base->m_AssociateCommand );
	gtk_widget_show( AssociateCommand );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( AssociateCommand ), FALSE, FALSE, 0 );
	g_signal_connect( AssociateCommand, "changed", G_CALLBACK( AssociateCommandChange ), base );
}

static void SetupForeground1Option( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base )
{
	GtkBox *hbox;

	hbox = CreateOptionLine( vbox, sg, _("Color 1:") );

	op->m_FG1 = gtk_button_new();
	op->m_ColorDA = gtk_drawing_area_new();

	gtk_widget_modify_bg( op->m_ColorDA, GTK_STATE_NORMAL, &base->m_ForeGround1 );
	gtk_widget_set_size_request( op->m_ColorDA, 12, 12 );
	gtk_container_add( GTK_CONTAINER( op->m_FG1 ), op->m_ColorDA );
	gtk_widget_show( GTK_WIDGET( op->m_FG1 ) );
	gtk_widget_show( GTK_WIDGET( op->m_ColorDA ) );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_FG1 ), FALSE, FALSE, 0 );

	g_signal_connect( op->m_FG1, "clicked", G_CALLBACK( ChangeColor1 ), base );
}

static void SetupForeground2Option( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base )
{
	GtkBox *hbox;

	hbox = CreateOptionLine( vbox, sg, _("Color 2:") );

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
}

static void SetupForeground3Option( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base )
{
	GtkBox *hbox;

	hbox = CreateOptionLine( vbox, sg, _("Color 3:") );

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

}

static void SetupBackgroundOption( GtkBox *vbox, GtkSizeGroup *sg, SOptions *op, CPUGraph *base )
{
	GtkBox *hbox;

	hbox = CreateOptionLine( vbox, sg, _("Background:") );

	op->m_BG = gtk_button_new();
	op->m_ColorDA3 = gtk_drawing_area_new();

	gtk_widget_modify_bg( op->m_ColorDA3, GTK_STATE_NORMAL, &base->m_BackGround );
	gtk_widget_set_size_request( op->m_ColorDA3, 12, 12 );
	gtk_container_add( GTK_CONTAINER( op->m_BG ), op->m_ColorDA3 );
	gtk_widget_show( GTK_WIDGET( op->m_BG ) );
	gtk_widget_show( GTK_WIDGET( op->m_ColorDA3 ) );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( op->m_BG ), FALSE, FALSE, 0 );

	g_signal_connect( op->m_BG, "clicked", G_CALLBACK( ChangeColor3 ), base );
}

static void SetupModesOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	const char *items[] = { _("Normal"),
	                        _("LED"),
	                        _("No history"),
				_("Grid")
	                      };
	size_t nb_items = sizeof( items ) / sizeof( char* );

	CreateDropDown( vbox, sg, _("Mode:"), items, nb_items, base->m_Mode, ModeChange, base);
}

static void SetupColormodeOption( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	const char *items[] = { _("None"),
	                        _("Gradient"),
	                        _("Fire"),
	                        _("cpufreq")
	                      };
	size_t nb_items = sizeof( items ) / sizeof( char* );

	CreateDropDown( vbox, sg, _("Color mode: "), items, nb_items, base->m_ColorMode, ColorModeChange, base);
}
