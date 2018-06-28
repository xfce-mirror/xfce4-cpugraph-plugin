/*  properties.c
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "cpu.h"
#include "properties.h"
#include "settings.h"

#include <glib/gprintf.h>
#include <libxfce4ui/libxfce4ui.h>

#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

static GtkBox *create_tab();
static GtkBox *create_option_line( GtkBox *tab, GtkSizeGroup *sg, const gchar *name );
static void create_check_box( GtkBox *tab, GtkSizeGroup *sg, const gchar *name, gboolean init, void (callback)( GtkToggleButton *, CPUGraph *), void *cb_data );
static void create_drop_down( GtkBox *tab, GtkSizeGroup *sg, const gchar * name, const gchar **items, gsize nb_items, guint init, void (callback)( GtkComboBox *, CPUGraph * ), void * cb_data);

static void setup_update_interval_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );
static void setup_tracked_core_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );
static void setup_size_option( GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, CPUGraph *base );
static void setup_command_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );
static void setup_color_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base, guint number, const gchar *name, GCallback cb );
static void setup_mode_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );
static void setup_color_mode_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base );

static void change_in_terminal( GtkToggleButton *button, CPUGraph *base );
static void change_startup_notification( GtkToggleButton *button, CPUGraph *base );
static void change_command( GtkEntry *entry, CPUGraph *base );
static void change_color_0( GtkColorButton *button, CPUGraph *base );
static void change_color_1( GtkColorButton * button, CPUGraph * base );
static void change_color_2( GtkColorButton *button, CPUGraph *base );
static void change_color_3( GtkColorButton *button, CPUGraph *base );
static void change_color_4( GtkColorButton *button, CPUGraph *base );
static void select_active_colors( CPUGraph * base );
static void select_active_barscolors( CPUGraph * base );
static void change_mode( GtkComboBox *om, CPUGraph *base );
static void change_color_mode( GtkComboBox *om, CPUGraph *base );
static void response_cb( GtkWidget *dlg, gint response, CPUGraph *base );
static void change_frame( GtkToggleButton *button, CPUGraph *base );
static void change_border( GtkToggleButton *button, CPUGraph *base );
static void change_bars( GtkToggleButton * button, CPUGraph * base );
static void change_size( GtkSpinButton *sb, CPUGraph *base );
static void change_time_scale( GtkToggleButton *button, CPUGraph *base );
static void change_update( GtkComboBox *om, CPUGraph *base );
static void change_core( GtkComboBox * combo, CPUGraph * base );

void create_options( XfcePanelPlugin *plugin, CPUGraph *base )
{
	GtkWidget *dlg, *header, *content;
	GtkBox *vbox, *vbox2;
	GtkWidget *label;
	GtkSizeGroup *sg;
	GtkWidget *Notebook;

	xfce_panel_plugin_block_menu( plugin );

	dlg = xfce_titled_dialog_new_with_buttons( _("CPU Graph Properties"),
	                                   GTK_WINDOW( gtk_widget_get_toplevel( GTK_WIDGET( plugin ) ) ),
	                                   GTK_DIALOG_DESTROY_WITH_PARENT,
	                                   "_Close",
	                                   GTK_RESPONSE_OK,
	                                   NULL
					 );

	g_signal_connect( dlg, "response", G_CALLBACK( response_cb ), base );

	gtk_window_set_icon_name( GTK_WINDOW (dlg), "xfce4-cpugraph-plugin" );

	sg = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );

	vbox = create_tab();
	setup_update_interval_option( vbox, sg, base );
	setup_tracked_core_option( vbox, sg, base );
	setup_size_option( vbox, sg, plugin, base );
	create_check_box( vbox, sg, _("Use non-linear time-scale"), base->non_linear, change_time_scale, base );
	create_check_box( vbox, sg, _("Show frame"), base->has_frame, change_frame, base );
	create_check_box( vbox, sg, _("Show border"), base->has_border, change_border, base );
	create_check_box( vbox, sg, ngettext( "Show current usage bar", "Show current usage bars", base->nr_cores ), base->has_bars, change_bars, base );
	setup_command_option( vbox, sg, base );
	create_check_box( vbox, sg, _("Run in terminal"), base->in_terminal, change_in_terminal, base );
	create_check_box( vbox, sg, _("Use startup notification"), base->startup_notification, change_startup_notification, base );

	vbox2 = create_tab();
	setup_color_option( vbox2, sg, base, 1, _("Color 1:"), G_CALLBACK( change_color_1 ) );
	setup_color_option( vbox2, sg, base, 2, _("Color 2:"), G_CALLBACK( change_color_2 ) );
	setup_color_option( vbox2, sg, base, 3, _("Color 3:"), G_CALLBACK( change_color_3 ) );
	setup_color_option( vbox2, sg, base, 0, _("Background:"), G_CALLBACK( change_color_0 ) );
	select_active_colors( base );
	setup_mode_option( vbox2, sg, base );
	setup_color_mode_option( vbox2, sg, base );
	setup_color_option( vbox2, sg, base, 4, _("Bars color:"), G_CALLBACK( change_color_4 ) );
	select_active_barscolors( base );

	Notebook = gtk_notebook_new();
	gtk_container_set_border_width( GTK_CONTAINER( Notebook ), BORDER - 2 );
	label = gtk_label_new( _("Appearance") );
	gtk_notebook_append_page( GTK_NOTEBOOK( Notebook ), GTK_WIDGET( vbox2 ), GTK_WIDGET( label ) );
	label = gtk_label_new( _("Advanced") );
	gtk_notebook_append_page( GTK_NOTEBOOK( Notebook ), GTK_WIDGET( vbox ), GTK_WIDGET( label ) );
	gtk_widget_show( Notebook );

	content = gtk_dialog_get_content_area( GTK_DIALOG( dlg ) );
	gtk_container_add( GTK_CONTAINER( content ), Notebook );

	gtk_widget_show( dlg );
}

static GtkBox *create_tab()
{
	GtkBox *tab;
	tab = GTK_BOX( gtk_box_new( GTK_ORIENTATION_VERTICAL, BORDER ) );
	gtk_container_set_border_width( GTK_CONTAINER( tab ), BORDER );
	gtk_widget_show( GTK_WIDGET( tab ) );
	return tab;
}

static GtkBox *create_option_line( GtkBox *tab, GtkSizeGroup *sg, const gchar *name )
{
	GtkBox *line;
	GtkWidget *label;

	line = GTK_BOX( gtk_box_new( GTK_ORIENTATION_HORIZONTAL, BORDER ) );
	gtk_widget_show( GTK_WIDGET( line ) );
	gtk_box_pack_start( GTK_BOX( tab ), GTK_WIDGET( line ), FALSE, FALSE, 0 );
	
	if( name )
	{
		label = gtk_label_new( name );
		gtk_label_set_xalign( GTK_LABEL( label ), 0.0 );
		gtk_label_set_yalign( GTK_LABEL( label ), 0.5 );
		gtk_size_group_add_widget( sg, label );
		gtk_widget_show( label );
		gtk_box_pack_start( GTK_BOX( line ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
	}

	return line;
}

static void create_check_box( GtkBox *tab, GtkSizeGroup *sg, const gchar *name, gboolean init, void (callback)( GtkToggleButton *, CPUGraph *), void *cb_data )
{
	GtkBox *hbox;
	GtkWidget * checkBox;

	hbox = create_option_line( tab, sg, NULL );

	checkBox = gtk_check_button_new_with_mnemonic( name );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( checkBox ), init );
	gtk_widget_show( checkBox );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( checkBox ), FALSE, FALSE, 0 );
	g_signal_connect( checkBox, "toggled", G_CALLBACK( callback ), cb_data );
}

static void create_drop_down( GtkBox *tab, GtkSizeGroup *sg, const gchar * name, const gchar ** items, gsize nb_items, guint init, void (callback)( GtkComboBox *, CPUGraph * ), void * cb_data)
{
	GtkBox *hbox;
	GtkWidget *combo;
	gint i;

	hbox = create_option_line( tab, sg, name );

	combo = gtk_combo_box_text_new();
	for( i = 0; i < nb_items; i++ )
	{
		gtk_combo_box_text_append( GTK_COMBO_BOX_TEXT( combo ), NULL, items[i] );
	}
	gtk_combo_box_set_active( GTK_COMBO_BOX( combo), init );
	gtk_box_pack_start( GTK_BOX( hbox ), combo, FALSE, FALSE, 0 );
	gtk_widget_show( combo );


	g_signal_connect( combo, "changed", G_CALLBACK( callback ), cb_data );
}

static void setup_update_interval_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	const gchar *items[] = { _("Fastest (~250ms)"),
	                        _("Fast (~500ms)"),
	                        _("Normal (~750ms)"),
	                        _("Slow (~1s)")
	                      };
	gsize nb_items = sizeof( items ) / sizeof( gchar* );

	create_drop_down( vbox, sg, _("Update Interval:"), items, nb_items, base->update_interval, change_update, base);
}

static void setup_tracked_core_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	gsize nb_items = base->nr_cores + 1;
	gchar *items[ nb_items ];
	guint i;
	items[0] = _("All");
	for( i = 1; i < nb_items; i++ )
	{
		items[i] = g_strdup_printf( "%u", i );
	}
	create_drop_down( vbox, sg, _("Tracked Core:"), (const gchar **)items, nb_items, base->tracked_core, change_core, base);
	for( i = 1; i < nb_items; i++ )
		g_free( items[i] );
}

static void setup_size_option( GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, CPUGraph *base )
{
	GtkBox *hbox;
	GtkWidget *Size;

	if( xfce_panel_plugin_get_orientation( plugin ) == GTK_ORIENTATION_HORIZONTAL )
		hbox = create_option_line( vbox, sg, _("Width:") );
	else
		hbox = create_option_line( vbox, sg, _("Height:") );

	Size = gtk_spin_button_new_with_range( 10, 128, 1 );
	gtk_spin_button_set_value( GTK_SPIN_BUTTON( Size ), base->size );
	gtk_widget_show( Size );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( Size ), FALSE, FALSE, 0 );
	g_signal_connect( Size, "value-changed", G_CALLBACK( change_size ), base );
}

static void setup_command_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	GtkBox *hbox;
	GtkWidget *AssociateCommand;

	hbox = create_option_line( vbox, sg, _("Associated command:") );

	AssociateCommand = gtk_entry_new();
	gtk_entry_set_text( GTK_ENTRY(AssociateCommand), base->command );
	gtk_widget_show( AssociateCommand );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( AssociateCommand ), FALSE, FALSE, 0 );
	g_signal_connect( AssociateCommand, "changed", G_CALLBACK( change_command ), base );
}

static void setup_color_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base, guint number, const gchar * name, GCallback cb )
{
	GtkBox *hbox;

	hbox = create_option_line( vbox, sg, name );

	base->color_buttons[number] = gtk_color_button_new_with_rgba( &base->colors[number] );
	gtk_widget_show( GTK_WIDGET( base->color_buttons[number] ) );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( base->color_buttons[number] ), FALSE, FALSE, 0 );

	g_signal_connect( base->color_buttons[number], "color-set", cb, base );
}

static void setup_mode_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	const gchar *items[] = { _("Normal"),
	                        _("LED"),
	                        _("No history"),
				_("Grid")
	                      };
	gsize nb_items = sizeof( items ) / sizeof( gchar* );

	create_drop_down( vbox, sg, _("Mode:"), items, nb_items, base->mode, change_mode, base);
}

static void setup_color_mode_option( GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base )
{
	const gchar *items[] = { _("Solid"),
	                        _("Gradient"),
	                        _("Fire"),
	                      };
	gsize nb_items = sizeof( items ) / sizeof( gchar* );

	create_drop_down( vbox, sg, _("Color mode: "), items, nb_items, base->color_mode, change_color_mode, base);
}

static void change_in_terminal( GtkToggleButton *button, CPUGraph *base )
{
	set_in_terminal( base, gtk_toggle_button_get_active( button ) );
}

static void change_startup_notification( GtkToggleButton *button, CPUGraph *base )
{
	set_startup_notification( base, gtk_toggle_button_get_active( button ) );
}

static void change_command( GtkEntry *entry, CPUGraph * base )
{
	set_command( base, gtk_entry_get_text( entry ) );
}

static void change_color( GtkColorButton * button, CPUGraph * base, guint number)
{
	GdkRGBA color;
	gtk_color_chooser_get_rgba( GTK_COLOR_CHOOSER( button ), &color );
	set_color( base, number, color );
}

static void change_color_1( GtkColorButton * button, CPUGraph * base )
{
	change_color( button, base, 1);
}

static void change_color_2( GtkColorButton * button, CPUGraph * base )
{
	change_color( button, base, 2);
}

static void change_color_3( GtkColorButton * button, CPUGraph * base )
{
	change_color( button, base, 3);
}

static void change_color_0( GtkColorButton * button, CPUGraph * base )
{
	change_color( button, base, 0);
}

static void change_color_4( GtkColorButton * button, CPUGraph * base )
{
	base->has_barcolor = TRUE;
	change_color( button, base, 4);
}

static void select_active_colors( CPUGraph * base )
{
	if( base->color_mode != 0 || base->mode == 1 || base->mode == 3 )
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[2] ), TRUE );
	else
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[2] ), FALSE );

	if( base->color_mode != 0 && base->mode == 1 )
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[3] ), TRUE );
	else
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[3] ), FALSE );
}

static void select_active_barscolors( CPUGraph * base )
{
	if( base->has_bars )
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[4] ), TRUE );
	else
		gtk_widget_set_sensitive( GTK_WIDGET( base->color_buttons[4] ), FALSE );
}

static void change_mode( GtkComboBox * combo, CPUGraph * base )
{
	set_mode( base, gtk_combo_box_get_active( combo ) );
	select_active_colors( base );
}

static void change_color_mode( GtkComboBox * combo, CPUGraph * base )
{
	set_color_mode( base, gtk_combo_box_get_active( combo ) );
	select_active_colors( base );
}

static void response_cb( GtkWidget *dlg, gint response, CPUGraph *base )
{
	gtk_widget_destroy( dlg );
	xfce_panel_plugin_unblock_menu( base->plugin );
	write_settings( base->plugin, base );
}

static void change_frame( GtkToggleButton * button, CPUGraph * base )
{
	set_frame( base, gtk_toggle_button_get_active( button ) );
}

static void change_border( GtkToggleButton * button, CPUGraph * base )
{
	set_border( base, gtk_toggle_button_get_active( button ) );
}

static void change_bars( GtkToggleButton * button, CPUGraph * base )
{
	set_bars( base, gtk_toggle_button_get_active( button ) );
	select_active_barscolors( base );
}

static void change_size( GtkSpinButton * sb, CPUGraph *base)
{
	set_size( base, gtk_spin_button_get_value_as_int( sb ) );
}

static void change_time_scale( GtkToggleButton * button, CPUGraph * base )
{
	set_nonlinear_time( base, gtk_toggle_button_get_active( button ) );
}

static void change_update( GtkComboBox * combo, CPUGraph * base )
{
	set_update_rate( base, gtk_combo_box_get_active( combo ) );
}

static void change_core( GtkComboBox * combo, CPUGraph * base )
{
	set_tracked_core( base, gtk_combo_box_get_active( combo ) );
}
