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

#ifndef _XFCE4PP_UTIL_COLLECTIONUTILS_H_
#define _XFCE4PP_UTIL_COLLECTIONUTILS_H_

#include <map>
#include <utility>

namespace xfce4 {

template<typename K, typename V>
void put(std::map<K, V> &map, const K &key, const V &value) {
    auto result = map.emplace(key, value);
    if(!result.second)
        result.first->second = value;
}

template<typename K, typename V>
void put(std::map<K, V> &map, K &&key, const V &value) {
    auto result = map.emplace(std::move(key), value);
    if(!result.second)
        result.first->second = value;
}

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_COLLECTIONUTILS_H_*/
