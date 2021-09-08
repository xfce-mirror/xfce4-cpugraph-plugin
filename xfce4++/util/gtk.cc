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

#include "gtk.h"

namespace xfce4 {

template<typename WidgetType, typename... Args>
struct HandlerData {
    typedef void FunctionType(WidgetType*, Args...);

    std::function<FunctionType> handler;

    HandlerData(const std::function<FunctionType> &_handler) : handler(_handler) {}

    static void call(WidgetType *widget, Args..., void *data) {
        ((HandlerData<WidgetType>*)data)->handler(widget);
    }

    static void destroy(void *data, GClosure*) {
        delete (HandlerData<WidgetType>*)data;
    }
};

template<typename WidgetType, typename... Args>
static void _connect(WidgetType *widget, const char *signal, const std::function<void(WidgetType*, Args...)> &handler) {
    auto data = new HandlerData<WidgetType, Args...>(handler);
    g_signal_connect_data(
        widget,
        signal,
        (GCallback) HandlerData<WidgetType, Args...>::call,
        data,
        HandlerData<WidgetType, Args...>::destroy,
        (GConnectFlags) 0
    );
}

void connect(GtkColorButton  *widget, const char *signal, const std::function<void(GtkColorButton*)>  &handler) { _connect(widget, signal, handler); }
void connect(GtkComboBox     *widget, const char *signal, const std::function<void(GtkComboBox*)>     &handler) { _connect(widget, signal, handler); }
void connect(GtkEntry        *widget, const char *signal, const std::function<void(GtkEntry*)>        &handler) { _connect(widget, signal, handler); }
void connect(GtkSpinButton   *widget, const char *signal, const std::function<void(GtkSpinButton*)>   &handler) { _connect(widget, signal, handler); }
void connect(GtkToggleButton *widget, const char *signal, const std::function<void(GtkToggleButton*)> &handler) { _connect(widget, signal, handler); }

void connect_destroy (GtkWidget *widget, const std::function<void(GtkWidget*)>       &handler) { _connect(widget, "destroy", handler); }
void connect_response(GtkDialog *widget, const std::function<void(GtkDialog*, gint)> &handler) { _connect(widget, "response", handler); }



struct TimeoutHandlerData {
    typedef bool FunctionType();

    std::function<FunctionType> handler;

    TimeoutHandlerData(const std::function<FunctionType> &_handler) : handler(_handler) {}

    static gboolean call(void *data) {
        return ((TimeoutHandlerData*)data)->handler();
    }

    static void destroy(void *data) {
        delete (TimeoutHandlerData*)data;
    }
};

guint timeout_add(guint interval_ms, const std::function<bool()> &handler) {
    auto data = new TimeoutHandlerData(handler);
    guint id = g_timeout_add_full(G_PRIORITY_DEFAULT, interval_ms, TimeoutHandlerData::call, data, TimeoutHandlerData::destroy);
    if(G_UNLIKELY(id == 0))
        delete data;
    return id;
}




void RGBA::clamp() {
    R = (R >= 0 ? R : 0);
    G = (G >= 0 ? G : 0);
    B = (B >= 0 ? B : 0);
    A = (A >= 0 ? A : 0);

    R = (R <= 1 ? R : 1);
    G = (G <= 1 ? G : 1);
    B = (B <= 1 ? B : 1);
    A = (A <= 1 ? A : 1);
}

bool RGBA::equals(const RGBA &b, double e) const {
    const RGBA &a = *this;
    if(a.R == b.R && a.G == b.G && a.B == b.B && a.A == b.A) {
        return true;
    }
    else {
        return (a.R >= b.R - e && a.R <= b.R + e) &&
               (a.G >= b.G - e && a.G <= b.G + e) &&
               (a.B >= b.B - e && a.B <= b.B + e) &&
               (a.A >= b.A - e && a.A <= b.A + e);
    }
}

bool RGBA::parse(RGBA &color, const std::string &s) {
    GdkRGBA c;
    if(gdk_rgba_parse(&c, s.c_str())) {
        color = c;
        return true;
    }
    else {
        return false;
    }
}

RGBA::operator std::string() const {
    GdkRGBA c = *this;
    char *s = gdk_rgba_to_string(&c);
    std::string s1(s);
    g_free(s);
    return s1;
}

RGBA operator+(const RGBA &a, const RGBA &b) {
    return RGBA(a.R + b.R, a.G + b.G, a.B + b.B, a.A + b.A);
}

RGBA operator-(const RGBA &a, const RGBA &b) {
    return RGBA(a.R - b.R, a.G - b.G, a.B - b.B, a.A - b.A);
}

RGBA operator*(const RGBA &a, double b) {
    return RGBA(a.R * b, a.G * b, a.B * b, a.A * b);
}

RGBA operator*(double a, const RGBA &b) {
    return RGBA(a * b.R, a * b.G, a * b.B, a * b.A);
}



void cairo_set_source(cairo_t *cr, const RGBA &color) {
    GdkRGBA c = color;
    gdk_cairo_set_source_rgba(cr, &c);
}

GtkColorButton* gtk_color_button_new(const RGBA &color, bool alpha) {
    GdkRGBA c = color;
    GtkWidget *b = gtk_color_button_new_with_rgba(&c);
    if(alpha)
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(b), true);
    return GTK_COLOR_BUTTON(b);
}

RGBA gtk_get_rgba(GtkColorButton *button) {
    GdkRGBA c;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &c);
    return c;
}

} /* namespace xfce4 */
