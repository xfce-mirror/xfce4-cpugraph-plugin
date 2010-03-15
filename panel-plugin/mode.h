#ifndef _XFCE_MODE_H_
#define _XFCE_MODE_H_

#include "cpu.h"

void draw_graph_normal( CPUGraph *base, GtkWidget *da, gint w, gint h );
void draw_graph_LED( CPUGraph *base, GtkWidget *da, gint w, gint h );
void draw_graph_no_history( CPUGraph *base, GtkWidget *da, gint w, gint h );
void draw_graph_grid( CPUGraph *base, GtkWidget *da, gint w, gint h );

#endif /* !_XFCE_MODE_H_ */
