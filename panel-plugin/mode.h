#ifndef __XFCE_MODE_H__
#define __XFCE_MODE_H__

#include "cpu.h"

void drawGraphModeNormal( CPUGraph *base, GdkGC *fg1, GtkWidget *da, int w, int h );
void drawGraphModeLED( CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h );
void drawGraphModeNoHistory( CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h );

#endif
