/*
 *  This file is part of Xfce (https://gitlab.xfce.org).
 *
 *  Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _XFCE4PP_UTIL_GTK_H_
#define _XFCE4PP_UTIL_GTK_H_

#include <functional>
#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <string>

/*
 * This file contains partially type-safe or fully type-safe C++ wrappers to selected GLib and GTK+ C functions.
 * The goal is to improve programming productivity compared to the GLib/GTK+ C interface - even at the cost of
 * compile-time performance, run-time performance, binary code size or memory consumption.
 */

namespace xfce4 {

/*
 * Partially type-safe functions for making signal -> handler connections.
 *
 * Only for signals that do not pass extra arguments to the handler.
 * An example of a signal incompatible with these functions is "response" generated by GtkDialog.
 *
 * If the handler is a C++ λ-function, variables captured by the handler
 * are automatically destroyed (via calling the corresponding C++ destructor)
 * when the object/widget generating the signal gets destroyed.
 */
void connect(GtkColorButton  *widget, const char *signal, const std::function<void(GtkColorButton  *widget)> &handler);
void connect(GtkComboBox     *widget, const char *signal, const std::function<void(GtkComboBox     *widget)> &handler);
void connect(GtkEntry        *widget, const char *signal, const std::function<void(GtkEntry        *widget)> &handler);
void connect(GtkSpinButton   *widget, const char *signal, const std::function<void(GtkSpinButton   *widget)> &handler);
void connect(GtkToggleButton *widget, const char *signal, const std::function<void(GtkToggleButton *widget)> &handler);



/*
 * Fully type-safe functions for making signal -> handler connections.
 *
 * If the handler is a C++ λ-function, variables captured by the handler
 * are automatically destroyed (via calling the corresponding C++ destructor)
 * when the object/widget generating the signal gets destroyed.
 */

typedef void ButtonHandler  (GtkWidget *widget, GdkEventButton *event);
typedef void DestroyHandler (GtkWidget *widget);
typedef void DrawHandler1   (cairo_t *cr);
typedef void DrawHandler2   (GtkWidget *widget, cairo_t *cr);
typedef void ResponseHandler(GtkDialog *widget, gint response);
typedef bool TooltipHandler (GtkWidget *widget, gint x, gint y, bool keyboard, GtkTooltip *tooltip);

void connect_after_draw   (GtkWidget *widget, const std::function<DrawHandler1>    &handler);
void connect_after_draw   (GtkWidget *widget, const std::function<DrawHandler2>    &handler);
void connect_button_press (GtkWidget *widget, const std::function<ButtonHandler>   &handler);
void connect_destroy      (GtkWidget *widget, const std::function<DestroyHandler>  &handler);
void connect_query_tooltip(GtkWidget *widget, const std::function<TooltipHandler>  &handler);
void connect_response     (GtkDialog *widget, const std::function<ResponseHandler> &handler);

typedef void PluginHandler    (XfcePanelPlugin *plugin);
typedef void ModeChangeHandler(XfcePanelPlugin *plugin, XfcePanelPluginMode mode);
typedef void SizeChangeHandler(XfcePanelPlugin *plugin, guint size);

void connect_about           (XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler);
void connect_configure_plugin(XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler);
void connect_free_data       (XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler);
void connect_mode_changed    (XfcePanelPlugin *plugin, const std::function<ModeChangeHandler> &handler);
void connect_save            (XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler);
void connect_size_changed    (XfcePanelPlugin *plugin, const std::function<SizeChangeHandler> &handler);

typedef bool TimeoutHandler();

guint timeout_add(guint interval_ms, const std::function<TimeoutHandler> &handler);



/*
 * An RGB color with an alpha channel. All color channels are 64-bit floats.
 *
 * This data structure is aligned to 32 bytes to improve performance
 * and to facilitate compiler auto-vectorization (x86: SSE, AVX).
 *
 * The operators == and != are unsupported. Please use the equals() member function instead.
 */
struct RGBA {
    double R, G, B, A;  /* Red, Green, Blue, Alpha */

    RGBA()                                             : R(0), G(0), B(0), A(1) {}
    RGBA(double r, double g, double b)                 : R(r), G(g), B(b), A(1) {}
    RGBA(double r, double g, double b, double a = 1.0) : R(r), G(g), B(b), A(a) {}
    RGBA(const GdkRGBA &x)                             : R(x.red), G(x.green), B(x.blue), A(x.alpha) {}

    static bool parse(RGBA &color, const std::string &s);

    bool equals(const RGBA&, double e = 1e-10) const;
    bool operator==(const RGBA&) const = delete;
    bool operator!=(const RGBA&) const = delete;

    void clamp();
    RGBA clamped() const       { RGBA a = *this; a.clamp(); return a; }
    bool isTransparent() const { return A == 0; }
    void removeAlpha()         { A = 1.0; }
    RGBA withoutAlpha() const  { return RGBA(R, G, B, 1.0); }

    operator GdkRGBA() const { return GdkRGBA{R, G, B, A}; }
    operator std::string() const; /* gdk_rgba_to_string() wrapper */
}
__attribute__ ((aligned (4*8)));

/*
 * The following operators return unclamped RGBA values.
 * All color components, including the alpha channel, are treated uniformly in the same manner.
 */
RGBA operator+(const RGBA &a, const RGBA &b);
RGBA operator-(const RGBA &a, const RGBA &b);
RGBA operator*(const RGBA &a, double b);
RGBA operator*(double a, const RGBA &b);



void cairo_set_source(cairo_t *cr, const RGBA &color);

GtkColorButton* gtk_color_button_new(const RGBA &color, bool alpha);
RGBA gtk_get_rgba (GtkColorButton *button);

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_GTK_H_ */