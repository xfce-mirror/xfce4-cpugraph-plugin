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
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <limits>

namespace xfce4 {

using namespace std;

/*
 * A string_view which takes ownership of "gchar *" data and frees it if "g_string_view" is destroyed.
 *
 * Warning: if you obtain a "string_view" from a "g_string_view", never use it after "g_string_view"
 * being destroyed!
 */

class g_string_view : public string_view
{
public:
    g_string_view () = default;
    g_string_view (gchar *data, gsize length)
        : string_view (data, length)
        , m_data(shared_ptr<gchar>(data, [](gchar *ptr) { g_free (ptr); }))
    {}
    g_string_view (gchar *data)
        : g_string_view (data, g_utf8_strlen (data, numeric_limits<gssize>::max ()))
    {}

private:
    shared_ptr<gchar> m_data;
};


static inline bool
starts_with (string_view s, string_view prefix)
{
    return s.size () >= prefix.size () && equal (prefix.begin(), prefix.end(), s.begin());
}

static inline bool
ends_with (string_view s, string_view suffix)
{
    return s.size () >= suffix.size () && equal (s.begin () + (s.size () - suffix.size ()), s.end (), suffix.begin ());
}


template<typename StringList> static inline string
join (const StringList &strings, string_view separator)
{
    size_t size = separator.size () * strings.size () - 1;
    for (auto &&str : strings)
    {
        size += str.size ();
    }

    string s;
    s.reserve (size);
    for (auto &&str : strings)
    {
        if (&str != &strings[0])
        {
            s.append (separator);
        }
        s.append (str);
    }
    return s;
}


static inline string
sprintf (const char *fmt, ...)
{
    string buf;

    va_list ap;
    va_start (ap, fmt);
    gint n = g_vsnprintf (nullptr, 0, fmt, ap);
    va_end (ap);

    if (n > 0 && n < numeric_limits<gint>::max ())
    {
        buf.resize (n + 1);
        va_start (ap, fmt);
        g_vsnprintf (buf.data (), buf.size (), fmt, ap);
        va_end (ap);
    }

    return buf;
}


static inline string_view
trim_left (string_view s)
{
    auto index = s.find_first_not_of (" \n\r\t");
    return index == string_view::npos ? string_view() : s.substr (index);
}
static inline string_view
trim_right (string_view s)
{
    auto index = s.find_last_not_of (" \n\r\t");
    return index == string_view::npos ? s : s.substr (0, index + 1);
}
static inline string_view
trim (string_view s)
{
    return trim_left (trim_right (s));
}


/*
 * Number parsing functions. If parsing fails, an optional without a value is returned.
 *
 * The string_view can contain just the number, with an optional prefix and suffix consisting from whitespace characters.
 */

template<typename T>
static optional<T> parse_number (string_view s, guint base)
{
    static_assert (is_same_v<T, gint64> || is_same_v<T, guint64> || is_same_v<T, gdouble>);
    if (const auto s1 = trim (s); !s1.empty ())
    {
        const auto s2 = s1.data ();
        gchar *end = nullptr;
        errno = 0;
        T value;
        if constexpr (is_same_v<T, gint64>)
            value = g_ascii_strtoll (s2, &end, base);
        else if constexpr (is_same_v<T, guint64>)
            value = g_ascii_strtoull (s2, &end, base);
        else if constexpr (is_same_v<T, gdouble>)
            value = g_ascii_strtod (s2, &end);
        if (errno == 0 && end == s2 + s1.size ())
            return value;
    }
    return nullopt;
}

static inline optional<gdouble>
parse_double (string_view s)
{
    return parse_number<gdouble> (s, 0);
}

static inline optional<guint64>
parse_ulong (string_view s, guint base = 0)
{
    return parse_number<guint64> (s, base);
}

static inline optional<gint64>
parse_long (string_view s, guint base = 0)
{
    return parse_number<gint64> (s, base);
}

} /* namespace xfce4 */
