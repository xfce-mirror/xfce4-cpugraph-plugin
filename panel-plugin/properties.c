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
    CPUGraph        *base;
    GtkWidget       *color_buttons[NUM_COLORS];
    GtkWidget       *color_mode_combobox;
    GtkBox          *hbox_highlight_smt;
    GtkBox          *hbox_in_terminal;
    GtkBox          *hbox_per_core_spacing;
    GtkBox          *hbox_startup_notification;
    GtkToggleButton *per_core, *show_bars_checkbox;
    GtkLabel        *smt_stats;
    gchar           *smt_stats_tooltip;
    guint           timeout_id;
} CPUGraphOptions;

static GtkBox *create_tab                    (void);
static GtkLabel *create_label_line           (GtkBox          *tab,
                                              const gchar     *text);
static GtkBox *create_option_line            (GtkBox          *tab,
                                              GtkSizeGroup    *sg,
                                              const gchar     *name,
                                              const gchar     *tooltip);
static GtkBox* create_check_box              (GtkBox          *tab,
                                              GtkSizeGroup    *sg,
                                              const gchar     *name,
                                              gboolean        init,
                                              void            (callback)(GtkToggleButton*, CPUGraphOptions*),
                                              CPUGraphOptions *cb_data,
                                              GtkToggleButton **out_checkbox);
static GtkWidget* create_drop_down           (GtkBox          *tab,
                                              GtkSizeGroup    *sg,
                                              const gchar     *name,
                                              const gchar     **items,
                                              gsize           nb_items,
                                              guint           init,
                                              void            (callback)(GtkComboBox*, CPUGraphOptions*),
                                              CPUGraphOptions *cb_data);
static void    destroy_cb                    (GtkWidget       *dlg,
                                              CPUGraphOptions *dlg_data);

static void    setup_update_interval_option  (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraphOptions *data);
static void    setup_tracked_core_option     (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraphOptions *data);
static void    setup_size_option             (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              XfcePanelPlugin *plugin,
                                              CPUGraph        *base);
static void    setup_command_option          (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraphOptions *data);
static void    setup_color_option            (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraphOptions *data,
                                              guint            number,
                                              const gchar     *name,
                                              const gchar     *tooltip,
                                              void            (callback)(GtkColorButton*, CPUGraph*));
static void    setup_mode_option             (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraphOptions *data);
static void    setup_color_mode_option       (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraphOptions *data);
static void    setup_load_threshold_option   (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base);
static GtkBox* setup_per_core_spacing_option (GtkBox          *vbox,
                                              GtkSizeGroup    *sg,
                                              CPUGraph        *base);

static void    change_in_terminal            (GtkToggleButton *button,
                                              CPUGraphOptions *data);
static void    change_startup_notification   (GtkToggleButton *button,
                                              CPUGraphOptions *data);
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
static void    change_color_5                (GtkColorButton  *button,
                                              CPUGraph        *base);
static void    update_sensitivity            (const CPUGraphOptions *data);
static void    change_mode                   (GtkComboBox     *om,
                                              CPUGraphOptions *data);
static void    change_color_mode             (GtkComboBox     *om,
                                              CPUGraphOptions *data);
static void    response_cb                   (GtkWidget       *dlg,
                                              gint             response,
                                              CPUGraph        *base);
static void    change_frame                  (GtkToggleButton *button,
                                              CPUGraphOptions *data);
static void    change_border                 (GtkToggleButton *button,
                                              CPUGraphOptions *data);
static void    change_bars                   (GtkToggleButton *button,
                                              CPUGraphOptions *data);
static void    change_per_core               (GtkToggleButton *button,
                                              CPUGraphOptions *data);
static void    change_per_core_spacing       (GtkSpinButton   *button,
                                              CPUGraph        *base);
static void    change_size                   (GtkSpinButton   *button,
                                              CPUGraph        *base);
static void    change_smt                    (GtkToggleButton *button,
                                              CPUGraphOptions *data);
static void    change_time_scale             (GtkToggleButton *button,
                                              CPUGraphOptions *data);
static void    change_update                 (GtkComboBox     *om,
                                              CPUGraphOptions *data);
static void    change_core                   (GtkComboBox     *combo,
                                              CPUGraphOptions *data);
static void    change_load_threshold         (GtkSpinButton   *button,
                                              CPUGraph        *base);
static gboolean update_cb                    (CPUGraphOptions *data);

void
create_options (XfcePanelPlugin *plugin, CPUGraph *base)
{
    GtkWidget *dlg, *content;
    GtkBox *vbox, *vbox2, *vbox3;
    GtkWidget *label, *notebook;
    GtkSizeGroup *sg;
    CPUGraphOptions *dlg_data;
    gchar *smt_issues_tooltip;

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
    setup_update_interval_option (vbox, sg, dlg_data);
    setup_tracked_core_option (vbox, sg, dlg_data);
    setup_size_option (vbox, sg, plugin, base);
    setup_load_threshold_option (vbox, sg, base);

    gtk_box_pack_start (vbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, BORDER/2);
    setup_command_option (vbox, sg, dlg_data);
    dlg_data->hbox_in_terminal = create_check_box (vbox, sg, _("Run in terminal"),
                                                   base->command_in_terminal, change_in_terminal, dlg_data,
                                                   NULL);
    dlg_data->hbox_startup_notification = create_check_box (vbox, sg, _("Use startup notification"),
                                                            base->command_startup_notification, change_startup_notification, dlg_data,
                                                            NULL);

    smt_issues_tooltip = _("Color used to highlight potentially suboptimal\nplacement of threads on CPUs with SMT");
    dlg_data->smt_stats_tooltip = g_strdup_printf("%s\n%s",
        _("'Overall' is showing the impact on the overall performance of the machine."),
        _("'Hotspots' is showing the momentary performance impact on just the threads involved in suboptimal SMT scheduling decisions."));

    gtk_box_pack_start (vbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, BORDER/2);
    dlg_data->hbox_highlight_smt = create_check_box (vbox, sg, _("Highlight suboptimal SMT scheduling"),
                                                     base->highlight_smt, change_smt, dlg_data,
                                                     NULL);
    setup_color_option (vbox, sg, dlg_data, SMT_ISSUES_COLOR, _("SMT issues color:"), smt_issues_tooltip, change_color_5);

    gtk_box_pack_start (vbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, BORDER/2);
    create_check_box (vbox, sg, _("Use non-linear time-scale"), base->non_linear, change_time_scale, dlg_data, NULL);
    create_check_box (vbox, sg, _("Per-core history graphs"), base->per_core, change_per_core, dlg_data, &dlg_data->per_core);
    dlg_data->hbox_per_core_spacing  = setup_per_core_spacing_option (vbox, sg, base);

    vbox2 = create_tab ();
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR1, _("Color 1:"), NULL, change_color_1);
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR2, _("Color 2:"), NULL, change_color_2);
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR3, _("Color 3:"), NULL, change_color_3);
    setup_color_option (vbox2, sg, dlg_data, BG_COLOR, _("Background:"), NULL, change_color_0);
    setup_mode_option (vbox2, sg, dlg_data);
    setup_color_mode_option (vbox2, sg, dlg_data);
    gtk_box_pack_start (vbox2, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, BORDER/2);
    create_check_box (vbox2, sg, ngettext ("Show current usage bar", "Show current usage bars", base->nr_cores),
                      base->has_bars, change_bars, dlg_data,
                      &dlg_data->show_bars_checkbox);
    setup_color_option (vbox2, sg, dlg_data, BARS_COLOR, _("Bars color:"), NULL, change_color_4);
    gtk_box_pack_start (vbox2, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, BORDER/2);
    create_check_box (vbox2, sg, _("Show frame"), base->has_frame, change_frame, dlg_data, NULL);
    create_check_box (vbox2, sg, _("Show border"), base->has_border, change_border, dlg_data, NULL);

    vbox3 = create_tab ();
    dlg_data->smt_stats = create_label_line (vbox3, "");

    notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (notebook), BORDER - 2);
    label = gtk_label_new (_("Appearance"));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (vbox2), GTK_WIDGET (label));
    label = gtk_label_new (_("Advanced"));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (vbox), GTK_WIDGET (label));
    label = gtk_label_new (_("Stats"));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (vbox3), GTK_WIDGET (label));

    content = gtk_dialog_get_content_area (GTK_DIALOG (dlg));
    gtk_container_add (GTK_CONTAINER (content), notebook);

    update_cb (dlg_data);
    dlg_data->timeout_id = g_timeout_add (100, (GSourceFunc) update_cb, dlg_data);

    update_sensitivity (dlg_data);
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

static GtkLabel *
create_label_line (GtkBox *tab, const gchar *text)
{
    GtkLabel *label;
    GtkBox *line;

    line = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER));
    gtk_box_pack_start (tab, GTK_WIDGET (line), FALSE, FALSE, 0);

    label = GTK_LABEL (gtk_label_new (text));
    gtk_box_pack_start (line, GTK_WIDGET (label), FALSE, FALSE, 0);
    gtk_label_set_xalign (label, 0.0);
    gtk_label_set_yalign (label, 0.5);

    return label;
}

static GtkBox *
create_option_line (GtkBox *tab, GtkSizeGroup *sg, const gchar *name, const gchar *tooltip)
{
    GtkBox *line;

    line = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER));
    gtk_box_pack_start (tab, GTK_WIDGET (line), FALSE, FALSE, 0);

    if (name)
    {
        GtkBox *line2 = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
        GtkWidget *label = gtk_label_new (name);
        gtk_box_pack_start (line2, label, FALSE, FALSE, 0);
        gtk_label_set_xalign (GTK_LABEL (label), 0.0);
        gtk_label_set_yalign (GTK_LABEL (label), 0.5);
        if (tooltip)
        {
            GtkWidget *icon = gtk_image_new_from_icon_name ("gtk-help", GTK_ICON_SIZE_MENU);
            gtk_widget_set_tooltip_text (icon, tooltip);
            gtk_box_pack_start (line2, icon, FALSE, FALSE, BORDER);
        }
        gtk_size_group_add_widget (sg, GTK_WIDGET (line2));
        gtk_box_pack_start (line, GTK_WIDGET (line2), FALSE, FALSE, 0);
    }

    return line;
}

static GtkBox*
create_check_box (GtkBox *tab, GtkSizeGroup *sg, const gchar *name, gboolean init,
                  void (callback)(GtkToggleButton*, CPUGraphOptions*), CPUGraphOptions *cb_data,
                  GtkToggleButton **out_checkbox)
{
    GtkBox *hbox;
    GtkWidget *checkbox;

    hbox = create_option_line (tab, sg, NULL, NULL);

    checkbox = gtk_check_button_new_with_mnemonic (name);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), init);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (checkbox), FALSE, FALSE, 0);
    g_signal_connect (checkbox, "toggled", G_CALLBACK (callback), cb_data);

    if (out_checkbox)
        *out_checkbox = GTK_TOGGLE_BUTTON (checkbox);

    return hbox;
}

static GtkWidget*
create_drop_down (GtkBox *tab, GtkSizeGroup *sg, const gchar *name,
                  const gchar **items, gsize nb_items, guint init,
                  void (callback)(GtkComboBox*, CPUGraphOptions*), CPUGraphOptions *cb_data)
{
    GtkBox *hbox;
    GtkWidget *combo;
    gsize i;

    hbox = create_option_line (tab, sg, name, NULL);

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
destroy_cb (GtkWidget *dlg, CPUGraphOptions *data)
{
    if (data->timeout_id)
        g_source_remove (data->timeout_id);
    g_free (data->smt_stats_tooltip);
    g_free (data);
}

static void
setup_update_interval_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraphOptions *data)
{
    const gchar *items[] = { _("Fastest (~250ms)"),
                             _("Fast (~500ms)"),
                             _("Normal (~750ms)"),
                             _("Slow (~1s)"),
                             _("Slowest (~3s)")
                           };
    gsize nb_items = sizeof (items) / sizeof (*items);

    create_drop_down (vbox, sg, _("Update Interval:"), items, nb_items,
                      data->base->update_interval, change_update, data);
}

static void
setup_tracked_core_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraphOptions *data)
{
    const gsize nb_items = data->base->nr_cores + 1;
    gchar *items[nb_items];
    guint i;

    items[0] = _("All");
    for (i = 1; i < nb_items; i++)
        items[i] = g_strdup_printf ("%u", i-1);

    create_drop_down (vbox, sg, _("Tracked Core:"), (const gchar **) items, nb_items,
                      data->base->tracked_core, change_core, data);

    for (i = 1; i < nb_items; i++)
        g_free (items[i]);
}

static void
setup_size_option (GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, CPUGraph *base)
{
    GtkBox *hbox;
    GtkWidget *size;

    if (xfce_panel_plugin_get_orientation (plugin) == GTK_ORIENTATION_HORIZONTAL)
        hbox = create_option_line (vbox, sg, _("Width:"), NULL);
    else
        hbox = create_option_line (vbox, sg, _("Height:"), NULL);

    size = gtk_spin_button_new_with_range (MIN_SIZE, MAX_SIZE, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (size), base->size);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (size), FALSE, FALSE, 0);
    g_signal_connect (size, "value-changed", G_CALLBACK (change_size), base);
}

static void
setup_load_threshold_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base)
{
    GtkBox *hbox;
    GtkWidget *threshold;

    hbox = create_option_line (vbox, sg, _("Threshold (%):"), NULL);
    threshold = gtk_spin_button_new_with_range (0, (gint) roundf (100 * MAX_LOAD_THRESHOLD), 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (threshold), (gint) roundf (100 * base->load_threshold));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (threshold), FALSE, FALSE, 0);
    g_signal_connect (threshold, "value-changed", G_CALLBACK (change_load_threshold), base);
}

static GtkBox*
setup_per_core_spacing_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraph *base)
{
    GtkBox *hbox;
    GtkWidget *spacing;

    hbox = create_option_line (vbox, sg, _("Spacing:"), NULL);
    spacing = gtk_spin_button_new_with_range (PER_CORE_SPACING_MIN, PER_CORE_SPACING_MAX, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spacing), base->per_core_spacing);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (spacing), FALSE, FALSE, 0);
    g_signal_connect (spacing, "value-changed", G_CALLBACK (change_per_core_spacing), base);
    return hbox;
}

static void
setup_command_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraphOptions *data)
{
    GtkBox *hbox;
    GtkWidget *associatecommand;

    hbox = create_option_line (vbox, sg, _("Associated command:"), NULL);

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
setup_color_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraphOptions *data,
                    guint number, const gchar *name, const gchar *tooltip,
                    void (callback)(GtkColorButton*, CPUGraph*))
{
    GtkBox *hbox = create_option_line (vbox, sg, name, tooltip);

    data->color_buttons[number] = gtk_color_button_new_with_rgba (&data->base->colors[number]);
    gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (data->color_buttons[number]), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (data->color_buttons[number]), FALSE, FALSE, 0);

    g_signal_connect (data->color_buttons[number], "color-set", G_CALLBACK (callback), data->base);
}

static void
setup_mode_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraphOptions *data)
{
    const gchar *items[] = { _("Disabled"),
                             _("Normal"),
                             _("LED"),
                             _("No history"),
                             _("Grid")
                           };
    gsize nb_items = sizeof (items) / sizeof (*items);
    gint selected;

    switch (data->base->mode)
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

    create_drop_down (vbox, sg, _("Mode:"), items, nb_items, selected, change_mode, data);
}

static void
setup_color_mode_option (GtkBox *vbox, GtkSizeGroup *sg, CPUGraphOptions *data)
{
    const gchar *items[] = { _("Solid"),
                             _("Gradient"),
                             _("Fire"),
                           };
    gsize nb_items = sizeof (items) / sizeof (*items);

    data->color_mode_combobox = create_drop_down (vbox, sg, _("Color mode: "), items, nb_items,
                                                  data->base->color_mode, change_color_mode, data);
}

static void
change_in_terminal (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_in_terminal (data->base, gtk_toggle_button_get_active (button));
    update_sensitivity (data);
}

static void
change_startup_notification (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_startup_notification (data->base, gtk_toggle_button_get_active (button));
    update_sensitivity (data);
}

static void
change_command (GtkEntry *entry, CPUGraphOptions *data)
{
    set_command (data->base, gtk_entry_get_text (entry));
    update_sensitivity (data);
}

static void
change_color (GtkColorButton *button, CPUGraph *base, guint number)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
    set_color (base, number, color);
}

static void
change_color_0 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, BG_COLOR);
}

static void
change_color_1 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, FG_COLOR1);
}

static void
change_color_2 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, FG_COLOR2);
}

static void
change_color_3 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, FG_COLOR3);
}

static void
change_color_4 (GtkColorButton *button, CPUGraph *base)
{
    base->has_barcolor = TRUE;
    change_color (button, base, BARS_COLOR);
}

static void
change_color_5 (GtkColorButton *button, CPUGraph *base)
{
    change_color (button, base, SMT_ISSUES_COLOR);
}

static void
update_sensitivity (const CPUGraphOptions *data)
{
    const CPUGraph *base = data->base;
    const gboolean default_command = (base->command == NULL);
    const gboolean per_core = base->nr_cores > 1 && base->tracked_core == 0 && base->mode != MODE_DISABLED;

    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_highlight_smt),
                              base->has_bars && base->topology && base->topology->smt);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_in_terminal), !default_command);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_startup_notification), !default_command);
    gtk_widget_set_sensitive (GTK_WIDGET (data->per_core), per_core);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_per_core_spacing), per_core && base->per_core);

    gtk_widget_set_sensitive (gtk_widget_get_parent (data->color_buttons[FG_COLOR2]),
                              base->color_mode != 0 || base->mode == MODE_LED || base->mode == MODE_GRID);
    gtk_widget_set_sensitive (gtk_widget_get_parent (data->color_buttons[FG_COLOR3]),
                              base->color_mode != 0 && base->mode == MODE_LED);

    gtk_widget_set_sensitive (gtk_widget_get_parent (data->color_buttons[BARS_COLOR]), base->has_bars);
    gtk_widget_set_sensitive (gtk_widget_get_parent (data->color_buttons[SMT_ISSUES_COLOR]),
                              base->has_bars && base->highlight_smt && base->topology && base->topology->smt);

    gtk_widget_set_sensitive (gtk_widget_get_parent (data->color_mode_combobox), base->mode != MODE_GRID);
    gtk_widget_set_sensitive (GTK_WIDGET (data->show_bars_checkbox), base->mode != MODE_DISABLED);
}

static void
change_mode (GtkComboBox *combo, CPUGraphOptions *data)
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

    set_mode (data->base, mode);
    if (mode == MODE_DISABLED && !data->base->has_bars)
        gtk_toggle_button_set_active (data->show_bars_checkbox, TRUE);
    update_sensitivity (data);
}

static void
change_color_mode (GtkComboBox *combo, CPUGraphOptions *data)
{
    set_color_mode (data->base, gtk_combo_box_get_active (combo));
    update_sensitivity (data);
}

static void
response_cb (GtkWidget *dlg, gint response, CPUGraph *base)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (base->plugin);
    write_settings (base->plugin, base);
}

static void
change_frame (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_frame (data->base, gtk_toggle_button_get_active (button));
}

static void
change_border (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_border (data->base, gtk_toggle_button_get_active (button));
}

static void
change_bars (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_bars (data->base, gtk_toggle_button_get_active (button));
    update_sensitivity (data);
}

static void
change_per_core (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_per_core (data->base, gtk_toggle_button_get_active (button));
    update_sensitivity (data);
}

static void
change_per_core_spacing (GtkSpinButton *button, CPUGraph *base)
{
    set_per_core_spacing (base, gtk_spin_button_get_value_as_int (button));
}

static void
change_size (GtkSpinButton *button, CPUGraph *base)
{
    set_size (base, gtk_spin_button_get_value_as_int (button));
}

static void
change_smt (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_smt (data->base, gtk_toggle_button_get_active (button));
    update_sensitivity (data);
}

static void
change_load_threshold (GtkSpinButton *button, CPUGraph *base)
{
    set_load_threshold (base, gtk_spin_button_get_value (button) / 100);
}

static void change_time_scale (GtkToggleButton *button, CPUGraphOptions *data)
{
    set_nonlinear_time (data->base, gtk_toggle_button_get_active (button));
}

static void change_update (GtkComboBox *combo, CPUGraphOptions *data)
{
    set_update_rate (data->base, gtk_combo_box_get_active (combo));
}

static void change_core (GtkComboBox *combo, CPUGraphOptions *data)
{
    set_tracked_core (data->base, gtk_combo_box_get_active (combo));
    if (data->base->tracked_core != 0)
        set_per_core (data->base, FALSE);
    else
        set_per_core (data->base, gtk_toggle_button_get_active (data->per_core));
    update_sensitivity (data);
}

static gboolean
update_cb (CPUGraphOptions *data)
{
    const CPUGraph *base = data->base;
    gchar *smt_text;
    gboolean show_tooltip = FALSE;

    if (base->topology)
    {
        const gchar *const smt_detected = base->topology->smt ? _("SMT detected: Yes") : _("SMT detected: No");

        if (base->topology->smt || base->stats.num_smt_incidents != 0)
        {
            gdouble actual, optimal;
            gdouble slowdown_overall = 0;
            gdouble slowdown_hotspots = 0;
            gchar lines[4][128];

            actual = base->stats.num_instructions_executed.total.actual;
            optimal = base->stats.num_instructions_executed.total.optimal;
            if (actual != 0)
            {
                slowdown_overall = 100.0 * (optimal - actual) / actual;
                slowdown_overall = round (slowdown_overall * 100) / 100;
            }

            actual = base->stats.num_instructions_executed.during_smt_incidents.actual;
            optimal = base->stats.num_instructions_executed.during_smt_incidents.optimal;
            if (actual != 0)
            {
                slowdown_hotspots = 100.0 * (optimal - actual) / actual;
                slowdown_hotspots = round (slowdown_hotspots * 100) / 100;
            }

            g_snprintf (lines[0], sizeof (lines[0]), _("Number of SMT scheduling incidents: %u"),
                        base->stats.num_smt_incidents);

            if (base->stats.num_smt_incidents == 0)
            {
                smt_text = g_strdup_printf ("%s\n%s", smt_detected, lines[0]);
            }
            else
            {
                g_snprintf (lines[1], sizeof (lines[1]), _("Estimated performance impact:"));
                g_snprintf (lines[2], sizeof (lines[2]), _("Overall: %.3g%%"), slowdown_overall);
                g_snprintf (lines[3], sizeof (lines[3]), _("Hotspots: %.3g%%"), slowdown_hotspots);

                smt_text = g_strdup_printf ("%s\n%s\n%s\n\t%s\n\t%s", smt_detected,
                                            lines[0], lines[1], lines[2], lines[3]);

                show_tooltip = TRUE;
            }
        }
        else
        {
            smt_text = g_strdup (smt_detected);
        }
    }
    else
    {
        smt_text = g_strdup (_("SMT detected: N/A"));
    }

    if (strcmp (gtk_label_get_text (data->smt_stats), smt_text))
    {
        gtk_label_set_text (data->smt_stats, smt_text);
        gtk_widget_set_tooltip_text (GTK_WIDGET (data->smt_stats), show_tooltip ? data->smt_stats_tooltip : "");
    }

    g_free (smt_text);

    return TRUE;
}
