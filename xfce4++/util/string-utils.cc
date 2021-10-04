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

#include "string-utils.h"

#include <cerrno>
#include <cstdarg>
#include <cstdio>

namespace xfce4 {

template<typename T, typename fT>
static T parse_number(gchar **s, unsigned base, bool *error, fT (*f)(const gchar*, gchar**, guint)) {
    gchar *end;
    errno = 0;
    auto value = f(*s, &end, base);
    if(errno || value != T(value)) {
        value = 0;
        if(error)
            *error = true;
    }
    else {
        g_assert(*s < end);
        *s = end;
        if(error)
            *error = false;
    }

    return T(value);
}

glong parse_long(gchar **s, unsigned base, bool *error) {
    return parse_number<glong>(s, base, error, g_ascii_strtoll);
}

gulong parse_ulong(gchar **s, unsigned base, bool *error) {
    return parse_number<gulong>(s, base, error, g_ascii_strtoull);
}

template<typename T, typename fT>
static Optional<T> parse_number_optional(const std::string &s, unsigned base, fT (*f)(const gchar*, gchar**, guint)) {
    const auto s1 = trim(s);
    const char *s2 = s1.c_str();

    gchar *end;
    errno = 0;
    auto value = f(s2, &end, base);
    if(errno || end != s2+s1.size() || value != T(value))
        return Optional<T>();
    else
        return Optional<T>(value);
}

Optional<glong> parse_long(const std::string &s, unsigned base) {
    return parse_number_optional<glong>(s, base, g_ascii_strtoll);
}

Optional<gulong> parse_ulong(const std::string &s, unsigned base) {
    return parse_number_optional<gulong>(s, base, g_ascii_strtoull);
}

std::string sprintf(const char *fmt, ...) {
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int n = std::vsnprintf(buf, G_N_ELEMENTS(buf), fmt, ap);
    va_end(ap);

    if(G_LIKELY(n >= 0)) {
        size_t n1 = size_t(n);
        if(G_LIKELY(n1 < sizeof(buf))) {
            return std::string(buf, n1);
        }
        else {
            auto heap_buf = (char*) g_malloc(n1+1);

            va_start(ap, fmt);
            n = std::vsnprintf(heap_buf, n1+1, fmt, ap);
            va_end(ap);

            if(G_LIKELY(n >= 0 && size_t(n) == n1)) {
                std::string s(heap_buf, n1);
                g_free(heap_buf);
                return s;
            }
        }
    }

    return "<xfce4::sprintf() failure>";
}

std::string trim(const std::string &s) {
    return trim_left(trim_right(s));
}

std::string trim_left(const std::string &s) {
    auto index = s.find_first_not_of(" \n\r\t");
    if(index == std::string::npos)
        return std::string();
    else
        return s.substr(index);
}

std::string trim_right(const std::string &s) {
    auto index = s.find_last_not_of(" \n\r\t");
    return index == std::string::npos ? s : s.substr(0, index+1);
}

} /* namespace xfce4 */
