#ifndef __XFCE_CPU_H__
#define __XFCE_CPU_H__

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "os.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>
#include <stdio.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/plugins.h>
#include <panel/xfce.h>

#define MAX_WIDTH 64

typedef struct
{
	GtkWidget *m_Updater;
	GtkWidget *m_Width;
	GtkWidget *m_Height;
	GtkWidget *m_FG1;
	GtkWidget *m_FG2;
	GtkWidget *m_BG;
	GtkWidget *m_ColorDA;
	GtkWidget *m_ColorDA2;
	GtkWidget *m_ColorDA3;
	
	GtkWidget *m_OptionMenu;
	GtkWidget *m_Menu;
	GtkWidget *m_MenuItem;
}SOptions;

typedef struct
{
   	GtkWidget *m_Parent; // Parent.
	GtkWidget *m_Alignment;
	GtkBox	  *m_Box;
	GtkWidget *m_DrawArea;
	GtkWidget *m_OptionsDialog;
	SOptions m_Options;
	GtkSizeGroup *m_Sg;
   
   	int m_UpdateInterval; // Number of ms between updates.
   	int m_Width; // The width of the plugin.
	int m_TmpWidth;
	int m_RealWidth;
	int m_Height;
	int m_TmpHeight;
	int m_RealHeight;
   	int m_Mode; // Eventual mode of the plugin.
   
   	GdkColor m_ForeGround1; // Inactive color.
   	GdkColor m_ForeGround2; // Active color.
   	GdkColor m_BackGround; // Background color.
	
	GtkTooltips *m_Tooltip; // Eventual tooltip.
	
	guint m_TimeoutID; // Timeout ID for the tooltip;
	long m_CPUUsage;
	long *m_History;
	int m_Values;

	int m_Orientation;

	int m_OldUsage;
	int m_OldTotal;
}CPUGraph;

gboolean CreateControl (Control *control);
void AttachCallback (Control *control, const char *signal, GCallback callback, gpointer data);
void Kill (Control *control);
void ReadSettings (Control *control, xmlNode *node);
void WriteSettings (Control *control, xmlNode *node);
void SetSize (Control *control, int size);
gboolean UpdateCPU (CPUGraph *base);
void UpdateTooltip (CPUGraph *base);
void DrawGraph (CPUGraph *base);
void DrawAreaExposeEvent (GtkWidget *da, GdkEventExpose *event, gpointer data);
void CreateOptions (Control *control, GtkContainer *container, GtkWidget *done);
void SetOrientation (Control *control, int orientation);
void SetHistorySize (CPUGraph *base, int size);
void SetRealGeometry (CPUGraph *base);
	

void ChangeColor1 (GtkButton *button, CPUGraph *base);
void ChangeColor2 (GtkButton *button, CPUGraph *base);
void ChangeColor3 (GtkButton *button, CPUGraph *base);
void ChangeColor (int color, CPUGraph *base);
void SpinChange (GtkSpinButton *sb, int *value);
void ModeChange (GtkOptionMenu *om, int *value);
void ApplyChanges (CPUGraph *base);

#endif
