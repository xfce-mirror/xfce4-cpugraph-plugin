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
	GtkWidget *m_Notebook;
	/* Update */
	GtkWidget *m_UpdateOption;
	GtkWidget *m_UpdateMenu;
	GtkWidget *m_UpdateMenuItem;
	
	GtkWidget *m_Width;
	GtkWidget *m_Height;
	GtkWidget *m_GraphFrame;
	
	GtkWidget *m_FG1;
	GtkWidget *m_FG2;
	GtkWidget *m_FG3;
	GtkWidget *m_BG;
	GtkWidget *m_FC;
	GtkWidget *m_ColorDA;
	GtkWidget *m_ColorDA2;
	GtkWidget *m_ColorDA3;
	GtkWidget *m_ColorDA4;
	GtkWidget *m_ColorDA5;
	
	GtkWidget *m_FrameApperance;
	GtkWidget *m_FrameColor;
	GtkWidget *m_FrameMode;

	GtkWidget *m_ModeOption;
	GtkWidget *m_ModeMenu;
	GtkWidget *m_ModeMenuItem;
	
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
	int m_ColorMode;
	int m_Frame;
   
   	GdkColor m_ForeGround1; // Inactive color.
   	GdkColor m_ForeGround2; // Active color.
	GdkColor m_ForeGround3;
   	GdkColor m_BackGround; // Background color.
	GdkColor m_FrameColor;
	
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
void UserSetSize (CPUGraph *base);
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
void ChangeColor4 (GtkButton *button, CPUGraph *base);
void ChangeColor5 (GtkButton *button, CPUGraph *base);
void ChangeColor (int color, CPUGraph *base);
void SpinChange (GtkSpinButton *sb, int *value);
void UpdateChange (GtkOptionMenu *om, CPUGraph *base);
void ModeChange (GtkOptionMenu *om, CPUGraph *base);
void ApplyChanges (CPUGraph *base);
void FrameChange (GtkToggleButton *button, CPUGraph *base);
void ColorModeChange (GtkOptionMenu *om, CPUGraph *base);

#endif
