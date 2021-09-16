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

#include "io.h"

#include <glib.h>

namespace xfce4 {

bool is_directory(const std::string &path) {
    return g_file_test(path.c_str(), G_FILE_TEST_IS_DIR);
}

bool read_file(const std::string &path, std::string &data) {
    gchar *contents = NULL;
    if(g_file_get_contents(path.c_str(), &contents, NULL, NULL)) {
        data = *contents;
        g_free(contents);
        return true;
    }
    else {
        return false;
    }
}

} /* namespace xfce4 */
