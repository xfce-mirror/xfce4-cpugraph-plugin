/*  properties.cc
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
 *  Copyright (c) 2021-2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpu.h"
#include "properties.h"
#include "settings.h"

#include <libxfce4ui/libxfce4ui.h>
#include <math.h>
#include <vector>
#include "../xfce4cpp/connections.hh"
#include "../xfce4cpp/timer.hh"

using namespace std;

struct CPUGraphOptions
{
    const shared_ptr<CPUGraph> base;

    GtkColorButton  *color_buttons[NUM_COLORS] = {};
    GtkWidget       *mode_combobox = nullptr;
    GtkWidget       *color_mode_combobox = nullptr;
    GtkBox          *hbox_stats_smt = nullptr;
    GtkBox          *hbox_highlight_smt = nullptr;
    GtkBox          *hbox_in_terminal = nullptr;
    GtkBox          *hbox_per_core_spacing = nullptr;
    GtkBox          *hbox_startup_notification = nullptr;
    GtkToggleButton *per_core = nullptr, *show_bars_checkbox = nullptr, *bars_perpendicular_checkbox;
    GtkLabel        *smt_stats = nullptr;
    GtkWidget       *notebook = nullptr;
    xfce4::SourceTag timeout_id;

    CPUGraphOptions(const shared_ptr<CPUGraph> &_base) : base(_base) {}

    ~CPUGraphOptions() {
        g_info ("%s", __PRETTY_FUNCTION__);
        removeTimer();
    }

    void
    removeTimer() {
        xfce4::source_remove (timeout_id);
    }

    static string
    smt_stats_tooltip() {
        return string() +
            _("'Overall' is showing the impact on the overall performance of the machine.") + "\n" +
            _("'Hotspots' is showing the momentary performance impact on just the threads involved in suboptimal SMT scheduling decisions.");
    }
};

static GtkBox*    create_tab ();
static GtkLabel*  create_label_line (GtkBox *tab, const gchar *text);
static GtkBox*    create_option_line (GtkBox *tab, GtkSizeGroup *sg, const gchar *name, const gchar *tooltip);
static GtkBox*    create_check_box (GtkBox *tab, GtkSizeGroup *sg, const gchar *name, bool init,
                                    GtkToggleButton **out_checkbox, const function<void (GtkToggleButton*)> &callback);
static GtkWidget* create_drop_down (GtkBox *tab, GtkSizeGroup *sg, const gchar *name,
                                    const vector<string> &items, size_t init,
                                    const function<void(GtkComboBox*)> &callback,
                                    bool text_only = true);
static void       setup_update_interval_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data);
static void       setup_tracked_core_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data);
static void       setup_size_option (GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, const shared_ptr<CPUGraph> &base);
static void       setup_command_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data);
static void       setup_color_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data,
                                      CPUGraphColorNumber number, const gchar *name, const gchar *tooltip,
                                      const function<void(GtkColorButton*)> &callback);
static void       setup_mode_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data);
static void       setup_color_mode_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data);
static void       setup_load_threshold_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraph> &base);
static GtkBox*    setup_per_core_spacing_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraph> &base);
static void       change_color (GtkColorButton  *button, const shared_ptr<CPUGraph> &base, CPUGraphColorNumber number);
static void       update_sensitivity (const shared_ptr<CPUGraphOptions> &data, bool initial = false);

static void       update_stats_smt_cb (const shared_ptr<CPUGraphOptions> &data);

void
create_options (XfcePanelPlugin *plugin, const shared_ptr<CPUGraph> &base)
{
    xfce_panel_plugin_block_menu (plugin);

    GtkWidget *dlg = xfce_titled_dialog_new_with_mixed_buttons (
        _("CPU Graph Properties"),
        GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "window-close-symbolic",
        _("_Close"),
        GTK_RESPONSE_OK,
        nullptr
    );

    auto dlg_data = make_shared<CPUGraphOptions>(base);

    xfce4::connect_destroy (dlg, [dlg_data](GtkWidget*) {
        dlg_data->removeTimer();
    });
    xfce4::connect_response (GTK_DIALOG (dlg), [base, dlg](GtkDialog*, gint response) {
        gtk_widget_destroy (dlg);
        xfce_panel_plugin_unblock_menu (base->plugin);
        Settings::write (base->plugin, base);
    });

    gtk_window_set_icon_name (GTK_WINDOW (dlg), "org.xfce.panel.cpugraph");

    GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    GtkBox *vbox = create_tab ();
    setup_update_interval_option (vbox, sg, dlg_data);
    setup_tracked_core_option (vbox, sg, dlg_data);
    setup_size_option (vbox, sg, plugin, base);
    setup_load_threshold_option (vbox, sg, base);

    gtk_box_pack_start (vbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), false, false, BORDER/2);
    setup_command_option (vbox, sg, dlg_data);
    dlg_data->hbox_in_terminal = create_check_box (vbox, sg, _("Run in terminal"),
        base->command_in_terminal, nullptr,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_in_terminal (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });
    dlg_data->hbox_startup_notification = create_check_box (vbox, sg, _("Use startup notification"),
        base->command_startup_notification, nullptr,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_startup_notification (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });

    const gchar *smt_issues_tooltip = _("Color used to highlight potentially suboptimal\nplacement of threads on CPUs with SMT");

    auto startSmtStatsTimer = [dlg_data] {
        gtk_widget_set_visible (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dlg_data->notebook), 2), true);
        update_stats_smt_cb (dlg_data);
        dlg_data->timeout_id = xfce4::timeout_add (250, [dlg_data] {
            update_stats_smt_cb (dlg_data);
            return xfce4::TimeoutResponse::Again();
        });
    };
    auto stopSmtStatsTimer = [dlg_data] {
        gtk_widget_set_visible (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dlg_data->notebook), 2), false);
        dlg_data->removeTimer ();
    };

    gtk_box_pack_start (vbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), false, false, BORDER/2);
    dlg_data->hbox_stats_smt = create_check_box (vbox, sg, _("Display SMT statistics"),
        base->stats_smt, nullptr,
        [dlg_data, startSmtStatsTimer, stopSmtStatsTimer](GtkToggleButton *button) {
            dlg_data->base->set_stats_smt (gtk_toggle_button_get_active (button));
            if (dlg_data->base->stats_smt)
                startSmtStatsTimer ();
            else
                stopSmtStatsTimer ();
            dlg_data->base->maybe_clear_smt_stats ();
        });
    dlg_data->hbox_highlight_smt = create_check_box (vbox, sg, _("Highlight suboptimal SMT scheduling"),
        base->highlight_smt, nullptr,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_smt (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
            dlg_data->base->maybe_clear_smt_stats ();
        });
    setup_color_option (vbox, sg, dlg_data, SMT_ISSUES_COLOR, _("SMT issues color:"), smt_issues_tooltip, [base](GtkColorButton *button) {
        change_color (button, base, SMT_ISSUES_COLOR);
    });

    gtk_box_pack_start (vbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), false, false, BORDER/2);
    create_check_box (vbox, sg, _("Use non-linear time-scale"), base->non_linear, nullptr,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_nonlinear_time (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });
    create_check_box (vbox, sg, _("Per-core history graphs"), base->per_core, &dlg_data->per_core,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_per_core (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });
    dlg_data->hbox_per_core_spacing  = setup_per_core_spacing_option (vbox, sg, base);

    GtkBox *vbox2 = create_tab ();
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR1, _("Color 1:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, FG_COLOR1);
    });
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR2, _("Color 2:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, FG_COLOR2);
    });
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR3, _("Color 3:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, FG_COLOR3);
    });
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR_SYSTEM, _("System:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, FG_COLOR_SYSTEM);
    });
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR_USER, _("User:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, FG_COLOR_USER);
    });
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR_NICE, _("Nice:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, FG_COLOR_NICE);
    });
    setup_color_option (vbox2, sg, dlg_data, FG_COLOR_IOWAIT, _("IO wait:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, FG_COLOR_IOWAIT);
    });
    setup_color_option (vbox2, sg, dlg_data, BG_COLOR, _("Background:"), nullptr, [base](GtkColorButton *button) {
        change_color (button, base, BG_COLOR);
    });
    setup_mode_option (vbox2, sg, dlg_data);
    setup_color_mode_option (vbox2, sg, dlg_data);
    gtk_box_pack_start (vbox2, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), false, false, BORDER/2);
    create_check_box (vbox2, sg, ngettext ("Show current usage bar", "Show current usage bars", base->nr_cores),
        base->has_bars, &dlg_data->show_bars_checkbox,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_bars (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });
    setup_color_option (vbox2, sg, dlg_data, BARS_COLOR, _("Bars color:"), nullptr, [base](GtkColorButton *button) {
        base->has_barcolor = true;
        change_color (button, base, BARS_COLOR);
    });
    create_check_box (vbox2, sg, _("Perpendicular"),
        base->bars_perpendicular, &dlg_data->bars_perpendicular_checkbox,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_bars_perpendicular (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });
    gtk_box_pack_start (vbox2, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), false, false, BORDER/2);
    create_check_box (vbox2, sg, _("Show frame"), base->has_frame, nullptr,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_frame (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });
    create_check_box (vbox2, sg, _("Show border"), base->has_border, nullptr,
        [dlg_data](GtkToggleButton *button) {
            dlg_data->base->set_border (gtk_toggle_button_get_active (button));
            update_sensitivity (dlg_data);
        });

    GtkBox *vbox3 = create_tab ();
    dlg_data->smt_stats = create_label_line (vbox3, "");

    dlg_data->notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (dlg_data->notebook), BORDER - 2);
    gtk_notebook_append_page (GTK_NOTEBOOK (dlg_data->notebook), GTK_WIDGET (vbox2), gtk_label_new (_("Appearance")));
    gtk_notebook_append_page (GTK_NOTEBOOK (dlg_data->notebook), GTK_WIDGET (vbox), gtk_label_new (_("Advanced")));
    gtk_notebook_append_page (GTK_NOTEBOOK (dlg_data->notebook), GTK_WIDGET (vbox3), gtk_label_new (_("Stats")));

    GtkWidget *content = gtk_dialog_get_content_area (GTK_DIALOG (dlg));
    gtk_container_add (GTK_CONTAINER (content), dlg_data->notebook);

    gtk_widget_show_all (dlg_data->notebook);

    if (base->stats_smt)
        startSmtStatsTimer ();
    else
        stopSmtStatsTimer ();

    update_sensitivity (dlg_data, true);
    gtk_widget_show (dlg);
}

static GtkBox *
create_tab ()
{
    GtkBox *tab = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER));
    gtk_container_set_border_width (GTK_CONTAINER (tab), BORDER);
    return tab;
}

static GtkLabel *
create_label_line (GtkBox *tab, const gchar *text)
{
    GtkBox *line = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER));
    gtk_box_pack_start (tab, GTK_WIDGET (line), false, false, 0);

    GtkLabel *label = GTK_LABEL (gtk_label_new (text));
    gtk_box_pack_start (line, GTK_WIDGET (label), false, false, 0);
    gtk_label_set_xalign (label, 0.0);
    gtk_label_set_yalign (label, 0.5);

    return label;
}

static GtkBox *
create_option_line (GtkBox *tab, GtkSizeGroup *sg, const gchar *name, const gchar *tooltip)
{
    GtkBox *line = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER));
    gtk_box_pack_start (tab, GTK_WIDGET (line), false, false, 0);

    if (name)
    {
        GtkBox *line2 = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
        GtkWidget *label = gtk_label_new (name);
        gtk_box_pack_start (line2, label, false, false, 0);
        gtk_label_set_xalign (GTK_LABEL (label), 0.0);
        gtk_label_set_yalign (GTK_LABEL (label), 0.5);
        if (tooltip)
        {
            GtkWidget *icon = gtk_image_new_from_icon_name ("gtk-help", GTK_ICON_SIZE_MENU);
            gtk_widget_set_tooltip_text (icon, tooltip);
            gtk_box_pack_start (line2, icon, false, false, BORDER);
        }
        gtk_size_group_add_widget (sg, GTK_WIDGET (line2));
        gtk_box_pack_start (line, GTK_WIDGET (line2), false, false, 0);
    }

    return line;
}

static GtkBox*
create_check_box (GtkBox *tab, GtkSizeGroup *sg, const gchar *name, bool init,
                  GtkToggleButton **out_checkbox,
                  const function<void (GtkToggleButton*)> &callback)
{
    GtkBox *hbox = create_option_line (tab, sg, nullptr, nullptr);

    GtkToggleButton *checkbox = GTK_TOGGLE_BUTTON (gtk_check_button_new_with_mnemonic (name));
    gtk_toggle_button_set_active (checkbox, init);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (checkbox), false, false, 0);
    xfce4::connect (GTK_TOGGLE_BUTTON (checkbox), "toggled", callback);

    if (out_checkbox)
        *out_checkbox = checkbox;

    return hbox;
}

static GtkWidget*
create_drop_down (GtkBox *tab, GtkSizeGroup *sg, const gchar *name,
                  const vector<string> &items, size_t init,
                  const function<void(GtkComboBox*)> &callback,
                  bool text_only)
{
    GtkBox *hbox = create_option_line (tab, sg, name, nullptr);

    GtkWidget *combo;
    if (text_only)
    {
        combo = gtk_combo_box_text_new ();
        for (const string &item : items)
            gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), nullptr, item.c_str());
    }
    else
    {
        GtkListStore *list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

        GtkTreeIter iter;
        for (const string &item : items)
        {
            gtk_list_store_append (list_store, &iter);
            gtk_list_store_set (list_store, &iter,
                                0, item.c_str(),
                                1,  true,
                                -1);
        }

        combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list_store));

        g_object_unref (list_store);

        GtkCellRenderer *cell = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, true);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
                                        "text", 0,
                                        "sensitive", 1,
                                        nullptr);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), init);
    gtk_box_pack_start (GTK_BOX (hbox), combo, false, false, 0);

    xfce4::connect (GTK_COMBO_BOX (combo), "changed", callback);

    return combo;
}

static void
setup_update_interval_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data)
{
    const vector<string> items = {
        _("Fastest (~250ms)"),
        _("Fast (~500ms)"),
        _("Normal (~750ms)"),
        _("Slow (~1s)"),
        _("Slowest (~3s)")
    };

    create_drop_down (vbox, sg, _("Update Interval:"), items, data->base->update_interval,
        [data](GtkComboBox *combo) {
            data->base->set_update_rate ((CPUGraphUpdateRate) gtk_combo_box_get_active (combo));
        });
}

static void
setup_tracked_core_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data)
{
    const gsize nb_items = data->base->nr_cores + 1;
    vector<string> items(nb_items);

    items[0] = _("All");
    for (gsize i = 1; i < nb_items; i++)
        items[i] = xfce4::sprintf ("%zu", i-1);

    create_drop_down (vbox, sg, _("Tracked Core:"), items, data->base->tracked_core,
        [data](GtkComboBox *combo) {
            data->base->set_tracked_core (gtk_combo_box_get_active (combo));
            if (data->base->tracked_core != 0)
                data->base->set_per_core (false);
            else
                data->base->set_per_core (gtk_toggle_button_get_active (data->per_core));
            update_sensitivity (data);
        });
}

static void
setup_size_option (GtkBox *vbox, GtkSizeGroup *sg, XfcePanelPlugin *plugin, const shared_ptr<CPUGraph> &base)
{
    GtkBox *hbox;
    if (xfce_panel_plugin_get_orientation (plugin) == GTK_ORIENTATION_HORIZONTAL)
        hbox = create_option_line (vbox, sg, _("Width:"), nullptr);
    else
        hbox = create_option_line (vbox, sg, _("Height:"), nullptr);

    GtkWidget *size = gtk_spin_button_new_with_range (MIN_SIZE, MAX_SIZE, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (size), base->size);
    gtk_box_pack_start (GTK_BOX (hbox), size, false, false, 0);
    xfce4::connect (GTK_SPIN_BUTTON (size), "value-changed", [base](GtkSpinButton *button) {
        base->set_size (gtk_spin_button_get_value_as_int (button));
    });
}

static void
setup_load_threshold_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraph> &base)
{
    GtkBox *hbox = create_option_line (vbox, sg, _("Threshold (%):"), nullptr);
    GtkWidget *threshold = gtk_spin_button_new_with_range (0, (gint) roundf (100 * MAX_LOAD_THRESHOLD), 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (threshold), (gint) roundf (100 * base->load_threshold));
    gtk_box_pack_start (GTK_BOX (hbox), threshold, false, false, 0);
    xfce4::connect (GTK_SPIN_BUTTON (threshold), "value-changed", [base](GtkSpinButton *button) {
        base->set_load_threshold (gtk_spin_button_get_value (button) / 100);
    });
}

static GtkBox*
setup_per_core_spacing_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraph> &base)
{
    GtkBox *hbox = create_option_line (vbox, sg, _("Spacing:"), nullptr);
    GtkWidget *spacing = gtk_spin_button_new_with_range (PER_CORE_SPACING_MIN, PER_CORE_SPACING_MAX, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spacing), base->per_core_spacing);
    gtk_widget_set_tooltip_text (GTK_WIDGET (hbox), _("Spacing between per-core history graphs"));
    gtk_box_pack_start (GTK_BOX (hbox), spacing, false, false, 0);
    xfce4::connect (GTK_SPIN_BUTTON (spacing), "value-changed", [base](GtkSpinButton *button) {
        base->set_per_core_spacing (gtk_spin_button_get_value_as_int (button));
    });
    return hbox;
}

static void
setup_command_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data)
{
    GtkBox *hbox = create_option_line (vbox, sg, _("Associated command:"), nullptr);

    GtkWidget *associatecommand = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (associatecommand), data->base->command.c_str());
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (associatecommand),
                                       GTK_ENTRY_ICON_SECONDARY,
                                       "help-contents");
    auto tooltip = string() +
        _("The command to run when the plugin is left-clicked.") + "\n" +
        _("If not specified, it defaults to xfce4-taskmanager, htop or top.");
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (associatecommand),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     tooltip.c_str());
    gtk_box_pack_start (GTK_BOX (hbox), associatecommand, false, false, 0);
    xfce4::connect (GTK_ENTRY (associatecommand), "changed", [data](GtkEntry *entry) {
        data->base->set_command (gtk_entry_get_text (entry));
        update_sensitivity (data);
    });
}

static void
setup_color_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data,
                    CPUGraphColorNumber number, const gchar *name, const gchar *tooltip,
                    const function<void(GtkColorButton*)> &callback)
{
    GtkBox *hbox = create_option_line (vbox, sg, name, tooltip);

    data->color_buttons[number] = xfce4::gtk_color_button_new (data->base->colors[number], true);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (data->color_buttons[number]), false, false, 0);

    xfce4::connect (data->color_buttons[number], "color-set", callback);
}

static void
setup_mode_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data)
{
    const vector<string> items = {
        _("Disabled"),
        _("Normal"),
        _("LED"),
        _("No history"),
        _("Grid")
    };

    gint selected = 0;
    switch (data->base->mode)
    {
        case MODE_DISABLED:   selected = 0; break;
        case MODE_NORMAL:     selected = 1; break;
        case MODE_LED:        selected = 2; break;
        case MODE_NO_HISTORY: selected = 3; break;
        case MODE_GRID:       selected = 4; break;
    }

    data->mode_combobox = create_drop_down (vbox, sg, _("Mode:"), items, selected,
        [data](GtkComboBox *combo) {
            gint active = gtk_combo_box_get_active (combo);
            CPUGraphMode mode;

            switch (active)
            {
                case MODE_DISABLED:
                case MODE_NORMAL:
                case MODE_LED:
                case MODE_NO_HISTORY:
                case MODE_GRID:
                    mode = (CPUGraphMode) active;
                    break;
                default:
                    mode = MODE_NORMAL;
            }

            data->base->set_mode (mode);
            if (mode == MODE_DISABLED && !data->base->has_bars)
                gtk_toggle_button_set_active (data->show_bars_checkbox, true);

            update_sensitivity (data);
        }, false);
}

static void
setup_color_mode_option (GtkBox *vbox, GtkSizeGroup *sg, const shared_ptr<CPUGraphOptions> &data)
{
    const vector<string> items = {
        _("Solid"),
        _("Gradient"),
        _("Fire"),
#if defined (__linux__) || defined (__FreeBSD_kernel__)
        _("Detailed"),
#endif
    };

    data->color_mode_combobox = create_drop_down (
        vbox, sg, _("Color mode: "), items, data->base->color_mode,
        [data](GtkComboBox *combo) {
            data->base->set_color_mode (gtk_combo_box_get_active (combo));
            update_sensitivity (data);
        }, false);
}

static void
change_color (GtkColorButton *button, const shared_ptr<CPUGraph> &base, CPUGraphColorNumber number)
{
    base->set_color (number, xfce4::gtk_get_rgba (button));
}

static void
update_sensitivity (const shared_ptr<CPUGraphOptions> &data, bool initial)
{
    const shared_ptr<CPUGraph> base = data->base;
    const bool default_command = base->command.empty();
    const bool per_core = base->nr_cores > 1 && base->tracked_core == 0 && base->mode != MODE_DISABLED;

    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_highlight_smt),
                              base->has_bars && base->topology && base->topology->smt);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_in_terminal), !default_command);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_startup_notification), !default_command);
    if (initial)
    {
        gtk_widget_set_visible (GTK_WIDGET (data->hbox_in_terminal), !default_command);
        gtk_widget_set_visible (GTK_WIDGET (data->hbox_startup_notification), !default_command);
    }
    else if (!default_command)
    {
        gtk_widget_set_visible (GTK_WIDGET (data->hbox_in_terminal), true);
        gtk_widget_set_visible (GTK_WIDGET (data->hbox_startup_notification), true);
    }
    gtk_widget_set_sensitive (GTK_WIDGET (data->per_core), per_core);
    gtk_widget_set_sensitive (GTK_WIDGET (data->hbox_per_core_spacing), per_core && base->per_core);

    auto get_color_button_parent = [&](CPUGraphColorNumber color) {
        return gtk_widget_get_parent (GTK_WIDGET (data->color_buttons[color]));
    };
    auto set_colors_visibility = [&](bool detailed_mode) {
        gtk_widget_set_visible (get_color_button_parent (FG_COLOR1), !detailed_mode);
        gtk_widget_set_visible (get_color_button_parent (FG_COLOR2), !detailed_mode);
        gtk_widget_set_visible (get_color_button_parent (FG_COLOR3), !detailed_mode);
        gtk_widget_set_visible (get_color_button_parent (FG_COLOR_SYSTEM), detailed_mode);
        gtk_widget_set_visible (get_color_button_parent (FG_COLOR_USER), detailed_mode);
        gtk_widget_set_visible (get_color_button_parent (FG_COLOR_NICE), detailed_mode);
        gtk_widget_set_visible (get_color_button_parent (FG_COLOR_IOWAIT), detailed_mode);
    };
    if (base->color_mode == COLOR_MODE_DETAILED)
    {
        set_colors_visibility (true);
        gtk_widget_set_sensitive (get_color_button_parent(FG_COLOR_SYSTEM),
                                  base->mode != MODE_DISABLED);
        gtk_widget_set_sensitive (get_color_button_parent(FG_COLOR_USER),
                                  base->mode != MODE_DISABLED);
        gtk_widget_set_sensitive (get_color_button_parent(FG_COLOR_NICE),
                                  base->mode != MODE_DISABLED);
        gtk_widget_set_sensitive (get_color_button_parent(FG_COLOR_IOWAIT),
                                  base->mode != MODE_DISABLED);
    }
    else
    {
        set_colors_visibility (false);
        gtk_widget_set_sensitive (get_color_button_parent(FG_COLOR2),
                                  base->mode != MODE_DISABLED && (base->color_mode != COLOR_MODE_SOLID || base->mode == MODE_LED || base->mode == MODE_GRID));
        gtk_widget_set_sensitive (get_color_button_parent(FG_COLOR3),
                                  base->mode != MODE_DISABLED && (base->color_mode != COLOR_MODE_SOLID && base->mode == MODE_LED));
    }
    gtk_widget_set_sensitive (get_color_button_parent(FG_COLOR1),
                              base->mode != MODE_DISABLED);
    gtk_widget_set_sensitive (get_color_button_parent(BARS_COLOR), base->has_bars);
    gtk_widget_set_sensitive (GTK_WIDGET (data->bars_perpendicular_checkbox), base->has_bars);
    gtk_widget_set_sensitive (get_color_button_parent(SMT_ISSUES_COLOR),
                              base->has_bars && base->highlight_smt && base->topology && base->topology->smt);

    gtk_widget_set_sensitive (gtk_widget_get_parent (data->color_mode_combobox),
                              base->mode != MODE_DISABLED && base->mode != MODE_GRID);
    gtk_widget_set_sensitive (GTK_WIDGET (data->show_bars_checkbox), base->mode != MODE_DISABLED);

    /* Set combobox items (in)sensitive depending on chosen modes. Detailed color mode is supported for:
     * - MODE_NORMAL
     * - MODE_NO_HISTORY
     */
    if (GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (data->mode_combobox)))
    {
        const bool is_detailed = (data->base->color_mode == COLOR_MODE_DETAILED);
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child (model, &iter, nullptr, MODE_LED))
            gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, !is_detailed, -1);
        if (gtk_tree_model_iter_nth_child (model, &iter, nullptr, MODE_GRID))
            gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, !is_detailed, -1);
    }
    if (GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (data->color_mode_combobox)))
    {
        const bool is_led = (data->base->mode == MODE_LED);
        GtkTreeIter iter;
        if (gtk_tree_model_iter_nth_child (model, &iter, nullptr, COLOR_MODE_DETAILED))
            gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, !is_led, -1);
    }
}

static void
update_stats_smt_cb (const shared_ptr<CPUGraphOptions> &data)
{
    const shared_ptr<CPUGraph> base = data->base;
    string smt_text;
    bool show_tooltip = false;

    if (base->topology)
    {
        const gchar *const smt_detected = base->topology->smt ? _("SMT detected: Yes") : _("SMT detected: No");

        if (base->topology->smt || base->stats.num_smt_incidents != 0)
        {
            gdouble slowdown_overall = 0;
            gdouble slowdown_hotspots = 0;

            gdouble actual = base->stats.num_instructions_executed.total.actual;
            gdouble optimal = base->stats.num_instructions_executed.total.optimal;
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

            smt_text = string() +
                smt_detected + "\n" +
                xfce4::sprintf (_("Number of SMT scheduling incidents: %u"), base->stats.num_smt_incidents) + "\n";

            if (base->stats.num_smt_incidents != 0)
            {
                smt_text += string() +
                    _("Estimated performance impact:") + "\n" +
                    "\t" + xfce4::sprintf (_("Overall: %.3g%%"), slowdown_overall) + "\n" +
                    "\t" + xfce4::sprintf (_("Hotspots: %.3g%%"), slowdown_hotspots) + "\n";

                show_tooltip = true;
            }
        }
        else
        {
            smt_text = smt_detected;
        }
    }
    else
    {
        smt_text = _("SMT detected: N/A");
    }

    if (gtk_label_get_text (data->smt_stats) != smt_text)
    {
        gtk_label_set_text (data->smt_stats, smt_text.c_str());
        gtk_widget_set_tooltip_text (GTK_WIDGET (data->smt_stats), show_tooltip ? data->smt_stats_tooltip().c_str() : "");
    }
}
