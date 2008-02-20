#include "cpu.h"
#include "mode.h"

guint16
_lerp (double t, guint16 a, guint16 b)
{
    return (guint16) (a + t * (b - a));
}


static void
cpugraph_construct (XfcePanelPlugin *plugin)
{
    CPUGraph *base;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    base = CreateControl (plugin);

    ReadSettings (plugin, base);

    g_signal_connect (plugin, "free-data", G_CALLBACK (Kill), base);

    g_signal_connect (plugin, "save", G_CALLBACK (WriteSettings), base);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", G_CALLBACK (CreateOptions),
                      base);

    g_signal_connect (plugin, "size-changed", G_CALLBACK (SetSize), base);

    g_signal_connect (plugin, "orientation-changed",
                      G_CALLBACK (SetOrientation), base);
}

/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (cpugraph_construct);


void
Kill (XfcePanelPlugin * plugin, CPUGraph * base)
{
  gint i;
  cpuData_free();
  base->m_CpuData = NULL;

  for(i=0; i<base->nrCores-1; i++)
    gtk_widget_destroy(base->m_pBar[i]);

  gtk_widget_destroy(base->m_Box);

  if (base->m_TimeoutID)
    g_source_remove (base->m_TimeoutID);

  if (base->m_History)
    g_free (base->m_History);

  g_object_unref (base->m_Tooltip);

  g_free (base);
}

void
ReadSettings (XfcePanelPlugin * plugin, CPUGraph * base)
{
    const char *value;
    char *file;
    XfceRc *rc;
    int update;

    base->m_Width = 70;

    base->m_ForeGround1.red = 0;
    base->m_ForeGround1.green = 65535;
    base->m_ForeGround1.blue = 0;

    base->m_ForeGround2.red = 65535;
    base->m_ForeGround2.green = 0;
    base->m_ForeGround2.blue = 0;

    base->m_ForeGround3.red = 0;
    base->m_ForeGround3.green = 0;
    base->m_ForeGround3.blue = 65535;

    base->m_BackGround.red = 65535;
    base->m_BackGround.green = 65535;
    base->m_BackGround.blue = 65535;

    base->m_TimeScale = 0;
    base->m_Frame = 0;
    base->m_AssociateCommand = "xterm top";
    base->m_ColorMode = 0;
    base->m_Mode = 0;

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc)
        {
            base->m_UpdateInterval =
                xfce_rc_read_int_entry (rc, "UpdateInterval",
                                        base->m_UpdateInterval);

            base->m_TimeScale =
                xfce_rc_read_int_entry (rc, "TimeScale",
                                        base->m_TimeScale);

            base->m_Width =
                xfce_rc_read_int_entry (rc, "Width", base->m_Width);

            base->m_Mode = xfce_rc_read_int_entry (rc, "Mode", base->m_Mode);

            base->m_Frame =
                xfce_rc_read_int_entry (rc, "Frame", base->m_Frame);

            if (value = xfce_rc_read_entry (rc, "AssociateCommand", base->m_AssociateCommand)) {
              base->m_AssociateCommand = g_strdup(value);
            }

            base->m_ColorMode =
                xfce_rc_read_int_entry (rc, "ColorMode", base->m_ColorMode);

            if ((value = xfce_rc_read_entry (rc, "Foreground1", NULL)))
            {
                gdk_color_parse (value, &base->m_ForeGround1);
            }
            if ((value = xfce_rc_read_entry (rc, "Foreground2", NULL)))
            {
                gdk_color_parse (value, &base->m_ForeGround2);
            }
            if ((value = xfce_rc_read_entry (rc, "Background", NULL)))
            {
                gdk_color_parse (value, &base->m_BackGround);
            }
            if ((value = xfce_rc_read_entry (rc, "Foreground3", NULL)))
            {
                gdk_color_parse (value, &base->m_ForeGround3);
            }

            xfce_rc_close (rc);
        }
    }
    SetHistorySize (base, base->m_Width);
    if (base->m_TimeoutID)
        g_source_remove (base->m_TimeoutID);
    switch (base->m_UpdateInterval)
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
    base->m_TimeoutID = g_timeout_add (update, (GtkFunction) UpdateCPU, base);
    gtk_frame_set_shadow_type (GTK_FRAME (base->m_FrameWidget),
            base->m_Frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

void
WriteSettings (XfcePanelPlugin *plugin, CPUGraph *base)
{
    char value[10];
    XfceRc *rc;
    char *file;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;

    xfce_rc_write_int_entry (rc, "UpdateInterval", base->m_UpdateInterval);

    xfce_rc_write_int_entry (rc, "TimeScale", base->m_TimeScale);

    xfce_rc_write_int_entry (rc, "Width", base->m_Width);

    xfce_rc_write_int_entry (rc, "Mode", base->m_Mode);

    xfce_rc_write_int_entry (rc, "Frame", base->m_Frame);

    xfce_rc_write_entry (rc, "AssociateCommand", base->m_AssociateCommand ? base->m_AssociateCommand : "");

    xfce_rc_write_int_entry (rc, "ColorMode", base->m_ColorMode);

    g_snprintf (value, 8, "#%02X%02X%02X", base->m_ForeGround1.red >> 8,
                                           base->m_ForeGround1.green >> 8,
                                           base->m_ForeGround1.blue >> 8);
    xfce_rc_write_entry (rc, "Foreground1", value);

    g_snprintf (value, 8, "#%02X%02X%02X", base->m_ForeGround2.red >> 8,
                                           base->m_ForeGround2.green >> 8,
                                           base->m_ForeGround2.blue >> 8);
    xfce_rc_write_entry (rc, "Foreground2", value);

    g_snprintf (value, 8, "#%02X%02X%02X", base->m_BackGround.red >> 8,
                                           base->m_BackGround.green >> 8,
                                           base->m_BackGround.blue >> 8);
    xfce_rc_write_entry (rc, "Background", value);

    g_snprintf (value, 8, "#%02X%02X%02X", base->m_ForeGround3.red >> 8,
                                           base->m_ForeGround3.green >> 8,
                                           base->m_ForeGround3.blue >> 8);
    xfce_rc_write_entry (rc, "Foreground3", value);

    xfce_rc_close (rc);
}

CPUGraph *
CreateControl (XfcePanelPlugin * plugin)
{
    gint i;
    GtkWidget *frame, *ebox;
    GtkOrientation orientation;
    GtkProgressBarOrientation barOrientation;
    CPUGraph *base = g_new0 (CPUGraph, 1);

    base->plugin = plugin;

    ebox = gtk_event_box_new ();
    gtk_widget_show (ebox);
    gtk_container_add (GTK_CONTAINER (plugin), ebox);

    orientation = xfce_panel_plugin_get_orientation(plugin);
    if(orientation == GTK_ORIENTATION_HORIZONTAL)
      barOrientation = GTK_PROGRESS_BOTTOM_TO_TOP;
    else
      barOrientation = GTK_PROGRESS_LEFT_TO_RIGHT;

    base->m_Box = xfce_hvbox_new(orientation, FALSE, 0);
    gtk_widget_show(base->m_Box);
    gtk_container_add(GTK_CONTAINER(ebox), base->m_Box);

    gtk_container_set_border_width(GTK_CONTAINER(frame), BORDER / 2);


    /* Multicore stuff */
    //fprintf(stderr,"Multicore stuff\n");
    if((base->nrCores = cpuData_init() - 1) < 0)
      fprintf(stderr,"Cannot init cpu data !\n");

    base->m_pBar = (GtkWidget **) malloc(sizeof(GtkWidget *) * base->nrCores);

    for(i=0; i<base->nrCores; i++) {
      base->m_pBar[i] = GTK_WIDGET(gtk_progress_bar_new());
      gtk_progress_bar_set_orientation(
                                       GTK_PROGRESS_BAR(base->m_pBar[i]),
                                       barOrientation);

      gtk_box_pack_start(
                         GTK_BOX(base->m_Box),
                         base->m_pBar[i],
                         FALSE,
                         FALSE,
                         0);

      gtk_widget_show(base->m_pBar[i]);
    }

    base->m_FrameWidget = frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

    gtk_box_pack_start(
                       GTK_BOX(base->m_Box),
                       frame,
                       TRUE,
                       TRUE,
                       2);

    gtk_widget_show (frame);

    xfce_panel_plugin_add_action_widget (plugin, ebox);
    g_signal_connect (ebox, "button-press-event", G_CALLBACK (LaunchCommand), base);

    base->m_DrawArea = gtk_drawing_area_new ();
    gtk_widget_set_app_paintable (base->m_DrawArea, TRUE);
    //gtk_container_add (GTK_CONTAINER (ebox), frame);
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (base->m_DrawArea));
    gtk_widget_show (base->m_DrawArea);


    xfce_panel_plugin_add_action_widget (plugin, base->m_DrawArea);

    base->m_Tooltip = gtk_tooltips_new ();
    g_object_ref (base->m_Tooltip);
    gtk_object_sink (GTK_OBJECT (base->m_Tooltip));

    g_signal_connect_after (base->m_DrawArea, "expose-event",
                      G_CALLBACK (DrawAreaExposeEvent), base);

    return base;
}

void
SetOrientation (XfcePanelPlugin * plugin, GtkOrientation orientation, CPUGraph *base)
{
    GtkProgressBarOrientation barOrientation;
    gpointer p_pBar[base->nrCores];
    gpointer p_FrameWidget;
    gint i;

    /* <-- Multicore stuff */

    orientation = xfce_panel_plugin_get_orientation(plugin);
    if(orientation == GTK_ORIENTATION_HORIZONTAL)
      barOrientation = GTK_PROGRESS_BOTTOM_TO_TOP;
    else
      barOrientation = GTK_PROGRESS_LEFT_TO_RIGHT;

    /* Unpack progress bars */
    for(i=0; i<base->nrCores; i++){
      /* reference progress bars to keep them alive */
      p_pBar[i] = g_object_ref(base->m_pBar[i]);
      gtk_container_remove(
                           GTK_CONTAINER(base->m_Box),
                           GTK_WIDGET(base->m_pBar[i]));
    }
    p_FrameWidget = g_object_ref(base->m_FrameWidget);
    gtk_container_remove(
                         GTK_CONTAINER(base->m_Box),
                         GTK_WIDGET(base->m_FrameWidget));


    xfce_hvbox_set_orientation(XFCE_HVBOX(base->m_Box), orientation);

    /* Pack progress bars again into hvbox */
    for(i=0; i<base->nrCores; i++){
      gtk_progress_bar_set_orientation(
                                       GTK_PROGRESS_BAR(base->m_pBar[i]),
                                       barOrientation);	
      gtk_box_pack_start(
                         GTK_BOX(base->m_Box),
                         base->m_pBar[i],
                         FALSE,
                         FALSE,
                         1);
      /* We dont need anymore this reference */
      g_object_unref(p_pBar[i]);
    }
    gtk_box_pack_start(
                       GTK_BOX(base->m_Box),
                       base->m_FrameWidget,
                       TRUE,
                       TRUE,
                       2);
    g_object_unref(p_FrameWidget);
    UserSetSize (base);
    gtk_widget_queue_draw (base->m_DrawArea);
}

void
UpdateTooltip (CPUGraph * base)
{
    char tooltip[32];
    fprintf(stderr, "update tooltip\n");
    int pos = snprintf (tooltip, 32, "Usage: %d%%", base->m_CpuData[0].load*100/CPU_SCALE);
    if( base->m_CpuData[0].scalCurFreq )
      snprintf (tooltip+pos, 32-pos, " (%d MHz)", base->m_CpuData[0].scalCurFreq/1000);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (base->m_Tooltip),
                          base->m_FrameWidget->parent, tooltip, NULL);
}

gboolean
SetSize (XfcePanelPlugin *plugin, int size, CPUGraph *base)
{
    gint i;
    gtk_container_set_border_width (GTK_CONTAINER (base->m_FrameWidget),
                                    size > 26 ? 2 : 0);

    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     base->m_Width, size);
        for(i=0; i<base->nrCores; i++)
          gtk_widget_set_size_request(
                                      GTK_WIDGET(base->m_pBar[i]),
                                      BORDER,
                                      size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     size, base->m_Width);
        for(i=0; i<base->nrCores; i++)
          gtk_widget_set_size_request(
                                      GTK_WIDGET(base->m_pBar[i]),
                                      size,
                                      BORDER);
    }

    return TRUE;
}

void
UserSetSize (CPUGraph * base)
{
    SetSize (base->plugin, xfce_panel_plugin_get_size (base->plugin), base);
}

static void
DialogResponse (GtkWidget *dlg, int response, CPUGraph *base)
{
    ApplyChanges (base);
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (base->plugin);
    WriteSettings (base->plugin, base);
}

void
CreateOptions (XfcePanelPlugin *plugin, CPUGraph *base)
{
    GtkWidget *dlg, *header;
    GtkBox *vbox, *vbox2, *hbox;
    GtkWidget *label;
    GtkSizeGroup *sg;
    SOptions *op = &base->m_Options;

    xfce_panel_plugin_block_menu (plugin);

    dlg = gtk_dialog_new_with_buttons (_("Configure CPU Graph"),
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    base->m_OptionsDialog = dlg;

    g_signal_connect (dlg, "response", G_CALLBACK (DialogResponse), base);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    header = xfce_create_header (NULL, _("CPU Graph"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER - 2);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0);

    vbox = GTK_BOX (gtk_vbox_new(FALSE, BORDER));
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER );
    gtk_widget_show(GTK_WIDGET (vbox));

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    /* Update Interval */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);
    label = gtk_label_new (_("Update Interval: "));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

    op->m_UpdateOption = gtk_option_menu_new ();
    gtk_widget_show (op->m_UpdateOption);
    gtk_box_pack_start (GTK_BOX (hbox), op->m_UpdateOption, FALSE, FALSE, 0);

    op->m_UpdateMenu = gtk_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (op->m_UpdateOption),
                              op->m_UpdateMenu);

    op->m_UpdateMenuItem =
        gtk_menu_item_new_with_label (_("Fastest (~250ms)"));
    gtk_widget_show (op->m_UpdateMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu),
                           op->m_UpdateMenuItem);

    op->m_UpdateMenuItem = gtk_menu_item_new_with_label (_("Fast (~500ms)"));
    gtk_widget_show (op->m_UpdateMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu),
                           op->m_UpdateMenuItem);

    op->m_UpdateMenuItem =
        gtk_menu_item_new_with_label (_("Normal (~750ms)"));
    gtk_widget_show (op->m_UpdateMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu),
                           op->m_UpdateMenuItem);

    op->m_UpdateMenuItem = gtk_menu_item_new_with_label (_("Slow (~1s)"));
    gtk_widget_show (op->m_UpdateMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu),
                           op->m_UpdateMenuItem);

    gtk_option_menu_set_history (GTK_OPTION_MENU (op->m_UpdateOption),
                                 base->m_UpdateInterval);

    g_signal_connect (op->m_UpdateOption, "changed",
                      G_CALLBACK (UpdateChange), base);

    /* Width */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
        label = gtk_label_new (_("Width:"));
    else
        label = gtk_label_new (_("Height:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

    op->m_Width =
        gtk_spin_button_new_with_range (10, 128, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (op->m_Width), base->m_Width);
    gtk_widget_show (op->m_Width);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_Width), FALSE,
                        FALSE, 0);
    g_signal_connect (op->m_Width, "value-changed", G_CALLBACK (SpinChange),
                      &base->m_Width);

    /* TimeScale */
    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    op->m_TimeScale = gtk_check_button_new_with_mnemonic (_("Non-linear time-scale"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (op->m_TimeScale),
                                  base->m_TimeScale);
    gtk_widget_show (op->m_TimeScale);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_TimeScale), FALSE,
                        FALSE, 0);
    g_signal_connect (op->m_TimeScale, "toggled", G_CALLBACK (TimeScaleChange),
                      base);
    gtk_size_group_add_widget (sg, op->m_TimeScale);

    /* Frame */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    op->m_GraphFrame = gtk_check_button_new_with_mnemonic (_("Show frame"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (op->m_GraphFrame),
                                  base->m_Frame);
    gtk_widget_show (op->m_GraphFrame);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_GraphFrame), FALSE,
                        FALSE, 0);
    g_signal_connect (op->m_GraphFrame, "toggled", G_CALLBACK (FrameChange),
                      base);
    gtk_size_group_add_widget (sg, op->m_GraphFrame);

    vbox2 = GTK_BOX (gtk_vbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (vbox2));
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);

    /* Associate Command */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
   	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);
    label = gtk_label_new (_("Associated command :"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);
    op->m_AssociateCommand = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(op->m_AssociateCommand), 32);
    gtk_entry_set_text (GTK_ENTRY(op->m_AssociateCommand), base->m_AssociateCommand);
    gtk_widget_show (op->m_AssociateCommand);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_AssociateCommand), FALSE, FALSE, 0);
    g_signal_connect (op->m_AssociateCommand, "changed", G_CALLBACK (AssociateCommandChange), base);

    /* Foreground 1 */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    label = gtk_label_new (_("Color 1:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

    op->m_FG1 = gtk_button_new ();
    op->m_ColorDA = gtk_drawing_area_new ();

    gtk_widget_modify_bg (op->m_ColorDA, GTK_STATE_NORMAL,
                          &base->m_ForeGround1);
    gtk_widget_set_size_request (op->m_ColorDA, 12, 12);
    gtk_container_add (GTK_CONTAINER (op->m_FG1), op->m_ColorDA);
    gtk_widget_show (GTK_WIDGET (op->m_FG1));
    gtk_widget_show (GTK_WIDGET (op->m_ColorDA));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_FG1), FALSE, FALSE,
                        0);

    g_signal_connect (op->m_FG1, "clicked", G_CALLBACK (ChangeColor1), base);

    /* Foreground2 */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    label = gtk_label_new (_("Color 2:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

    op->m_FG2 = gtk_button_new ();
    op->m_ColorDA2 = gtk_drawing_area_new ();

    gtk_widget_modify_bg (op->m_ColorDA2, GTK_STATE_NORMAL,
                          &base->m_ForeGround2);
    gtk_widget_set_size_request (op->m_ColorDA2, 12, 12);
    gtk_container_add (GTK_CONTAINER (op->m_FG2), op->m_ColorDA2);
    gtk_widget_show (GTK_WIDGET (op->m_FG2));
    gtk_widget_show (GTK_WIDGET (op->m_ColorDA2));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_FG2), FALSE, FALSE,
                        0);

    g_signal_connect (op->m_FG2, "clicked", G_CALLBACK (ChangeColor2), base);

    if (base->m_Mode == 1)
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);

    /* Foreground3 */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    label = gtk_label_new (_("Color 3:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);
    op->m_FG3 = gtk_button_new ();
    op->m_ColorDA5 = gtk_drawing_area_new ();
    gtk_widget_modify_bg (op->m_ColorDA5, GTK_STATE_NORMAL,
                          &base->m_ForeGround3);
    gtk_widget_set_size_request (op->m_ColorDA5, 12, 12);
    gtk_container_add (GTK_CONTAINER (op->m_FG3), op->m_ColorDA5);
    gtk_widget_show (GTK_WIDGET (op->m_FG3));
    gtk_widget_show (GTK_WIDGET (op->m_ColorDA5));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_FG3), FALSE, FALSE,
                        0);
    g_signal_connect (op->m_FG3, "clicked", G_CALLBACK (ChangeColor4), base);

    if (base->m_Mode == 0 || base->m_Mode == 2 || base->m_ColorMode == 0)
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
    else
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), TRUE);


    /* Background */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    label = gtk_label_new (_("Background:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

    op->m_BG = gtk_button_new ();
    op->m_ColorDA3 = gtk_drawing_area_new ();

    gtk_widget_modify_bg (op->m_ColorDA3, GTK_STATE_NORMAL,
                          &base->m_BackGround);
    gtk_widget_set_size_request (op->m_ColorDA3, 12, 12);
    gtk_container_add (GTK_CONTAINER (op->m_BG), op->m_ColorDA3);
    gtk_widget_show (GTK_WIDGET (op->m_BG));
    gtk_widget_show (GTK_WIDGET (op->m_ColorDA3));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_BG), FALSE, FALSE,
                        0);

    g_signal_connect (op->m_BG, "clicked", G_CALLBACK (ChangeColor3), base);

    /* Modes */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

    label = gtk_label_new (_("Mode:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

    op->m_OptionMenu = gtk_option_menu_new ();
    gtk_widget_show (op->m_OptionMenu);
    gtk_box_pack_start (GTK_BOX (hbox), op->m_OptionMenu, FALSE, FALSE, 0);

    op->m_Menu = gtk_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (op->m_OptionMenu), op->m_Menu);

    op->m_MenuItem = gtk_menu_item_new_with_label (_("Normal"));
    gtk_widget_show (op->m_MenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_Menu), op->m_MenuItem);

    op->m_MenuItem = gtk_menu_item_new_with_label (_("LED"));
    gtk_widget_show (op->m_MenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_Menu), op->m_MenuItem);

    op->m_MenuItem = gtk_menu_item_new_with_label (_("No history"));
    gtk_widget_show (op->m_MenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_Menu), op->m_MenuItem);

    gtk_option_menu_set_history (GTK_OPTION_MENU (op->m_OptionMenu),
                                 base->m_Mode);

    g_signal_connect (op->m_OptionMenu, "changed", G_CALLBACK (ModeChange),
                      base);

    /* Color mode */

    hbox = GTK_BOX (gtk_hbox_new (FALSE, BORDER));
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);
    label = gtk_label_new (_("Color mode: "));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

    op->m_ModeOption = gtk_option_menu_new ();
    gtk_widget_show (op->m_ModeOption);
    gtk_box_pack_start (GTK_BOX (hbox), op->m_ModeOption, FALSE, FALSE, 0);

    op->m_ModeMenu = gtk_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (op->m_ModeOption),
                              op->m_ModeMenu);

    op->m_ModeMenuItem = gtk_menu_item_new_with_label (_("None"));
    gtk_widget_show (op->m_ModeMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_ModeMenu),
                           op->m_ModeMenuItem);

    op->m_ModeMenuItem = gtk_menu_item_new_with_label (_("Gradient"));
    gtk_widget_show (op->m_ModeMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_ModeMenu),
                           op->m_ModeMenuItem);

    op->m_ModeMenuItem = gtk_menu_item_new_with_label (_("Fire"));
    gtk_widget_show (op->m_ModeMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_ModeMenu),
                           op->m_ModeMenuItem);

    op->m_ModeMenuItem = gtk_menu_item_new_with_label ("cpufreq");
    gtk_widget_show (op->m_ModeMenuItem);
    gtk_menu_shell_append (GTK_MENU_SHELL (op->m_ModeMenu),
                            op->m_ModeMenuItem);

    gtk_option_menu_set_history (GTK_OPTION_MENU (op->m_ModeOption),
                                 base->m_ColorMode);

    g_signal_connect (op->m_ModeOption, "changed",
                      G_CALLBACK (ColorModeChange), base);

    gtk_widget_show_all (GTK_WIDGET (hbox));

    op->m_Notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (op->m_Notebook),
                                    BORDER - 2);
    label = gtk_label_new (_("Appearance"));
    gtk_notebook_append_page (GTK_NOTEBOOK (op->m_Notebook),
                              GTK_WIDGET (vbox2), GTK_WIDGET (label));
    label = gtk_label_new (_("Advanced"));
    gtk_notebook_append_page (GTK_NOTEBOOK (op->m_Notebook),
                              GTK_WIDGET (vbox), GTK_WIDGET (label));
    gtk_widget_show (op->m_Notebook);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
                        GTK_WIDGET (op->m_Notebook), TRUE, TRUE, 0);

    gtk_widget_show (dlg);
}

gboolean
UpdateCPU (CPUGraph * base)
{
    //fprintf(stderr,"update cpu\n");
    gint i;
    base->m_CpuData = cpuData_read();
    for(i=0; i<base->nrCores; i++){
      //fprintf(stderr,"bar load = %f\n",(gdouble)base->m_CpuData[i+1].load);
      gtk_progress_bar_set_fraction(
                                    GTK_PROGRESS_BAR(base->m_pBar[i]),
                                    (gdouble)base->m_CpuData[i+1].load/CPU_SCALE);
    }

    if(base->m_TimeScale)
    {
        int i = base->m_Values - 1;
        int j = i + base->m_Values;
        while(i > 0)
        {
            int a, b;

            a = base->m_History[i], b = base->m_History[i-1];
            if( a < b ) a++;
            int factor = (i*2);
            base->m_History[i--] = ( a*(factor-1) + b )/factor;

            a = base->m_History[j], b = base->m_History[j-1];
            if( a < b ) a++;
            base->m_History[j--] = ( a*(factor-1) + b )/factor;
        }
    } else {
        memmove (base->m_History + 1
                , base->m_History
                , (base->m_Values*2-1)*sizeof(int));
    }
    fprintf(stderr,"cpu data load %f\n",base->m_CpuData[0].load);
    base->m_History[0] = (long)base->m_CpuData[0].load;
    base->m_History[base->m_Values] = base->m_CpuData[0].scalCurFreq;

    /* Tooltip */
    UpdateTooltip (base);

    /* Draw the graph. */
    gtk_widget_queue_draw (base->m_DrawArea);
    return TRUE;
}

void
DrawGraph (CPUGraph * base)
{
    GtkWidget *da = base->m_DrawArea;
    int w, h;

    w = da->allocation.width;
    h = da->allocation.height;

    /* Dynamically allocated everytime just in case depth changes */
    GdkGC *fg1 = gdk_gc_new (da->window);
    GdkGC *fg2 = gdk_gc_new (da->window);
    GdkGC *bg = gdk_gc_new (da->window);
    gdk_gc_set_rgb_fg_color (bg, &base->m_BackGround);

    gdk_draw_rectangle (da->window, bg, TRUE, 0, 0, w, h);

    if (base->m_Mode == 0)
    {
      drawGraphModeNormal(base, fg1, da, w, h);
    }
    else if (base->m_Mode == 1)
    {
      drawGraphModeLED(base, fg1, fg2, da, w, h);
    }
    else if (base->m_Mode == 2)
    {
      drawGraphModeNoHistory(base, fg1, fg2, da, w, h);
    }

    g_object_unref (fg2);
    g_object_unref (fg1);
    g_object_unref (bg);
}

void
DrawAreaExposeEvent (GtkWidget * da, GdkEventExpose * event, gpointer data)
{
    CPUGraph *base = (CPUGraph *) data;

    DrawGraph (base);
}

void
SpinChange (GtkSpinButton * sb, int *value)
{
    (*value) = gtk_spin_button_get_value_as_int (sb);
}

void
ApplyChanges (CPUGraph * base)
{
    int update;

    if (base->m_TimeoutID)
        g_source_remove (base->m_TimeoutID);
    switch (base->m_UpdateInterval)
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
    base->m_TimeoutID = g_timeout_add (update, (GtkFunction) UpdateCPU, base);

    UserSetSize (base);
    SetHistorySize (base, base->m_Width);
}

void
ChangeColor1 (GtkButton * button, CPUGraph * base)
{
    ChangeColor (0, base);
}

void
ChangeColor2 (GtkButton * button, CPUGraph * base)
{
    ChangeColor (1, base);
}

void
ChangeColor3 (GtkButton * button, CPUGraph * base)
{
    ChangeColor (2, base);
}

void
ChangeColor4 (GtkButton * button, CPUGraph * base)
{
    ChangeColor (3, base);
}

void
ChangeColor (int color, CPUGraph * base)
{
    GtkWidget *dialog;
    GtkColorSelection *colorsel;
    gint response;

    dialog = gtk_color_selection_dialog_new ("Select color");
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                  GTK_WINDOW (base->m_OptionsDialog));

    colorsel =
        GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel);

    if (color == 0)
    {
        gtk_color_selection_set_previous_color (colorsel,
                                                &base->m_ForeGround1);
        gtk_color_selection_set_current_color (colorsel,
                                               &base->m_ForeGround1);
    }
    else if (color == 1)
    {
        gtk_color_selection_set_previous_color (colorsel,
                                                &base->m_ForeGround2);
        gtk_color_selection_set_current_color (colorsel,
                                               &base->m_ForeGround2);
    }
    else if (color == 2)
    {
        gtk_color_selection_set_previous_color (colorsel,
                                                &base->m_BackGround);
        gtk_color_selection_set_current_color (colorsel, &base->m_BackGround);
    }
    else if (color == 3)
    {
        gtk_color_selection_set_previous_color (colorsel,
                                                &base->m_ForeGround3);
        gtk_color_selection_set_current_color (colorsel,
                                               &base->m_ForeGround3);
    }

    gtk_color_selection_set_has_palette (colorsel, TRUE);

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    if (response == GTK_RESPONSE_OK)
    {
        if (color == 0)
        {
            gtk_color_selection_get_current_color (colorsel,
                                                   &base->m_ForeGround1);
            gtk_widget_modify_bg (base->m_Options.m_ColorDA, GTK_STATE_NORMAL,
                                  &base->m_ForeGround1);
        }
        else if (color == 1)
        {
            gtk_color_selection_get_current_color (colorsel,
                                                   &base->m_ForeGround2);
            gtk_widget_modify_bg (base->m_Options.m_ColorDA2,
                                  GTK_STATE_NORMAL, &base->m_ForeGround2);
        }
        else if (color == 2)
        {
            gtk_color_selection_get_current_color (colorsel,
                                                   &base->m_BackGround);
            gtk_widget_modify_bg (base->m_Options.m_ColorDA3,
                                  GTK_STATE_NORMAL, &base->m_BackGround);
        }
        else if (color == 3)
        {
            gtk_color_selection_get_current_color (colorsel,
                                                   &base->m_ForeGround3);
            gtk_widget_modify_bg (base->m_Options.m_ColorDA5,
                                  GTK_STATE_NORMAL, &base->m_ForeGround3);
        }
    }

    gtk_widget_destroy (dialog);

}

void
SetHistorySize (CPUGraph * base, int size)
{
    int i;
    base->m_History =
        (long *) realloc (base->m_History, 2 * size * sizeof (long));

    base->m_CpuData = cpuData_read();
    base->m_CpuData[0].pUsed = 0;
    base->m_CpuData[0].pTotal = 0;
    long usage = base->m_CpuData[0].load;
    for (i = size - 1; i >= base->m_Values; i--)
    {
        base->m_History[i] = usage;
        base->m_History[i+size] = base->m_CpuData[0].scalCurFreq;
    }
    base->m_Values = size;

}

void
ModeChange (GtkOptionMenu * om, CPUGraph * base)
{
    base->m_Mode = gtk_option_menu_get_history (om);
    if (base->m_Mode == 0)
    {
        if (base->m_ColorMode > 0)
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2),
                                      TRUE);
        else
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2),
                                      FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
    }
    else if (base->m_Mode == 1)
    {
        if (base->m_ColorMode > 0)
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3),
                                      TRUE);
        else
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3),
                                      FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
    }
    else if (base->m_Mode == 2)
    {
        if (base->m_ColorMode > 0)
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2),
                                      TRUE);
        else
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2),
                                      FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
    }
}
void
UpdateChange (GtkOptionMenu * om, CPUGraph * base)
{
    base->m_UpdateInterval = gtk_option_menu_get_history (om);
}

void
FrameChange (GtkToggleButton * button, CPUGraph * base)
{
    base->m_Frame = gtk_toggle_button_get_active (button);

    gtk_frame_set_shadow_type (GTK_FRAME (base->m_FrameWidget),
            base->m_Frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

void
AssociateCommandChange (GtkEntry *entry, CPUGraph * base)
{
    base->m_AssociateCommand = g_strdup (gtk_entry_get_text (entry));
}

gboolean
LaunchCommand(GtkWidget *w,GdkEventButton *event, CPUGraph *base)
{
    if(event->button == 1){
        GString *cmd;
        if (strlen(base->m_AssociateCommand) == 0) {
            return;
        }
        cmd = g_string_new (base->m_AssociateCommand);
        xfce_exec (cmd->str, FALSE, FALSE, NULL);
        g_string_free (cmd, TRUE);
    }
    return FALSE;
}

void
TimeScaleChange (GtkToggleButton * button, CPUGraph * base)
{
    base->m_TimeScale = gtk_toggle_button_get_active (button);
}

void
ColorModeChange (GtkOptionMenu * om, CPUGraph * base)
{
    base->m_ColorMode = gtk_option_menu_get_history (om);

    if (base->m_ColorMode == 0)
    {
        if (base->m_Mode == 0 || base->m_Mode == 2)
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2),
                                      FALSE);
        else
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2),
                                      TRUE);
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
    }
    else if (base->m_ColorMode == 1)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
        if (base->m_Mode == 1)
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3),
                                      TRUE);
        else
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3),
                                      FALSE);
    }
    else if (base->m_ColorMode == 2)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
        if (base->m_Mode == 1)
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3),
                                      TRUE);
        else
            gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3),
                                      FALSE);
    }
	else if (base->m_ColorMode == 3)
	{
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
	        if (base->m_Mode == 1)
        	        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), TRUE);
	        else
        	        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
}
}

