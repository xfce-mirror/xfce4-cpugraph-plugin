#ifndef _XFCE_OS_H_
#define _XFCE_OS_H_

#define CPU_SCALE 256

#include <glib.h>

typedef struct
{
	guint load;
	guint previous_used;
	guint previous_total;
} CpuData;

guint detect_cpu_number();
gboolean read_cpu_data( CpuData *data, guint nb_cpu );

#endif /* !_XFCE_OS_H */
