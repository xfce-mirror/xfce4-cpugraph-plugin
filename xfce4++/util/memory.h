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

#ifndef _XFCE4PP_UTIL_MEMORY_H_
#define _XFCE4PP_UTIL_MEMORY_H_

#include <memory>
#include <utility>

namespace xfce4 {

/* A pointer that cannot be null */
template<typename T>
struct Ptr final {
    std::shared_ptr<T> ptr;

    template<typename... Args>
    static Ptr<T> make(Args&&... args) {
        Ptr p;
        p.ptr = std::make_shared<T>(std::forward<Args>(args)...);
        return p;
    }

    Ptr(const Ptr<T> &p) : ptr(p.ptr) {}

    T* operator->() const { return ptr.get(); }

private:
    Ptr() {}
};

/* A pointer that can be null */
template<typename T>
struct Ptr0 final : std::shared_ptr<T> {
    Ptr0() : std::shared_ptr<T>(nullptr) {}
    Ptr0(std::nullptr_t) : std::shared_ptr<T>(nullptr) {}
    Ptr0(const std::shared_ptr<T> &p) : std::shared_ptr<T>(p) {}
    Ptr0(std::shared_ptr<T> &&p) : std::shared_ptr<T>(std::move(p)) {}
    Ptr0(const Ptr<T> &p) : std::shared_ptr<T>(p.ptr) {}
};

/* Allocates a new instance of T on the heap, passing args to T's constructor */
template<typename T, typename... Args>
inline Ptr<T> make(Args&&... args) {
    return Ptr<T>::template make<Args...>(std::forward<Args>(args)...);
}

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_MEMORY_H_ */
