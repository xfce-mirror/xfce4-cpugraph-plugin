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
#include "string-utils.hh"

namespace xfce4 {

using namespace std;

static inline bool
is_directory (string_view path)
{
    return g_file_test (path.data(), G_FILE_TEST_IS_DIR);
}

static inline bool
read_file (string_view path, g_string_view &data)
{
    gchar *contents = nullptr;
    gsize length = 0;
    if (g_file_get_contents (path.data(), &contents, &length, nullptr))
    {
        data = g_string_view (contents, length);
        return true;
    }
    return false;
}

} /* namespace xfce4 */
