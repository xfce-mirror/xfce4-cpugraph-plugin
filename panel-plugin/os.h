#ifndef _XFCE_OS_H_
#define _XFCE_OS_H_

#define CPU_SCALE 256

typedef struct
{
	unsigned int load;
	unsigned int previous_used;
	unsigned int previous_total;
} CpuData;

unsigned int detect_cpu_number();
int read_cpu_data( CpuData *data, unsigned int nb_cpu );

#endif /* !_XFCE_OS_H */
