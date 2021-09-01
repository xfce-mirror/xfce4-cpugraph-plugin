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

#include "rc.h"

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

void Rc::delete_entry(const std::string &key, bool global) {
    xfce_rc_delete_entry(rc, key.c_str(), global);
}

Ptr0<std::string> Rc::read_entry(const std::string &key, const char *fallback_orNull) const {
    const gchar *e = xfce_rc_read_entry(rc, key.c_str(), fallback_orNull);
    if(e)
        return make<std::string>(e);
    else
        return nullptr;
}

gint Rc::read_int_entry(const std::string &key, gint fallback) const {
    return xfce_rc_read_int_entry(rc, key.c_str(), fallback);
}

Ptr0<Rc> Rc::simple_open(const std::string &filename, bool readonly) {
    XfceRc *rc = xfce_rc_simple_open(filename.c_str(), readonly);
    if(rc)
        return make<Rc>(rc);
    else
        return nullptr;
}

void Rc::write_entry(const std::string &key, const std::string &value) {
    xfce_rc_write_entry(rc, key.c_str(), value.c_str());
}

void Rc::write_int_entry(const std::string &key, gint value) {
    xfce_rc_write_int_entry(rc, key.c_str(), value);
}

} /* namespace xfce4 */
