/*  mode.c
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

#include "mode.h"

/*** MODE 0 : Normal ***/
void drawGraphNormal (CPUGraph *base, GdkGC *fg1, GtkWidget *da, int w, int h){
  int x, y;

  //fprintf(stderr, "usage %f\n", base->m_History[0]);
  for (x = w; x >= 0; x--)
  {
    long usage = h * base->m_History[w - x];

    if(usage == 0) continue;

    if (base->m_ColorMode == 0) { /* none color mode */
      gdk_gc_set_rgb_fg_color (fg1, &base->m_ForeGround1);
      gdk_draw_line (da->window, fg1, x, h-usage, x, h-1);
    }
    if(base->m_ColorMode == 1){ /* color mode == 1 : gradient */
      int tmp = 0;
      for (y = h; y >= h - usage; y--)
      {
        GdkColor color;
        double t = tmp / (double) (h);
        color.red =
          _lerp (t, base->m_ForeGround1.red,
                 base->m_ForeGround2.red);
        color.green =
          _lerp (t, base->m_ForeGround1.green,
                 base->m_ForeGround2.green);
        color.blue =
          _lerp (t, base->m_ForeGround1.blue,
                 base->m_ForeGround2.blue);
        gdk_gc_set_rgb_fg_color (fg1, &color);
        tmp++;
        gdk_draw_point (da->window, fg1, x, y);
      }
    }
  }
}


/*** MODE 1 : LED ***/
void drawGraphLED (CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h){
  int nrx = (w + 1) / 3.0;
  int nry = (h + 1) / 2.0;
  int x, y;

  gdk_gc_set_rgb_fg_color (fg1, &base->m_ForeGround1);
  gdk_gc_set_rgb_fg_color (fg2, &base->m_ForeGround2);

  for (x = nrx ; x >= 0; x--)
  {
    int tmp = 0;
    int limit = nry * (1 - base->m_History[nrx - x]);

    for (y = nry; y >= 0; y--)
    {
      if (base->m_ColorMode == 1){// color mode == 1 gradient
        GdkColor color;
        double t = tmp / (double) (nry);
        color.red =
          _lerp (t, base->m_ForeGround2.red,
                 base->m_ForeGround3.red);
        color.green =
          _lerp (t, base->m_ForeGround2.green,
                 base->m_ForeGround3.green);
        color.blue =
          _lerp (t, base->m_ForeGround2.blue,
                 base->m_ForeGround3.blue);
        gdk_gc_set_rgb_fg_color (fg2, &color);
        tmp++;
      }
      gdk_draw_rectangle (da->window,
                          (y > limit) ? fg1 : fg2,
                          TRUE, x * 3, y * 2, 2, 1);
    }
  }
}

/*** MODE 2 : No History ***/
void drawGraphNoHistory(CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h){
  int y, tmp, limit;
  long usage = h * base->m_History[0];

  if (base->m_ColorMode == 0) { /* none color mode */
    gdk_gc_set_rgb_fg_color (fg1, &base->m_ForeGround1);
    /* gdk_draw_rectangle(xMin, yMin, xLength, yLength) origin=top-left */
    gdk_draw_rectangle (da->window, fg1, TRUE, 0, h-usage, w, usage);
  }else if(base->m_ColorMode == 1){ /* color mode == 1 : gradient */
    tmp = 0;
    limit = h-1-usage;
    for (y = h-1; y >= limit; y--){
      GdkColor color;
      double t = tmp / (double) (h);
      color.red =
        _lerp (t, base->m_ForeGround1.red,
               base->m_ForeGround2.red);
      color.green =
        _lerp (t, base->m_ForeGround1.green,
               base->m_ForeGround2.green);
      color.blue =
        _lerp (t, base->m_ForeGround1.blue,
               base->m_ForeGround2.blue);
      gdk_gc_set_rgb_fg_color (fg1, &color);
      tmp++;
      gdk_draw_line (da->window, fg1, 0, y, w-1, y);
    }
  }
}

/*** MODE 3 : Grid ***/
void drawGraphGrid (CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h){
  int nrx = w / 6.0;
  int nry = h / 4.0;
  int x, y;

  point last, current;
  last.x = -1;

  /* draw grid */
  gdk_gc_set_rgb_fg_color (fg1, &base->m_ForeGround1);
  for(x = nrx; x >= 0; x--){
    gdk_draw_line (da->window, fg1, x*6, 0, x*6, h);
  }
  for(y = nry; y>=0; y--){
    gdk_draw_line (da->window, fg1, 0, y*4, w, y*4);
  }

  /* draw data */
  gdk_gc_set_rgb_fg_color (fg2, &base->m_ForeGround2);
  for (x = w; x >= 0; x--)
  {
    current.x = x;
    current.y = (h-1) * (1-base->m_History[w - x]);
    if(last.x == -1) last = current;
    gdk_draw_line (da->window, fg2, current.x, current.y, last.x, last.y);
    last = current;
    }
}
