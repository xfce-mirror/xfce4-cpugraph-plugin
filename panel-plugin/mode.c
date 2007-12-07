#include "mode.h"

/*** MODE 0 ***/
void drawGraphMode0 (CPUGraph *base, GdkGC *fg1, GtkWidget *da, int w, int h){
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


/*** MODE 1 ***/
void drawGraphMode1 (CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h){
  int nrx = (w + 1) / 3.0;
  int nry = (h + 1) / 2.0;
  float tstep = nry / CPU_SCALE;
  int x, y;

  for (x = nrx ; x >= 0; x--)
  {
    float usage = base->m_History[nrx - x] * tstep;
    int tmp = 0;
    int length = usage;

    gdk_gc_set_rgb_fg_color (fg2, &base->m_ForeGround2);
    for (y = nry; y >= 0; y--)
    {
      GdkGC *draw = fg2;

      if (base->m_ColorMode > 0)
      {
        GdkColor color;
        double t =
          (base->m_ColorMode == 1) ?
          (tmp / (double) (nry)) :
          (tmp / (double) (length));
        color.red =
          _lerp (t, base->m_ForeGround2.red,
                 base->m_ForeGround3.red);
        color.green =
          _lerp (t, base->m_ForeGround2.green,
                 base->m_ForeGround3.green);
        color.blue =
          _lerp (t, base->m_ForeGround2.blue,
                 base->m_ForeGround3.blue);
        gdk_gc_set_rgb_fg_color (fg1, &color);
        tmp++;
        draw = fg1;
      }

      gdk_draw_rectangle (da->window,
                          draw,
                          TRUE, x * 3, y * 2, 2, 1);
    }
  }
}

/*** MODE 2 ***/
void drawGraphMode2(CPUGraph *base, GdkGC *fg1, GdkGC *fg2, GtkWidget *da, int w, int h){
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

/*** MODE 4 ***/
void drawGraphMode4(CPUGraph *base, GdkGC *fg1, GtkWidget *da, int w, int h){
  gdk_draw_rectangle (da->window,
                      fg1,
                      TRUE,
                      0, (h - (base->m_History[0]*h/CPU_SCALE)),
                      w, (base->m_History[0]*h/CPU_SCALE));
}
