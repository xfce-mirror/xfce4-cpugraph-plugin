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

#ifndef _XFCE4PP_UTIL_RC_H_
#define _XFCE4PP_UTIL_RC_H_

#include "memory.h"
#include <libxfce4util/libxfce4util.h>
#include <string>

namespace xfce4 {

struct Rc {
    XfceRc *rc;

    static Ptr0<Rc> simple_open(const std::string &filename, bool readonly);

    Rc(XfceRc *rc);
    virtual ~Rc();

    void close();
    void delete_entry(const std::string &key, bool global);
    const gchar* read_entry(const std::string &key, const char *fallback_orNull) const G_GNUC_WARN_UNUSED_RESULT;
    gint read_int_entry(const std::string &key, gint fallback) const G_GNUC_WARN_UNUSED_RESULT;
    void write_entry(const std::string &key, const std::string &value);
    void write_int_entry(const std::string &key, gint value);
};

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_RC_H_ */
