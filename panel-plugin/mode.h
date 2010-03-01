#ifndef __XFCE_MODE_H__
#define __XFCE_MODE_H__

#include "cpu.h"

void drawGraphModeNormal( CPUGraph *base, GtkWidget *da, int w, int h );
void drawGraphModeLED( CPUGraph *base, GtkWidget *da, int w, int h );
void drawGraphModeNoHistory( CPUGraph *base, GtkWidget *da, int w, int h );
void drawGraphGrid( CPUGraph *base, GtkWidget *da, int w, int h );

#endif
