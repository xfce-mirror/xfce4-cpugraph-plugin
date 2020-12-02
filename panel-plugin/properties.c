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
#include <math.h>

#ifndef _
# include <libintl.h>
# define _(String) gettext (String)
#endif

typedef struct
{
    CPUGraph *base;
    GtkBox *hbox_in_terminal, *hbox_startup_notification;
} CPUGraphOptions;

static GtkBox *create_tab                    ();
static GtkBox *create_option_line            (GtkBox       *tab,
                                              GtkSizeGroup *sg,
                                              const gchar  *name);
static GtkBox* create_check_box              (GtkBox       *tab,
                                              GtkSizeGroup *sg,
                                              const gchar  *name,
                                              gboolean      init,
                                              void (callback)(GtkToggleButton *, CPUGraph *),
                                              void         *cb_data);
static GtkWidget* create_drop_down           (GtkBox       *tab,
                                              GtkSizeGroup *sg,
                                              const gchar  *name,
                                              const gchar **items,
                                              gsize         nb_items,
                                              guint         init,
                                              void (callback)(GtkComboBox *, CPUGraph *),
                                              void         *cb_data);
static void    destroy_cb                    (GtkWidget       *dlg,
                                              CPUGraphOptions *dlg_data);

static void    setup_update_interval_option  (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base);
static void    setup_tracked_core_option     (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base);
static void    setup_size_option             (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              XfcePanelPlugin *plugin,
                                              CPUGraph        *base);
static void    setup_command_option          (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraphOptions *data);
static void    setup_color_option            (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base,
                                              guint            number,
                                              const gchar     *name,
                                              GCallback        cb);
static void    setup_mode_option             (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base);
static void    setup_color_mode_option       (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base);
static void    setup_load_threshold_option   (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base);

static void    change_in_terminal            (GtkToggleButton *button,
                                              CPUGraph        *base);
static void    change_startup_notification   (GtkToggleButton *button,
                                              CPUGraph        *base);
static void    change_command                (GtkEntry        *entry,
                                              CPUGraphOptions *data);
static void    change_color_0                (GtkColorButton  *button,
                                              CPUGraph        *base);
static void    change_color_1                (GtkColorButton  *button,
                                              CPUGraph        *base);
static void    change_color_2                (GtkColorButton  *button,
                                              CPUGraph        *base);
static void    change_color_3                (GtkColorButton  *button,
                                              CPUGraph        *base);
static void    change_color_4                (GtkColorButton  *button,
                                              CPUGraph        *base);
static void    update_sensitivity            (CPUGraph        *base);
static void    change_mode                   (GtkComboBox     *om,
                                              CPUGraph        *base);
static void    change_color_mode             (GtkComboBox     *om,
                                              CPUGraph        *base);
static void    response_cb                   (GtkWidget       *dlg,
                                              gint             response,
                                              CPUGraph        *base);
static void    change_frame                  (GtkToggleButton *button,
                                              CPUGraph        *base);
static void    change_border                 (GtkToggleButton *button,
                                              CPUGraph        *base);
static void    change_bars                   (GtkToggleButton *button,
                                              CPUGraph        *base);
static void    change_size                   (GtkSpinButton   *sb,
                                              CPUGraph        *base);
static void    change_time_scale             (GtkToggleButton *button,
                                              CPUGraph        *base);
static void    change_update                 (GtkComboBox     *om,
                                              CPUGraph        *base);
static void    change_core                   (GtkComboBox     *combo,
                                              CPUGraph        *base);
static void    change_load_threshold         (GtkSpinButton   *sb,
                                              CPUGraph        *base);

void
create_options (XfcePanelPlugin *plugin, CPUGraph *base)
{
    GtkWidget *dlg, *content;
    GtkBox *vbox, *vbox2;
    GtkWidget *label, *notebook;
    GtkSizeGroup *sg;
    CPUGraphOptions *dlg_data;

    xfce_panel_plugin_block_menu (plugin);

    dlg = xfce_titled_dialog_new_with_buttons (_("CPU Graph Properties"),
                                       GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       "_Close",
                                       GTK_RESPONSE_OK,
                                       NULL);

    dlg_data = g_new0 (CPUGraphOptions, 1);
    dlg_data->base = base;

    g_signal_connect (dlg, "destroy", G_CALLBACK (destroy_cb), dlg_data);
    g_signal_connect (dlg, "response", G_CALLBACK (response_cb), base);

    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-cpugraph-plugin");

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    vbox = create_tab ();
    setup_update_interval_option (vbox, sg, base);
    setup_tracked_core_option (vbox, sg, base);
    setup_size_option (vbox, sg, plugin, base);
    setup_load_threshold_option (vbox, sg, base);
    create_check_box (vbox, sg, _("Use non-linear time-scale"), base->non_linear, change_time_scale, base);
    create_check_box (vbox, sg, _("Show frame"), base->has_frame, change_frame, base);
    create_check_box (vbox, sg, _("Show border"), base->has_border, change_border, base);
    create_check_box (vbox, sg, ngettext ("Show current usage bar", "Show current usage bars", base->nr_cores), base->has_bars, change_bars, base);

    setup_command_option (vbox, sg, dlg_data);
    dlg_data->hbox_in_terminal = create_check_box (vbox, sg, _("Run in terminal"),
                                                   base->in_terminal, change_in_terminal, base);
    dlg_data->hbox_startup_notification = create_check_box (vbox, sg, _("Use startup notification"),
                                                            base->startup_notification, change_startup_notification, base);
    if (!base->command)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (dlg_data->hbox_in_terminal), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (dlg_data->hbox_startup_notification), FALSE);
    }

    vbox2 = create_tab ();
    setup_color_option (vbox2, sg, base, 1, _("Color 1:"), G_CALLBACK (change_color_1));
    setup_color_option (vbox2, sg, base, 2, _("Color 2:"), G_CALLBACK (change_color_2));
    setup_color_option (vbox2, sg, base, 3, _("Color 3:"), G_CALLBACK (change_color_3));
    setup_color_option (vbox2, sg, base, 0, _("Background:"), G_CALLBACK (change_color_0));
    setup_mode_option (vbox2, sg, base);
    setup_color_mode_option (vbox2, sg, base);
    setup_color_option (vbox2, sg, base, 4, _("Bars color:"), G_CALLBACK (change_color_4));
    update_sensitivity (base);

    notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (notebook), BORDER - 2);
    label = gtk_label_new (_("Appearance"));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (vbox2), GTK_WIDGET (label));
    label = gtk_label_new (_("Advanced"));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (vbox), GTK_WIDGET (label));

    content = gtk_dialog_get_content_area (GTK_DIALOG (dlg));
    gtk_container_add (GTK_CONTAINER (content), notebook);

    gtk_widget_show_all (dlg);
}

static GtkBox *
create_tab (void)
{
    GtkBox *tab;
    tab = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER));
    gtk_container_set_border_width (GTK_CONTAINER (tab), BORDER);
    return tab;
}

static GtkBox *
create_option_line (GtkBox *tab, GtkSizeGroup *sg, const gchar *name)
{
    GtkBox *line;
    GtkWidget *label;

    line = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER));
    gtk_box_pack_start (GTK_BOX (tab), GTK_WIDGET (line), FALSE, FALSE, 0);

    if (name)
    {
        label = gtk_label_new (name);
        gtk_label_set_xalign (GTK_LABEL (label), 0.0);
        gtk_label_set_yalign (GTK_LABEL (label), 0.5);
        gtk_size_group_add_widget (sg, label);
        gtk_box_pack_start (GTK_BOX (line), GTK_WIDGET (label), FALSE, FALSE, 0);
    }

    return line;
}

static GtkBox*
create_check_box (GtkBox *tab, GtkSizeGroup *sg, const gchar *name, gboolean init,
                  void (callback)(GtkToggleButton *, CPUGraph *), void *cb_data)
{
    GtkBox *hbox;
    GtkWidget *checkbox;

    hbox = create_option_line (tab, sg, NULL);

    checkbox = gtk_check_button_new_with_mnemonic (name);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), init);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (checkbox), FALSE, FALSE, 0);
    g_signal_connect (checkbox, "toggled", G_CALLBACK (callback), cb_data);

    return hbox;
}

static GtkWidget*
create_drop_down (GtkBox *tab, GtkSizeGroup *sg, const gchar *name,
                  const gchar **items, gsize nb_items, guint init,
                  void (callback)(GtkComboBox *, CPUGraph *), void * cb_data)
{
    GtkBox *hbox;
    GtkWidget *combo;
    gsize i;

    hbox = create_option_line (tab, sg, name);

    combo = gtk_combo_box_text_new ();
    for (i = 0; i < nb_items; i++)
    {
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), NULL, items[i]);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), init);
    gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);

    g_signal_connect (combo, "changed", G_CALLBACK (callback), cb_data);

    return combo;
}

static void
destroy_cb (GtkWidget *dlg, CPUGraphOptions *dlg_data)
{
    g_free (dlg_data);
}

static void
setup_update_interval_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base)
{
    const gchar *items[] = { _("Fastest (~250ms)"),
                             _("Fast (~500ms)"),
                             _("Normal (~750ms)"),
                             _("Slow (~1s)"),
                             _("Slowest (~3s)")
                           };
    gsize nb_items = sizeof (items) / sizeof (gchar*);

    create_drop_down (vbox, sg, _("Update Interval:"), items, nb_items, base->update_interval, change_update, base);
}

static void
setup_tracked_core_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base)
{
    gsize nb_items = base->nr_cores + 1;
    gchar *items[ nb_items ];
    guint i;
    items[0] = _("All");

    for (i = 1; i < nb_items; i++)
    {
        items[i] = g_strdup_printf ("%u", i);
    }

    create_drop_down (vbox, sg, _("Tracked Core:"), (const gchar **) items, nb_items, base->tracked_core, change_core, base);
    for (i = 1; i < nb_items; i++)
        g_free (items[i]);
}

static void
setup_size_option (GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, CPUGraph *base)
{
    GtkBox *hbox;
    GtkWidget *size;

    if (xfce_panel_plugin_get_orientation (plugin) == GTK_ORIENTATION_HORIZONTAL)
        hbox = create_option_line (vbox, sg, _("Width:"));
    else
        hbox = create_option_line (vbox, sg, _("Height:"));

    size = gtk_spin_button_new_with_range (10, 128, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (size), base->size);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (size), FALSE, FALSE, 0);
    g_signal_connect (size, "value-changed", G_CALLBACK (change_size), base);
}

static void
setup_load_threshold_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base)
{
    GtkBox *hbox;
    GtkWidget *threshold;

    hbox = create_option_line (vbox, sg, _("Threshold (%):"));
    threshold = gtk_spin_button_new_with_range (0, (gint) roundf (100 * MAX_LOAD_THRESHOLD), 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (threshold), (gint) roundf (100 * base->load_threshold));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (threshold), FALSE, FALSE, 0);
    g_signal_connect (threshold, "value-changed", G_CALLBACK (change_load_threshold), base);
}

static void
setup_command_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraphOptions *data)
{
    GtkBox *hbox;
    GtkWidget *associatecommand;

    hbox = create_option_line (vbox, sg, _("Associated command:"));

    associatecommand = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (associatecommand), data->base->command ? data->base->command : "");
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (associatecommand),
                                       GTK_ENTRY_ICON_SECONDARY,
                                       "help-contents");
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (associatecommand),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Defaults to xfce4-taskmanager, htop or top."));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (associatecommand), FALSE, FALSE, 0);
    g_signal_connect (associatecommand, "changed", G_CALLBACK (change_command), data);
}

static void
setup_color_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base,
                    guint number, const gchar * name, GCallback cb)
{
    GtkBox *hbox;

    hbox = create_option_line (vbox, sg, name);

    base->color_buttons[number] = gtk_color_button_new_with_rgba (&base->colors[number]);
    gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (base->color_buttons[number]), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (base->color_buttons[number]), FALSE, FALSE, 0);

    g_signal_connect (base->color_buttons[number], "color-set", cb, base);
}

static void
setup_mode_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base)
{
    const gchar *items[] = { _("Disabled"),
                             _("Normal"),
                             _("LED"),
                             _("No history"),
                             _("Grid")
                           };
    gsize nb_items = sizeof (items) / sizeof (gchar*);
    gint selected;

    switch (base->mode)
    {
        case MODE_DISABLED:
            selected = 0;
            break;
        case MODE_NORMAL:
            selected = 1;
            break;
        case MODE_LED:
            selected = 2;
            break;
        case MODE_NO_HISTORY:
            selected = 3;
            break;
        case MODE_GRID:
            selected = 4;
            break;
        default:
            selected = 0;
    }

    create_drop_down (vbox, sg, _("Mode:"), items, nb_items, selected, change_mode, base);
}

static void
setup_color_mode_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base)
{
    const gchar *items[] = { _("Solid"),
                             _("Gradient"),
                             _("Fire"),
                           };
    gsize nb_items = sizeof (items) / sizeof (gchar*);

    base->color_mode_combobox = create_drop_down (vbox, sg, _("Color mode: "), items, nb_items,
                                                  base->color_mode, change_color_mode, base);
}

static void
change_in_terminal (GtkToggleButton *button, CPUGraph *base)
{
    set_in_terminal (base, gtk_toggle_button_get_active (button));
}

static void
change_startup_notification (GtkToggleButton *button, CPUGraph *base)
{
    set_startup_notification (base, gtk_toggle_button_get_active (button));
}

static void
change_command (GtkEntry *entry, CPUGraphOptions *data)
{
    gboolean default_command;
    set_command (data->base, gtk_entry_get_text (entry));
    default_command = (data->base->command == NULL);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_in_terminal), !default_command);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_startup_notification), !default_command);
}

static void
change_color (GtkColorButton *button, CPUGraph *base, guint number)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
    set_color (base, number, color);
}

static void
change_color_1 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, 1);
}

static void
change_color_2 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, 2);
}

static void
change_color_3 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, 3);
}

static void
change_color_0 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, 0);
}

static void
change_color_4 (GtkColorButton *button, CPUGraph *base)
{
    base->has_barcolor = TRUE;
    change_color (button, base, 4);
}

static void
update_sensitivity (CPUGraph *base)
{
    if (base->color_mode != 0 || base->mode == MODE_LED || base->mode == MODE_GRID)
        gtk_widget_set_sensitive (gtk_widget_get_parent (base->color_buttons[2]), TRUE);
    else
        gtk_widget_set_sensitive (gtk_widget_get_parent (base->color_buttons[2]), FALSE);

    if (base->color_mode != 0 && base->mode == MODE_LED)
        gtk_widget_set_sensitive (gtk_widget_get_parent (base->color_buttons[3]), TRUE);
    else
        gtk_widget_set_sensitive (gtk_widget_get_parent (base->color_buttons[3]), FALSE);

    gtk_widget_set_sensitive (gtk_widget_get_parent (base->color_buttons[4]), base->has_bars);

    gtk_widget_set_sensitive (gtk_widget_get_parent (base->color_mode_combobox), base->mode != MODE_GRID);
}

static void
change_mode (GtkComboBox *combo, CPUGraph *base)
{
    /* 'Disabled' mode was introduced in 1.1.0 as '-1'
     * for this reason we need to decrement the selected value */
    gint selected = gtk_combo_box_get_active (combo) - 1;
    CPUGraphMode mode;

    switch (selected)
    {
        case MODE_DISABLED:
        case MODE_NORMAL:
        case MODE_LED:
        case MODE_NO_HISTORY:
        case MODE_GRID:
            mode = selected;
            break;
        default:
            mode = MODE_NORMAL;
    }

    set_mode (base, mode);
    update_sensitivity (base);
}

static void
change_color_mode (GtkComboBox *combo, CPUGraph *base)
{
    set_color_mode (base, gtk_combo_box_get_active (combo));
    update_sensitivity (base);
}

static void
response_cb (GtkWidget *dlg, gint response, CPUGraph *base)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (base->plugin);
    write_settings (base->plugin, base);
}

static void
change_frame (GtkToggleButton *button, CPUGraph *base)
{
    set_frame (base, gtk_toggle_button_get_active (button));
}

static void
change_border (GtkToggleButton *button, CPUGraph *base)
{
    set_border (base, gtk_toggle_button_get_active (button));
}

static void
change_bars (GtkToggleButton *button, CPUGraph *base)
{
    set_bars (base, gtk_toggle_button_get_active (button));
    update_sensitivity (base);
}

static void
change_size (GtkSpinButton *sb, CPUGraph *base)
{
    set_size (base, gtk_spin_button_get_value_as_int (sb));
}

static void
change_load_threshold (GtkSpinButton *sb, CPUGraph *base)
{
    set_load_threshold (base, gtk_spin_button_get_value (sb) / 100);
}

static void change_time_scale (GtkToggleButton *button, CPUGraph *base)
{
    set_nonlinear_time (base, gtk_toggle_button_get_active (button));
}

static void change_update (GtkComboBox *combo, CPUGraph *base)
{
    set_update_rate (base, gtk_combo_box_get_active (combo));
}

static void change_core (GtkComboBox *combo, CPUGraph *base)
{
    set_tracked_core (base, gtk_combo_box_get_active (combo));
}
