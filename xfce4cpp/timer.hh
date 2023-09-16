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
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <functional>
#include <memory>

namespace xfce4 {

using namespace std;

class TimeoutResponse
{
    /* Invoke the timeout handler again, otherwise stop the timer */

    TimeoutResponse (gboolean again)
        : m_again(again)
    {}

    const bool m_again;

public:
    operator gboolean () const
    {
        return m_again;
    }

    static TimeoutResponse Remove ()
    {
        return false;
    }
    static TimeoutResponse Again ()
    {
        return true;
    }
};

using SourceTag = weak_ptr<guint>;

static SourceTag
timeout_add (guint interval_ms, const function<TimeoutResponse ()> &fn, gint priority = G_PRIORITY_DEFAULT)
{
    static_assert(is_same_v<SourceTag::element_type, guint>);
    using FunctionType = pair<decay_t<decltype (fn)>, shared_ptr<guint>>;

    auto fnPtr = new FunctionType (fn, nullptr);

    guint id = g_timeout_add_full (
        priority,
        interval_ms,
        [](gpointer data)->gboolean {
            auto &fnRef = *reinterpret_cast<FunctionType *> (data);
            return fnRef.first();
        },
        fnPtr,
        [](gpointer data) {
            delete reinterpret_cast<FunctionType *> (data);
        }
    );

    if (G_UNLIKELY (id == 0))
    {
        delete fnPtr;
        return SourceTag();
    }

    fnPtr->second = make_shared<guint>(id);
    return fnPtr->second;
}

static inline void
source_remove (const SourceTag &sourceTag)
{
    if (auto sourceTagPtr = sourceTag.lock())
        g_source_remove (*sourceTagPtr);
}

static inline SourceTag
invoke_later (const function<void ()> &task, gint priority = G_PRIORITY_DEFAULT)
{
    return timeout_add (0, [task] {
        task ();
        return TimeoutResponse::Remove ();
    }, priority);
}

} /* namespace xfce4 */
