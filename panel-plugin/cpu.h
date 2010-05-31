#ifndef _XFCE_CPU_H_
#define _XFCE_CPU_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfce4panel/xfce-panel-plugin.h>

#include "os.h"

#define BORDER  8

typedef struct
{
	/* GUI components */
	XfcePanelPlugin *plugin;
	GtkWidget *frame_widget;
	GtkWidget *draw_area;
	GtkWidget *box;
	GtkWidget **bars;
	GtkWidget *color_buttons[4];
	GtkWidget *tooltip_text;

	/* Settings */
	guint update_interval; /* Number of ms between updates. */
	gboolean non_linear;
	guint size;
	guint mode;
	guint color_mode;
	gboolean has_frame;
	gboolean has_border;
	gboolean has_bars;
	gchar  *command;
	gboolean in_terminal;
	gboolean startup_notification;
	GdkColor colors[4];

	/* Runtime data */
	guint nr_cores;
	guint timeout_id;
	guint *history;
	gssize history_size;
	CpuData *cpu_data;
} CPUGraph;

void set_startup_notification( CPUGraph *base, gboolean startup_notification );
void set_in_terminal( CPUGraph *base, gboolean in_terminal );
void set_command( CPUGraph *base, const gchar *command );
void set_bars( CPUGraph * base, gboolean bars);
void set_border( CPUGraph *base, gboolean border);
void set_frame( CPUGraph *base, gboolean frame );
void set_nonlinear_time( CPUGraph *base, gboolean nonlinear );
void set_update_rate( CPUGraph *base, guint rate );
void set_size( CPUGraph *base, guint width );
void set_color_mode( CPUGraph *base, guint color_mode );
void set_mode( CPUGraph *base, guint mode );
void set_color( CPUGraph *base, guint number, GdkColor color );
#endif /* !_XFCE_CPU_H_ */
