#ifndef _XFCE_OS_H_
#define _XFCE_OS_H_

#define CPU_SCALE 100000

typedef struct
{
	float load; /* cpu utilization */
	unsigned long pUsed; /* Previous value of used cpu time */
	unsigned long pTotal; /* Previous value of total cpu time */
} CpuData;


int init_cpu_data();
void free_cpu_data();
CpuData *read_cpu_data();

#endif /* !_XFCE_OS_H */
