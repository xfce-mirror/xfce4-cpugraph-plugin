dnl configure.ac
dnl
dnl xfce4-cpugraph-plugin - A new system load XFce4 panel-plugin.
dnl
dnl 2004 Alexander Nordfelth <alex.nordfelth@telia.com>
dnl

AC_INIT([panel-plugin/cpu.c])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([xfce4-cpugraph-plugin], [0.2.2])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.0.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
])
