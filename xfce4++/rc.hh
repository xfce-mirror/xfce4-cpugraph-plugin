/*
 *  This file is part of Xfce (https://gitlab.xfce.org).
 *
 *  Copyright (c) 2021-2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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
#include <libxfce4util/libxfce4util.h>
#include "string-utils.hh"

namespace xfce4 {

using namespace std;

struct Rc
{
    XfceRc *m_rc;

    static unique_ptr<Rc>
    simple_open (string_view filename, bool readonly)
    {
        if (XfceRc *rc = xfce_rc_simple_open (filename.data (), readonly))
            return make_unique<Rc> (rc);
        return nullptr;
    }

    Rc (XfceRc *rc)
        : m_rc(rc)
    {}
    ~Rc ()
    {
        if (m_rc)
        {
            xfce_rc_close (m_rc);
        }
    }

    void
    delete_entry (string_view key, bool global)
    {
        xfce_rc_delete_entry (m_rc, key.data (), global);
    }

    [[nodiscard]] string_view
    read_entry (string_view key, string_view fallback = string_view()) const
    {
        return xfce_rc_read_entry (m_rc, key.data (), fallback.data ());
    }

    [[nodiscard]] bool
    read_bool_entry (string_view key, bool fallback) const
    {
        return xfce_rc_read_bool_entry (m_rc, key.data (), fallback);
    }

    [[nodiscard]] float
    read_float_entry (string_view key, float fallback) const
    {
        const auto f = parse_double (read_entry (key));
        if (f.has_value ())
            return f.value ();
        return fallback;
    }

    [[nodiscard]] gint
    read_int_entry (string_view key, gint fallback) const
    {
        return xfce_rc_read_int_entry (m_rc, key.data (), fallback);
    }

    bool
    has_group (string_view group) const
    {
         return xfce_rc_has_group (m_rc, group.data ());
    }

    void
    set_group (string_view group)
    {
        xfce_rc_set_group (m_rc, group.data ());
    }

    void
    write_bool_entry (string_view key, bool value)
    {
        xfce_rc_write_bool_entry (m_rc, key.data (), value);
    }

    void
    write_entry (string_view key, string_view value)
    {
        xfce_rc_write_entry (m_rc, key.data (), value.data ());
    }

    void
    write_float_entry (string_view key, float value)
    {
        gchar buf[G_ASCII_DTOSTR_BUF_SIZE + 1];
        g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, value);
        buf[G_ASCII_DTOSTR_BUF_SIZE] = '\0';
        write_entry (key, buf);
    }

    void
    write_int_entry (string_view key, gint value)
    {
        xfce_rc_write_int_entry (m_rc, key.data (), value);
    }
};

} /* namespace xfce4 */
