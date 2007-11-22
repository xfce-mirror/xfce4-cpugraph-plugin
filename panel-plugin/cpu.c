/*  cpu.c
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "cpu.h"
#include "mode.h"
#include "option.h"
#include "settings.h"
#include "cpu_os.h"


static GtkTooltips *tooltips = NULL;

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
	cpuLoadMon_free();
	for(i=0; i<base->nrCores-1 ; i++)
		gtk_widget_destroy(base->m_pBar[i]);

	gtk_widget_destroy(base->m_Box);
	free(base->m_pBar);
	if (base->m_TimeoutID)
		g_source_remove (base->m_TimeoutID);

	if (base->m_History)
		g_free (base->m_History);

	g_object_unref (base->m_Tooltip);

	g_free (base);
}


CPUGraph *
CreateControl (XfcePanelPlugin * plugin)
{
    GtkWidget *frame, *ebox;
    GtkOrientation orientation;
    GtkProgressBarOrientation barOrientation;
    CPUGraph *base = g_new0 (CPUGraph, 1);
    gint i;

    base->plugin = plugin;

    ebox = gtk_event_box_new ();
    gtk_widget_show (ebox);
    gtk_container_add (GTK_CONTAINER (plugin), ebox);

    xfce_panel_plugin_add_action_widget (plugin, ebox);

    orientation = xfce_panel_plugin_get_orientation(plugin);
    if(orientation == GTK_ORIENTATION_HORIZONTAL)
      barOrientation = GTK_PROGRESS_BOTTOM_TO_TOP;
    else
      barOrientation = GTK_PROGRESS_LEFT_TO_RIGHT;

    base->m_Box = xfce_hvbox_new(orientation, FALSE, 0);
    gtk_widget_show(base->m_Box);
    gtk_container_add(GTK_CONTAINER(ebox), base->m_Box);
    
    gtk_container_set_border_width(GTK_CONTAINER(frame), BORDER / 2);
 
    /* <-- Multicore stuff */
    if((base->nrCores = cpuLoadMon_init() - 1) < 0)
      DBG("Cannot init base monitor!\n");

    base->m_pBar =
        (GtkWidget **) malloc(sizeof(GtkWidget *) * base->nrCores);

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

    /* <-- End of multicore stuff */

    base->m_FrameWidget = frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
 

    gtk_box_pack_start(
                  GTK_BOX(base->m_Box),
                  frame,
                  TRUE,
                  TRUE,
                  2);
    gtk_widget_show (frame);






    base->m_DrawArea = gtk_drawing_area_new ();
    gtk_widget_set_app_paintable (base->m_DrawArea, TRUE);
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (base->m_DrawArea));
    gtk_widget_show (base->m_DrawArea);

    xfce_panel_plugin_add_action_widget (plugin, base->m_DrawArea);

    base->m_Tooltip = gtk_tooltips_new ();
    g_object_ref (base->m_Tooltip);
    gtk_object_sink (GTK_OBJECT (base->m_Tooltip));

    g_signal_connect (ebox, "button-press-event", G_CALLBACK (LaunchCommand), base);
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
	for(i=0; i<base->nrCores; i++)
	{
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
	for(i=0; i<base->nrCores; i++)
	{
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
    int pos = snprintf (tooltip, 32, "Usage: %d%%", base->m_CPUUsage);
    if( scaling_cur_freq )
        snprintf (tooltip+pos, 32-pos, " (%d MHz)", scaling_cur_freq/1000);
    gtk_tooltips_set_tip (GTK_TOOLTIPS (base->m_Tooltip),
                          base->m_Box->parent, tooltip, NULL);
}

/**
 * size : size of pannel height/width
 * BORDER : use for size of bar
 */
gboolean
SetSize (XfcePanelPlugin *plugin, int size, CPUGraph *base)
{
    gint i, coreWidth;
    gtk_container_set_border_width (GTK_CONTAINER (base->m_FrameWidget), 0);
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
      for(i=0; i<base->nrCores; i++)
          gtk_widget_set_size_request(
                                GTK_WIDGET(base->m_pBar[i]),
                                BORDER,
                                size);

      coreWidth = base->nrCores*BORDER;
      gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                   base->m_Width+coreWidth, size);
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


gboolean
UpdateCPU (CPUGraph * base)
{
  gint i;
  cpuLoadData *data = cpuLoadMon_read();

  base->m_CPUUsage = data[0].value * 100.0;

  for(i=0; i<base->nrCores; i++){
    gtk_progress_bar_set_fraction(
                                  GTK_PROGRESS_BAR(base->m_pBar[i]),
                                  (gdouble)data[i+1].value);
  }

  memmove (base->m_History + 1, base->m_History,
           (base->m_Values-1)*sizeof(float));
  base->m_History[0] = data[0].value;

  /* Tooltip */
  UpdateTooltip (base);

  //fprintf(stderr, "update cpu %f\n", base->m_History[0]);

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

    /*fprintf(stderr, "mode selected %d\n", base->m_Mode);*/

    if (base->m_Mode == 0)
    {
      drawGraphNormal(base, fg1, da, w, h);
    }
    else if (base->m_Mode == 1)
    {
      drawGraphLED(base, fg1, fg2, da, w, h);
    }
    else if (base->m_Mode == 2)
    {
      drawGraphNoHistory(base, fg1, fg2, da, w, h);
    }
    else if (base->m_Mode == 3){
      drawGraphGrid(base, fg1, fg2, da, w, h);
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

    fprintf(stderr, "m_Width %d\n", base->m_Width);
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
    gint i;
    cpuLoadData *data = cpuLoadMon_read();
    float usage = data[0].value;

    base->m_History =
        (float *) realloc (base->m_History, size * sizeof (float));

    for (i = size - 1; i >= 0; i--)
    {
      base->m_History[i] = usage;
    }
    base->m_Values = size;

}

void
SetSensitive(CPUGraph *base, GtkWidget *fgA, GtkWidget *fgB,
             gboolean flag1, gboolean flag2){
  if (base->m_ColorMode > 0)
    gtk_widget_set_sensitive (GTK_WIDGET (fgA), flag1);
  else
    gtk_widget_set_sensitive (GTK_WIDGET (fgA), !flag1);
  gtk_widget_set_sensitive (GTK_WIDGET (fgB), flag2);
}

void
ModeChange (GtkOptionMenu * om, CPUGraph * base)
{
  base->m_Mode = gtk_option_menu_get_history (om);
  if (base->m_Mode == 0){
    SetSensitive(base, base->m_Options.m_FG2, base->m_Options.m_FG3, TRUE, FALSE);
  }
  else if (base->m_Mode == 1){
    SetSensitive(base, base->m_Options.m_FG3, base->m_Options.m_FG2, TRUE, TRUE);
  }
  else if (base->m_Mode == 2){
    SetSensitive(base, base->m_Options.m_FG2, base->m_Options.m_FG3, TRUE, FALSE);
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

