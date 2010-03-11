#ifndef _XFCE_CPU_H_
#define _XFCE_CPU_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "os.h"

#define BORDER  8

typedef struct
{
	/* GUI components */
	XfcePanelPlugin *plugin;
	GtkWidget *m_FrameWidget;
	GtkWidget *m_DrawArea;
	GtkWidget *m_OptionsDialog;
	GtkWidget *m_Box;
	GtkWidget **m_pBar;
	GtkWidget *color_buttons[4];

	/* Settings */
	int update_interval; /* Number of ms between updates. */
	gboolean non_linear;
	int size;
	int mode;
	int color_mode;
	gboolean frame;
	gboolean border;
	gboolean bars;
	gchar  *command;
	GdkColor colors[4];

	/* Runtime data */
	guint nr_cores;
	guint timeout_id;
	int *history;
	gssize history_size;
	int orientation;
	CpuData *cpu_data;

} CPUGraph;

void set_command( CPUGraph *base, const gchar *command );
void set_bars( CPUGraph * base, gboolean bars);
void set_border( CPUGraph *base, gboolean border);
void set_frame( CPUGraph *base, gboolean frame );
void set_nonlinear_time( CPUGraph *base, gboolean nonlinear );
void set_update_rate( CPUGraph *base, int rate );
void set_size( CPUGraph *base, int width );
void set_color_mode( CPUGraph *base, int color_mode );
void set_mode( CPUGraph *base, int mode );
void set_color( CPUGraph *base, int number, GdkColor color );
#endif /* !_XFCE_CPU_H_ */
