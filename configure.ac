dnl configure.ac
dnl
dnl xfce4-showdesktop-plugin - <Plugin for xfce4-panel>
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

AC_INIT([panel-plugin/cpu.cc])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([xfce4-cpugraph-plugin], [0.1])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.0.0])

AC_SUBST(libcpugraph_CFLAGS)
AC_SUBST(libcpugraph_LIBS)

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
])
