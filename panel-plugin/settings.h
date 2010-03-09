#ifndef _XFCE_SETTINGS_H_
#define _XFCE_SETTINGS_H_

#include "cpu.h"

void read_settings( XfcePanelPlugin *plugin, CPUGraph *base );
void write_settings( XfcePanelPlugin *plugin, CPUGraph *base );

#endif /* !_XFCE_SETTINGS_H_ */
