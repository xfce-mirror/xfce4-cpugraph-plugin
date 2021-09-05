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

} /* namespace xfce4 */
