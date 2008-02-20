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
void drawGraphModeNormal (CPUGraph *base, GdkGC *fg1, GtkWidget *da, int w, int h){
  int x, y;

  for (x = w; x >= 0; x--)
  {
    long usage = h * base->m_History[w - x] / CPU_SCALE;

    if(usage == 0) continue;

    if (base->m_ColorMode == 0) {
      gdk_gc_set_rgb_fg_color (fg1, &base->m_ForeGround1);

      if(base->m_Frame)
        gdk_draw_line (da->window, fg1, x+1, h+1-usage, x+1, h);
      else
        gdk_draw_line (da->window, fg1, x, h-usage, x, h-1);
    }
    else if (base->m_ColorMode == 3) /* cpu freq. based */
    {
      GdkColor color;
      double t = (double) (base->m_History[base->m_Values+ w - x] - base->m_CpuData[0].scalMinFreq)
        / (base->m_CpuData[0].scalMaxFreq - base->m_CpuData[0].scalMinFreq);

      color.red = _lerp (t, base->m_ForeGround1.red,
                         base->m_ForeGround2.red);
      color.green = _lerp (t, base->m_ForeGround1.green,
                           base->m_ForeGround2.green);
      color.blue = _lerp (t, base->m_ForeGround1.blue,
                          base->m_ForeGround2.blue);
      gdk_gc_set_rgb_fg_color (fg1, &color);

      if(base->m_Frame)
        gdk_draw_line (da->window, fg1
                       , x+1, h+1-usage, x+1, h);
      else
        gdk_draw_line (da->window, fg1
                       , x, h-usage, x, h-1);

    }
    else /* 1 or 2 */
    {
      int tmp = 0;
      int length = h - (h - usage);
      for (y = h; y >= h - usage; y--, tmp++)
      {
        if (base->m_ColorMode > 0)
        {
          GdkColor color;
          double t =
            (base->m_ColorMode == 1) ? (tmp / (double) (h)) :
            (tmp / (double) (length));
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
        }
        gdk_draw_point (da->window, fg1, x, y);
      }
    }
  }
}


/*** MODE 1 : LED ***/
void drawGraphModeLED (CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h){
  int nrx = (int)((w + 1) / 3.0);
  int nry = (int)((h + 1) / 2.0);
  int x, y;

  gdk_gc_set_rgb_fg_color (fg1, &base->m_ForeGround1);
  gdk_gc_set_rgb_fg_color (fg2, &base->m_ForeGround2);


  //fprintf(stderr,"nry %i %d\n", nry, CPU_SCALE);

  for (x = nrx ; x >= 0; x--)
  {
    int tmp = 0;
    int idx = nrx-x;
    int limit = (int)(nry - nry * base->m_History[idx]/CPU_SCALE);
    //fprintf(stderr,"lim=%d %d %d %d\n",limit, idx, nry, base->m_History[idx]);
    for (y = nry; y >= 0; y--)
    {
      if (base->m_ColorMode > 0)
      {
        GdkColor color;
        double t =
          (base->m_ColorMode == 1) ?
          (tmp / nry) :
          (tmp / limit);
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
                          (y >= limit) ? fg1 : fg2,
                          TRUE, x * 3, y * 2, 2, 1);

    }
  }
}

/*** MODE 2 : NoHistory ***/
void drawGraphModeNoHistory(CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h){
  int y;
  long usage = h * base->m_History[0] / CPU_SCALE;
  int tmp = 0;
  int length = usage;

  for (y = h; y >= h - usage; y--)
  {
    if (base->m_ColorMode > 0)
    {
      GdkColor color;
      double t =
        (base->m_ColorMode == 1) ? (tmp / (double) (h)) :
        (tmp / (double) (length));
      color.red =
        _lerp (t, base->m_ForeGround1.red,
               base->m_ForeGround2.red);
      color.green =
        _lerp (t, base->m_ForeGround1.green,
               base->m_ForeGround2.green);
      color.blue =
        _lerp (t, base->m_ForeGround1.blue,
               base->m_ForeGround2.blue);
      gdk_gc_set_rgb_fg_color (fg2, &color);
      tmp++;
    }
    gdk_draw_line (da->window,
                   (base->m_ColorMode > 0) ? fg2 : fg1,
                   0, y, w, y);
  }
}
