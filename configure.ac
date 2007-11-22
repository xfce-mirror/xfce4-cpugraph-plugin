dnl configure.ac
dnl
dnl xfce4-cpugraph-plugin - A new system load XFce4 panel-plugin.
dnl
dnl 2004 Alexander Nordfelth <alex.nordfelth@telia.com>
dnl

m4_define([cpugraph_version],[0.4.0])

AC_INIT([xfce4-cpugraph-plugin], [cpugraph_version],
	[xfce-goodies-dev@lists.berlios.de])

CPUGRAPH_VERSION=cpugraph_version()
AM_INIT_AUTOMAKE([xfce4-cpugraph-plugin], [$CPUGRAPH_VERSION])
AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_INTLTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.20])

dnl translations
XDT_I18N([be ca cs en_GB eu fr hu nb_NO pl pt_PT sq uk])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
])
