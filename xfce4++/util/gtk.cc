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

template<typename ObjectType, typename ReturnType, typename... Args>
struct HandlerData {
    static const uint32_t MAGIC = 0x1A2AB40F;
    const uint32_t magic = MAGIC;

    typedef ReturnType FunctionType(ObjectType*, Args...);
    std::function<FunctionType> handler;

    HandlerData(const std::function<FunctionType> &_handler) : handler(_handler) {}

    static ReturnType call(ObjectType *object, Args... args, void *data) {
        auto h = (HandlerData*)data;
        g_assert(h->magic == MAGIC);  /* Try to detect invalid number of parameters in the function signature of h->handler */
        return h->handler(object, args...);
    }

    static void destroy(void *data, GClosure*) {
        delete (HandlerData*)data;
    }
};

template<typename ObjectType, typename ReturnType, typename... Args>
static void _connect(ObjectType *object, const char *signal, const std::function<ReturnType(ObjectType*, Args...)> &handler, bool after = false) {
    auto data = new HandlerData<ObjectType, ReturnType, Args...>(handler);
    g_signal_connect_data(
        object,
        signal,
        (GCallback) HandlerData<ObjectType, ReturnType, Args...>::call,
        data,
        HandlerData<ObjectType, ReturnType, Args...>::destroy,
        after ? G_CONNECT_AFTER : (GConnectFlags) 0
    );
}

void connect(GtkColorButton  *widget, const char *signal, const std::function<void(GtkColorButton*)>  &handler) { _connect(widget, signal, handler); }
void connect(GtkComboBox     *widget, const char *signal, const std::function<void(GtkComboBox*)>     &handler) { _connect(widget, signal, handler); }
void connect(GtkEntry        *widget, const char *signal, const std::function<void(GtkEntry*)>        &handler) { _connect(widget, signal, handler); }
void connect(GtkSpinButton   *widget, const char *signal, const std::function<void(GtkSpinButton*)>   &handler) { _connect(widget, signal, handler); }
void connect(GtkToggleButton *widget, const char *signal, const std::function<void(GtkToggleButton*)> &handler) { _connect(widget, signal, handler); }

void connect_after_draw   (GtkWidget *widget, const std::function<DrawHandler1>    &handler) { connect_after_draw(widget, [handler](GtkWidget*, cairo_t *cr) { handler(cr); }); }
void connect_after_draw   (GtkWidget *widget, const std::function<DrawHandler2>    &handler) { _connect(widget, "draw", handler, true); }
void connect_button_press (GtkWidget *widget, const std::function<ButtonHandler>   &handler) { _connect(widget, "button-press-event", handler); }
void connect_destroy      (GtkWidget *widget, const std::function<DestroyHandler>  &handler) { _connect(widget, "destroy", handler); }
void connect_query_tooltip(GtkWidget *widget, const std::function<TooltipHandler>  &handler) { _connect(widget, "query-tooltip", handler); }
void connect_response     (GtkDialog *widget, const std::function<ResponseHandler> &handler) { _connect(widget, "response", handler); }

void connect_about           (XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler) { _connect(plugin, "about", handler); }
void connect_configure_plugin(XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler) { _connect(plugin, "configure-plugin", handler); }
void connect_free_data       (XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler) { _connect(plugin, "free-data", handler); }
void connect_mode_changed    (XfcePanelPlugin *plugin, const std::function<ModeChangeHandler> &handler) { _connect(plugin, "mode-changed", handler); }
void connect_save            (XfcePanelPlugin *plugin, const std::function<PluginHandler>     &handler) { _connect(plugin, "save", handler); }
void connect_size_changed    (XfcePanelPlugin *plugin, const std::function<SizeChangeHandler> &handler) { _connect(plugin, "size-changed", handler); }



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

guint timeout_add(guint interval_ms, const std::function<TimeoutHandler> &handler) {
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
