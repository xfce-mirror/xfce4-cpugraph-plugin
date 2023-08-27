/*  settings.h
 *  Part of xfce4-cpugraph-plugin
 *
 *  Copyright (c) Alexander Nordfelth <alex.nordfelth@telia.com>
 *  Copyright (c) gatopeich <gatoguan-os@yahoo.com>
 *  Copyright (c) 2007-2008 Angelo Arrifano <miknix@gmail.com>
 *  Copyright (c) 2007-2008 Lidiriel <lidiriel@coriolys.org>
 *  Copyright (c) 2010 Florian Rivoal <frivoal@gmail.com>
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
#ifndef _XFCE_CPUGRAPH_SETTINGS_H_
#define _XFCE_CPUGRAPH_SETTINGS_H_

#include "cpu.h"

namespace Settings {

void init (XfcePanelPlugin *plugin, const std::shared_ptr<CPUGraph> &base);
void finalize ();
void read (XfcePanelPlugin *plugin, const std::shared_ptr<CPUGraph> &base);
void write (XfcePanelPlugin *plugin, const std::shared_ptr<const CPUGraph> &base);

}

#endif /* _XFCE_CPUGRAPH_SETTINGS_H_ */
