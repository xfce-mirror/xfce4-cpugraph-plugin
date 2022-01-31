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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#include <errno.h>
#include "rc.h"
#include "string-utils.h"

namespace xfce4 {

Rc::Rc(XfceRc *_rc) : rc(_rc) {}

Rc::~Rc() {
    close();
}

void Rc::close() {
    if(rc) {
        xfce_rc_close(rc);
        rc = NULL;
    }
}

void Rc::delete_entry(const char *key, bool global) {
    xfce_rc_delete_entry(rc, key, global);
}

void Rc::delete_entry(const string &key, bool global) { delete_entry(key.c_str(), global); }

bool Rc::has_group(const char *group) const {
    return xfce_rc_has_group(rc, group);
}

bool Rc::has_group(const string &group) const { return has_group(group.c_str()); }

bool Rc::read_bool_entry(const char *key, bool fallback) const {
    return xfce_rc_read_bool_entry(rc, key, fallback);
}

bool Rc::read_bool_entry(const string &key, bool fallback) const { return read_bool_entry(key.c_str(), fallback); }

Ptr0<std::string> Rc::read_entry(const char *key, const char *fallback_orNull) const {
    const gchar *e = xfce_rc_read_entry(rc, key, fallback_orNull);
    if(e)
        return make<string>(e);
    else if(fallback_orNull)
        return make<string>(fallback_orNull);
    else
        return nullptr;
}

Ptr0<std::string> Rc::read_entry(const string &key, const char *fallback_orNull) const {
    return read_entry(key.c_str(), fallback_orNull);
}

std::string Rc::read_entry(const char *key, const string &fallback) const {
    const gchar *e = xfce_rc_read_entry(rc, key, fallback.c_str());
    if(e)
        return e;
    else
        return fallback;
}

std::string Rc::read_entry(const string &key, const string &fallback) const {
    return read_entry(key.c_str(), fallback);
}

float Rc::read_float_entry(const char *key, float fallback) const {
    Ptr0<string> e = read_entry(key, nullptr);
    if(e) {
        const std::string s = trim(*e.toPtr());
        gchar *endptr = NULL;
        errno = 0;
        gdouble value = g_ascii_strtod(s.c_str(), &endptr);
        if(errno == 0 && endptr == s.c_str() + s.size()) {
            return value;
        }
    }
    return fallback;
}

float Rc::read_float_entry(const string &key, float fallback) const { return read_float_entry(key.c_str(), fallback); }

gint Rc::read_int_entry(const char *key, gint fallback) const {
    return xfce_rc_read_int_entry(rc, key, fallback);
}

gint Rc::read_int_entry(const string &key, gint fallback) const { return read_int_entry(key.c_str(), fallback); }

void Rc::set_group(const char *group) {
    xfce_rc_set_group(rc, group);
}

void Rc::set_group(const string &group) { set_group(group.c_str()); }

Ptr0<Rc> Rc::simple_open(const string &filename, bool readonly) {
    XfceRc *rc = xfce_rc_simple_open(filename.c_str(), readonly);
    if(rc)
        return make<Rc>(rc);
    else
        return nullptr;
}

void Rc::write_bool_entry(const char   *key, bool value) { xfce_rc_write_bool_entry(rc, key        , value); }
void Rc::write_bool_entry(const string &key, bool value) { xfce_rc_write_bool_entry(rc, key.c_str(), value); }

void Rc::write_entry(const char   *key, const char   *value) { xfce_rc_write_entry(rc, key        , value        ); }
void Rc::write_entry(const char   *key, const string &value) { xfce_rc_write_entry(rc, key        , value.c_str()); }
void Rc::write_entry(const string &key, const char   *value) { xfce_rc_write_entry(rc, key.c_str(), value        ); }
void Rc::write_entry(const string &key, const string &value) { xfce_rc_write_entry(rc, key.c_str(), value.c_str()); }

void Rc::write_float_entry(const char *key, float value) {
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE+1];
    g_ascii_dtostr(buf, G_ASCII_DTOSTR_BUF_SIZE, value);
    buf[G_ASCII_DTOSTR_BUF_SIZE] = '\0';
    write_entry(key, buf);
}

void Rc::write_float_entry(const string &key, float value) { write_float_entry(key.c_str(), value); }

void Rc::write_int_entry(const char   *key, gint value) { xfce_rc_write_int_entry(rc, key        , value); }
void Rc::write_int_entry(const string &key, gint value) { xfce_rc_write_int_entry(rc, key.c_str(), value); }

void Rc::write_default_bool_entry(const char *key, bool value, bool default_value) {
    if(value == default_value)
        delete_entry(key, false);
    else
        write_bool_entry(key, value);
}

void Rc::write_default_bool_entry(const string &key, bool value, bool default_value) {
    if(value == default_value)
        delete_entry(key, false);
    else
        write_bool_entry(key, value);
}

void Rc::write_default_entry(const char *key, const char *value, const char *default_value) {
    if(value && default_value && strcmp(value, default_value) == 0)
        delete_entry(key, false);
    else
        write_entry(key, value);
}

void Rc::write_default_entry(const char *key, const string &value, const string &default_value) {
    if(value == default_value)
        delete_entry(key, false);
    else
        write_entry(key, value);
}

void Rc::write_default_entry(const string &key, const char *value, const char *default_value) {
    if(value && default_value && strcmp(value, default_value) == 0)
        delete_entry(key, false);
    else
        write_entry(key, value);
}

void Rc::write_default_entry(const string &key, const string &value, const string &default_value) {
    if(value == default_value)
        delete_entry(key, false);
    else
        write_entry(key, value);
}

void Rc::write_default_float_entry(const char *key, float value, float default_value, float epsilon) {
    if(value >= default_value - epsilon && value <= default_value + epsilon)
        delete_entry(key, false);
    else
        write_float_entry(key, value);
}

void Rc::write_default_float_entry(const string &key, float value, float default_value, float epsilon) {
    if(value == default_value)
        delete_entry(key, false);
    else
        write_float_entry(key, value);
}

void Rc::write_default_int_entry(const char *key, gint value, gint default_value) {
    if(value == default_value)
        delete_entry(key, false);
    else
        write_int_entry(key, value);
}

void Rc::write_default_int_entry(const string &key, gint value, gint default_value) {
    if(value == default_value)
        delete_entry(key, false);
    else
        write_int_entry(key, value);
}

} /* namespace xfce4 */
