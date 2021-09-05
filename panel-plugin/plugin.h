/*  plugin.h
 *  Part of xfce4-cpugraph-plugin
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

#ifndef _XFCE_CPUGRAPH_PLUGIN_H_
#define _XFCE_CPUGRAPH_PLUGIN_H_

#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

void cpugraph_construct (XfcePanelPlugin *plugin);

G_END_DECLS

#endif /* _XFCE_CPUGRAPH_PLUGIN_H_ */
