#ifndef __XFCE_MODE_H__
#define __XFCE_MODE_H__

#include "cpu.h"

void drawGraphMode0(CPUGraph *base, GdkGC *fg1, GtkWidget *da, int w, int h);
void drawGraphMode1(CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h);
void drawGraphMode2(CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h);
void drawGraphMode4(CPUGraph *base, GdkGC *fg1, GtkWidget *da, int w, int h);

#endif
