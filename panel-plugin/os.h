#ifndef __OS_H__
#define __OS_H__

#include <stdio.h>

class OSBase
{
public:
	virtual long GetCPUUsage (void) { return 0; }
protected:
	long m_OldUsage;
	long m_OldTotal;
};

class CPULinux : public OSBase
{
public:
	long GetCPUUsage (void);
};

#if defined (__FreeBSD__)
#include <osreldate.h>
#include <sys/types.h>
#if __FreeBSD_version < 500101
#include <sys/dkstat.h>
#else
#include <sys/resource.h>
#endif
#include <sys/sysctl.h>
#include <devstat.h>
#include <fcntl.h>
#include <nlist.h>
#endif

class CPUFreeBSD : public OSBase
{
public:
	long GetCPUUsage (void);
};

#if defined (__NetBSD__)
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <fcntl.h>
#include <nlist.h>
#endif

class CPUNetBSD : public OSBase
{
public:
	long GetCPUUsage (void);
};

#if defined (__OpenBSD__)
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/dkstat.h>
#include <fcntl.h>
#include <nlist.h>
#endif

class CPUOpenBSD : public OSBase
{
public:
	long GetCPUUsage (void);
};

#endif
