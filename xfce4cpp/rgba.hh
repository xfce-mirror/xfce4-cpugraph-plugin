/*
 *  This file is part of Xfce (https://gitlab.xfce.org).
 *
 *  Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
 *  Copyright (c) 2023 Błażej Szczygieł
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

#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include "fuzzy-cmp.hh"
#include "string-utils.hh"

namespace xfce4 {

using namespace std;

/*
 * An RGB color with an alpha channel. All color channels are 64-bit floats.
 */
struct RGBA : public GdkRGBA
{
    static bool
    parse (RGBA &color, string_view s)
    {
        return gdk_rgba_parse (&color, s.data());
    }

    RGBA()
    {
        red = green = blue = alpha = 0.0;
    }
    RGBA(double r, double g, double b, double a = 1.0)
    {
        red = r;
        green = g;
        blue = b;
        alpha = a;
    }

    RGBA &
    clamp ()
    {
        red = std::clamp (red, 0.0, 1.0);
        green = std::clamp (green, 0.0, 1.0);
        blue = std::clamp (blue, 0.0, 1.0);
        alpha = std::clamp (alpha, 0.0, 1.0);
        return *this;
    }

    bool
    is_transparent () const
    {
        return fuzzy_cmp(alpha, 0.0);
    }
    void
    remove_alpha ()
    {
        alpha = 1.0;
    }
    RGBA
    without_alpha () const
    {
        return RGBA(red, green, blue, 1.0);
    }

    g_string_view
    to_string () const
    {
        return g_string_view(gdk_rgba_to_string(this));
    }

    bool operator ==(const RGBA &a) const
    {
        return fuzzy_cmp(red, a.red) &&
                fuzzy_cmp(green, a.green) &&
                fuzzy_cmp(blue, a.blue) &&
                fuzzy_cmp(alpha, a.alpha);
    }
    bool operator !=(const RGBA &a) const
    {
        return !(*this == a);
    }

    /*
     * The following operators return unclamped RGBA values.
     * All color components, including the alpha channel, are treated uniformly in the same manner.
     */
    RGBA operator +(const RGBA &a) const
    {
        return RGBA(red + a.red, green + a.green, blue + a.blue, alpha + a.alpha);
    }

    RGBA operator -(const RGBA &a) const
    {
        return RGBA(red - a.red, green - a.green, blue - a.blue, alpha - a.alpha);
    }

    RGBA operator *(gdouble a) const
    {
        return RGBA(red * a, green * a, blue * a, alpha * a);
    }
};
static_assert(sizeof(RGBA) == sizeof(GdkRGBA));


static inline void
cairo_set_source_rgba (cairo_t *cr, const RGBA &color)
{
    gdk_cairo_set_source_rgba (cr, &color);
}

static inline GtkColorButton *
gtk_color_button_new (const RGBA &color, bool alpha)
{
    GtkWidget *b = gtk_color_button_new_with_rgba (&color);
    if (alpha)
        gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (b), true);
    return GTK_COLOR_BUTTON (b);
}

static inline RGBA
gtk_get_rgba (GtkColorButton *button)
{
    RGBA c;
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &c);
    return c;
}

} /* namespace xfce4 */
